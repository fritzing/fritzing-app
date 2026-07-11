/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2019 Fritzing

Fritzing is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Fritzing is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************/

#include "breadboardautorouter.h"
#include "breadboardpartpolicy.h"
#include "breadboardroutegraph.h"
#include "breadboardtopology.h"

#include <QHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QSet>
#include <QStandardPaths>
#include <QTextStream>
#include <QtMath>
#include <QMessageBox>
#include <QElapsedTimer>
#include <QUndoCommand>
#include <algorithm>
#include <limits>

#include "../commands.h"
#include "../items/itembase.h"
#include "../items/wire.h"
#include "../sketch/breadboardsketchwidget.h"
#include "../waitpushundostack.h"

namespace
{
	struct PlacementCandidate
	{
		ItemBase *item = nullptr;
		QPointF oldLoc;
		QPointF newLoc;
		QHash<ConnectorItem *, ConnectorItem *> pinToHole;
		QHash<ConnectorItem *, QPolygonF> pinToLeg;
		double score = std::numeric_limits<double>::max();
		bool usesLegPlacement = false;
	};

	constexpr double HoleMatchTolerance = 12.0;
	constexpr double PlacementKeepoutMargin = 8.0;
	constexpr double BendablePlacementKeepoutMargin = 1.0;
	constexpr double MaxBendableLegLength = 120.0;

	ViewGeometry::WireFlags generatedWireFlags()
	{
		return ViewGeometry::NormalFlag | ViewGeometry::AutoroutableFlag;
	}

	QLineF connectorLine(ConnectorItem *from, ConnectorItem *to)
	{
		if (from == nullptr || to == nullptr) return QLineF();
		return QLineF(from->sceneAdjustedTerminalPoint(nullptr), to->sceneAdjustedTerminalPoint(nullptr));
	}

	BreadboardRouteGraph::Options routeGraphOptions(const QList<QLineF> &plannedSegments)
	{
		BreadboardRouteGraph::Options options = BreadboardRouteGraph::Options::fromEnvironment();
		options.existingSegments = plannedSegments;
		return options;
	}

	double leadCongestionPenalty(ConnectorItem *from, ConnectorItem *to, const QList<QLineF> &plannedSegments)
	{
		const BreadboardRouteGraph::Options options = BreadboardRouteGraph::Options::fromEnvironment();
		return BreadboardRouteGraph::congestionPenalty(connectorLine(from, to),
		                                                   plannedSegments,
		                                                   options.crossingPenalty,
		                                                   options.overlapPenalty);
	}

	struct HoleBounds
	{
		bool valid = false;
		double minX = 0.0;
		double maxX = 0.0;
		double minY = 0.0;
		double maxY = 0.0;
	};

	double manhattanDistance(const QPointF &a, const QPointF &b)
	{
		return qAbs(a.x() - b.x()) + qAbs(a.y() - b.y());
	}

	double polylineLength(const QPolygonF &polygon)
	{
		double length = 0.0;
		for (int i = 1; i < polygon.count(); i++) length += QLineF(polygon.at(i - 1), polygon.at(i)).length();
		return length;
	}

	bool routeIsBetter(const BreadboardRouteGraph::Result &candidate,
	                   const BreadboardRouteGraph::Result &current)
	{
		return candidate.found && (!current.found || candidate.score < current.score);
	}

	HoleBounds boundsForHoles(const QList<ConnectorItem *> &holes)
	{
		HoleBounds bounds;
		Q_FOREACH (ConnectorItem *hole, holes)
		{
			if (hole == nullptr)
				continue;
			const QPointF point = hole->sceneAdjustedTerminalPoint(nullptr);
			if (!bounds.valid)
			{
				bounds.valid = true;
				bounds.minX = bounds.maxX = point.x();
				bounds.minY = bounds.maxY = point.y();
				continue;
			}
			bounds.minX = qMin(bounds.minX, point.x());
			bounds.maxX = qMax(bounds.maxX, point.x());
			bounds.minY = qMin(bounds.minY, point.y());
			bounds.maxY = qMax(bounds.maxY, point.y());
		}
		return bounds;
	}

	double boardEdgeEntryScore(ConnectorItem *terminal, ConnectorItem *entry, const HoleBounds &bounds)
	{
		if (terminal == nullptr || entry == nullptr || !bounds.valid)
		{
			return std::numeric_limits<double>::max();
		}

		const QPointF terminalPoint = terminal->sceneAdjustedTerminalPoint(nullptr);
		const QPointF entryPoint = entry->sceneAdjustedTerminalPoint(nullptr);
		const double centerX = (bounds.minX + bounds.maxX) * 0.5;
		const double centerY = (bounds.minY + bounds.maxY) * 0.5;

		// Peripheral wires should enter at the breadboard edge facing the part.
		// This keeps long off-board leads out of the middle of the board and lets
		// the bus graph handle board-local jumpers from that entry point.
		double edgeDistance = 0.0;
		double perpendicularDistance = 0.0;
		if (terminalPoint.x() < bounds.minX)
		{
			edgeDistance = qAbs(entryPoint.x() - bounds.minX);
			perpendicularDistance = qAbs(entryPoint.y() - terminalPoint.y()) * 0.35;
		}
		else if (terminalPoint.x() > bounds.maxX)
		{
			edgeDistance = qAbs(entryPoint.x() - bounds.maxX);
			perpendicularDistance = qAbs(entryPoint.y() - terminalPoint.y()) * 0.35;
		}
		else if (terminalPoint.y() < bounds.minY)
		{
			edgeDistance = qAbs(entryPoint.y() - bounds.minY);
			perpendicularDistance = qAbs(entryPoint.x() - terminalPoint.x()) * 0.35;
		}
		else if (terminalPoint.y() > bounds.maxY)
		{
			edgeDistance = qAbs(entryPoint.y() - bounds.maxY);
			perpendicularDistance = qAbs(entryPoint.x() - terminalPoint.x()) * 0.35;
		}
		else
		{
			const double left = qAbs(entryPoint.x() - bounds.minX);
			const double right = qAbs(entryPoint.x() - bounds.maxX);
			const double top = qAbs(entryPoint.y() - bounds.minY);
			const double bottom = qAbs(entryPoint.y() - bounds.maxY);
			edgeDistance = qMin(qMin(left, right), qMin(top, bottom));
			perpendicularDistance = manhattanDistance(entryPoint, QPointF(centerX, centerY)) * 0.1;
		}

		return edgeDistance * 8.0 + perpendicularDistance + manhattanDistance(terminalPoint, entryPoint) * 0.1;
	}

	QPolygonF translatedLegForTarget(ConnectorItem *pin, const QPointF &offset, ConnectorItem *hole)
	{
		QPolygonF translated;
		if (pin == nullptr || hole == nullptr)
			return translated;

		QPolygonF oldLeg = pin->sceneAdjustedLeg();
		if (oldLeg.count() < 2)
		{
			translated << pin->sceneAdjustedTerminalPoint(nullptr) + offset;
			translated << hole->sceneAdjustedTerminalPoint(nullptr);
			return translated;
		}

		Q_FOREACH (QPointF point, oldLeg)
		{
			translated << point + offset;
		}
		translated.replace(translated.count() - 1, hole->sceneAdjustedTerminalPoint(nullptr));
		return translated;
	}

	bool allPinsHaveBendableLegs(const QList<ConnectorItem *> &pins)
	{
		if (pins.isEmpty())
			return false;
		Q_FOREACH (ConnectorItem *pin, pins)
		{
			if (pin == nullptr || !pin->hasRubberBandLeg())
				return false;
		}
		return true;
	}

	QList<ConnectorItem *> freeBusHoles(ConnectorItem *breadboardHole, const QList<ConnectorItem *> &allowedHoles, const QSet<ConnectorItem *> &reservedHoles)
	{
		QList<ConnectorItem *> result;
		if (breadboardHole == nullptr)
			return result;

		ItemBase *breadboard = breadboardHole->attachedTo();
		if (breadboard == nullptr)
			return result;

		QList<ConnectorItem *> busHoles;
		if (!breadboard->busConnectorItems(breadboardHole, busHoles))
			return result;

		Q_FOREACH (ConnectorItem *candidate, busHoles)
		{
			if (candidate == nullptr)
				continue;
			if (!allowedHoles.contains(candidate))
				continue;
			if (reservedHoles.contains(candidate))
				continue;
			if (candidate->connectorType() != Connector::Female)
				continue;
			if (candidate->connectionsCount() != 0)
				continue;
			if (!candidate->attachedTo()->isEverVisible())
				continue;
			if (!result.contains(candidate))
				result.append(candidate);
		}

		std::sort(result.begin(), result.end(), [breadboardHole](ConnectorItem *a, ConnectorItem *b)
				  {
		QPointF origin = breadboardHole->sceneAdjustedTerminalPoint(nullptr);
		double ad = QLineF(origin, a->sceneAdjustedTerminalPoint(nullptr)).length();
		double bd = QLineF(origin, b->sceneAdjustedTerminalPoint(nullptr)).length();
		return ad < bd; });

		return result;
	}
}

BreadboardAutorouter::BreadboardAutorouter(BreadboardSketchWidget *sketchWidget)
	: m_sketchWidget(sketchWidget)
{
}

BreadboardAutorouter::~BreadboardAutorouter()
{
	clearCollectedNets();
}

void BreadboardAutorouter::start()
{
	if (m_sketchWidget == nullptr)
		return;
	QElapsedTimer elapsed;
	elapsed.start();
	m_componentLeadLength = 0.0;
	m_lastRoutingScore = BreadboardRoutingScore();

	QFile::remove(logFilePath());
	logAutoroute("========== breadboard autoroute start ==========");
	logAutoroute(QString("log file: %1").arg(logFilePath()));

	const int cleared = clearPreviousAutorouteWires();
	if (cleared > 0)
	{
		Q_EMIT setMaximumProgress(1);
		Q_EMIT setProgressValue(1);
		Q_EMIT setProgressMessage(QObject::tr("Breadboard routes cleared."));
		Q_EMIT setProgressMessage2(QObject::tr("Removed %1 generated breadboard wire(s).").arg(cleared));
		logAutoroute(QString("clear complete: removedWires=%1").arg(cleared));
		logAutoroute("========== breadboard autoroute end ==========");
		return;
	}

	clearCollectedNets();

	QHash<ConnectorItem *, int> indexer;
	m_sketchWidget->collectAllNets(indexer, m_allPartConnectorItems, false, false, false);
	logAutoroute(QString("collectAllNets: nets=%1 indexer=%2").arg(m_allPartConnectorItems.count()).arg(indexer.count()));

	if (m_allPartConnectorItems.isEmpty())
	{
		logAutoroute("abort: no breadboard connections to route");
		QMessageBox::information(nullptr, QObject::tr("Fritzing"), QObject::tr("No breadboard connections to route."));
		return;
	}

	Q_EMIT setMaximumProgress(m_allPartConnectorItems.count());
	Q_EMIT setProgressValue(0);
	Q_EMIT setProgressMessage(QObject::tr("Placing breadboard parts..."));
	Q_EMIT setProgressMessage2(QString());

	auto *undoStack = m_sketchWidget->undoStack();
	const int undoCountBefore = undoStack->count();
	const int undoIndexBefore = undoStack->index();
	logAutoroute(QString("undo transaction begin: count=%1 index=%2")
				 .arg(undoCountBefore)
				 .arg(undoIndexBefore));
	undoStack->beginMacro(QObject::tr("Breadboard autoroute"));
	int placed = autoplacePartsOnBreadboard();
	logAutoroute(QString("undo transaction after placement: count=%1 index=%2")
				 .arg(undoStack->count())
				 .arg(undoStack->index()));
	logAutoroute(QString("autoplace complete: placed=%1").arg(placed));
	if (placed > 0)
	{
		clearCollectedNets();
		indexer.clear();
		m_sketchWidget->collectAllNets(indexer, m_allPartConnectorItems, false, false, false);
		logAutoroute(QString("collectAllNets after placement: nets=%1 indexer=%2").arg(m_allPartConnectorItems.count()).arg(indexer.count()));
		Q_EMIT setMaximumProgress(m_allPartConnectorItems.count());
	}

	Q_EMIT setProgressMessage(QObject::tr("Routing breadboard jumpers..."));

	auto *parentCommand = new QUndoCommand(QObject::tr("Route breadboard jumpers"));

	int created = routeCollectedNets(parentCommand);
	logAutoroute(QString("route complete: createdWires=%1").arg(created));

	Q_EMIT setProgressValue(m_allPartConnectorItems.count());

	if (placed <= 0 && created <= 0)
	{
		delete parentCommand;
		undoStack->endMacro();
		logAutoroute(QString("undo transaction empty end: count=%1 index=%2 delta=%3")
					 .arg(undoStack->count())
					 .arg(undoStack->index())
					 .arg(undoStack->count() - undoCountBefore));
		logAutoroute("abort: no valid placement or route");
		if (!m_lastPlacementReport.isEmpty())
			logAutoroute(QString("placement report:\n%1").arg(m_lastPlacementReport));
		QMessageBox messageBox(QMessageBox::Information,
							   QObject::tr("Fritzing"),
							   QObject::tr("Breadboard autoroute did not find a valid placement or route."));
		if (!m_lastPlacementReport.isEmpty())
		{
			messageBox.setDetailedText(m_lastPlacementReport);
		}
		messageBox.exec();
		return;
	}

	new CleanUpRatsnestsCommand(m_sketchWidget, CleanUpWiresCommand::RedoOnly, parentCommand);
	new CleanUpWiresCommand(m_sketchWidget, CleanUpWiresCommand::RedoOnly, parentCommand);
	undoStack->push(parentCommand);
	logAutoroute(QString("undo transaction after routing: count=%1 index=%2")
				 .arg(undoStack->count())
				 .arg(undoStack->index()));
	undoStack->endMacro();
	logAutoroute(QString("undo transaction end: count=%1 index=%2 delta=%3")
				 .arg(undoStack->count())
				 .arg(undoStack->index())
				 .arg(undoStack->count() - undoCountBefore));
	m_lastRoutingScore.failedNets = countUnresolvedNets();
	logAutoroute(QString("benchmark: %1 elapsedMs=%2")
				 .arg(m_lastRoutingScore.toString())
				 .arg(elapsed.elapsed()));

	Q_EMIT setProgressMessage2(QObject::tr("Placed %1 part(s), created %2 breadboard jumper wire(s).").arg(placed).arg(created));
	logAutoroute(QString("success: placed=%1 createdWires=%2").arg(placed).arg(created));
	logAutoroute("========== breadboard autoroute end ==========");
}

int BreadboardAutorouter::clearPreviousAutorouteWires()
{
	QList<Wire *> generatedWires;
	QList<Wire *> normalBreadboardWires;
	int ratsnestCount = 0;
	Q_FOREACH (QGraphicsItem *graphicsItem, m_sketchWidget->scene()->items())
	{
		auto *wire = dynamic_cast<Wire *>(graphicsItem);
		if (wire == nullptr)
			continue;

		if (wire->getRatsnest())
		{
			ratsnestCount++;
			continue;
		}
		if (!wire->getNormal())
			continue;
		if (wire->viewID() != ViewLayer::BreadboardView)
			continue;

		ConnectorItem *from = wire->connector0() == nullptr ? nullptr : wire->connector0()->firstConnectedToIsh();
		ConnectorItem *to = wire->connector1() == nullptr ? nullptr : wire->connector1()->firstConnectedToIsh();
		const bool touchesBreadboard = connectedBreadboardHoleFor(from) != nullptr
		                            || connectedBreadboardHoleFor(to) != nullptr
		                            || isTargetBreadboardHole(from)
		                            || isTargetBreadboardHole(to);
		if (!touchesBreadboard)
			continue;

		normalBreadboardWires.append(wire);
		if (wire->getAutoroutable() || wire->hasFlag(ViewGeometry::AutoroutableFlag))
			generatedWires.append(wire);
	}

	if (generatedWires.isEmpty() && ratsnestCount == 0)
	{
		generatedWires = normalBreadboardWires;
	}
	else if (!generatedWires.isEmpty() && ratsnestCount == 0)
	{
		Q_FOREACH (Wire *wire, normalBreadboardWires)
		{
			if (!generatedWires.contains(wire))
				generatedWires.append(wire);
		}
	}

	QSet<Wire *> unique;
	QList<Wire *> uniqueGeneratedWires;
	Q_FOREACH (Wire *wire, generatedWires)
	{
		if (wire == nullptr || unique.contains(wire))
			continue;
		unique.insert(wire);
		uniqueGeneratedWires.append(wire);
	}
	generatedWires = uniqueGeneratedWires;

	if (generatedWires.isEmpty())
	{
		logAutoroute(QString("clear previous: none ratsnests=%1 normalBreadboardWires=%2")
		             .arg(ratsnestCount)
		             .arg(normalBreadboardWires.count()));
		return 0;
	}

	auto *parentCommand = new QUndoCommand(QObject::tr("Clear breadboard autoroute"));
	new CleanUpWiresCommand(m_sketchWidget, CleanUpWiresCommand::UndoOnly, parentCommand);
	new CleanUpRatsnestsCommand(m_sketchWidget, CleanUpWiresCommand::UndoOnly, parentCommand);
	m_sketchWidget->makeWiresChangeConnectionCommands(generatedWires, parentCommand);
	Q_FOREACH (Wire *wire, generatedWires)
	{
		m_sketchWidget->makeDeleteItemCommand(wire, BaseCommand::SingleView, parentCommand);
	}
	new CleanUpRatsnestsCommand(m_sketchWidget, CleanUpWiresCommand::RedoOnly, parentCommand);
	new CleanUpWiresCommand(m_sketchWidget, CleanUpWiresCommand::RedoOnly, parentCommand);
	m_sketchWidget->undoStack()->push(parentCommand);

	logAutoroute(QString("clear previous: removed=%1 ratsnests=%2 normalBreadboardWires=%3")
	             .arg(generatedWires.count())
	             .arg(ratsnestCount)
	             .arg(normalBreadboardWires.count()));
	return generatedWires.count();
}

int BreadboardAutorouter::autoplacePartsOnBreadboard()
{
	m_lastPlacementReport.clear();

	QList<ItemBase *> parts;
	m_sketchWidget->collectParts(parts);
	logAutoroute(QString("autoplace: visible parts collected=%1").arg(parts.count()));
	if (parts.isEmpty())
	{
		m_lastPlacementReport = QObject::tr("No breadboard-view parts were found.");
		logAutoroute("autoplace abort: no parts");
		return 0;
	}

	QList<ConnectorItem *> breadboardHoles;
	QList<ItemBase *> targetBreadboards;
	QSet<ConnectorItem *> reservedHoles;
	QList<QRectF> occupiedRects;
	QHash<ConnectorItem *, int> netForConnector;
	QHash<int, QList<ConnectorItem *>> connectorsForNet;
	QHash<ConnectorItem *, ConnectorItem *> placedTargets;
	int candidateAttempts = 0;
	int rejectedPinGeometry = 0;
	int rejectedSameBus = 0;
	int rejectedOffBoard = 0;
	int rejectedOverlap = 0;
	int acceptedCandidates = 0;
	int partsWithPlaceablePins = 0;
	int rejectedByPolicy = 0;
	int leftPeripheral = 0;
	int failedBoardFit = 0;
	int placedRigid = 0;
	int placedWithLegs = 0;

	for (int netIndex = 0; netIndex < m_allPartConnectorItems.count(); netIndex++)
	{
		QList<ConnectorItem *> *net = m_allPartConnectorItems.at(netIndex);
		if (net == nullptr)
			continue;
		Q_FOREACH (ConnectorItem *connectorItem, *net)
		{
			if (connectorItem == nullptr)
				continue;
			netForConnector.insert(connectorItem, netIndex);
			connectorsForNet[netIndex].append(connectorItem);
			ConnectorItem *breadboardHole = breadboardHoleFor(connectorItem);
			if (breadboardHole != nullptr && breadboardHole->connectorType() == Connector::Female)
			{
				placedTargets.insert(connectorItem, breadboardHole);
			}
		}
	}

	BreadboardTopology topology;
	topology.discover(m_sketchWidget->scene(), m_sketchWidget->scene()->selectedItems());
	Q_FOREACH (const QString &line, topology.diagnosticLines())
	{
		logAutoroute(line);
	}

	breadboardHoles = topology.holes();
	targetBreadboards = topology.boardItems();
	reservedHoles = topology.reservedHoles();
	QRectF targetBoardBounds = topology.bounds();
	int targetBoardConnectors = topology.itemConnectorCount();
	int targetSceneConnectors = topology.sceneConnectorCount();

	if (breadboardHoles.isEmpty())
	{
		m_lastPlacementReport = QObject::tr(
									"No target breadboard holes were found.\n"
									"Target breadboard(s): %1\n"
									"Connectors on target breadboard item(s): %2\n"
									"Scene connectors inside target board bounds: %3\n\n"
									"If both connector counts are zero, no scene item owns enough breadboard holes.")
									.arg(targetBreadboards.count())
									.arg(targetBoardConnectors)
									.arg(targetSceneConnectors);
		logAutoroute(QString("autoplace abort: no holes\n%1").arg(m_lastPlacementReport));
		return 0;
	}

	QRectF targetKeepoutBounds = targetBoardBounds.adjusted(-PlacementKeepoutMargin, -PlacementKeepoutMargin, PlacementKeepoutMargin, PlacementKeepoutMargin);

	std::sort(breadboardHoles.begin(), breadboardHoles.end(), [](ConnectorItem *a, ConnectorItem *b)
			  {
		QPointF ap = a->sceneAdjustedTerminalPoint(nullptr);
		QPointF bp = b->sceneAdjustedTerminalPoint(nullptr);
		if (!qFuzzyCompare(ap.y(), bp.y())) return ap.y() < bp.y();
		return ap.x() < bp.x(); });

	QPointF breadboardCenter;
	Q_FOREACH (ConnectorItem *hole, breadboardHoles)
	{
		breadboardCenter += hole->sceneAdjustedTerminalPoint(nullptr);
	}
	breadboardCenter /= breadboardHoles.count();

	QList<ItemBase *> movableParts;
	int skippedNull = 0;
	int skippedInvisible = 0;
	int skippedRatsnest = 0;
	int skippedBreadboard = 0;
	int skippedWire = 0;
	int skippedLocked = 0;
	int skippedAlreadyOnBreadboard = 0;
	Q_FOREACH (ItemBase *part, parts)
	{
		BreadboardPartPolicy::Decision policy = BreadboardPartPolicy::classify(part);
		QString skipReason;
		if (part == nullptr)
		{
			skippedNull++;
			skipReason = "null";
		}
		else if (policy.classification == BreadboardPartPolicy::Classification::Ignore && policy.reason == "not visible")
		{
			skippedInvisible++;
			skipReason = policy.reason;
		}
		else if (policy.classification == BreadboardPartPolicy::Classification::Ignore && policy.reason == "ratsnest")
		{
			skippedRatsnest++;
			skipReason = policy.reason;
		}
		else if (policy.classification == BreadboardPartPolicy::Classification::Ignore && policy.reason == "breadboard")
		{
			skippedBreadboard++;
			skipReason = policy.reason;
		}
		else if (policy.classification == BreadboardPartPolicy::Classification::Ignore && policy.reason == "breadboard decoration")
		{
			skippedBreadboard++;
			skipReason = policy.reason;
		}
		else if (policy.classification == BreadboardPartPolicy::Classification::Ignore && policy.reason == "wire")
		{
			skippedWire++;
			skipReason = policy.reason;
		}
		else if (policy.classification == BreadboardPartPolicy::Classification::Ignore && policy.reason == "move locked")
		{
			skippedLocked++;
			skipReason = policy.reason;
		}
		else if (policy.classification == BreadboardPartPolicy::Classification::Peripheral)
		{
			leftPeripheral++;
			skipReason = QString("peripheral: %1").arg(policy.reason);
		}
		else if (policy.classification != BreadboardPartPolicy::Classification::BoardPlaceable)
		{
			rejectedByPolicy++;
			skipReason = policy.reason;
		}
		else
		{
			bool alreadyOnBreadboard = false;
			Q_FOREACH (ConnectorItem *connectorItem, part->cachedConnectorItems())
			{
				if (connectorItem == nullptr)
					continue;
				if (!isPlaceablePin(connectorItem))
					continue;
				Q_FOREACH (ConnectorItem *connected, connectorItem->connectedToItems())
				{
					if (connected != nullptr && connected->connectorType() == Connector::Female)
					{
						alreadyOnBreadboard = true;
						break;
					}
				}
				if (alreadyOnBreadboard)
					break;
			}
			if (alreadyOnBreadboard)
			{
				skippedAlreadyOnBreadboard++;
				skipReason = "already connected to female connector";
			}
		}

		logAutoroute(QString("part policy: class=%1 reason=%2 pins=%3 bendableLegs=%4 family='%5' taxonomy='%6' package='%7' module=%8 title='%9'")
						 .arg(BreadboardPartPolicy::classificationName(policy.classification))
						 .arg(policy.reason)
						 .arg(policy.placeablePins)
						 .arg(policy.hasBendableLegs ? "yes" : "no")
						 .arg(policy.family)
						 .arg(policy.taxonomy)
						 .arg(policy.package)
						 .arg(policy.moduleID)
						 .arg(policy.title));

		if (skipReason.isEmpty())
		{
			movableParts.append(part);
			logAutoroute(QString("movable part: %1 connectors=%2 placeablePins=%3 bendableLegs=%4 bounds=[%5,%6 %7x%8]")
							 .arg(itemSummary(part))
							 .arg(part->cachedConnectorItems().count())
							 .arg(policy.placeablePins)
							 .arg(policy.hasBendableLegs ? "yes" : "no")
							 .arg(part->sceneBoundingRect().x())
							 .arg(part->sceneBoundingRect().y())
							 .arg(part->sceneBoundingRect().width())
							 .arg(part->sceneBoundingRect().height()));
		}
		else
		{
			logAutoroute(QString("skip part: reason=%1 item=%2").arg(skipReason).arg(itemSummary(part)));
		}
	}
	logAutoroute(QString("movable filter summary: movable=%1 null=%2 invisible=%3 ratsnest=%4 breadboard=%5 wire=%6 locked=%7 alreadyFemale=%8 rejectedByPolicy=%9 leftPeripheral=%10")
					 .arg(movableParts.count())
					 .arg(skippedNull)
					 .arg(skippedInvisible)
					 .arg(skippedRatsnest)
					 .arg(skippedBreadboard)
					 .arg(skippedWire)
					 .arg(skippedLocked)
					 .arg(skippedAlreadyOnBreadboard)
					 .arg(rejectedByPolicy)
					 .arg(leftPeripheral));

	Q_FOREACH (ItemBase *part, parts)
	{
		if (part == nullptr)
			continue;
		if (!part->isEverVisible())
			continue;
		ItemBase *chief = part->layerKinChief();
		if (isBreadboardItem(part) || isBreadboardItem(chief))
			continue;
		if (isBreadboardDecorationItem(part) || isBreadboardDecorationItem(chief))
			continue;
		if (targetBreadboards.contains(part) || targetBreadboards.contains(chief))
			continue;
		if (movableParts.contains(part))
			continue;

		QRectF partBounds = part->sceneBoundingRect().adjusted(-PlacementKeepoutMargin, -PlacementKeepoutMargin, PlacementKeepoutMargin, PlacementKeepoutMargin);
		if (partBounds.intersects(targetKeepoutBounds))
		{
			occupiedRects.append(partBounds);
			logAutoroute(QString("occupied rect: item=%1 bounds=[%2,%3 %4x%5]")
							 .arg(itemSummary(part))
							 .arg(partBounds.x())
							 .arg(partBounds.y())
							 .arg(partBounds.width())
							 .arg(partBounds.height()));
		}
	}
	logAutoroute(QString("occupied rects considered=%1").arg(occupiedRects.count()));

	std::sort(movableParts.begin(), movableParts.end(), [this, &netForConnector, &connectorsForNet](ItemBase *a, ItemBase *b)
			  {
		double aScore = partConnectivityScore(a, netForConnector, connectorsForNet);
		double bScore = partConnectivityScore(b, netForConnector, connectorsForNet);
		if (!qFuzzyCompare(aScore, bScore)) return aScore > bScore;

		double aArea = a == nullptr ? 0 : a->sceneBoundingRect().width() * a->sceneBoundingRect().height();
		double bArea = b == nullptr ? 0 : b->sceneBoundingRect().width() * b->sceneBoundingRect().height();
		if (!qFuzzyCompare(aArea, bArea)) return aArea > bArea;

		QPointF ap = a == nullptr ? QPointF() : a->sceneBoundingRect().center();
		QPointF bp = b == nullptr ? QPointF() : b->sceneBoundingRect().center();
		if (!qFuzzyCompare(ap.x(), bp.x())) return ap.x() < bp.x();
		return ap.y() < bp.y(); });

	QStringList placementOrder;
	Q_FOREACH (ItemBase *part, movableParts)
	{
		placementOrder.append(QString("%1 score=%2")
								  .arg(itemSummary(part))
								  .arg(partConnectivityScore(part, netForConnector, connectorsForNet)));
	}
	logAutoroute(QString("placement order: %1").arg(placementOrder.join(" || ")));

	auto *parentCommand = new QUndoCommand(QObject::tr("Autoplace breadboard parts"));
	// This command is the first child of the outer autoroute macro, so its
	// undo-only cleanup runs after both generated wires and placement
	// connections have been undone. Cleaning inside the routing child leaves
	// stale ratsnests because placement is undone later.
	new CleanUpWiresCommand(m_sketchWidget, CleanUpWiresCommand::UndoOnly, parentCommand);
	new CleanUpRatsnestsCommand(m_sketchWidget, CleanUpWiresCommand::UndoOnly, parentCommand);
	int moved = 0;

	Q_FOREACH (ItemBase *part, movableParts)
	{
		QList<ConnectorItem *> pins;
		Q_FOREACH (ConnectorItem *connectorItem, part->cachedConnectorItems())
		{
			if (connectorItem == nullptr)
				continue;
			if (isPlaceablePin(connectorItem))
				pins.append(connectorItem);
		}
		if (pins.isEmpty())
			continue;
		partsWithPlaceablePins++;
		bool canUseBendableLegPlacement = pins.count() == 2 && allPinsHaveBendableLegs(pins);
		logAutoroute(QString("placement begin: %1 placeablePins=%2 strategy=%3")
						 .arg(itemSummary(part))
						 .arg(pins.count())
						 .arg(canUseBendableLegPlacement ? "rigid-or-bendable-leg" : "rigid"));

		PlacementCandidate best;
		best.item = part;
		best.oldLoc = part->getViewGeometry().loc();

		if (!canUseBendableLegPlacement)
		{
			Q_FOREACH (ConnectorItem *anchorPin, pins)
			{
				QPointF anchorPinPos = anchorPin->sceneAdjustedTerminalPoint(nullptr);
				Q_FOREACH (ConnectorItem *anchorHole, breadboardHoles)
				{
					if (reservedHoles.contains(anchorHole))
						continue;
					candidateAttempts++;

					QPointF offset = anchorHole->sceneAdjustedTerminalPoint(nullptr) - anchorPinPos;
					QSet<ConnectorItem *> candidateReserved;
					QHash<ConnectorItem *, ConnectorItem *> pinToHole;
					bool fits = true;

					Q_FOREACH (ConnectorItem *pin, pins)
					{
						QPointF target = pin->sceneAdjustedTerminalPoint(nullptr) + offset;
						ConnectorItem *nearestHole = nullptr;
						double nearestDistance = HoleMatchTolerance;

						Q_FOREACH (ConnectorItem *hole, breadboardHoles)
						{
							if (reservedHoles.contains(hole) || candidateReserved.contains(hole))
								continue;
							double distance = QLineF(target, hole->sceneAdjustedTerminalPoint(nullptr)).length();
							if (distance <= nearestDistance)
							{
								nearestHole = hole;
								nearestDistance = distance;
							}
						}

						if (nearestHole == nullptr)
						{
							fits = false;
							break;
						}

						candidateReserved.insert(nearestHole);
						pinToHole.insert(pin, nearestHole);
					}

					if (!fits)
					{
						rejectedPinGeometry++;
						continue;
					}

					bool shortsPart = false;
					QList<ConnectorItem *> mappedHoles = pinToHole.values();
					for (int fromIndex = 0; fromIndex < mappedHoles.count(); fromIndex++)
					{
						for (int toIndex = fromIndex + 1; toIndex < mappedHoles.count(); toIndex++)
						{
							if (connectorsShareBreadboardBus(mappedHoles.at(fromIndex), mappedHoles.at(toIndex)))
							{
								shortsPart = true;
								break;
							}
						}
						if (shortsPart)
							break;
					}
					if (shortsPart)
					{
						rejectedSameBus++;
						continue;
					}

					QRectF movedBounds = part->sceneBoundingRect().translated(offset);
					QRectF movedKeepout = movedBounds.adjusted(-PlacementKeepoutMargin, -PlacementKeepoutMargin, PlacementKeepoutMargin, PlacementKeepoutMargin);
					if (!targetBoardBounds.contains(movedBounds))
					{
						rejectedOffBoard++;
						continue;
					}

					bool overlaps = false;
					Q_FOREACH (const QRectF &occupied, occupiedRects)
					{
						if (movedKeepout.intersects(occupied))
						{
							overlaps = true;
							break;
						}
					}
					if (overlaps)
					{
						rejectedOverlap++;
						continue;
					}

					acceptedCandidates++;
					double score = manhattanDistance(movedBounds.center(), breadboardCenter) * 0.15;

					Q_FOREACH (ConnectorItem *pin, pins)
					{
						int netIndex = netForConnector.value(pin, -1);
						if (netIndex < 0)
							continue;

						ConnectorItem *targetHole = pinToHole.value(pin, nullptr);
						if (targetHole == nullptr)
							continue;
						QPointF targetPos = targetHole->sceneAdjustedTerminalPoint(nullptr);

						double bestNetDistance = std::numeric_limits<double>::max();
						Q_FOREACH (ConnectorItem *other, connectorsForNet.value(netIndex))
						{
							if (other == pin)
								continue;
							ConnectorItem *otherTarget = placedTargets.value(other, nullptr);
							if (otherTarget == nullptr)
								continue;
							bestNetDistance = qMin(bestNetDistance, manhattanDistance(targetPos, otherTarget->sceneAdjustedTerminalPoint(nullptr)));
						}

						if (bestNetDistance == std::numeric_limits<double>::max())
						{
							score += manhattanDistance(targetPos, breadboardCenter) * 0.05;
						}
						else
						{
							score += bestNetDistance;
						}
					}

					if (score < best.score)
					{
						best.newLoc = best.oldLoc + offset;
						best.pinToHole = pinToHole;
						best.pinToLeg.clear();
						best.score = score;
						best.usesLegPlacement = false;
						logAutoroute(QString("placement best update: part=%1 score=%2 offset=(%3,%4) anchorHole=%5")
										 .arg(itemSummary(part))
										 .arg(score)
										 .arg(offset.x())
										 .arg(offset.y())
										 .arg(connectorSummary(anchorHole)));
					}
				}
			}
		}

		if (canUseBendableLegPlacement)
		{
			ConnectorItem *firstPin = pins.at(0);
			ConnectorItem *secondPin = pins.at(1);
			logAutoroute(QString("bendable placement search: %1 pins=[%2 | %3]")
							 .arg(itemSummary(part))
							 .arg(connectorSummary(firstPin))
							 .arg(connectorSummary(secondPin)));

			Q_FOREACH (ConnectorItem *firstHole, breadboardHoles)
			{
				if (reservedHoles.contains(firstHole))
					continue;
				QPointF firstHolePos = firstHole->sceneAdjustedTerminalPoint(nullptr);
				Q_FOREACH (ConnectorItem *secondHole, breadboardHoles)
				{
					if (firstHole == secondHole)
						continue;
					if (reservedHoles.contains(secondHole))
						continue;
					candidateAttempts++;

					if (connectorsShareBreadboardBus(firstHole, secondHole))
					{
						rejectedSameBus++;
						continue;
					}

					QPointF secondHolePos = secondHole->sceneAdjustedTerminalPoint(nullptr);
					QPointF desiredCenter = (firstHolePos + secondHolePos) / 2.0;
					QPointF offset = desiredCenter - part->sceneBoundingRect().center();
					QRectF movedBounds = part->sceneBoundingRect().translated(offset);
					QRectF movedKeepout = movedBounds.adjusted(-BendablePlacementKeepoutMargin, -BendablePlacementKeepoutMargin, BendablePlacementKeepoutMargin, BendablePlacementKeepoutMargin);

					if (!targetBoardBounds.contains(movedBounds))
					{
						rejectedOffBoard++;
						continue;
					}

					bool overlaps = false;
					Q_FOREACH (const QRectF &occupied, occupiedRects)
					{
						if (movedKeepout.intersects(occupied))
						{
							overlaps = true;
							break;
						}
					}
					if (overlaps)
					{
						rejectedOverlap++;
						continue;
					}

					QPointF movedFirstPin = firstPin->sceneAdjustedTerminalPoint(nullptr) + offset;
					QPointF movedSecondPin = secondPin->sceneAdjustedTerminalPoint(nullptr) + offset;
					double firstLegLength = QLineF(movedFirstPin, firstHolePos).length();
					double secondLegLength = QLineF(movedSecondPin, secondHolePos).length();
					if (firstLegLength > MaxBendableLegLength || secondLegLength > MaxBendableLegLength)
					{
						rejectedPinGeometry++;
						continue;
					}

					acceptedCandidates++;
					double score = manhattanDistance(movedBounds.center(), breadboardCenter) * 0.15 + firstLegLength + secondLegLength;
					QHash<ConnectorItem *, ConnectorItem *> pinToHole;
					pinToHole.insert(firstPin, firstHole);
					pinToHole.insert(secondPin, secondHole);

					Q_FOREACH (ConnectorItem *pin, pins)
					{
						int netIndex = netForConnector.value(pin, -1);
						if (netIndex < 0)
							continue;

						ConnectorItem *targetHole = pinToHole.value(pin, nullptr);
						if (targetHole == nullptr)
							continue;
						QPointF targetPos = targetHole->sceneAdjustedTerminalPoint(nullptr);

						double bestNetDistance = std::numeric_limits<double>::max();
						Q_FOREACH (ConnectorItem *other, connectorsForNet.value(netIndex))
						{
							if (other == pin)
								continue;
							ConnectorItem *otherTarget = placedTargets.value(other, nullptr);
							if (otherTarget == nullptr)
								continue;
							bestNetDistance = qMin(bestNetDistance, manhattanDistance(targetPos, otherTarget->sceneAdjustedTerminalPoint(nullptr)));
						}

						if (bestNetDistance == std::numeric_limits<double>::max())
						{
							score += manhattanDistance(targetPos, breadboardCenter) * 0.05;
						}
						else
						{
							score += bestNetDistance;
						}
					}

					if (score < best.score)
					{
						best.newLoc = best.oldLoc + offset;
						best.pinToHole = pinToHole;
						best.pinToLeg.clear();
						best.pinToLeg.insert(firstPin, translatedLegForTarget(firstPin, offset, firstHole));
						best.pinToLeg.insert(secondPin, translatedLegForTarget(secondPin, offset, secondHole));
						best.score = score;
						best.usesLegPlacement = true;
						logAutoroute(QString("bendable placement best update: part=%1 score=%2 offset=(%3,%4) holes=[%5 | %6] legLengths=[%7,%8]")
										 .arg(itemSummary(part))
										 .arg(score)
										 .arg(offset.x())
										 .arg(offset.y())
										 .arg(connectorSummary(firstHole))
										 .arg(connectorSummary(secondHole))
										 .arg(firstLegLength)
										 .arg(secondLegLength));
					}
				}
			}
		}

		if (best.pinToHole.isEmpty())
		{
			logAutoroute(QString("placement no candidate: %1").arg(itemSummary(part)));
			failedBoardFit++;
			continue;
		}

		ViewGeometry oldGeometry(part->getViewGeometry());
		ViewGeometry newGeometry(part->getViewGeometry());
		newGeometry.setLoc(best.newLoc);
		new MoveItemCommand(m_sketchWidget, part->id(), oldGeometry, newGeometry, false, parentCommand);
		logAutoroute(QString("placement accepted: %1 old=(%2,%3) new=(%4,%5) score=%6")
						 .arg(itemSummary(part))
						 .arg(best.oldLoc.x())
						 .arg(best.oldLoc.y())
						 .arg(best.newLoc.x())
						 .arg(best.newLoc.y())
						 .arg(best.score));

		for (auto it = best.pinToHole.constBegin(); it != best.pinToHole.constEnd(); ++it)
		{
			ConnectorItem *pin = it.key();
			ConnectorItem *hole = it.value();
			if (pin == nullptr || hole == nullptr)
				continue;
			new ChangeConnectionCommand(m_sketchWidget, BaseCommand::CrossView,
										pin->attachedToID(), pin->connectorSharedID(),
										hole->attachedToID(), hole->connectorSharedID(),
										ViewLayer::specFromID(hole->attachedToViewLayerID()),
										true, parentCommand);
			logAutoroute(QString("placement connection: pin=%1 hole=%2 sameBus?=%3")
							 .arg(connectorSummary(pin))
							 .arg(connectorSummary(hole))
							 .arg(connectorsShareBreadboardBus(pin, hole) ? "yes" : "no"));
			reservedHoles.insert(hole);
			placedTargets.insert(pin, hole);

			QPolygonF newLeg = best.pinToLeg.value(pin);
			if (best.usesLegPlacement && newLeg.count() >= 2)
			{
				m_componentLeadLength += polylineLength(newLeg);
				QPolygonF oldLeg = pin->sceneAdjustedLeg();
				QPolygonF movedOldLeg;
				QPointF offset = best.newLoc - best.oldLoc;
				Q_FOREACH (QPointF point, oldLeg)
				{
					movedOldLeg << point + offset;
				}
				auto *legCommand = new ChangeLegCommand(m_sketchWidget,
														pin->attachedToID(),
														pin->connectorSharedID(),
														movedOldLeg,
														newLeg,
														false,
														true,
														"breadboard autoroute",
														parentCommand);
				legCommand->setSimple();
				logAutoroute(QString("placement leg: pin=%1 points=%2")
								 .arg(connectorSummary(pin))
								 .arg(newLeg.count()));
			}
		}

		occupiedRects.append(part->sceneBoundingRect().translated(best.newLoc - best.oldLoc).adjusted(-PlacementKeepoutMargin, -PlacementKeepoutMargin, PlacementKeepoutMargin, PlacementKeepoutMargin));
		if (best.usesLegPlacement)
			placedWithLegs++;
		else
			placedRigid++;
		moved++;
	}

	if (moved <= 0)
	{
		delete parentCommand;
		m_lastPlacementReport = QObject::tr(
									"Target breadboard(s): %1\n"
									"Breadboard holes considered: %2\n"
									"Scene connectors inside target board bounds: %3\n"
									"Visible parts: %4\n"
									"Movable parts after filters: %5\n"
									"Movable parts with placeable pins: %6\n"
									"Placement candidates tried: %7\n"
									"Rejected because pins did not line up with free holes: %8\n"
									"Rejected because one part would be shorted on the same breadboard bus: %9\n"
									"Rejected because placement was outside target board: %10\n"
									"Rejected because placement overlapped another part: %11\n"
									"Accepted candidate placements: %12\n"
									"Rejected by policy: %13\n"
									"Left as peripheral: %14\n"
									"Failed board fit: %15\n\n"
									"If pin-geometry rejections dominate, the next implementation needs bendable-leg placement instead of whole-SVG pin alignment.")
									.arg(targetBreadboards.count())
									.arg(breadboardHoles.count())
									.arg(targetSceneConnectors)
									.arg(parts.count())
									.arg(movableParts.count())
									.arg(partsWithPlaceablePins)
									.arg(candidateAttempts)
									.arg(rejectedPinGeometry)
									.arg(rejectedSameBus)
									.arg(rejectedOffBoard)
									.arg(rejectedOverlap)
									.arg(acceptedCandidates)
									.arg(rejectedByPolicy)
									.arg(leftPeripheral)
									.arg(failedBoardFit);
		logAutoroute(QString("autoplace failed:\n%1").arg(m_lastPlacementReport));
		return 0;
	}

	m_sketchWidget->undoStack()->push(parentCommand);
	logAutoroute(QString("autoplace counters: moved=%1 placedRigid=%2 placedWithLegs=%3 candidateAttempts=%4 acceptedCandidates=%5 rejectedPinGeometry=%6 rejectedSameBus=%7 rejectedOffBoard=%8 rejectedOverlap=%9 rejectedByPolicy=%10 leftPeripheral=%11 failedBoardFit=%12")
					 .arg(moved)
					 .arg(placedRigid)
					 .arg(placedWithLegs)
					 .arg(candidateAttempts)
					 .arg(acceptedCandidates)
					 .arg(rejectedPinGeometry)
					 .arg(rejectedSameBus)
					 .arg(rejectedOffBoard)
					 .arg(rejectedOverlap)
					 .arg(rejectedByPolicy)
					 .arg(leftPeripheral)
					 .arg(failedBoardFit));
	return moved;
}

int BreadboardAutorouter::routeRatsnestDemands(QUndoCommand *parentCommand)
{
	BreadboardTopology topology;
	topology.discover(m_sketchWidget->scene(), m_sketchWidget->scene()->selectedItems());
	const QList<ConnectorItem *> routeHoles = topology.holes();
	QSet<ConnectorItem *> reservedHoles = topology.reservedHoles();
	const HoleBounds holeBounds = boundsForHoles(routeHoles);
	QList<QLineF> plannedSegments;
	QList<Wire *> demands;

	Q_FOREACH (QGraphicsItem *graphicsItem, m_sketchWidget->scene()->items())
	{
		auto *wire = dynamic_cast<Wire *>(graphicsItem);
		if (wire != nullptr && wire->getRatsnest()) demands.append(wire);
	}

	std::sort(demands.begin(), demands.end(), [](Wire *first, Wire *second) {
		if (first == nullptr || second == nullptr) return first != nullptr;
		const double firstLength = QLineF(first->connector0()->sceneAdjustedTerminalPoint(nullptr),
		                                  first->connector1()->sceneAdjustedTerminalPoint(nullptr)).length();
		const double secondLength = QLineF(second->connector0()->sceneAdjustedTerminalPoint(nullptr),
		                                   second->connector1()->sceneAdjustedTerminalPoint(nullptr)).length();
		return firstLength < secondLength;
	});

	int created = 0;
	int failed = 0;
	auto addWire = [&](ConnectorItem *from, ConnectorItem *to) {
		if (from == nullptr || to == nullptr || from == to) return;
		m_sketchWidget->createWire(from, to, generatedWireFlags(), false, BaseCommand::SingleView, parentCommand);
		plannedSegments.append(connectorLine(from, to));
		created++;
	};
	auto applyGraphRoute = [&](const BreadboardRouteGraph::Result &route) {
		Q_FOREACH (const BreadboardRouteGraph::Segment &segment, route.segments)
		{
			addWire(segment.from, segment.to);
			reservedHoles.insert(segment.from);
			reservedHoles.insert(segment.to);
		}
	};
	auto chooseEntry = [&](ConnectorItem *terminal, const QSet<ConnectorItem *> &extraReserved) {
		ConnectorItem *best = nullptr;
		double bestScore = std::numeric_limits<double>::max();
		Q_FOREACH (ConnectorItem *hole, routeHoles)
		{
			if (hole == nullptr || reservedHoles.contains(hole) || extraReserved.contains(hole)) continue;
			if (hole->connectionsCount() != 0) continue;
			const double score = boardEdgeEntryScore(terminal, hole, holeBounds);
			if (score < bestScore) {
				best = hole;
				bestScore = score;
			}
		}
		return best;
	};

	logAutoroute(QString("ratsnest demands: %1").arg(demands.count()));
	Q_FOREACH (Wire *demand, demands)
	{
		const int createdBeforeDemand = created;
		ConnectorItem *fromPart = demand->connector0() == nullptr ? nullptr : demand->connector0()->firstConnectedToIsh();
		ConnectorItem *toPart = demand->connector1() == nullptr ? nullptr : demand->connector1()->firstConnectedToIsh();
		ConnectorItem *fromHole = breadboardHoleFor(fromPart);
		ConnectorItem *toHole = breadboardHoleFor(toPart);
		logAutoroute(QString("ratsnest demand begin: from=[%1] fromHole=[%2] to=[%3] toHole=[%4]")
					 .arg(connectorSummary(fromPart))
					 .arg(connectorSummary(fromHole))
					 .arg(connectorSummary(toPart))
					 .arg(connectorSummary(toHole)));

		if (fromPart == nullptr || toPart == nullptr)
		{
			failed++;
			logAutoroute("ratsnest demand failed: missing endpoint");
			continue;
		}

		if (fromHole != nullptr && toHole != nullptr)
		{
			BreadboardRouteGraph graph(routeHoles, reservedHoles, routeGraphOptions(plannedSegments));
			BreadboardRouteGraph::Result route = graph.route(fromHole, toHole);
			if (!route.found) {
				failed++;
				logAutoroute(QString("ratsnest demand failed: no board route from=[%1] to=[%2]")
							 .arg(connectorSummary(fromPart), connectorSummary(toPart)));
				continue;
			}
			applyGraphRoute(route);
			logAutoroute(QString("ratsnest demand complete: wires=%1").arg(created - createdBeforeDemand));
			continue;
		}

		if (fromHole == nullptr && toHole == nullptr)
		{
			ConnectorItem *fromEntry = chooseEntry(fromPart, QSet<ConnectorItem *>());
			ConnectorItem *toEntry = nearestFreeBusHole(fromEntry);
			if (fromEntry == nullptr || toEntry == nullptr) {
				failed++;
				logAutoroute("ratsnest demand failed: no entries for off-board pair");
				continue;
			}
			addWire(fromPart, fromEntry);
			addWire(toPart, toEntry);
			reservedHoles.insert(fromEntry);
			reservedHoles.insert(toEntry);
			logAutoroute(QString("ratsnest demand complete: wires=%1").arg(created - createdBeforeDemand));
			continue;
		}

		ConnectorItem *terminal = fromHole == nullptr ? fromPart : toPart;
		ConnectorItem *targetHole = fromHole == nullptr ? toHole : fromHole;
		BreadboardPartPolicy::Decision terminalPolicy = BreadboardPartPolicy::classify(terminal->attachedTo()->layerKinChief());
		if (terminalPolicy.classification != BreadboardPartPolicy::Classification::Peripheral) {
			failed++;
			logAutoroute(QString("ratsnest demand failed: board-placeable endpoint has no hole [%1]")
						 .arg(connectorSummary(terminal)));
			continue;
		}

		if (targetHole == nullptr) {
			failed++;
			logAutoroute(QString("ratsnest demand failed: no peripheral target [%1]").arg(connectorSummary(terminal)));
			continue;
		}
		addWire(terminal, targetHole);
		reservedHoles.insert(targetHole);
		logAutoroute(QString("ratsnest demand complete: wires=%1").arg(created - createdBeforeDemand));
	}

	double jumperLength = 0.0;
	Q_FOREACH (const QLineF &segment, plannedSegments) jumperLength += segment.length();
	m_lastRoutingScore.failedNets = failed;
	m_lastRoutingScore.jumperCount = created;
	m_lastRoutingScore.jumperLength = jumperLength;
	m_lastRoutingScore.componentLeadLength = m_componentLeadLength;
	logAutoroute(QString("ratsnest route result: demands=%1 failed=%2 created=%3 length=%4")
				 .arg(demands.count()).arg(failed).arg(created).arg(jumperLength));
	return created;
}

int BreadboardAutorouter::routeCollectedNets(QUndoCommand *parentCommand)
{
	int created = 0;
	int wiredPeripheral = 0;
	int allocatedPeripheralLanes = 0;

	BreadboardTopology topology;
	topology.discover(m_sketchWidget->scene(), m_sketchWidget->scene()->selectedItems());
	QList<ConnectorItem *> routeHoles = topology.holes();
	QSet<ConnectorItem *> routeReservedHoles = topology.reservedHoles();
	const HoleBounds routeHoleBounds = boundsForHoles(routeHoles);
	QList<QLineF> plannedSegments;
	QSet<int> failedNetIndices;
	const BreadboardRouteGraph::Options activeOptions = BreadboardRouteGraph::Options::fromEnvironment();
	logAutoroute(QString("route options: maxJumperLength=%1 candidatesPerBusPair=%2 crossingPenalty=%3 overlapPenalty=%4")
				 .arg(activeOptions.maxJumperLength)
				 .arg(activeOptions.candidatesPerBusPair)
				 .arg(activeOptions.crossingPenalty)
				 .arg(activeOptions.overlapPenalty));

	QList<int> routeOrder;
	for (int netIndex = 0; netIndex < m_allPartConnectorItems.count(); netIndex++)
	{
		routeOrder.append(netIndex);
	}
	std::sort(routeOrder.begin(), routeOrder.end(), [this](int firstIndex, int secondIndex) {
		auto difficulty = [this](QList<ConnectorItem *> *net) {
			if (net == nullptr) return 0.0;
			const QList<ConnectorItem *> candidates = routingCandidatesForSubnet(*net);
			const int groups = collectCandidateGroups(candidates).count();
			QRectF bounds;
			Q_FOREACH (ConnectorItem *candidate, candidates) {
				if (candidate == nullptr) continue;
				const QRectF point(candidate->sceneAdjustedTerminalPoint(nullptr), QSizeF(1.0, 1.0));
				bounds = bounds.isNull() ? point : bounds | point;
			}
			return groups * 100000.0 + candidates.count() * 1000.0 + bounds.width() + bounds.height();
		};
		return difficulty(m_allPartConnectorItems.value(firstIndex)) > difficulty(m_allPartConnectorItems.value(secondIndex));
	});
	QStringList routeOrderText;
	Q_FOREACH (int netIndex, routeOrder) routeOrderText.append(QString::number(netIndex));
	logAutoroute(QString("route order (most constrained first): %1").arg(routeOrderText.join(",")));

	for (int orderIndex = 0; orderIndex < routeOrder.count(); orderIndex++)
	{
		Q_EMIT setProgressValue(orderIndex);
		const int i = routeOrder.at(orderIndex);

		QList<ConnectorItem *> *net = m_allPartConnectorItems.at(i);
		if (net == nullptr)
			continue;

		QList<ConnectorItem *> candidates = routingCandidatesForSubnet(*net);
		QList<QList<ConnectorItem *>> groups = collectCandidateGroups(candidates);
		logAutoroute(QString("route net %1: netConnectors=%2 candidates=%3 groups=%4")
						 .arg(i)
						 .arg(net->count())
						 .arg(candidates.count())
						 .arg(groups.count()));
		for (int groupIndex = 0; groupIndex < groups.count(); groupIndex++)
		{
			QStringList connectorLines;
			QList<ConnectorItem *> group = groups.at(groupIndex);
			for (int connectorIndex = 0; connectorIndex < group.count() && connectorIndex < 6; connectorIndex++)
			{
				connectorLines.append(connectorSummary(group.at(connectorIndex)));
			}
			logAutoroute(QString("route net %1 group %2 size=%3 sample=[%4]")
							 .arg(i)
							 .arg(groupIndex)
							 .arg(group.count())
							 .arg(connectorLines.join(" | ")));
		}

		QList<ConnectorItem *> breadboardAnchors;
		QList<ConnectorItem *> offBoardTerminals;
		Q_FOREACH (ConnectorItem *connectorItem, *net)
		{
			if (connectorItem == nullptr)
				continue;
			ItemBase *itemBase = connectorItem->attachedTo();
			if (itemBase == nullptr)
				continue;
			if (!itemBase->isEverVisible())
				continue;
			if (itemBase->getRatsnest())
				continue;
			if (connectorItem->attachedToItemType() == ModelPart::Wire)
				continue;
			if (!isPlaceablePin(connectorItem) && connectorItem->connectorType() != Connector::Female)
				continue;

			ConnectorItem *connectedHole = connectedBreadboardHoleFor(connectorItem);
			if (connectedHole != nullptr)
			{
				if (!breadboardAnchors.contains(connectedHole))
					breadboardAnchors.append(connectedHole);
				continue;
			}

			BreadboardPartPolicy::Decision policy = BreadboardPartPolicy::classify(itemBase->layerKinChief());
			if (policy.classification == BreadboardPartPolicy::Classification::Peripheral && connectorItem->connectorType() != Connector::Female && !offBoardTerminals.contains(connectorItem))
			{
				offBoardTerminals.append(connectorItem);
				logAutoroute(QString("route peripheral terminal: net=%1 terminal=%2 class=%3 reason=%4")
								 .arg(i)
								 .arg(connectorSummary(connectorItem))
								 .arg(BreadboardPartPolicy::classificationName(policy.classification))
								 .arg(policy.reason));
			}
			else if (policy.classification == BreadboardPartPolicy::Classification::BoardPlaceable && connectorItem->connectorType() != Connector::Female)
			{
				logAutoroute(QString("route skip board-placeable offboard terminal: net=%1 terminal=%2 reason=not a peripheral")
								 .arg(i)
								 .arg(connectorSummary(connectorItem)));
			}
		}

		if (breadboardAnchors.isEmpty() && offBoardTerminals.count() > 1)
		{
			QList<ConnectorItem *> terminalEntries;
			QSet<ConnectorItem *> usedEntries;
			QList<QLineF> entryLeadSegments = plannedSegments;
			bool entriesReady = true;

			Q_FOREACH (ConnectorItem *terminal, offBoardTerminals)
			{
				ConnectorItem *bestEntry = nullptr;
				double bestEntryScore = std::numeric_limits<double>::max();
				Q_FOREACH (ConnectorItem *entry, routeHoles)
				{
					if (entry == nullptr || routeReservedHoles.contains(entry) || usedEntries.contains(entry))
						continue;
					if (entry->connectionsCount() > 0)
						continue;
					const double score = boardEdgeEntryScore(terminal, entry, routeHoleBounds)
					                   + leadCongestionPenalty(terminal, entry, entryLeadSegments);
					if (score < bestEntryScore)
					{
						bestEntry = entry;
						bestEntryScore = score;
					}
				}
				if (bestEntry == nullptr)
				{
					entriesReady = false;
					failedNetIndices.insert(i);
					logAutoroute(QString("route peripheral entry skipped: net=%1 terminal=%2 no free edge entry")
									 .arg(i)
									 .arg(connectorSummary(terminal)));
					break;
				}
				terminalEntries.append(bestEntry);
				usedEntries.insert(bestEntry);
				entryLeadSegments.append(connectorLine(terminal, bestEntry));
				logAutoroute(QString("route peripheral entry: net=%1 terminal=%2 entry=%3 score=%4")
								 .arg(i)
								 .arg(connectorSummary(terminal))
								 .arg(connectorSummary(bestEntry))
								 .arg(bestEntryScore));
			}

			QList<BreadboardRouteGraph::Result> entryRoutes;
			if (entriesReady)
			{
				QSet<ConnectorItem *> temporaryReserved = routeReservedHoles;
				QList<QLineF> entryPlanningSegments = plannedSegments;
				Q_FOREACH (ConnectorItem *entry, terminalEntries)
				{
					temporaryReserved.insert(entry);
				}

				QList<ConnectorItem *> connectedEntries;
				if (!terminalEntries.isEmpty())
					connectedEntries.append(terminalEntries.first());

				for (int targetIndex = 1; targetIndex < terminalEntries.count(); targetIndex++)
				{
					ConnectorItem *targetEntry = terminalEntries.at(targetIndex);
					BreadboardRouteGraph::Result bestRoute;
					BreadboardRouteGraph routeGraph(routeHoles, temporaryReserved, routeGraphOptions(entryPlanningSegments));
					Q_FOREACH (ConnectorItem *connectedEntry, connectedEntries)
					{
						BreadboardRouteGraph::Result route = routeGraph.route(connectedEntry, targetEntry);
						if (!route.found)
							continue;
						if (routeIsBetter(route, bestRoute))
						{
							bestRoute = route;
						}
					}
					if (!bestRoute.found)
					{
						entriesReady = false;
						failedNetIndices.insert(i);
						logAutoroute(QString("route peripheral entries skipped: net=%1 entry=%2 no graph route")
										 .arg(i)
										 .arg(connectorSummary(targetEntry)));
						break;
					}
					entryRoutes.append(bestRoute);
					Q_FOREACH (const BreadboardRouteGraph::Segment &segment, bestRoute.segments) {
						entryPlanningSegments.append(connectorLine(segment.from, segment.to));
						temporaryReserved.insert(segment.from);
						temporaryReserved.insert(segment.to);
					}
					connectedEntries.append(targetEntry);
				}
			}

			if (entriesReady && terminalEntries.count() == offBoardTerminals.count())
			{
				allocatedPeripheralLanes++;
				logAutoroute(QString("route peripheral entries: net=%1 terminals=%2 routes=%3")
								 .arg(i)
								 .arg(offBoardTerminals.count())
								 .arg(entryRoutes.count()));
				for (int terminalIndex = 0; terminalIndex < offBoardTerminals.count(); terminalIndex++)
				{
					ConnectorItem *terminal = offBoardTerminals.at(terminalIndex);
					ConnectorItem *target = terminalEntries.at(terminalIndex);
					if (terminal == nullptr || target == nullptr)
						continue;
					m_sketchWidget->createWire(terminal, target, generatedWireFlags(), false, BaseCommand::SingleView, parentCommand);
					plannedSegments.append(connectorLine(terminal, target));
					routeReservedHoles.insert(target);
					created++;
					wiredPeripheral++;
				}
				Q_FOREACH (const BreadboardRouteGraph::Result &route, entryRoutes)
				{
					Q_FOREACH (const BreadboardRouteGraph::Segment &segment, route.segments)
					{
						if (segment.from == nullptr || segment.to == nullptr || segment.from == segment.to)
							continue;
						m_sketchWidget->createWire(segment.from, segment.to, generatedWireFlags(), false, BaseCommand::SingleView, parentCommand);
						plannedSegments.append(connectorLine(segment.from, segment.to));
						routeReservedHoles.insert(segment.from);
						routeReservedHoles.insert(segment.to);
						created++;
					}
				}
				breadboardAnchors.append(terminalEntries.first());
				offBoardTerminals.clear();
			}
		}

		if (!breadboardAnchors.isEmpty() && !offBoardTerminals.isEmpty())
		{
			QSet<ConnectorItem *> usedBridgeTargets;
			Q_FOREACH (ConnectorItem *terminal, offBoardTerminals)
			{
				ConnectorItem *bestAnchor = nullptr;
				ConnectorItem *bestEntry = nullptr;
				BreadboardRouteGraph::Result bestRoute;
				BreadboardRoutingScore bestBridgeScore;
				bool haveBridgeScore = false;
				BreadboardRouteGraph routeGraph(routeHoles, routeReservedHoles, routeGraphOptions(plannedSegments));

				Q_FOREACH (ConnectorItem *entry, routeHoles)
				{
					if (entry == nullptr || routeReservedHoles.contains(entry) || usedBridgeTargets.contains(entry))
						continue;
					if (entry->connectionsCount() > 0)
						continue;
					const double entryScore = boardEdgeEntryScore(terminal, entry, routeHoleBounds)
					                        + leadCongestionPenalty(terminal, entry, plannedSegments);
					if (entryScore == std::numeric_limits<double>::max())
						continue;

					Q_FOREACH (ConnectorItem *anchor, breadboardAnchors)
					{
						if (anchor == nullptr)
							continue;
						BreadboardRouteGraph::Result route = routeGraph.route(entry, anchor);
						if (!route.found)
							continue;

						BreadboardRoutingScore score = route.score;
						score.jumperCount++;
						score.jumperLength += connectorLine(terminal, entry).length();
						score.congestion += entryScore;
						if (!haveBridgeScore || score < bestBridgeScore)
						{
							bestAnchor = anchor;
							bestEntry = entry;
							bestRoute = route;
							bestBridgeScore = score;
							haveBridgeScore = true;
						}
					}
				}

				if (bestEntry == nullptr || !bestRoute.found)
				{
					failedNetIndices.insert(i);
					logAutoroute(QString("route bridge skipped: net=%1 terminal=%2 no breadboard target")
									 .arg(i)
									 .arg(connectorSummary(terminal)));
					continue;
				}

				logAutoroute(QString("route bridge: net=%1 terminal=%2 anchor=%3 entry=%4 segments=%5 score=%6")
								 .arg(i)
								 .arg(connectorSummary(terminal))
								 .arg(connectorSummary(bestAnchor))
								 .arg(connectorSummary(bestEntry))
								 .arg(bestRoute.segments.count())
							 .arg(bestBridgeScore.toString()));
				m_sketchWidget->createWire(terminal, bestEntry, generatedWireFlags(), false, BaseCommand::SingleView, parentCommand);
				plannedSegments.append(connectorLine(terminal, bestEntry));
				usedBridgeTargets.insert(bestEntry);
				routeReservedHoles.insert(bestEntry);
				created++;
				wiredPeripheral++;

				Q_FOREACH (const BreadboardRouteGraph::Segment &segment, bestRoute.segments)
				{
					if (segment.from == nullptr || segment.to == nullptr || segment.from == segment.to)
						continue;
					m_sketchWidget->createWire(segment.from, segment.to, generatedWireFlags(), false, BaseCommand::SingleView, parentCommand);
					plannedSegments.append(connectorLine(segment.from, segment.to));
					routeReservedHoles.insert(segment.from);
					routeReservedHoles.insert(segment.to);
					logAutoroute(QString("route bridge graph wire: net=%1 from=%2 to=%3 cost=%4")
									 .arg(i)
									 .arg(connectorSummary(segment.from))
									 .arg(connectorSummary(segment.to))
									 .arg(segment.cost));
					created++;
				}
			}
		}

		if (groups.count() < 2)
			continue;

		while (groups.count() > 1)
		{
			int bestFromSubnet = -1;
			int bestToSubnet = -1;
			BreadboardRouteGraph::Result bestRoute;
			BreadboardRoutingScore bestRoutingScore;
			double bestTieBreak = std::numeric_limits<double>::max();
			bool haveBestScore = false;
			BreadboardRouteGraph routeGraph(routeHoles, routeReservedHoles, routeGraphOptions(plannedSegments));

			for (int fromSubnet = 0; fromSubnet < groups.count(); fromSubnet++)
			{
				QList<ConnectorItem *> fromCandidates = groups.at(fromSubnet);
				if (fromCandidates.isEmpty())
					continue;

				for (int toSubnet = fromSubnet + 1; toSubnet < groups.count(); toSubnet++)
				{
					QList<ConnectorItem *> toCandidates = groups.at(toSubnet);
					if (toCandidates.isEmpty())
						continue;

					Q_FOREACH (ConnectorItem *fromCandidate, fromCandidates)
					{
						Q_FOREACH (ConnectorItem *toCandidate, toCandidates)
						{
							if (fromCandidate == nullptr || toCandidate == nullptr || fromCandidate == toCandidate)
								continue;
							BreadboardRouteGraph::Result route = routeGraph.route(fromCandidate, toCandidate);
							if (!route.found)
								continue;
							const double tieBreak = routeScore(fromCandidate, toCandidate);
							if (!haveBestScore || route.score < bestRoutingScore
							    || (route.score == bestRoutingScore && tieBreak < bestTieBreak))
							{
								bestRoute = route;
								bestRoutingScore = route.score;
								bestTieBreak = tieBreak;
								haveBestScore = true;
								bestFromSubnet = fromSubnet;
								bestToSubnet = toSubnet;
							}
						}
					}
				}
			}

			if (!bestRoute.found)
			{
				failedNetIndices.insert(i);
				logAutoroute(QString("route graph failed: net=%1 groups=%2 buses=%3 edges=%4")
								 .arg(i)
								 .arg(groups.count())
								 .arg(routeGraph.busCount())
								 .arg(routeGraph.edgeCount()));
				break;
			}

			logAutoroute(QString("route choose: net=%1 fromGroup=%2 toGroup=%3 segments=%4 score=%5")
							 .arg(i)
							 .arg(bestFromSubnet)
							 .arg(bestToSubnet)
							 .arg(bestRoute.segments.count())
							 .arg(bestRoutingScore.toString()));
			Q_FOREACH (const BreadboardRouteGraph::Segment &segment, bestRoute.segments)
			{
				if (segment.from == nullptr || segment.to == nullptr || segment.from == segment.to)
					continue;
				m_sketchWidget->createWire(segment.from, segment.to, generatedWireFlags(), false, BaseCommand::SingleView, parentCommand);
				plannedSegments.append(connectorLine(segment.from, segment.to));
				routeReservedHoles.insert(segment.from);
				routeReservedHoles.insert(segment.to);
				logAutoroute(QString("route graph wire: net=%1 from=%2 to=%3 cost=%4")
								 .arg(i)
								 .arg(connectorSummary(segment.from))
								 .arg(connectorSummary(segment.to))
								 .arg(segment.cost));
				created++;
			}

			groups[bestFromSubnet].append(groups.at(bestToSubnet));
			groups.removeAt(bestToSubnet);
		}
	}

	double jumperLength = 0.0;
	Q_FOREACH (const QLineF &segment, plannedSegments) jumperLength += segment.length();
	m_lastRoutingScore.failedNets = failedNetIndices.count();
	m_lastRoutingScore.jumperCount = created;
	m_lastRoutingScore.jumperLength = jumperLength;
	m_lastRoutingScore.componentLeadLength = m_componentLeadLength;
	logAutoroute(QString("route counters: wiredPeripheral=%1 allocatedPeripheralLanes=%2 failedNets=%3")
				 .arg(wiredPeripheral)
				 .arg(allocatedPeripheralLanes)
				 .arg(failedNetIndices.count()));
	return created;
}

QList<QList<ConnectorItem *>> BreadboardAutorouter::collectCandidateGroups(const QList<ConnectorItem *> &candidates) const
{
	QList<QList<ConnectorItem *>> groups;
	QList<ConnectorItem *> todo = candidates;

	while (!todo.isEmpty())
	{
		ConnectorItem *first = todo.takeFirst();
		if (first == nullptr)
			continue;

		QList<ConnectorItem *> equalPotential;
		equalPotential.append(first);
		ConnectorItem::collectEqualPotential(equalPotential, false, ViewGeometry::RatsnestFlag);

		QList<ConnectorItem *> group;
		Q_FOREACH (ConnectorItem *candidate, candidates)
		{
			if (candidate == nullptr)
				continue;
			if (!equalPotential.contains(candidate))
				continue;
			if (!group.contains(candidate))
				group.append(candidate);
			todo.removeOne(candidate);
		}

		if (!group.isEmpty())
			groups.append(group);
	}

	return groups;
}

int BreadboardAutorouter::countUnresolvedNets() const
{
	int residualRatsnests = 0;
	Q_FOREACH (QGraphicsItem *graphicsItem, m_sketchWidget->scene()->items())
	{
		auto *wire = dynamic_cast<Wire *>(graphicsItem);
		if (wire == nullptr || !wire->getRatsnest()) continue;
		residualRatsnests++;
		ConnectorItem *from = wire->connector0() == nullptr ? nullptr : wire->connector0()->firstConnectedToIsh();
		ConnectorItem *to = wire->connector1() == nullptr ? nullptr : wire->connector1()->firstConnectedToIsh();
		logAutoroute(QString("validation residual ratsnest: %1 from=[%2] to=[%3]")
					 .arg(itemSummary(wire))
					 .arg(connectorSummary(from))
					 .arg(connectorSummary(to)));
	}
	return residualRatsnests;
}

bool BreadboardAutorouter::isBreadboardItem(ItemBase *itemBase) const
{
	return BreadboardTopology::isBreadboardItem(itemBase);
}

bool BreadboardAutorouter::isBreadboardDecorationItem(ItemBase *itemBase) const
{
	return BreadboardTopology::isBreadboardDecorationItem(itemBase);
}

bool BreadboardAutorouter::isMovableBreadboardPart(ItemBase *itemBase) const
{
	BreadboardPartPolicy::Decision policy = BreadboardPartPolicy::classify(itemBase);
	return policy.classification == BreadboardPartPolicy::Classification::BoardPlaceable;
}

bool BreadboardAutorouter::isPlaceablePin(ConnectorItem *connectorItem) const
{
	if (connectorItem == nullptr)
		return false;
	Connector::ConnectorType connectorType = connectorItem->connectorType();
	return connectorType == Connector::Male || connectorType == Connector::Pad || connectorItem->isHybrid();
}

bool BreadboardAutorouter::isTargetBreadboardHole(ConnectorItem *connectorItem) const
{
	return BreadboardTopology::isTargetBreadboardHole(connectorItem);
}

bool BreadboardAutorouter::connectorsShareBreadboardBus(ConnectorItem *first, ConnectorItem *second) const
{
	return BreadboardTopology::connectorsShareBus(first, second);
}

QString BreadboardAutorouter::connectorSummary(ConnectorItem *connectorItem) const
{
	if (connectorItem == nullptr)
		return QString("<null connector>");

	QPointF p = connectorItem->sceneAdjustedTerminalPoint(nullptr);
	return QString("%1:%2 type=%3 bus=%4 at=(%5,%6) conns=%7")
		.arg(connectorItem->attachedToTitle())
		.arg(connectorItem->connectorSharedID())
		.arg(connectorItem->connectorType())
		.arg(connectorItem->busID())
		.arg(p.x())
		.arg(p.y())
		.arg(connectorItem->connectionsCount());
}

QString BreadboardAutorouter::itemSummary(ItemBase *itemBase) const
{
	if (itemBase == nullptr)
		return QString("<null item>");

	return QString("%1 id=%2 module=%3")
		.arg(itemBase->title())
		.arg(itemBase->id())
		.arg(itemBase->moduleID());
}

QString BreadboardAutorouter::logFilePath() const
{
	QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
	if (dir.isEmpty())
		dir = QDir::tempPath();
	return QDir(dir).filePath("fritzing-breadboard-autorouter.log");
}

void BreadboardAutorouter::logAutoroute(const QString &message) const
{
	QFile file(logFilePath());
	if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
		return;

	QTextStream stream(&file);
	stream << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << " " << message << '\n';
}

ConnectorItem *BreadboardAutorouter::connectedPartConnector(ConnectorItem *wireConnector) const
{
	if (wireConnector == nullptr)
		return nullptr;

	Q_FOREACH (ConnectorItem *connectorItem, wireConnector->connectedToItems())
	{
		if (connectorItem == nullptr)
			continue;
		ItemBase *itemBase = connectorItem->attachedTo();
		if (itemBase == nullptr)
			continue;
		if (!itemBase->isEverVisible())
			continue;
		if (itemBase->getRatsnest())
			continue;
		if (connectorItem->attachedToItemType() == ModelPart::Wire)
			continue;

		return connectorItem;
	}

	return nullptr;
}

ConnectorItem *BreadboardAutorouter::connectedBreadboardHoleFor(ConnectorItem *partConnector) const
{
	if (partConnector == nullptr)
		return nullptr;
	if (partConnector->connectorType() == Connector::Female)
		return partConnector;

	Q_FOREACH (ConnectorItem *connectorItem, partConnector->connectedToItems())
	{
		if (connectorItem == nullptr)
			continue;
		ItemBase *itemBase = connectorItem->attachedTo();
		if (itemBase == nullptr)
			continue;
		if (!itemBase->isEverVisible())
			continue;
		if (itemBase->getRatsnest())
			continue;
		if (connectorItem->attachedToItemType() == ModelPart::Wire)
			continue;
		if (connectorItem->connectorType() == Connector::Female)
			return connectorItem;
	}

	return nullptr;
}

ConnectorItem *BreadboardAutorouter::breadboardHoleFor(ConnectorItem *partConnector) const
{
	if (partConnector == nullptr)
		return nullptr;
	if (partConnector->connectorType() == Connector::Female)
	{
		if (partConnector->connectionsCount() == 0)
			return partConnector;
		return nearestFreeBusHole(partConnector);
	}

	Q_FOREACH (ConnectorItem *connectorItem, partConnector->connectedToItems())
	{
		if (connectorItem == nullptr)
			continue;
		ItemBase *itemBase = connectorItem->attachedTo();
		if (itemBase == nullptr)
			continue;
		if (!itemBase->isEverVisible())
			continue;
		if (itemBase->getRatsnest())
			continue;
		if (connectorItem->attachedToItemType() == ModelPart::Wire)
			continue;
		if (connectorItem->connectorType() != Connector::Female)
			continue;

		if (connectorItem->connectionsCount() == 0)
			return connectorItem;

		ConnectorItem *freeHole = nearestFreeBusHole(connectorItem);
		if (freeHole != nullptr)
			return freeHole;
	}

	return nullptr;
}

ConnectorItem *BreadboardAutorouter::nearestFreeBusHole(ConnectorItem *breadboardHole) const
{
	if (breadboardHole == nullptr)
		return nullptr;

	ItemBase *breadboard = breadboardHole->attachedTo();
	if (breadboard == nullptr)
		return nullptr;

	QList<ConnectorItem *> busHoles;
	if (!breadboard->busConnectorItems(breadboardHole, busHoles))
		return nullptr;

	ConnectorItem *nearest = nullptr;
	double nearestDistance = 0;
	QPointF from = breadboardHole->sceneAdjustedTerminalPoint(nullptr);

	Q_FOREACH (ConnectorItem *candidate, busHoles)
	{
		if (candidate == nullptr)
			continue;
		if (candidate == breadboardHole)
			continue;
		if (candidate->connectorType() != Connector::Female)
			continue;
		if (candidate->connectionsCount() != 0)
			continue;
		if (!candidate->attachedTo()->isEverVisible())
			continue;

		QPointF to = candidate->sceneAdjustedTerminalPoint(nullptr);
		double distance = QLineF(from, to).length();
		if (nearest == nullptr || distance < nearestDistance)
		{
			nearest = candidate;
			nearestDistance = distance;
		}
	}

	return nearest;
}

ConnectorItem *BreadboardAutorouter::routingConnectorFor(ConnectorItem *wireConnector) const
{
	ConnectorItem *partConnector = connectedPartConnector(wireConnector);
	ConnectorItem *breadboardHole = breadboardHoleFor(partConnector);
	return breadboardHole == nullptr ? partConnector : breadboardHole;
}

QList<QList<ConnectorItem *>> BreadboardAutorouter::collectRoutableSubnets(QList<ConnectorItem *> *net) const
{
	QList<QList<ConnectorItem *>> subnets;
	if (net == nullptr)
		return subnets;

	QList<ConnectorItem *> todo = *net;
	while (!todo.isEmpty())
	{
		ConnectorItem *first = todo.takeFirst();
		QList<ConnectorItem *> subnet;
		subnet.append(first);

		ConnectorItem::collectEqualPotential(subnet, false, ViewGeometry::RatsnestFlag);
		Q_FOREACH (ConnectorItem *connectorItem, subnet)
		{
			todo.removeOne(connectorItem);
		}

		ConnectorItem *representative = chooseRepresentative(subnet);
		if (representative != nullptr)
		{
			subnets.append(subnet);
		}
	}

	return subnets;
}

QList<ConnectorItem *> BreadboardAutorouter::routingCandidatesForSubnet(const QList<ConnectorItem *> &subnet) const
{
	QList<ConnectorItem *> candidates;

	Q_FOREACH (ConnectorItem *connectorItem, subnet)
	{
		if (connectorItem == nullptr)
			continue;
		ItemBase *itemBase = connectorItem->attachedTo();
		if (itemBase == nullptr)
			continue;
		if (!itemBase->isEverVisible())
			continue;
		if (itemBase->getRatsnest())
			continue;
		if (connectorItem->attachedToItemType() == ModelPart::Wire)
			continue;

		ConnectorItem *breadboardHole = breadboardHoleFor(connectorItem);
		if (breadboardHole == nullptr)
		{
			logAutoroute(QString("route candidate skipped off-board terminal: %1").arg(connectorSummary(connectorItem)));
			continue;
		}
		if (!candidates.contains(breadboardHole))
			candidates.append(breadboardHole);
	}

	return candidates;
}

ConnectorItem *BreadboardAutorouter::chooseRepresentative(const QList<ConnectorItem *> &subnet) const
{
	QList<ConnectorItem *> candidates = routingCandidatesForSubnet(subnet);
	return candidates.isEmpty() ? nullptr : candidates.first();
}

double BreadboardAutorouter::partConnectivityScore(ItemBase *part, const QHash<ConnectorItem *, int> &netForConnector, const QHash<int, QList<ConnectorItem *>> &connectorsForNet) const
{
	if (part == nullptr)
		return 0;

	QSet<int> seenNets;
	double score = 0;
	Q_FOREACH (ConnectorItem *connectorItem, part->cachedConnectorItems())
	{
		if (connectorItem == nullptr)
			continue;
		if (!isPlaceablePin(connectorItem))
			continue;

		int netIndex = netForConnector.value(connectorItem, -1);
		if (netIndex < 0 || seenNets.contains(netIndex))
			continue;
		seenNets.insert(netIndex);

		int visiblePinsInNet = 0;
		Q_FOREACH (ConnectorItem *netConnector, connectorsForNet.value(netIndex))
		{
			if (netConnector == nullptr)
				continue;
			ItemBase *itemBase = netConnector->attachedTo();
			if (itemBase == nullptr)
				continue;
			if (!itemBase->isEverVisible())
				continue;
			if (itemBase->getRatsnest())
				continue;
			if (netConnector->attachedToItemType() == ModelPart::Wire)
				continue;
			visiblePinsInNet++;
		}

		score += qMax(1, visiblePinsInNet);
	}

	return score;
}

double BreadboardAutorouter::routeScore(ConnectorItem *from, ConnectorItem *to) const
{
	if (from == nullptr || to == nullptr)
		return std::numeric_limits<double>::max();

	QPointF fromPos = from->sceneAdjustedTerminalPoint(nullptr);
	QPointF toPos = to->sceneAdjustedTerminalPoint(nullptr);
	double score = qAbs(fromPos.x() - toPos.x()) + qAbs(fromPos.y() - toPos.y());

	bool fromBreadboard = from->connectorType() == Connector::Female;
	bool toBreadboard = to->connectorType() == Connector::Female;
	if (fromBreadboard && toBreadboard)
		score *= 0.5;
	else if (!fromBreadboard && !toBreadboard)
		score *= 4.0;

	return score;
}

void BreadboardAutorouter::clearCollectedNets()
{
	qDeleteAll(m_allPartConnectorItems);
	m_allPartConnectorItems.clear();
}
