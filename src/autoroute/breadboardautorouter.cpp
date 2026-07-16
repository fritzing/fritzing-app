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
#include "breadboardplacementkernel.h"
#include "breadboardroutegraph.h"
#include "breadboardroutegraphcore.h"
#include "breadboardtopology.h"

#include <climits>
#include <functional>
#include <memory>

#include <QHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QSet>
#include <QSettings>
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
#include "../processeventblocker.h"
#include "../sketch/breadboardsketchwidget.h"
#include "../waitpushundostack.h"

namespace
{
	// How to swap the two pin positions of a part so crossed legs uncross
	// while every net stays on its own pin. Mirroring keeps the body visually
	// upright and is preferred when the part's fzp allows it; parts without
	// flip support (e.g. resistors, which are axially symmetric anyway) fall
	// back to a 180 degree rotation.
	enum class PinSwap
	{
		None,
		FlipHorizontal,
		FlipVertical,
		Rotate180
	};

	QPointF swappedPoint(const QPointF &point, PinSwap swap, const QPointF &center)
	{
		switch (swap)
		{
		case PinSwap::FlipHorizontal:
			return QPointF(center.x() * 2.0 - point.x(), point.y());
		case PinSwap::FlipVertical:
			return QPointF(point.x(), center.y() * 2.0 - point.y());
		case PinSwap::Rotate180:
			return center * 2.0 - point;
		default:
			return point;
		}
	}

	struct PlacementCandidate
	{
		ItemBase *item = nullptr;
		QPointF oldLoc;
		QPointF newLoc;
		QHash<ConnectorItem *, ConnectorItem *> pinToHole;
		QHash<ConnectorItem *, QPolygonF> pinToLeg;
		double score = std::numeric_limits<double>::max();
		bool usesLegPlacement = false;
		PinSwap pinSwap = PinSwap::None;
	};

	constexpr double FlipRotationDegrees = 180.0;

	// Progress is reported as a weighted percentage: placement dominates
	// wall-clock on real sketches, routing search is comparatively quick.
	constexpr int PlacementProgressSpan = 70;
	constexpr int RoutingProgressEnd = 95;

	constexpr double HoleMatchTolerance = 12.0;
	constexpr double PlacementKeepoutMargin = 8.0;
	constexpr double BendablePlacementKeepoutMargin = 1.0;

	ViewGeometry::WireFlags generatedWireFlags()
	{
		return ViewGeometry::NormalFlag | ViewGeometry::AutoroutableFlag;
	}

	QLineF connectorLine(ConnectorItem *from, ConnectorItem *to)
	{
		if (from == nullptr || to == nullptr) return QLineF();
		return QLineF(from->sceneAdjustedTerminalPoint(nullptr), to->sceneAdjustedTerminalPoint(nullptr));
	}

	// A female connector only counts as a breadboard hole when its owner IS
	// a breadboard. Breakout boards (LSM303C etc.) have female header
	// sockets too; treating those as holes turned them into phantom board
	// anchors that routing could never reach.
	bool isBreadboardHoleConnector(ConnectorItem *connectorItem)
	{
		if (connectorItem == nullptr || connectorItem->connectorType() != Connector::Female)
			return false;
		ItemBase *owner = connectorItem->attachedTo();
		if (owner == nullptr)
			return false;
		return BreadboardTopology::isBreadboardItem(owner->layerKinChief());
	}

	BreadboardRouteGraphCore::Options coreRouteOptions()
	{
		const BreadboardRouteGraph::Options env = BreadboardRouteGraph::Options::fromEnvironment();
		BreadboardRouteGraphCore::Options options;
		options.maxJumperLength = env.maxJumperLength;
		options.jumperPenalty = env.jumperPenalty;
		options.crossingPenalty = env.crossingPenalty;
		options.overlapPenalty = env.overlapPenalty;
		options.candidatesPerBusPair = env.candidatesPerBusPair;
		return options;
	}

	// Builds the static bus graph ONCE per routing pass. Everything that
	// changes as routing proceeds - reserved holes and planned segments -
	// is supplied per route() query. Results are mapped back to the old
	// ConnectorItem-based Result so call sites stay unchanged.
	struct RouteGraphSession
	{
		QList<ConnectorItem *> holes;
		QHash<ConnectorItem *, int> indexForHole;
		QVector<bool> baseBlocked;
		QVector<int> busGroupForHole;    // global bus group id per hole
		QVector<int> denseBusForHole;    // session-dense bus id per hole
		int denseBusCount = 0;
		std::function<int(int)> ownerOfBusGroup;
		std::shared_ptr<BreadboardRouteGraphCore> core;

		static RouteGraphSession build(const QList<ConnectorItem *> &routeHoles,
									   const std::function<int(ConnectorItem *)> &busGroup,
									   const std::function<int(int)> &ownerOfBusGroup)
		{
			RouteGraphSession session;
			session.holes = routeHoles;
			session.ownerOfBusGroup = ownerOfBusGroup;
			QVector<QPointF> positions(routeHoles.count());
			QVector<int> busIds(routeHoles.count(), -1);
			session.baseBlocked = QVector<bool>(routeHoles.count(), false);
			session.busGroupForHole = QVector<int>(routeHoles.count(), -1);
			QHash<int, int> denseBusFor;
			for (int i = 0; i < routeHoles.count(); i++)
			{
				ConnectorItem *hole = routeHoles.at(i);
				if (hole == nullptr)
					continue;
				session.indexForHole.insert(hole, i);
				positions[i] = hole->sceneAdjustedTerminalPoint(nullptr);
				const int group = busGroup(hole);
				session.busGroupForHole[i] = group;
				auto found = denseBusFor.constFind(group);
				if (found == denseBusFor.constEnd())
				{
					busIds[i] = denseBusFor.count();
					denseBusFor.insert(group, busIds[i]);
				}
				else
				{
					busIds[i] = found.value();
				}
				// Occupancy is stable during a pass: wire commands only
				// execute at push, after routing has finished.
				session.baseBlocked[i] = hole->connectionsCount() != 0;
			}
			session.denseBusForHole = busIds;
			session.denseBusCount = denseBusFor.count();
			session.core = std::make_shared<BreadboardRouteGraphCore>(positions, busIds, coreRouteOptions());
			return session;
		}

		// Build once per batch of route() calls sharing the same reserved
		// set, planned segments, and querying net - this replaces the old
		// per-batch graph reconstruction at a fraction of its cost. Buses
		// owned by any other key are blocked outright (prime invariant).
		BreadboardRouteGraphCore::QueryContext prepare(const QSet<ConnectorItem *> &reserved,
													   const QList<QLineF> &plannedSegments,
													   int netOwnerKey) const
		{
			QVector<bool> blocked = baseBlocked;
			Q_FOREACH (ConnectorItem *hole, reserved)
			{
				const int index = indexForHole.value(hole, -1);
				if (index >= 0)
					blocked[index] = true;
			}
			QVector<bool> busBlocked(denseBusCount, false);
			for (int i = 0; i < holes.count(); i++)
			{
				const int dense = denseBusForHole.at(i);
				if (dense < 0 || busBlocked.at(dense))
					continue;
				const int owner = ownerOfBusGroup ? ownerOfBusGroup(busGroupForHole.at(i)) : -1;
				if (owner != -1 && owner != netOwnerKey)
					busBlocked[dense] = true;
			}
			return core->prepareQuery(blocked, busBlocked, plannedSegments);
		}

		BreadboardRouteGraph::Result mapResult(const BreadboardRouteGraphCore::Result &result) const
		{
			BreadboardRouteGraph::Result mapped;
			mapped.found = result.found;
			mapped.cost = result.cost;
			mapped.score = result.score;
			mapped.reason = result.reason;
			Q_FOREACH (const BreadboardRouteGraphCore::Segment &segment, result.segments)
			{
				BreadboardRouteGraph::Segment mappedSegment;
				mappedSegment.from = holes.at(segment.fromHole);
				mappedSegment.to = holes.at(segment.toHole);
				mappedSegment.cost = segment.cost;
				mapped.segments.append(mappedSegment);
			}
			return mapped;
		}

		BreadboardRouteGraph::Result route(ConnectorItem *from, ConnectorItem *to,
										   const BreadboardRouteGraphCore::QueryContext &context) const
		{
			const int fromIndex = indexForHole.value(from, -1);
			const int toIndex = indexForHole.value(to, -1);
			if (fromIndex < 0 || toIndex < 0)
			{
				BreadboardRouteGraph::Result mapped;
				mapped.reason = "endpoint is not a routable breadboard hole";
				return mapped;
			}
			return mapResult(core->route(fromIndex, toIndex, context));
		}

		// One Dijkstra from `source` answers every later extract() in O(path).
		BreadboardRouteGraphCore::MultiResult routeFrom(ConnectorItem *source,
														const BreadboardRouteGraphCore::QueryContext &context) const
		{
			const int index = indexForHole.value(source, -1);
			if (index < 0)
				return BreadboardRouteGraphCore::MultiResult();
			return core->routeFrom(index, context);
		}

		BreadboardRouteGraph::Result extract(const BreadboardRouteGraphCore::MultiResult &multi,
											 ConnectorItem *target) const
		{
			const int index = indexForHole.value(target, -1);
			if (index < 0)
			{
				BreadboardRouteGraph::Result mapped;
				mapped.reason = "endpoint is not a routable breadboard hole";
				return mapped;
			}
			return mapResult(core->extractRoute(multi, index));
		}
	};

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

	QPolygonF translatedLegForTarget(ConnectorItem *pin, const QPointF &offset, ConnectorItem *hole,
									 PinSwap swap = PinSwap::None, const QPointF &swapCenter = QPointF())
	{
		QPolygonF translated;
		if (pin == nullptr || hole == nullptr)
			return translated;

		// Do not preserve the leg's historical bend points: after the part
		// moves they describe a stale shape, and snapping only the endpoint
		// to the hole produces zigzag leads. Bend a fresh straight lead from
		// the leg root at the body to the assigned hole instead.
		QPolygonF oldLeg = pin->sceneAdjustedLeg();
		QPointF root = oldLeg.count() >= 2
			? oldLeg.first()
			: pin->sceneAdjustedTerminalPoint(nullptr);
		root = swappedPoint(root, swap, swapCenter);
		translated << root + offset;
		translated << hole->sceneAdjustedTerminalPoint(nullptr);
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

QString BreadboardAutorouter::PhaseStats::toString() const
{
	return QString("clear=%1ms collect=%2ms placeSearch=%3ms placeExec=%4ms routeSearch=%5ms routeExec=%6ms completion=%7ms cleanup=%8ms")
		.arg(clearMs)
		.arg(collectMs)
		.arg(placeSearchMs)
		.arg(placeExecMs)
		.arg(routeSearchMs)
		.arg(routeExecMs)
		.arg(completionMs)
		.arg(cleanupMs);
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
	m_phaseStats = PhaseStats();
	QElapsedTimer phaseTimer;
	phaseTimer.start();
	auto takePhaseMs = [&phaseTimer]() {
		const qint64 ms = phaseTimer.elapsed();
		phaseTimer.restart();
		return ms;
	};

	QFile::remove(logFilePath());
	logAutoroute("========== breadboard autoroute start ==========");
	logAutoroute(QString("log file: %1").arg(logFilePath()));
	loadTuning();
	logAutoroute(QString("tuning: maxLegLength=%1 leadLengthWeight=%2 jumperPenalty=%3 leadAngleWeight=%4 foldbackWeight=%5")
				 .arg(m_maxLegLength)
				 .arg(m_leadLengthWeight)
				 .arg(m_jumperPenalty)
				 .arg(m_leadAngleWeight)
				 .arg(m_foldbackWeight));
	// Flush immediately so external watchers see the log recreated at start.
	flushAutorouteLog();

	auto *undoStack = m_sketchWidget->undoStack();
	const int undoCountBefore = undoStack->count();
	const int undoIndexBefore = undoStack->index();
	logAutoroute(QString("undo transaction begin: count=%1 index=%2")
				 .arg(undoCountBefore)
				 .arg(undoIndexBefore));
	undoStack->beginMacro(QObject::tr("Breadboard autoroute"));

	invalidateRoutingCaches();
	phaseTimer.restart();
	const int cleared = clearPreviousAutorouteWires();
	m_phaseStats.clearMs = takePhaseMs();
	invalidateRoutingCaches();
	if (cleared > 0)
	{
		logAutoroute(QString("clear complete: removedWires=%1").arg(cleared));
		Q_EMIT setProgressMessage(QObject::tr("Cleared previous breadboard routes..."));
		Q_EMIT setProgressMessage2(QObject::tr("Removed %1 generated breadboard wire(s).").arg(cleared));
	}

	clearCollectedNets();

	QHash<ConnectorItem *, int> indexer;
	m_sketchWidget->collectAllNets(indexer, m_allPartConnectorItems, false, false, false);
	sortCollectedNets();
	m_phaseStats.collectMs = takePhaseMs();
	logAutoroute(QString("collectAllNets: nets=%1 indexer=%2").arg(m_allPartConnectorItems.count()).arg(indexer.count()));

	if (m_allPartConnectorItems.isEmpty())
	{
		undoStack->endMacro();
		logAutoroute("abort: no breadboard connections to route");
		flushAutorouteLog();
		QMessageBox::information(nullptr, QObject::tr("Fritzing"), QObject::tr("No breadboard connections to route."));
		return;
	}

	Q_EMIT setMaximumProgress(100);
	Q_EMIT setProgressValue(0);
	Q_EMIT setProgressMessage(QObject::tr("Placing breadboard parts..."));
	Q_EMIT setProgressMessage2(QString());
	m_progressPumpTimer.start();

	// Prime invariant machinery: seed bus ownership from existing
	// connections and record net contacts that already exist so the
	// post-route conformance audit only fails on connections WE created.
	seedBusOwnership();
	m_preExistingNetContacts.clear();
	{
		QStringList ignored;
		verifySchematicConformance(ignored, true);
		if (!m_preExistingNetContacts.isEmpty())
			logAutoroute(QString("pre-existing net contacts (exempt from audit): %1").arg(m_preExistingNetContacts.count()));
	}

	phaseTimer.restart();
	int placed = autoplacePartsOnBreadboard();
	// placeExecMs is recorded inside autoplace around its command push.
	m_phaseStats.placeSearchMs = takePhaseMs() - m_phaseStats.placeExecMs;
	logAutoroute(QString("undo transaction after placement: count=%1 index=%2")
				 .arg(undoStack->count())
				 .arg(undoStack->index()));
	logAutoroute(QString("autoplace complete: placed=%1").arg(placed));
	if (placed < 0)
	{
		clearCollectedNets();
		undoStack->endMacro();
		undoStack->undo();
		logAutoroute("abort: placement connection verification failed; transaction rolled back");
		flushAutorouteLog();
		QMessageBox messageBox(QMessageBox::Critical,
							   QObject::tr("Fritzing"),
							   QObject::tr("Breadboard placement produced detached component pins and was rolled back."));
		messageBox.setDetailedText(m_lastPlacementReport);
		messageBox.exec();
		return;
	}
	if (placed > 0)
	{
		clearCollectedNets();
		indexer.clear();
		m_sketchWidget->collectAllNets(indexer, m_allPartConnectorItems, false, false, false);
		sortCollectedNets();
		logAutoroute(QString("collectAllNets after placement: nets=%1 indexer=%2").arg(m_allPartConnectorItems.count()).arg(indexer.count()));

		// Placement merges equal-potential groups (pins now share buses), so
		// this collection numbers nets differently from the one that seeded
		// bus ownership - and net indices double as ownership keys. Re-derive
		// ownership and the audit baseline from the current scene so routing,
		// claims and the conformance audit all key off THIS collection.
		// Everything placement claimed is re-derivable: its pins are
		// physically connected by now, and any placement-created short would
		// surface here as a logged seed conflict.
		seedBusOwnership();
		m_preExistingNetContacts.clear();
		{
			QStringList ignored;
			verifySchematicConformance(ignored, true);
		}
	}

	Q_EMIT setProgressMessage(QObject::tr("Routing breadboard jumpers..."));

	auto *parentCommand = new QUndoCommand(QObject::tr("Route breadboard jumpers"));

	phaseTimer.restart();
	int created = routeCollectedNets(parentCommand);
	m_phaseStats.routeSearchMs = takePhaseMs();
	logAutoroute(QString("route complete: createdWires=%1").arg(created));

	Q_EMIT setProgressValue(RoutingProgressEnd);

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
		flushAutorouteLog();
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
	phaseTimer.restart();
	undoStack->push(parentCommand);
	m_phaseStats.routeExecMs = takePhaseMs();
	// Pushed commands created wires: the cached wire-ends list is stale.
	invalidateRoutingCaches();

	// The net-level planner minimizes jumpers, but Fritzing's ratsnest model is
	// the authoritative completion check. Route only demands that remain after
	// the optimized commands have executed; never report success over ratnests.
	int residualRatsnests = countUnresolvedNets();
	if (residualRatsnests > 0)
	{
		logAutoroute(QString("completion pass begin: residualRatsnests=%1").arg(residualRatsnests));
		auto *completionCommand = new QUndoCommand(QObject::tr("Complete breadboard routes"));
		const int completed = routeRatsnestDemands(completionCommand);
		if (completed > 0)
		{
			new CleanUpRatsnestsCommand(m_sketchWidget, CleanUpWiresCommand::RedoOnly, completionCommand);
			new CleanUpWiresCommand(m_sketchWidget, CleanUpWiresCommand::RedoOnly, completionCommand);
			undoStack->push(completionCommand);
			invalidateRoutingCaches();
			created += completed;
		}
		else
		{
			delete completionCommand;
		}
		residualRatsnests = countUnresolvedNets();
		logAutoroute(QString("completion pass end: createdWires=%1 residualRatsnests=%2")
					 .arg(completed)
					 .arg(residualRatsnests));
	}
	m_phaseStats.completionMs = takePhaseMs();

	// PRIME INVARIANT audit: the routed result must not connect nets that
	// the schematic keeps separate. Any violation is our bug - roll the
	// entire autoroute back rather than ship a wrong circuit.
	// PRIME INVARIANT audit: bus-ownership prevents shorts by construction;
	// this is the independent belt-and-braces check (breadboard-only bus
	// union-find, verified to report 0 on valid routes). Any violation is
	// our bug - roll the whole autoroute back rather than ship a wrong
	// circuit.
	{
		QStringList violations;
		if (!verifySchematicConformance(violations))
		{
			logAutoroute(QString("SCHEMATIC CONFORMANCE FAILED (%1 shorts):\n%2")
							 .arg(violations.count())
							 .arg(violations.join('\n')));
			undoStack->endMacro();
			undoStack->undo();
			flushAutorouteLog();
			QMessageBox messageBox(QMessageBox::Critical,
								   QObject::tr("Fritzing"),
								   QObject::tr("Autoroute created connections that do not exist in the schematic and was rolled back."));
			messageBox.setDetailedText(violations.join('\n'));
			messageBox.exec();
			return;
		}
		logAutoroute("schematic conformance verified: no breadboard shorts between schematic nets");
	}

	logAutoroute(QString("undo transaction after routing: count=%1 index=%2")
				 .arg(undoStack->count())
				 .arg(undoStack->index()));
	undoStack->endMacro();
	m_phaseStats.cleanupMs = takePhaseMs();
	logAutoroute(QString("undo transaction end: count=%1 index=%2 delta=%3")
				 .arg(undoStack->count())
				 .arg(undoStack->index())
				 .arg(undoStack->count() - undoCountBefore));
	m_lastRoutingScore.failedNets = residualRatsnests;
	logAutoroute(QString("phase-summary: %1 total=%2ms score={%3}")
				 .arg(m_phaseStats.toString())
				 .arg(elapsed.elapsed())
				 .arg(m_lastRoutingScore.toString()));
	logAutoroute(QString("benchmark: %1 elapsedMs=%2")
				 .arg(m_lastRoutingScore.toString())
				 .arg(elapsed.elapsed()));

	if (residualRatsnests > 0)
	{
		Q_EMIT setProgressMessage2(QObject::tr("Routing incomplete: %1 connection(s) remain.").arg(residualRatsnests));
		logAutoroute(QString("failure: placed=%1 createdWires=%2 residualRatsnests=%3")
					 .arg(placed)
					 .arg(created)
					 .arg(residualRatsnests));
		flushAutorouteLog();
		// Honest guidance: the usual cause is exhausted free bus space.
		QMessageBox::information(nullptr, QObject::tr("Fritzing"),
								 QObject::tr("Autoroute could not complete %1 connection(s).\n\n"
											 "The breadboard's free bus space may be exhausted. "
											 "Adding another breadboard next to the existing one and "
											 "running Autoroute again may allow the circuit to complete.")
									 .arg(residualRatsnests));
	}
	else
	{
		Q_EMIT setProgressMessage2(QObject::tr("Placed %1 part(s), created %2 breadboard jumper wire(s).").arg(placed).arg(created));
		logAutoroute(QString("success: placed=%1 createdWires=%2").arg(placed).arg(created));
	}
	// Diagnostic: any rubber-band leg with more than two points has been
	// reshaped after placement set it to a straight root-to-hole lead.
	Q_FOREACH (QGraphicsItem *graphicsItem, m_sketchWidget->scene()->items())
	{
		auto *connectorItem = dynamic_cast<ConnectorItem *>(graphicsItem);
		if (connectorItem == nullptr || !connectorItem->hasRubberBandLeg())
			continue;
		const QPolygonF leg = connectorItem->sceneAdjustedLeg();
		if (leg.count() <= 2)
			continue;
		QStringList points;
		Q_FOREACH (const QPointF &point, leg)
			points << QString("(%1,%2)").arg(point.x()).arg(point.y());
		logAutoroute(QString("leg audit: %1 points=%2 %3")
						 .arg(connectorSummary(connectorItem))
						 .arg(leg.count())
						 .arg(points.join(" ")));
	}
	Q_EMIT setProgressValue(100);
	logAutoroute("========== breadboard autoroute end ==========");
	flushAutorouteLog();
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

// One part-placement pass over the sketch. The pass owns the state every
// phase shares (board topology, hole caches, incremental planning state,
// rejection counters); autoplacePartsOnBreadboard() is reduced to building
// the pass and driving parts through its phases in connectivity order.
//
// Search and commit are deliberately separate per part: the searches only
// read scene state and fill a PlacementCandidate, while commitBest() alone
// creates undo commands - and those still execute only when finish() pushes
// the parent command. This is the seam a future plan/commit split (parallel
// candidate search) hangs off.
struct BreadboardAutorouter::PlacementPass
{
	BreadboardAutorouter *self = nullptr;

	// Scene input: every visible breadboard-view part.
	QList<ItemBase *> parts;

	// Schematic netlist view, indexed once up front.
	QHash<ConnectorItem *, int> netForConnector;
	QHash<int, QList<ConnectorItem *> > connectorsForNet;

	// Board topology and per-run hole caches.
	QList<ConnectorItem *> breadboardHoles;
	QList<ItemBase *> targetBreadboards;
	QSet<ConnectorItem *> reservedHoles;
	QRectF targetBoardBounds;
	QVector<QRectF> boardRects;
	QPointF breadboardCenter;
	QVector<QPointF> boardCenters;              // per-board hole centroid
	QHash<ConnectorItem *, int> boardIdForHole; // hole -> index into boardRects
	QHash<ConnectorItem *, int> busIdForHole;
	QHash<ConnectorItem *, QPointF> holePositions;
	BreadboardPlacementKernel::HoleSpatialIndex holeSpatialIndex;
	int targetSceneConnectors = 0;

	// Mutable planning state, updated as each part is placed.
	QList<QRectF> occupiedRects;
	QList<ItemBase *> movableParts;
	QHash<ConnectorItem *, ConnectorItem *> placedTargets;      // pin -> hole, incl. pre-placed pins
	QHash<ConnectorItem *, ConnectorItem *> newlyPlacedTargets; // pin -> hole, this run only
	QUndoCommand *parentCommand = nullptr;
	int moved = 0;

	// Rejection/progress counters feeding the failure report and summary log.
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
	int skippedNull = 0;
	int skippedInvisible = 0;
	int skippedRatsnest = 0;
	int skippedBreadboard = 0;
	int skippedWire = 0;
	int skippedLocked = 0;
	int skippedAlreadyOnBreadboard = 0;

	explicit PlacementPass(BreadboardAutorouter *router)
		: self(router)
	{
	}

	// Phase 1: index the schematic netlist. Also seeds placedTargets with
	// pins the user already plugged in, so netPlacementCost() pulls each
	// net's remaining pins toward its seated ones from the very first part.
	void indexNets()
	{
		for (int netIndex = 0; netIndex < self->m_allPartConnectorItems.count(); netIndex++)
		{
			QList<ConnectorItem *> *net = self->m_allPartConnectorItems.at(netIndex);
			if (net == nullptr)
				continue;
			Q_FOREACH (ConnectorItem *connectorItem, *net)
			{
				if (connectorItem == nullptr)
					continue;
				netForConnector.insert(connectorItem, netIndex);
				connectorsForNet[netIndex].append(connectorItem);
				if (connectorItem->connectorType() == Connector::Female
				    || connectorItem->attachedToItemType() == ModelPart::Wire)
					continue;
				ConnectorItem *breadboardHole = self->connectedBreadboardHoleFor(connectorItem);
				if (breadboardHole != nullptr && breadboardHole->connectorType() == Connector::Female)
					placedTargets.insert(connectorItem, breadboardHole);
			}
		}
	}

	// Phase 2: discover the target breadboard(s). Returns false - with the
	// user-facing report set - when the scene offers no breadboard holes.
	bool discoverBoards()
	{
		BreadboardTopology topology;
		topology.discover(self->m_sketchWidget->scene(), self->m_sketchWidget->scene()->selectedItems());
		Q_FOREACH (const QString &line, topology.diagnosticLines())
		{
			self->logAutoroute(line);
		}

		breadboardHoles = topology.holes();
		targetBreadboards = topology.boardItems();
		reservedHoles = topology.reservedHoles();
		targetBoardBounds = topology.bounds();
		// Union bounds cover the empty gap BETWEEN boards on multi-board
		// sketches; a body is only legal fully inside some single board.
		Q_FOREACH (const BreadboardTopology::Board &board, topology.boards())
		{
			const int boardId = boardRects.count();
			boardRects.append(board.bounds);
			Q_FOREACH (ConnectorItem *hole, board.holes)
				boardIdForHole.insert(hole, boardId);
		}
		Q_FOREACH (const QRectF &boardRect, boardRects)
			self->logAutoroute(QString("board rect: (%1,%2)-(%3,%4)")
							   .arg(boardRect.left()).arg(boardRect.top())
							   .arg(boardRect.right()).arg(boardRect.bottom()));
		targetSceneConnectors = topology.sceneConnectorCount();

		if (breadboardHoles.isEmpty())
		{
			self->m_lastPlacementReport = QObject::tr(
										"No target breadboard holes were found.\n"
										"Target breadboard(s): %1\n"
										"Connectors on target breadboard item(s): %2\n"
										"Scene connectors inside target board bounds: %3\n\n"
										"If both connector counts are zero, no scene item owns enough breadboard holes.")
										.arg(targetBreadboards.count())
										.arg(topology.itemConnectorCount())
										.arg(targetSceneConnectors);
			self->logAutoroute(QString("autoplace abort: no holes\n%1").arg(self->m_lastPlacementReport));
			return false;
		}
		return true;
	}

	// Phase 3: hole caches. connectorsShareBreadboardBus() rebuilds the bus
	// member list on every call, which is far too slow inside the O(holes^2)
	// candidate loops; precompute one integer bus id per hole plus every
	// hole's scene position and use those instead.
	void buildHoleCaches()
	{
		std::sort(breadboardHoles.begin(), breadboardHoles.end(), [](ConnectorItem *a, ConnectorItem *b)
				  {
			QPointF ap = a->sceneAdjustedTerminalPoint(nullptr);
			QPointF bp = b->sceneAdjustedTerminalPoint(nullptr);
			if (!qFuzzyCompare(ap.y(), bp.y())) return ap.y() < bp.y();
			return ap.x() < bp.x(); });

		Q_FOREACH (ConnectorItem *hole, breadboardHoles)
		{
			breadboardCenter += hole->sceneAdjustedTerminalPoint(nullptr);
		}
		breadboardCenter /= breadboardHoles.count();

		// Per-board hole centroids, accumulated in the same sorted hole order
		// as breadboardCenter so a single-board sketch computes bitwise the
		// same centre it did before multi-board support.
		QVector<QPointF> boardSums(boardRects.count(), QPointF());
		QVector<int> boardHoleCounts(boardRects.count(), 0);
		Q_FOREACH (ConnectorItem *hole, breadboardHoles)
		{
			const int boardId = boardIdForHole.value(hole, -1);
			if (boardId < 0 || boardId >= boardSums.count())
				continue;
			boardSums[boardId] += hole->sceneAdjustedTerminalPoint(nullptr);
			boardHoleCounts[boardId]++;
		}
		for (int boardId = 0; boardId < boardSums.count(); boardId++)
			boardCenters.append(boardHoleCounts.at(boardId) > 0
									? boardSums.at(boardId) / boardHoleCounts.at(boardId)
									: breadboardCenter);

		int busCount = 0;
		QVector<QPointF> indexedPositions;
		QVector<int> indexedBoardIds;
		indexedPositions.reserve(breadboardHoles.count());
		indexedBoardIds.reserve(breadboardHoles.count());
		Q_FOREACH (ConnectorItem *hole, breadboardHoles)
		{
			const QPointF position = hole->sceneAdjustedTerminalPoint(nullptr);
			holePositions.insert(hole, position);
			indexedPositions.append(position);
			indexedBoardIds.append(boardIdForHole.value(hole, -1));
			if (busIdForHole.contains(hole))
				continue;
			const int busId = busCount++;
			busIdForHole.insert(hole, busId);
			ItemBase *board = hole->attachedTo();
			QList<ConnectorItem *> busHoles;
			if (board == nullptr || !board->busConnectorItems(hole, busHoles))
				continue;
			Q_FOREACH (ConnectorItem *sibling, busHoles)
				busIdForHole.insert(sibling, busId);
		}
		// Four breadboard pitches per cell keeps each bucket compact while
		// avoiding excessive cell traversal for normal bendable lead radii.
		holeSpatialIndex = BreadboardPlacementKernel::HoleSpatialIndex(indexedPositions, indexedBoardIds, 36.0);
	}

	bool holesShareBus(ConnectorItem *first, ConnectorItem *second) const
	{
		return busIdForHole.value(first, -1) == busIdForHole.value(second, -2);
	}

	// A body is only legal fully inside a single board; fall back to the
	// union when board rects are unavailable so we never reject everything.
	bool anyBoardContains(const QRectF &bounds) const
	{
		if (boardRects.isEmpty())
			return targetBoardBounds.contains(bounds);
		Q_FOREACH (const QRectF &boardRect, boardRects)
		{
			if (boardRect.contains(bounds))
				return true;
		}
		return false;
	}

	bool overlapsOccupied(const QRectF &keepout) const
	{
		Q_FOREACH (const QRectF &occupied, occupiedRects)
		{
			if (keepout.intersects(occupied))
				return true;
		}
		return false;
	}

	// Pull candidates toward the nearest board's own hole centroid: the mean
	// of ALL holes sits in the empty gap between boards on multi-board
	// sketches and would drag every part there.
	double distanceToNearestBoardCenter(const QPointF &point) const
	{
		if (boardCenters.isEmpty())
			return manhattanDistance(point, breadboardCenter);
		double best = std::numeric_limits<double>::max();
		Q_FOREACH (const QPointF &center, boardCenters)
			best = qMin(best, manhattanDistance(point, center));
		return best;
	}

	// A part body cannot span two boards: every mapped hole must belong to
	// the same board.
	bool mappedHolesShareBoard(const QHash<ConnectorItem *, ConnectorItem *> &pinToHole) const
	{
		int sharedBoardId = -2;
		for (auto it = pinToHole.constBegin(); it != pinToHole.constEnd(); ++it)
		{
			const int holeBoardId = boardIdForHole.value(it.value(), -1);
			if (sharedBoardId == -2)
				sharedBoardId = holeBoardId;
			if (holeBoardId != sharedBoardId)
				return false;
		}
		return true;
	}

	// A part with any placeable pin already seated in a female socket was
	// positioned by the user (or a previous run); leave it where it is.
	bool isAlreadySeated(ItemBase *part) const
	{
		Q_FOREACH (ConnectorItem *connectorItem, part->cachedConnectorItems())
		{
			if (connectorItem == nullptr)
				continue;
			if (!self->isPlaceablePin(connectorItem))
				continue;
			Q_FOREACH (ConnectorItem *connected, connectorItem->connectedToItems())
			{
				if (connected != nullptr && connected->connectorType() == Connector::Female)
					return true;
			}
		}
		return false;
	}

	// Ignore-classified parts bucket into per-reason counters so the failure
	// report can say WHY nothing was movable.
	QString ignoreReasonFor(const BreadboardPartPolicy::Decision &policy)
	{
		if (policy.reason == "not visible")
		{
			skippedInvisible++;
			return policy.reason;
		}
		if (policy.reason == "ratsnest")
		{
			skippedRatsnest++;
			return policy.reason;
		}
		if (policy.reason == "breadboard" || policy.reason == "breadboard decoration")
		{
			skippedBreadboard++;
			return policy.reason;
		}
		if (policy.reason == "wire")
		{
			skippedWire++;
			return policy.reason;
		}
		if (policy.reason == "move locked")
		{
			skippedLocked++;
			return policy.reason;
		}
		rejectedByPolicy++;
		return policy.reason;
	}

	// Empty result = movable. One guard per skip cause, each with its own
	// report counter.
	QString skipReasonFor(ItemBase *part, const BreadboardPartPolicy::Decision &policy)
	{
		if (part == nullptr)
		{
			skippedNull++;
			return QStringLiteral("null");
		}
		if (policy.classification == BreadboardPartPolicy::Classification::Ignore)
			return ignoreReasonFor(policy);
		if (policy.classification == BreadboardPartPolicy::Classification::Peripheral)
		{
			leftPeripheral++;
			return QString("peripheral: %1").arg(policy.reason);
		}
		if (policy.classification != BreadboardPartPolicy::Classification::BoardPlaceable)
		{
			rejectedByPolicy++;
			return policy.reason;
		}
		if (isAlreadySeated(part))
		{
			skippedAlreadyOnBreadboard++;
			return QStringLiteral("already connected to female connector");
		}
		return QString();
	}

	// Phase 4: split the scene's parts into movable candidates and skips,
	// logging every decision (the log is the primary answer to "why did my
	// part not move?").
	void collectMovableParts()
	{
		Q_FOREACH (ItemBase *part, parts)
		{
			const BreadboardPartPolicy::Decision policy = BreadboardPartPolicy::classify(part);
			const QString skipReason = skipReasonFor(part, policy);

			self->logAutoroute(QString("part policy: class=%1 reason=%2 pins=%3 bendableLegs=%4 family='%5' taxonomy='%6' package='%7' module=%8 title='%9'")
							 .arg(BreadboardPartPolicy::classificationName(policy.classification))
							 .arg(policy.reason)
							 .arg(policy.placeablePins)
							 .arg(policy.hasBendableLegs ? "yes" : "no")
							 .arg(policy.family)
							 .arg(policy.taxonomy)
							 .arg(policy.package)
							 .arg(policy.moduleID)
							 .arg(policy.title));

			if (!skipReason.isEmpty())
			{
				self->logAutoroute(QString("skip part: reason=%1 item=%2").arg(skipReason).arg(self->itemSummary(part)));
				continue;
			}

			movableParts.append(part);
			self->logAutoroute(QString("movable part: %1 connectors=%2 placeablePins=%3 bendableLegs=%4 bounds=[%5,%6 %7x%8]")
							 .arg(self->itemSummary(part))
							 .arg(part->cachedConnectorItems().count())
							 .arg(policy.placeablePins)
							 .arg(policy.hasBendableLegs ? "yes" : "no")
							 .arg(part->sceneBoundingRect().x())
							 .arg(part->sceneBoundingRect().y())
							 .arg(part->sceneBoundingRect().width())
							 .arg(part->sceneBoundingRect().height()));
		}
		self->logAutoroute(QString("movable filter summary: movable=%1 null=%2 invisible=%3 ratsnest=%4 breadboard=%5 wire=%6 locked=%7 alreadyFemale=%8 rejectedByPolicy=%9 leftPeripheral=%10")
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
	}

	// Phase 5: everything staying put near the board blocks area candidates
	// may not overlap (bodies of peripherals, user-locked parts, ...).
	void collectOccupiedRects()
	{
		const QRectF targetKeepoutBounds = targetBoardBounds.adjusted(-PlacementKeepoutMargin, -PlacementKeepoutMargin, PlacementKeepoutMargin, PlacementKeepoutMargin);
		Q_FOREACH (ItemBase *part, parts)
		{
			if (part == nullptr)
				continue;
			if (!part->isEverVisible())
				continue;
			ItemBase *chief = part->layerKinChief();
			if (self->isBreadboardItem(part) || self->isBreadboardItem(chief))
				continue;
			if (self->isBreadboardDecorationItem(part) || self->isBreadboardDecorationItem(chief))
				continue;
			if (targetBreadboards.contains(part) || targetBreadboards.contains(chief))
				continue;
			if (movableParts.contains(part))
				continue;

			QRectF partBounds = part->sceneBoundingRect().adjusted(-PlacementKeepoutMargin, -PlacementKeepoutMargin, PlacementKeepoutMargin, PlacementKeepoutMargin);
			if (!partBounds.intersects(targetKeepoutBounds))
				continue;
			occupiedRects.append(partBounds);
			self->logAutoroute(QString("occupied rect: item=%1 bounds=[%2,%3 %4x%5]")
							 .arg(self->itemSummary(part))
							 .arg(partBounds.x())
							 .arg(partBounds.y())
							 .arg(partBounds.width())
							 .arg(partBounds.height()));
		}
		self->logAutoroute(QString("occupied rects considered=%1").arg(occupiedRects.count()));
	}

	// Phase 6: most-connected (then largest) parts place first, so the parts
	// with the least freedom get first pick of the board.
	void sortByConnectivity()
	{
		std::sort(movableParts.begin(), movableParts.end(), [this](ItemBase *a, ItemBase *b)
				  {
			double aScore = self->partConnectivityScore(a, netForConnector, connectorsForNet);
			double bScore = self->partConnectivityScore(b, netForConnector, connectorsForNet);
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
									  .arg(self->itemSummary(part))
									  .arg(self->partConnectivityScore(part, netForConnector, connectorsForNet)));
		}
		self->logAutoroute(QString("placement order: %1").arg(placementOrder.join(" || ")));
	}

	// Phase 7: the pass's undo command. It is the first child of the outer
	// autoroute macro, so its undo-only cleanup runs after both generated
	// wires and placement connections have been undone. Cleaning inside the
	// routing child leaves stale ratsnests because placement is undone later.
	void beginCommand()
	{
		parentCommand = new QUndoCommand(QObject::tr("Autoplace breadboard parts"));
		new CleanUpWiresCommand(self->m_sketchWidget, CleanUpWiresCommand::UndoOnly, parentCommand);
		new CleanUpRatsnestsCommand(self->m_sketchWidget, CleanUpWiresCommand::UndoOnly, parentCommand);
	}

	// Prime invariant, placement side: a candidate may only put a pin on a
	// bus that is free or already owned by that pin's net. No-net pins
	// (unused DIP outputs are still outputs!) may only take FREE buses.
	bool conflictsWithPlacedNets(const QHash<ConnectorItem *, ConnectorItem *> &pinToHole) const
	{
		for (auto candidate = pinToHole.constBegin(); candidate != pinToHole.constEnd(); ++candidate)
		{
			if (candidate.value() == nullptr)
				continue;
			const int busGroup = self->busGroupFor(candidate.value());
			const int candidateNet = netForConnector.value(candidate.key(), -1);
			if (candidateNet < 0)
			{
				if (self->busOwner(busGroup) != -1)
					return true;
				continue;
			}
			if (!self->busAvailableFor(busGroup, candidateNet))
				return true;
		}
		return false;
	}

	// How much routing a hole choice buys or costs: free when it lands on a
	// bus already carrying the pin's net, a distance pull toward the net's
	// placed pins otherwise, plus the tunable jumper penalty because one
	// additional occupied bus implies at least one additional jumper. The
	// penalty keeps jumper count lexicographically more important than
	// geometric terms by default; lowering it makes the router trade jumpers
	// for shorter, straighter component leads.
	double netPlacementCost(ConnectorItem *pin, ConnectorItem *targetHole) const
	{
		const int netIndex = netForConnector.value(pin, -1);
		if (netIndex < 0 || targetHole == nullptr)
			return 0.0;

		const QPointF targetPos = holePositions.value(targetHole, targetHole->sceneAdjustedTerminalPoint(nullptr));
		double bestDistance = std::numeric_limits<double>::max();
		bool hasPlacedTarget = false;
		bool sharesPlacedBus = false;
		Q_FOREACH (ConnectorItem *other, connectorsForNet.value(netIndex))
		{
			if (other == pin)
				continue;
			ConnectorItem *otherTarget = placedTargets.value(other, nullptr);
			if (otherTarget == nullptr)
				continue;
			hasPlacedTarget = true;
			bestDistance = qMin(bestDistance, manhattanDistance(targetPos, holePositions.value(otherTarget, otherTarget->sceneAdjustedTerminalPoint(nullptr))));
			if (holesShareBus(targetHole, otherTarget))
				sharesPlacedBus = true;
		}

		if (!hasPlacedTarget)
			return distanceToNearestBoardCenter(targetPos) * 0.05;
		if (sharesPlacedBus)
			return bestDistance * 0.01;
		return self->m_jumperPenalty + bestDistance;
	}

	// Nearest free hole within snap tolerance of a target point, or null.
	ConnectorItem *nearestFreeHole(const QPointF &target, const QSet<ConnectorItem *> &candidateReserved) const
	{
		ConnectorItem *nearestHole = nullptr;
		double nearestDistance = HoleMatchTolerance;
		const QVector<int> nearbyHoleIndices = holeSpatialIndex.withinRadius(target, HoleMatchTolerance);
		for (int holeIndex : nearbyHoleIndices)
		{
			ConnectorItem *hole = breadboardHoles.at(holeIndex);
			if (reservedHoles.contains(hole) || candidateReserved.contains(hole))
				continue;
			double distance = QLineF(target, holePositions.value(hole)).length();
			if (distance <= nearestDistance)
			{
				nearestHole = hole;
				nearestDistance = distance;
			}
		}
		return nearestHole;
	}

	// Different pins of one part must never land on one bus: that solders
	// the part's own pins together (e.g. a DIP not straddling the channel).
	bool anyMappedPairSharesBus(const QHash<ConnectorItem *, ConnectorItem *> &pinToHole) const
	{
		QList<ConnectorItem *> mappedHoles = pinToHole.values();
		for (int fromIndex = 0; fromIndex < mappedHoles.count(); fromIndex++)
		{
			for (int toIndex = fromIndex + 1; toIndex < mappedHoles.count(); toIndex++)
			{
				if (self->connectorsShareBreadboardBus(mappedHoles.at(fromIndex), mappedHoles.at(toIndex)))
					return true;
			}
		}
		return false;
	}

	// One rigid candidate: the offset that drops the anchor pin exactly onto
	// anchorHole. Rejection order runs cheapest-first; counters record which
	// filter killed the candidate.
	void evaluateRigidOffset(ItemBase *part, const QList<ConnectorItem *> &pins, const QPointF &offset,
							 ConnectorItem *anchorHole, PlacementCandidate &best)
	{
		QSet<ConnectorItem *> candidateReserved;
		QHash<ConnectorItem *, ConnectorItem *> pinToHole;
		Q_FOREACH (ConnectorItem *pin, pins)
		{
			ConnectorItem *nearestHole = nearestFreeHole(pin->sceneAdjustedTerminalPoint(nullptr) + offset, candidateReserved);
			if (nearestHole == nullptr)
			{
				rejectedPinGeometry++;
				return;
			}
			candidateReserved.insert(nearestHole);
			pinToHole.insert(pin, nearestHole);
		}

		if (!mappedHolesShareBoard(pinToHole))
		{
			rejectedPinGeometry++;
			return;
		}
		if (anyMappedPairSharesBus(pinToHole))
		{
			rejectedSameBus++;
			return;
		}
		if (conflictsWithPlacedNets(pinToHole))
		{
			rejectedSameBus++;
			return;
		}

		QRectF movedBounds = part->sceneBoundingRect().translated(offset);
		if (!anyBoardContains(movedBounds))
		{
			rejectedOffBoard++;
			return;
		}
		if (overlapsOccupied(movedBounds.adjusted(-PlacementKeepoutMargin, -PlacementKeepoutMargin, PlacementKeepoutMargin, PlacementKeepoutMargin)))
		{
			rejectedOverlap++;
			return;
		}

		acceptedCandidates++;
		double score = distanceToNearestBoardCenter(movedBounds.center()) * 0.15;
		Q_FOREACH (ConnectorItem *pin, pins)
			score += netPlacementCost(pin, pinToHole.value(pin, nullptr));
		if (score >= best.score)
			return;

		best.newLoc = best.oldLoc + offset;
		best.pinToHole = pinToHole;
		best.pinToLeg.clear();
		best.score = score;
		best.usesLegPlacement = false;
		self->logAutoroute(QString("placement best update: part=%1 score=%2 offset=(%3,%4) anchorHole=%5")
						 .arg(self->itemSummary(part))
						 .arg(score)
						 .arg(offset.x())
						 .arg(offset.y())
						 .arg(self->connectorSummary(anchorHole)));
	}

	// Rigid search: slide the whole part so one pin (the "anchor") lands
	// exactly on a free hole, then require EVERY pin to find its own free
	// hole within HoleMatchTolerance of where that slide puts it. No
	// rotation, no leg bending - the strategy for multi-pin packages whose
	// pin grid must match the hole grid (DIPs, headers).
	void searchRigid(ItemBase *part, const QList<ConnectorItem *> &pins, PlacementCandidate &best)
	{
		Q_FOREACH (ConnectorItem *anchorPin, pins)
		{
			QPointF anchorPinPos = anchorPin->sceneAdjustedTerminalPoint(nullptr);
			Q_FOREACH (ConnectorItem *anchorHole, breadboardHoles)
			{
				if (reservedHoles.contains(anchorHole))
					continue;
				candidateAttempts++;
				evaluateRigidOffset(part, pins,
									anchorHole->sceneAdjustedTerminalPoint(nullptr) - anchorPinPos,
									anchorHole, best);
			}
		}
	}

	// Working data one bendable-leg candidate needs; bundled so the hole-pair
	// evaluation can be a named function instead of a fourth nesting level.
	struct BendableCandidate
	{
		ItemBase *part = nullptr;
		ConnectorItem *firstPin = nullptr;
		ConnectorItem *secondPin = nullptr;
		ConnectorItem *firstHole = nullptr;
		ConnectorItem *secondHole = nullptr;
		QPointF firstPinPos;  // pin positions after the candidate swap
		QPointF secondPinPos;
		QPointF firstHolePos;
		QPointF secondHolePos;
		QPointF pinAxisDir;
		QPointF partCenter;
		QRectF partBounds;
		double pinPairSpan = 0.0;
		PinSwap swap = PinSwap::None;
	};

	// Profile counters for one part's bendable search, logged once per part.
	struct BendableSearchProfile
	{
		qint64 pairCount = 0;
		qint64 shiftIterations = 0;
		qint64 postCutoffEvaluations = 0;
	};

	// Score one concrete body position for a bendable-leg candidate and fold
	// it into `best` when it wins. Rejection order runs cheapest-first.
	void evaluateBendablePlacement(const BendableCandidate &candidate, const QPointF &desiredCenter,
								   BendableSearchProfile &profile, PlacementCandidate &best)
	{
		QPointF offset = desiredCenter - candidate.partCenter;
		QRectF movedBounds = candidate.partBounds.translated(offset);
		if (!anyBoardContains(movedBounds))
		{
			rejectedOffBoard++;
			return;
		}
		if (overlapsOccupied(movedBounds.adjusted(-BendablePlacementKeepoutMargin, -BendablePlacementKeepoutMargin, BendablePlacementKeepoutMargin, BendablePlacementKeepoutMargin)))
		{
			rejectedOverlap++;
			return;
		}

		QPointF movedFirstPin = candidate.firstPinPos + offset;
		QPointF movedSecondPin = candidate.secondPinPos + offset;
		double firstLegLength = QLineF(movedFirstPin, candidate.firstHolePos).length();
		double secondLegLength = QLineF(movedSecondPin, candidate.secondHolePos).length();
		if (firstLegLength > self->m_maxLegLength || secondLegLength > self->m_maxLegLength)
		{
			rejectedPinGeometry++;
			return;
		}

		acceptedCandidates++;
		double score = distanceToNearestBoardCenter(movedBounds.center()) * 0.15
					 + (firstLegLength + secondLegLength) * self->m_leadLengthWeight;

		// Straight leads only look straight when they leave along the body
		// axis. Penalize perpendicular drift (diagonal leads) and hole pairs
		// tighter than the pin spacing (leads folding back under the body).
		if (candidate.pinPairSpan > 0.001)
		{
			const QPointF firstLegVector = candidate.firstHolePos - movedFirstPin;
			const QPointF secondLegVector = candidate.secondHolePos - movedSecondPin;
			const double perpendicularDrift =
				qAbs(candidate.pinAxisDir.x() * firstLegVector.y() - candidate.pinAxisDir.y() * firstLegVector.x())
				+ qAbs(candidate.pinAxisDir.x() * secondLegVector.y() - candidate.pinAxisDir.y() * secondLegVector.x());
			const double holeSpan = (candidate.secondHolePos.x() - candidate.firstHolePos.x()) * candidate.pinAxisDir.x()
								  + (candidate.secondHolePos.y() - candidate.firstHolePos.y()) * candidate.pinAxisDir.y();
			const double compression = qMax(0.0, candidate.pinPairSpan - holeSpan);
			score += perpendicularDrift * self->m_leadAngleWeight + compression * self->m_foldbackWeight;
		}

		// Net placement costs only ever add, so a candidate whose geometric
		// score already loses cannot win: skip the expensive conflict and
		// net-cost evaluation entirely.
		if (score >= best.score)
			return;
		profile.postCutoffEvaluations++;

		QHash<ConnectorItem *, ConnectorItem *> pinToHole;
		pinToHole.insert(candidate.firstPin, candidate.firstHole);
		pinToHole.insert(candidate.secondPin, candidate.secondHole);
		if (conflictsWithPlacedNets(pinToHole))
		{
			rejectedSameBus++;
			return;
		}
		score += netPlacementCost(candidate.firstPin, candidate.firstHole);
		score += netPlacementCost(candidate.secondPin, candidate.secondHole);
		if (score >= best.score)
			return;

		best.newLoc = best.oldLoc + offset;
		best.pinToHole = pinToHole;
		best.pinToLeg.clear();
		best.pinToLeg.insert(candidate.firstPin, translatedLegForTarget(candidate.firstPin, offset, candidate.firstHole, candidate.swap, candidate.partCenter));
		best.pinToLeg.insert(candidate.secondPin, translatedLegForTarget(candidate.secondPin, offset, candidate.secondHole, candidate.swap, candidate.partCenter));
		best.score = score;
		best.usesLegPlacement = true;
		best.pinSwap = candidate.swap;
		self->logAutoroute(QString("bendable placement best update: part=%1 score=%2 offset=(%3,%4) holes=[%5 | %6] legLengths=[%7,%8]")
						 .arg(self->itemSummary(candidate.part))
						 .arg(score)
						 .arg(offset.x())
						 .arg(offset.y())
						 .arg(self->connectorSummary(candidate.firstHole))
						 .arg(self->connectorSummary(candidate.secondHole))
						 .arg(firstLegLength)
						 .arg(secondLegLength));
	}

	// One hole pair: the body need not sit centered between its holes -
	// sliding it along the pin axis lets both leads leave the body forward
	// instead of folding back when the hole pair is narrower than the pin
	// spacing. Leg length varies linearly with the slide along the axis, so
	// the only shifts worth evaluating are the ones that zero each leg's
	// axial component, plus their midpoint.
	void evaluateBendablePair(const BendableCandidate &candidate, BendableSearchProfile &profile, PlacementCandidate &best)
	{
		const QPointF baseCenter = (candidate.firstHolePos + candidate.secondHolePos) / 2.0;
		const QPointF baseOffset = baseCenter - candidate.partCenter;
		const QPointF baseFirstPin = candidate.firstPinPos + baseOffset;
		const QPointF baseSecondPin = candidate.secondPinPos + baseOffset;
		const double firstAxial = (candidate.firstHolePos.x() - baseFirstPin.x()) * candidate.pinAxisDir.x()
								+ (candidate.firstHolePos.y() - baseFirstPin.y()) * candidate.pinAxisDir.y();
		const double secondAxial = (candidate.secondHolePos.x() - baseSecondPin.x()) * candidate.pinAxisDir.x()
								 + (candidate.secondHolePos.y() - baseSecondPin.y()) * candidate.pinAxisDir.y();
		const double axialShifts[] = {(firstAxial + secondAxial) / 2.0, firstAxial, secondAxial};
		for (double axialShift : axialShifts)
		{
			profile.shiftIterations++;
			const QPointF desiredCenter = baseCenter
										+ QPointF(candidate.pinAxisDir.x() * axialShift, candidate.pinAxisDir.y() * axialShift);
			evaluateBendablePlacement(candidate, desiredCenter, profile, best);
		}
	}

	// Bendable-leg search for two-pin parts: pick any two free holes on
	// different buses within leg reach and bend each leg to its hole.
	void searchBendable(ItemBase *part, ConnectorItem *firstPin, ConnectorItem *secondPin, PlacementCandidate &best)
	{
		self->logAutoroute(QString("bendable placement search: %1 pins=[%2 | %3]")
						 .arg(self->itemSummary(part))
						 .arg(self->connectorSummary(firstPin))
						 .arg(self->connectorSummary(secondPin)));

		BendableCandidate candidate;
		candidate.part = part;
		candidate.firstPin = firstPin;
		candidate.secondPin = secondPin;
		const QPointF unflippedFirstPinPos = firstPin->sceneAdjustedTerminalPoint(nullptr);
		const QPointF unflippedSecondPinPos = secondPin->sceneAdjustedTerminalPoint(nullptr);
		candidate.pinPairSpan = QLineF(unflippedFirstPinPos, unflippedSecondPinPos).length();
		candidate.partBounds = part->sceneBoundingRect();
		candidate.partCenter = candidate.partBounds.center();

		QElapsedTimer searchTimer;
		searchTimer.start();
		BendableSearchProfile profile;

		// Both legs are capped, so viable hole pairs lie within an annulus
		// around the pin spacing; everything else can be rejected on a
		// single distance test before any geometry work.
		const double maxHoleSpan = candidate.pinPairSpan + 2.0 * self->m_maxLegLength;

		// Also search with the pins swapped, so crossed legs can uncross.
		// Prefer a mirror across the pin axis (body stays upright) when the
		// part's breadboard view allows flipping; otherwise fall back to a
		// 180 degree rotation.
		const bool axisMostlyHorizontal =
			qAbs(unflippedSecondPinPos.x() - unflippedFirstPinPos.x())
			>= qAbs(unflippedSecondPinPos.y() - unflippedFirstPinPos.y());
		const Qt::Orientations flipOrientation = axisMostlyHorizontal ? Qt::Horizontal : Qt::Vertical;
		const PinSwap swapMode = part->canFlip(flipOrientation)
			? (axisMostlyHorizontal ? PinSwap::FlipHorizontal : PinSwap::FlipVertical)
			: PinSwap::Rotate180;

		for (int flip = 0; flip <= 1; flip++)
		{
			candidate.swap = flip == 1 ? swapMode : PinSwap::None;
			candidate.firstPinPos = swappedPoint(unflippedFirstPinPos, candidate.swap, candidate.partCenter);
			candidate.secondPinPos = swappedPoint(unflippedSecondPinPos, candidate.swap, candidate.partCenter);
			candidate.pinAxisDir = candidate.pinPairSpan > 0.001
				? QPointF((candidate.secondPinPos.x() - candidate.firstPinPos.x()) / candidate.pinPairSpan,
						  (candidate.secondPinPos.y() - candidate.firstPinPos.y()) / candidate.pinPairSpan)
				: QPointF(1.0, 0.0);

			for (int firstHoleIndex = 0; firstHoleIndex < breadboardHoles.count(); firstHoleIndex++)
			{
				ConnectorItem *firstHole = breadboardHoles.at(firstHoleIndex);
				if (reservedHoles.contains(firstHole))
					continue;
				candidate.firstHole = firstHole;
				candidate.firstHolePos = holePositions.value(firstHole);
				const int boardId = boardIdForHole.value(firstHole, -1);
				const QVector<int> nearbyHoleIndices = holeSpatialIndex.withinRadius(candidate.firstHolePos, maxHoleSpan, boardId);
				for (int secondHoleIndex : nearbyHoleIndices)
				{
					ConnectorItem *secondHole = breadboardHoles.at(secondHoleIndex);
					if (firstHole == secondHole)
						continue;
					if (reservedHoles.contains(secondHole))
						continue;
					candidateAttempts++;

					candidate.secondHole = secondHole;
					candidate.secondHolePos = holePositions.value(secondHole);
					const double spanDx = candidate.secondHolePos.x() - candidate.firstHolePos.x();
					const double spanDy = candidate.secondHolePos.y() - candidate.firstHolePos.y();
					if (spanDx * spanDx + spanDy * spanDy > maxHoleSpan * maxHoleSpan)
					{
						rejectedPinGeometry++;
						continue;
					}
					if (holesShareBus(firstHole, secondHole))
					{
						rejectedSameBus++;
						continue;
					}
					profile.pairCount++;
					evaluateBendablePair(candidate, profile, best);
				}
			}
		}
		self->logAutoroute(QString("bendable search profile: %1 elapsedMs=%2 pairs=%3 shiftIterations=%4 postCutoffEvaluations=%5")
						 .arg(self->itemSummary(part))
						 .arg(searchTimer.elapsed())
						 .arg(profile.pairCount)
						 .arg(profile.shiftIterations)
						 .arg(profile.postCutoffEvaluations));
	}

	// Mirror or rotate the part so swapped-pin candidates land as searched.
	void emitSwapCommand(ItemBase *part, PinSwap swap)
	{
		switch (swap)
		{
		case PinSwap::FlipHorizontal:
			new FlipItemCommand(self->m_sketchWidget, part->id(), Qt::Horizontal, parentCommand);
			self->logAutoroute(QString("placement flip: %1 mirrored horizontally").arg(self->itemSummary(part)));
			return;
		case PinSwap::FlipVertical:
			new FlipItemCommand(self->m_sketchWidget, part->id(), Qt::Vertical, parentCommand);
			self->logAutoroute(QString("placement flip: %1 mirrored vertically").arg(self->itemSummary(part)));
			return;
		case PinSwap::Rotate180:
			new RotateItemCommand(self->m_sketchWidget, part->id(), &FlipRotationDegrees, parentCommand);
			self->logAutoroute(QString("placement flip: %1 rotated 180").arg(self->itemSummary(part)));
			return;
		default:
			return;
		}
	}

	// The leg command for one placed pin. Bendable placements planned their
	// leg shape during the search; rigid moves translate the loose sketch's
	// bent leg shape verbatim, so historical bends would survive placement -
	// replace the shape with a direct root-to-hole lead instead.
	void emitLegCommand(ConnectorItem *pin, ConnectorItem *hole, const PlacementCandidate &best)
	{
		QPolygonF newLeg = best.pinToLeg.value(pin);
		if (!best.usesLegPlacement && pin->hasRubberBandLeg())
		{
			QPolygonF looseLeg = pin->sceneAdjustedLeg();
			if (looseLeg.count() >= 2)
			{
				newLeg.clear();
				newLeg << looseLeg.first() + (best.newLoc - best.oldLoc);
				newLeg << hole->sceneAdjustedTerminalPoint(nullptr);
			}
		}
		if (newLeg.count() < 2)
			return;

		self->m_componentLeadLength += polylineLength(newLeg);
		QPolygonF oldLeg = pin->sceneAdjustedLeg();
		QPolygonF movedOldLeg;
		QPointF offset = best.newLoc - best.oldLoc;
		Q_FOREACH (QPointF point, oldLeg)
		{
			movedOldLeg << point + offset;
		}
		auto *legCommand = new ChangeLegCommand(self->m_sketchWidget,
												pin->attachedToID(),
												pin->connectorSharedID(),
												movedOldLeg,
												newLeg,
												false,
												true,
												"breadboard autoroute",
												parentCommand);
		legCommand->setSimple();
		self->logAutoroute(QString("placement leg: pin=%1 points=%2")
						 .arg(self->connectorSummary(pin))
						 .arg(newLeg.count()));
	}

	// One pin of the winning candidate: connection command plus all the
	// planning bookkeeping (hole reservation, net target maps, bus claim -
	// the prime invariant's placement-side ledger).
	void connectPinToHole(ConnectorItem *pin, ConnectorItem *hole, const PlacementCandidate &best)
	{
		auto *connectionCommand = new ChangeConnectionCommand(self->m_sketchWidget, BaseCommand::CrossView,
									pin->attachedToID(), pin->connectorSharedID(),
									hole->attachedToID(), hole->connectorSharedID(),
									ViewLayer::specFromID(hole->attachedToViewLayerID()),
									true, parentCommand);
		// Placement already specifies the exact pin and hole. Geometry-driven
		// updates while the part and its legs are moving can detach that pair.
		connectionCommand->setUpdateConnections(false);
		self->logAutoroute(QString("placement connection: pin=%1 hole=%2 sameBus?=%3")
						 .arg(self->connectorSummary(pin))
						 .arg(self->connectorSummary(hole))
						 .arg(self->connectorsShareBreadboardBus(pin, hole) ? "yes" : "no"));
		reservedHoles.insert(hole);
		placedTargets.insert(pin, hole);
		newlyPlacedTargets.insert(pin, hole);
		// Claim the hole's bus for the pin's net (or exclusively, for a
		// no-net pin) so later placements and routing cannot join it.
		const int placedPinNet = netForConnector.value(pin, -1);
		self->claimBus(self->busGroupFor(hole), placedPinNet >= 0 ? placedPinNet : self->makeNoNetOwnerKey());

		emitLegCommand(pin, hole, best);
	}

	// Commit the winning candidate as undo commands (they execute only when
	// finish() pushes the parent command).
	void commitBest(ItemBase *part, const PlacementCandidate &best)
	{
		// Transform strictly BEFORE the move and before any pins are bound:
		// transforming a part whose legs are already attached would drag the
		// leg geometry off its assigned holes.
		emitSwapCommand(part, best.pinSwap);

		ViewGeometry oldGeometry(part->getViewGeometry());
		ViewGeometry newGeometry(part->getViewGeometry());
		newGeometry.setLoc(best.newLoc);
		new MoveItemCommand(self->m_sketchWidget, part->id(), oldGeometry, newGeometry, false, parentCommand);
		self->logAutoroute(QString("placement accepted: %1 old=(%2,%3) new=(%4,%5) score=%6")
						 .arg(self->itemSummary(part))
						 .arg(best.oldLoc.x())
						 .arg(best.oldLoc.y())
						 .arg(best.newLoc.x())
						 .arg(best.newLoc.y())
						 .arg(best.score));

		for (auto it = best.pinToHole.constBegin(); it != best.pinToHole.constEnd(); ++it)
		{
			if (it.key() == nullptr || it.value() == nullptr)
				continue;
			connectPinToHole(it.key(), it.value(), best);
		}

		occupiedRects.append(part->sceneBoundingRect().translated(best.newLoc - best.oldLoc).adjusted(-PlacementKeepoutMargin, -PlacementKeepoutMargin, PlacementKeepoutMargin, PlacementKeepoutMargin));
		if (best.usesLegPlacement)
			placedWithLegs++;
		else
			placedRigid++;
		moved++;
	}

	// Phase 8, per part: search (bendable-leg for two-pin leg parts, rigid
	// otherwise), then commit the best candidate. Parts with no placeable
	// pins are silently left alone.
	void placePart(ItemBase *part)
	{
		QList<ConnectorItem *> pins;
		Q_FOREACH (ConnectorItem *connectorItem, part->cachedConnectorItems())
		{
			if (connectorItem != nullptr && self->isPlaceablePin(connectorItem))
				pins.append(connectorItem);
		}
		if (pins.isEmpty())
			return;
		partsWithPlaceablePins++;

		const bool canUseBendableLegPlacement = pins.count() == 2 && allPinsHaveBendableLegs(pins);
		self->logAutoroute(QString("placement begin: %1 placeablePins=%2 strategy=%3")
						 .arg(self->itemSummary(part))
						 .arg(pins.count())
						 .arg(canUseBendableLegPlacement ? "rigid-or-bendable-leg" : "rigid"));

		PlacementCandidate best;
		best.item = part;
		best.oldLoc = part->getViewGeometry().loc();

		if (canUseBendableLegPlacement)
			searchBendable(part, pins.at(0), pins.at(1), best);
		else
			searchRigid(part, pins, best);

		if (best.pinToHole.isEmpty())
		{
			self->logAutoroute(QString("placement no candidate: %1").arg(self->itemSummary(part)));
			failedBoardFit++;
			return;
		}
		commitBest(part, best);
	}

	// Why nothing was placed, in user-facing terms; every counter names the
	// filter that consumed the candidates.
	QString failureReport() const
	{
		return QObject::tr(
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
	}

	// Phase 9: push (the commands execute here) and verify every planned pin
	// actually attached to its assigned hole. Returns the number of parts
	// moved, 0 when nothing placed (with the failure report set), or -1 when
	// verification failed.
	int finish()
	{
		if (moved <= 0)
		{
			delete parentCommand;
			self->m_lastPlacementReport = failureReport();
			self->logAutoroute(QString("autoplace failed:\n%1").arg(self->m_lastPlacementReport));
			return 0;
		}

		QElapsedTimer execTimer;
		execTimer.start();
		self->m_sketchWidget->undoStack()->push(parentCommand);
		self->m_phaseStats.placeExecMs = execTimer.elapsed();

		QStringList connectionFailures;
		if (!self->verifyPlacedConnections(newlyPlacedTargets, connectionFailures))
		{
			self->m_lastPlacementReport = QObject::tr("Placed component pins did not attach to their assigned breadboard holes:\n%1")
									.arg(connectionFailures.join('\n'));
			self->logAutoroute(QString("autoplace connection verification failed:\n%1").arg(self->m_lastPlacementReport));
			return -1;
		}
		self->logAutoroute(QString("autoplace counters: moved=%1 placedRigid=%2 placedWithLegs=%3 candidateAttempts=%4 acceptedCandidates=%5 rejectedPinGeometry=%6 rejectedSameBus=%7 rejectedOffBoard=%8 rejectedOverlap=%9 rejectedByPolicy=%10 leftPeripheral=%11 failedBoardFit=%12")
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
};

int BreadboardAutorouter::autoplacePartsOnBreadboard()
{
	m_lastPlacementReport.clear();

	PlacementPass pass(this);
	m_sketchWidget->collectParts(pass.parts);
	logAutoroute(QString("autoplace: visible parts collected=%1").arg(pass.parts.count()));
	if (pass.parts.isEmpty())
	{
		m_lastPlacementReport = QObject::tr("No breadboard-view parts were found.");
		logAutoroute("autoplace abort: no parts");
		return 0;
	}

	pass.indexNets();
	if (!pass.discoverBoards())
		return 0;
	pass.buildHoleCaches();
	pass.collectMovableParts();
	pass.collectOccupiedRects();
	pass.sortByConnectivity();
	pass.beginCommand();
	const int movableCount = pass.movableParts.count();
	int partsDone = 0;
	Q_FOREACH (ItemBase *part, pass.movableParts)
	{
		reportProgress(movableCount > 0 ? (PlacementProgressSpan * partsDone) / movableCount : 0,
					   QObject::tr("Placing %1 (%2 of %3)...")
						   .arg(part == nullptr ? QString() : part->title())
						   .arg(partsDone + 1)
						   .arg(movableCount));
		pass.placePart(part);
		partsDone++;
	}
	return pass.finish();
}

bool BreadboardAutorouter::verifyPlacedConnections(const QHash<ConnectorItem *, ConnectorItem *> &placedTargets,
													 QStringList &failures) const
{
	constexpr double EndpointTolerance = 1.0;
	for (auto it = placedTargets.constBegin(); it != placedTargets.constEnd(); ++it)
	{
		ConnectorItem *pin = it.key();
		ConnectorItem *hole = it.value();
		if (pin == nullptr || hole == nullptr)
		{
			failures.append(QObject::tr("Null pin or hole in placement result."));
			continue;
		}

		const bool pinConnected = pin->connectedToItems().contains(hole);
		const bool holeConnected = hole->connectedToItems().contains(pin);
		double endpointDistance = 0.0;
		if (pin->hasRubberBandLeg())
		{
			const QPolygonF leg = pin->sceneAdjustedLeg();
			endpointDistance = leg.isEmpty()
				? std::numeric_limits<double>::infinity()
				: QLineF(leg.last(), hole->sceneAdjustedTerminalPoint(nullptr)).length();
		}

		if (!pinConnected || !holeConnected || endpointDistance > EndpointTolerance)
		{
			failures.append(QObject::tr("%1 -> %2: pinConnected=%3, holeConnected=%4, legEndpointDistance=%5")
							.arg(connectorSummary(pin))
							.arg(connectorSummary(hole))
							.arg(pinConnected ? "yes" : "no")
							.arg(holeConnected ? "yes" : "no")
							.arg(endpointDistance));
		}
		else
		{
			logAutoroute(QString("placement verified: pin=%1 hole=%2 endpointDistance=%3")
							 .arg(connectorSummary(pin))
							 .arg(connectorSummary(hole))
							 .arg(endpointDistance));
		}
	}
	return failures.isEmpty();
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
	auto chooseEntry = [&](ConnectorItem *terminal, const QSet<ConnectorItem *> &extraReserved, int ownerKey) {
		ConnectorItem *best = nullptr;
		double bestScore = std::numeric_limits<double>::max();
		Q_FOREACH (ConnectorItem *hole, routeHoles)
		{
			if (hole == nullptr || reservedHoles.contains(hole) || extraReserved.contains(hole)) continue;
			if (hole->connectionsCount() != 0) continue;
			if (!busAvailableFor(busGroupFor(hole), ownerKey)) continue;
			const double score = boardEdgeEntryScore(terminal, hole, holeBounds);
			if (score < bestScore) {
				best = hole;
				bestScore = score;
			}
		}
		return best;
	};

	logAutoroute(QString("ratsnest demands: %1").arg(demands.count()));
	const RouteGraphSession routeSession = RouteGraphSession::build(
		routeHoles,
		[this](ConnectorItem *connectorItem) { return busGroupFor(connectorItem); },
		[this](int busGroup) { return busOwner(busGroup); });
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

		// The demand's owner key: its schematic net, or a fresh exclusive
		// sentinel if the endpoints are unexpectedly netless.
		int demandNet = m_netForConnector.value(fromPart, INT_MIN);
		if (demandNet == INT_MIN)
			demandNet = m_netForConnector.value(toPart, INT_MIN);
		if (demandNet == INT_MIN)
			demandNet = makeNoNetOwnerKey();

		if (fromHole != nullptr && toHole != nullptr)
		{
			const BreadboardRouteGraphCore::QueryContext routeContext = routeSession.prepare(reservedHoles, plannedSegments, demandNet);
			BreadboardRouteGraph::Result route = routeSession.route(fromHole, toHole, routeContext);
			if (!route.found) {
				failed++;
				logAutoroute(QString("ratsnest demand failed: no board route from=[%1] to=[%2]")
							 .arg(connectorSummary(fromPart), connectorSummary(toPart)));
				continue;
			}
			Q_FOREACH (const BreadboardRouteGraph::Segment &segment, route.segments)
			{
				claimBus(busGroupFor(segment.from), demandNet);
				claimBus(busGroupFor(segment.to), demandNet);
			}
			applyGraphRoute(route);
			logAutoroute(QString("ratsnest demand complete: wires=%1").arg(created - createdBeforeDemand));
			continue;
		}

		if (fromHole == nullptr && toHole == nullptr)
		{
			ConnectorItem *fromEntry = chooseEntry(fromPart, QSet<ConnectorItem *>(), demandNet);
			ConnectorItem *toEntry = nearestFreeBusHole(fromEntry);
			if (toEntry != nullptr && !busAvailableFor(busGroupFor(toEntry), demandNet))
				toEntry = nullptr;
			if (fromEntry == nullptr || toEntry == nullptr) {
				failed++;
				logAutoroute("ratsnest demand failed: no entries for off-board pair");
				continue;
			}
			addWire(fromPart, fromEntry);
			addWire(toPart, toEntry);
			reservedHoles.insert(fromEntry);
			reservedHoles.insert(toEntry);
			claimBus(busGroupFor(fromEntry), demandNet);
			claimBus(busGroupFor(toEntry), demandNet);
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
		if (!busAvailableFor(busGroupFor(targetHole), demandNet)) {
			failed++;
			logAutoroute(QString("ratsnest demand failed: peripheral target bus owned by another net [%1]").arg(connectorSummary(targetHole)));
			continue;
		}
		addWire(terminal, targetHole);
		reservedHoles.insert(targetHole);
		claimBus(busGroupFor(targetHole), demandNet);
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

// One net-routing pass over the sketch. The pass owns the state every phase
// shares (route graph session, reserved holes, planned wire segments, score
// counters); routeCollectedNets() is reduced to building the pass and driving
// nets through its phases in most-constrained-first order.
struct BreadboardAutorouter::NetRoutingPass
{
	BreadboardAutorouter *self = nullptr;
	QUndoCommand *parentCommand = nullptr;

	QList<ConnectorItem *> routeHoles;
	QSet<ConnectorItem *> reservedHoles;
	HoleBounds holeBounds;
	RouteGraphSession session;
	QList<QLineF> plannedSegments;
	QSet<int> failedNetIndices;
	int created = 0;
	int wiredPeripheral = 0;
	int allocatedPeripheralLanes = 0;
	double peripheralLeadLength = 0.0;

	// Working data for the net currently being routed.
	int netIndex = -1;
	QList<ConnectorItem *> breadboardAnchors;
	QList<ConnectorItem *> offBoardTerminals;
	QList<QList<ConnectorItem *> > groups;

	NetRoutingPass(BreadboardAutorouter *router, QUndoCommand *command)
		: self(router)
		, parentCommand(command)
	{
		BreadboardTopology topology;
		topology.discover(self->m_sketchWidget->scene(), self->m_sketchWidget->scene()->selectedItems());
		routeHoles = topology.holes();
		reservedHoles = topology.reservedHoles();
		holeBounds = boundsForHoles(routeHoles);
		session = RouteGraphSession::build(
			routeHoles,
			[this](ConnectorItem *connectorItem) { return self->busGroupFor(connectorItem); },
			[this](int busGroup) { return self->busOwner(busGroup); });

		const BreadboardRouteGraph::Options activeOptions = BreadboardRouteGraph::Options::fromEnvironment();
		self->logAutoroute(QString("route options: maxJumperLength=%1 candidatesPerBusPair=%2 crossingPenalty=%3 overlapPenalty=%4")
						   .arg(activeOptions.maxJumperLength)
						   .arg(activeOptions.candidatesPerBusPair)
						   .arg(activeOptions.crossingPenalty)
						   .arg(activeOptions.overlapPenalty));
	}

	// Wire a lead from an off-board terminal to a board hole, with all the
	// bookkeeping a peripheral lead requires (lead-length accounting so it
	// is not counted as a board jumper, reservation, bus claim).
	void addPeripheralLead(ConnectorItem *terminal, ConnectorItem *entry)
	{
		self->m_sketchWidget->createWire(terminal, entry, generatedWireFlags(), false, BaseCommand::SingleView, parentCommand);
		const QLineF lead = connectorLine(terminal, entry);
		plannedSegments.append(lead);
		peripheralLeadLength += lead.length();
		reservedHoles.insert(entry);
		self->claimBus(self->busGroupFor(entry), netIndex);
		created++;
		wiredPeripheral++;
	}

	// Wire one board jumper segment produced by the route graph, claiming
	// both end buses for the net (prime invariant bookkeeping).
	void addGraphWire(const BreadboardRouteGraph::Segment &segment)
	{
		self->m_sketchWidget->createWire(segment.from, segment.to, generatedWireFlags(), false, BaseCommand::SingleView, parentCommand);
		plannedSegments.append(connectorLine(segment.from, segment.to));
		reservedHoles.insert(segment.from);
		reservedHoles.insert(segment.to);
		self->claimBus(self->busGroupFor(segment.from), netIndex);
		self->claimBus(self->busGroupFor(segment.to), netIndex);
		created++;
	}

	// Route nets hardest-first so constrained nets grab scarce buses before
	// easy nets consume them. Difficulty is precomputed: the comparator
	// would otherwise re-run subnet grouping O(n log n) times.
	QList<int> netOrderMostConstrainedFirst() const
	{
		QList<int> routeOrder;
		for (int index = 0; index < self->m_allPartConnectorItems.count(); index++)
			routeOrder.append(index);

		QHash<int, double> difficultyForNet;
		Q_FOREACH (int index, routeOrder)
		{
			QList<ConnectorItem *> *net = self->m_allPartConnectorItems.value(index);
			if (net == nullptr)
			{
				difficultyForNet.insert(index, 0.0);
				continue;
			}
			const QList<ConnectorItem *> candidates = self->routingCandidatesForSubnet(*net);
			const int groupCount = self->collectCandidateGroups(candidates).count();
			QRectF bounds;
			Q_FOREACH (ConnectorItem *candidate, candidates) {
				if (candidate == nullptr) continue;
				const QRectF point(candidate->sceneAdjustedTerminalPoint(nullptr), QSizeF(1.0, 1.0));
				bounds = bounds.isNull() ? point : bounds | point;
			}
			difficultyForNet.insert(index, groupCount * 100000.0 + candidates.count() * 1000.0 + bounds.width() + bounds.height());
		}
		std::sort(routeOrder.begin(), routeOrder.end(), [&difficultyForNet](int firstIndex, int secondIndex) {
			return difficultyForNet.value(firstIndex) > difficultyForNet.value(secondIndex);
		});
		return routeOrder;
	}

	// Split the net's connectors into breadboard anchors (pins already in
	// holes) and off-board peripheral terminals that need jumper leads.
	void collectTerminals(const QList<ConnectorItem *> &net)
	{
		breadboardAnchors.clear();
		offBoardTerminals.clear();
		Q_FOREACH (ConnectorItem *connectorItem, net)
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
			if (!self->isPlaceablePin(connectorItem) && connectorItem->connectorType() != Connector::Female)
				continue;

			ConnectorItem *connectedHole = self->connectedBreadboardHoleFor(connectorItem);
			if (connectedHole != nullptr)
			{
				if (!breadboardAnchors.contains(connectedHole))
					breadboardAnchors.append(connectedHole);
				continue;
			}

			// Female sockets on peripheral parts (breakout headers) are
			// legitimate jumper terminals; only breadboard holes are excluded
			// (they became anchors above via connectedBreadboardHoleFor).
			BreadboardPartPolicy::Decision policy = BreadboardPartPolicy::classify(itemBase->layerKinChief());
			if (policy.classification == BreadboardPartPolicy::Classification::Peripheral && !offBoardTerminals.contains(connectorItem))
			{
				offBoardTerminals.append(connectorItem);
				self->logAutoroute(QString("route peripheral terminal: net=%1 terminal=%2 class=%3 reason=%4")
								   .arg(netIndex)
								   .arg(self->connectorSummary(connectorItem))
								   .arg(BreadboardPartPolicy::classificationName(policy.classification))
								   .arg(policy.reason));
			}
			else if (policy.classification == BreadboardPartPolicy::Classification::BoardPlaceable && connectorItem->connectorType() != Connector::Female)
			{
				self->logAutoroute(QString("route skip board-placeable offboard terminal: net=%1 terminal=%2 reason=not a peripheral")
								   .arg(netIndex)
								   .arg(self->connectorSummary(connectorItem)));
			}
		}
	}

	// A net with NO board presence yet (all terminals off-board, e.g. a pot
	// wired straight to a jack) gets a "lane": one board entry hole per
	// terminal, chosen near the board edge facing it, plus graph routes
	// joining those entries into one electrical group. All-or-nothing: the
	// lane is only wired when every terminal found an entry and every entry
	// pair found a route.
	void wirePeripheralLane()
	{
		if (!breadboardAnchors.isEmpty() || offBoardTerminals.count() <= 1)
			return;

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
				if (entry == nullptr || reservedHoles.contains(entry) || usedEntries.contains(entry))
					continue;
				if (entry->connectionsCount() > 0)
					continue;
				if (!self->busAvailableFor(self->busGroupFor(entry), netIndex))
					continue;
				const double score = boardEdgeEntryScore(terminal, entry, holeBounds)
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
				failedNetIndices.insert(netIndex);
				self->logAutoroute(QString("route peripheral entry skipped: net=%1 terminal=%2 no free edge entry")
								   .arg(netIndex)
								   .arg(self->connectorSummary(terminal)));
				break;
			}
			terminalEntries.append(bestEntry);
			usedEntries.insert(bestEntry);
			entryLeadSegments.append(connectorLine(terminal, bestEntry));
			self->logAutoroute(QString("route peripheral entry: net=%1 terminal=%2 entry=%3 score=%4")
							   .arg(netIndex)
							   .arg(self->connectorSummary(terminal))
							   .arg(self->connectorSummary(bestEntry))
							   .arg(bestEntryScore));
		}

		QList<BreadboardRouteGraph::Result> entryRoutes;
		if (entriesReady)
		{
			QSet<ConnectorItem *> temporaryReserved = reservedHoles;
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
				const BreadboardRouteGraphCore::QueryContext entryContext = session.prepare(temporaryReserved, entryPlanningSegments, netIndex);
				Q_FOREACH (ConnectorItem *connectedEntry, connectedEntries)
				{
					BreadboardRouteGraph::Result route = session.route(connectedEntry, targetEntry, entryContext);
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
					failedNetIndices.insert(netIndex);
					self->logAutoroute(QString("route peripheral entries skipped: net=%1 entry=%2 no graph route")
									   .arg(netIndex)
									   .arg(self->connectorSummary(targetEntry)));
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
			self->logAutoroute(QString("route peripheral entries: net=%1 terminals=%2 routes=%3")
							   .arg(netIndex)
							   .arg(offBoardTerminals.count())
							   .arg(entryRoutes.count()));
			for (int terminalIndex = 0; terminalIndex < offBoardTerminals.count(); terminalIndex++)
			{
				ConnectorItem *terminal = offBoardTerminals.at(terminalIndex);
				ConnectorItem *target = terminalEntries.at(terminalIndex);
				if (terminal == nullptr || target == nullptr)
					continue;
				addPeripheralLead(terminal, target);
			}
			Q_FOREACH (const BreadboardRouteGraph::Result &route, entryRoutes)
			{
				Q_FOREACH (const BreadboardRouteGraph::Segment &segment, route.segments)
				{
					if (segment.from == nullptr || segment.to == nullptr || segment.from == segment.to)
						continue;
					addGraphWire(segment);
				}
			}
			breadboardAnchors.append(terminalEntries.first());
			offBoardTerminals.clear();
		}
	}

	// Bridge each remaining off-board terminal to the net's existing board
	// presence: pick the (entry hole, anchor) pair with the best combined
	// lead + graph-route score, wire the lead, then wire the route.
	void bridgeTerminalsToAnchors()
	{
		if (breadboardAnchors.isEmpty() || offBoardTerminals.isEmpty())
			return;

		QSet<ConnectorItem *> usedBridgeTargets;
		Q_FOREACH (ConnectorItem *terminal, offBoardTerminals)
		{
			ConnectorItem *bestAnchor = nullptr;
			ConnectorItem *bestEntry = nullptr;
			BreadboardRouteGraph::Result bestRoute;
			BreadboardRoutingScore bestBridgeScore;
			bool haveBridgeScore = false;
			const BreadboardRouteGraphCore::QueryContext bridgeContext = session.prepare(reservedHoles, plannedSegments, netIndex);
			// One Dijkstra per anchor; each of the ~holes entries below is
			// then a constant-time extract instead of its own search.
			QList<BreadboardRouteGraphCore::MultiResult> anchorRoutes;
			Q_FOREACH (ConnectorItem *anchor, breadboardAnchors)
				anchorRoutes.append(session.routeFrom(anchor, bridgeContext));

			Q_FOREACH (ConnectorItem *entry, routeHoles)
			{
				if (entry == nullptr || reservedHoles.contains(entry) || usedBridgeTargets.contains(entry))
					continue;
				if (entry->connectionsCount() > 0)
					continue;
				if (!self->busAvailableFor(self->busGroupFor(entry), netIndex))
					continue;
				const double entryScore = boardEdgeEntryScore(terminal, entry, holeBounds)
				                        + leadCongestionPenalty(terminal, entry, plannedSegments);
				if (entryScore == std::numeric_limits<double>::max())
					continue;

				for (int anchorIndex = 0; anchorIndex < breadboardAnchors.count(); anchorIndex++)
				{
					ConnectorItem *anchor = breadboardAnchors.at(anchorIndex);
					if (anchor == nullptr)
						continue;
					BreadboardRouteGraph::Result route = session.extract(anchorRoutes.at(anchorIndex), entry);
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
				failedNetIndices.insert(netIndex);
				self->logAutoroute(QString("route bridge skipped: net=%1 terminal=%2 no breadboard target")
								   .arg(netIndex)
								   .arg(self->connectorSummary(terminal)));
				continue;
			}

			self->logAutoroute(QString("route bridge: net=%1 terminal=%2 anchor=%3 entry=%4 segments=%5 score=%6")
							   .arg(netIndex)
							   .arg(self->connectorSummary(terminal))
							   .arg(self->connectorSummary(bestAnchor))
							   .arg(self->connectorSummary(bestEntry))
							   .arg(bestRoute.segments.count())
							   .arg(bestBridgeScore.toString()));
			addPeripheralLead(terminal, bestEntry);
			usedBridgeTargets.insert(bestEntry);

			Q_FOREACH (const BreadboardRouteGraph::Segment &segment, bestRoute.segments)
			{
				if (segment.from == nullptr || segment.to == nullptr || segment.from == segment.to)
					continue;
				addGraphWire(segment);
				self->logAutoroute(QString("route bridge graph wire: net=%1 from=%2 to=%3 cost=%4")
								   .arg(netIndex)
								   .arg(self->connectorSummary(segment.from))
								   .arg(self->connectorSummary(segment.to))
								   .arg(segment.cost));
			}
		}
	}

	// Merge the net's electrically separate groups with graph-routed
	// jumpers, always joining the cheapest available pair first, until one
	// group remains or no legal route exists.
	void mergeSubnetGroups()
	{
		if (groups.count() < 2)
			return;

		while (groups.count() > 1)
		{
			int bestFromSubnet = -1;
			int bestToSubnet = -1;
			BreadboardRouteGraph::Result bestRoute;
			BreadboardRoutingScore bestRoutingScore;
			double bestTieBreak = std::numeric_limits<double>::max();
			bool haveBestScore = false;
			const BreadboardRouteGraphCore::QueryContext mergeContext = session.prepare(reservedHoles, plannedSegments, netIndex);

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
							BreadboardRouteGraph::Result route = session.route(fromCandidate, toCandidate, mergeContext);
							if (!route.found)
								continue;
							const double tieBreak = self->routeScore(fromCandidate, toCandidate);
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
				failedNetIndices.insert(netIndex);
				self->logAutoroute(QString("route graph failed: net=%1 groups=%2 buses=%3 edges=%4")
								   .arg(netIndex)
								   .arg(groups.count())
								   .arg(session.core->busCount())
								   .arg(session.core->edgeCount()));
				return;
			}

			self->logAutoroute(QString("route choose: net=%1 fromGroup=%2 toGroup=%3 segments=%4 score=%5")
							   .arg(netIndex)
							   .arg(bestFromSubnet)
							   .arg(bestToSubnet)
							   .arg(bestRoute.segments.count())
							   .arg(bestRoutingScore.toString()));
			Q_FOREACH (const BreadboardRouteGraph::Segment &segment, bestRoute.segments)
			{
				if (segment.from == nullptr || segment.to == nullptr || segment.from == segment.to)
					continue;
				addGraphWire(segment);
				self->logAutoroute(QString("route graph wire: net=%1 from=%2 to=%3 cost=%4")
								   .arg(netIndex)
								   .arg(self->connectorSummary(segment.from))
								   .arg(self->connectorSummary(segment.to))
								   .arg(segment.cost));
			}

			groups[bestFromSubnet].append(groups.at(bestToSubnet));
			groups.removeAt(bestToSubnet);
		}
	}

	// Route one net: log its electrical groups, split terminals into
	// anchors and off-board peripherals, then run the three wiring phases.
	void routeNet(int index, QList<ConnectorItem *> *net)
	{
		netIndex = index;
		const QList<ConnectorItem *> candidates = self->routingCandidatesForSubnet(*net);
		groups = self->collectCandidateGroups(candidates);
		self->logAutoroute(QString("route net %1: netConnectors=%2 candidates=%3 groups=%4")
						   .arg(netIndex)
						   .arg(net->count())
						   .arg(candidates.count())
						   .arg(groups.count()));
		for (int groupIndex = 0; groupIndex < groups.count(); groupIndex++)
		{
			QStringList connectorLines;
			const QList<ConnectorItem *> group = groups.at(groupIndex);
			for (int connectorIndex = 0; connectorIndex < group.count() && connectorIndex < 6; connectorIndex++)
				connectorLines.append(self->connectorSummary(group.at(connectorIndex)));
			self->logAutoroute(QString("route net %1 group %2 size=%3 sample=[%4]")
							   .arg(netIndex)
							   .arg(groupIndex)
							   .arg(group.count())
							   .arg(connectorLines.join(" | ")));
		}

		collectTerminals(*net);
		wirePeripheralLane();
		bridgeTerminalsToAnchors();
		mergeSubnetGroups();
	}

	// Fill in the run's score and counters once every net is done.
	void finish()
	{
		double jumperLength = 0.0;
		Q_FOREACH (const QLineF &segment, plannedSegments) jumperLength += segment.length();
		jumperLength = qMax(0.0, jumperLength - peripheralLeadLength);
		self->m_lastRoutingScore.failedNets = failedNetIndices.count();
		self->m_lastRoutingScore.jumperCount = created - wiredPeripheral;
		self->m_lastRoutingScore.jumperLength = jumperLength;
		self->m_lastRoutingScore.componentLeadLength = self->m_componentLeadLength;
		self->logAutoroute(QString("route counters: totalWires=%1 boardJumpers=%2 wiredPeripheral=%3 peripheralLeadLength=%4 allocatedPeripheralLanes=%5 failedNets=%6")
						   .arg(created)
						   .arg(created - wiredPeripheral)
						   .arg(wiredPeripheral)
						   .arg(peripheralLeadLength)
						   .arg(allocatedPeripheralLanes)
						   .arg(failedNetIndices.count()));
	}
};

int BreadboardAutorouter::routeCollectedNets(QUndoCommand *parentCommand)
{
	NetRoutingPass pass(this, parentCommand);

	const QList<int> routeOrder = pass.netOrderMostConstrainedFirst();
	QStringList routeOrderText;
	Q_FOREACH (int netIndex, routeOrder) routeOrderText.append(QString::number(netIndex));
	logAutoroute(QString("route order (most constrained first): %1").arg(routeOrderText.join(",")));

	for (int orderIndex = 0; orderIndex < routeOrder.count(); orderIndex++)
	{
		reportProgress(PlacementProgressSpan
						   + ((RoutingProgressEnd - PlacementProgressSpan) * orderIndex) / qMax(1, routeOrder.count()),
					   QObject::tr("Routing net %1 of %2...").arg(orderIndex + 1).arg(routeOrder.count()));
		const int netIndex = routeOrder.at(orderIndex);
		QList<ConnectorItem *> *net = m_allPartConnectorItems.at(netIndex);
		if (net == nullptr)
			continue;
		pass.routeNet(netIndex, net);
	}

	pass.finish();
	return pass.created;
}

QList<QList<ConnectorItem *>> BreadboardAutorouter::collectCandidateGroups(const QList<ConnectorItem *> &candidates) const
{
	QList<ConnectorItem *> validCandidates;
	Q_FOREACH (ConnectorItem *candidate, candidates)
	{
		if (candidate != nullptr && !validCandidates.contains(candidate))
			validCandidates.append(candidate);
	}

	QVector<int> parents(validCandidates.count());
	for (int i = 0; i < parents.count(); i++) parents[i] = i;
	auto findRoot = [&parents](int value) {
		int root = value;
		while (parents[root] != root) root = parents[root];
		while (parents[value] != value) {
			const int next = parents[value];
			parents[value] = root;
			value = next;
		}
		return root;
	};
	auto unite = [&parents, &findRoot](int first, int second) {
		const int firstRoot = findRoot(first);
		const int secondRoot = findRoot(second);
		if (firstRoot != secondRoot) parents[secondRoot] = firstRoot;
	};

	// Breadboard connectivity is deliberately narrower than Fritzing's global
	// equal-potential graph. A ratsnest describes intent, not copper. Only a
	// discovered breadboard bus or a real Breadboard View wire joins groups.
	for (int first = 0; first < validCandidates.count(); first++)
	{
		for (int second = first + 1; second < validCandidates.count(); second++)
		{
			if (connectorsShareBreadboardBus(validCandidates.at(first), validCandidates.at(second)))
				unite(first, second);
		}
	}

	using WireEnds = QPair<ConnectorItem *, ConnectorItem *>;
	Q_FOREACH (const WireEnds &wireEnds, normalBreadboardWireEnds())
	{
		ConnectorItem *from = wireEnds.first;
		ConnectorItem *to = wireEnds.second;

		for (int first = 0; first < validCandidates.count(); first++)
		{
			if (!connectorsShareBreadboardBus(validCandidates.at(first), from))
				continue;
			for (int second = 0; second < validCandidates.count(); second++)
			{
				if (connectorsShareBreadboardBus(validCandidates.at(second), to))
					unite(first, second);
			}
		}
	}

	// Emit groups ordered by first-member appearance (not QHash::values(),
	// whose order follows the per-process hash seed): with the candidate
	// list deterministic, group order - and so routing order - is too.
	QList<QList<ConnectorItem *>> groups;
	QHash<int, int> groupIndexByRoot;
	for (int i = 0; i < validCandidates.count(); i++)
	{
		const int root = findRoot(i);
		if (!groupIndexByRoot.contains(root))
		{
			groupIndexByRoot.insert(root, groups.count());
			groups.append(QList<ConnectorItem *>());
		}
		groups[groupIndexByRoot.value(root)].append(validCandidates.at(i));
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
	if (first == nullptr || second == nullptr)
		return false;
	if (first == second)
		return true;
	return busGroupFor(first) == busGroupFor(second);
}

int BreadboardAutorouter::busGroupFor(ConnectorItem *connectorItem) const
{
	auto found = m_busGroupForConnector.constFind(connectorItem);
	if (found != m_busGroupForConnector.constEnd())
		return found.value();

	const int groupId = m_busGroupCount++;
	m_busGroupForConnector.insert(connectorItem, groupId);
	ItemBase *item = connectorItem->attachedTo();
	QList<ConnectorItem *> busSiblings;
	if (item != nullptr && item->busConnectorItems(connectorItem, busSiblings))
	{
		Q_FOREACH (ConnectorItem *sibling, busSiblings)
		{
			if (sibling != nullptr && !m_busGroupForConnector.contains(sibling))
				m_busGroupForConnector.insert(sibling, groupId);
		}
	}
	return groupId;
}

const QList<QPair<ConnectorItem *, ConnectorItem *> > &BreadboardAutorouter::normalBreadboardWireEnds() const
{
	if (m_wireEndsCacheValid)
		return m_normalBreadboardWireEnds;

	m_normalBreadboardWireEnds.clear();
	Q_FOREACH (QGraphicsItem *graphicsItem, m_sketchWidget->scene()->items())
	{
		auto *wire = dynamic_cast<Wire *>(graphicsItem);
		if (wire == nullptr || wire->getRatsnest() || !wire->getNormal())
			continue;
		if (wire->viewID() != ViewLayer::BreadboardView)
			continue;

		ConnectorItem *from = routingConnectorFor(wire->connector0());
		ConnectorItem *to = routingConnectorFor(wire->connector1());
		if (from == nullptr || to == nullptr)
			continue;
		m_normalBreadboardWireEnds.append(qMakePair(from, to));
	}
	m_wireEndsCacheValid = true;
	return m_normalBreadboardWireEnds;
}

int BreadboardAutorouter::busOwner(int busGroup) const
{
	return m_busOwnerForGroup.value(busGroup, -1);
}

bool BreadboardAutorouter::busAvailableFor(int busGroup, int ownerKey) const
{
	const int owner = busOwner(busGroup);
	return owner == -1 || owner == ownerKey;
}

void BreadboardAutorouter::claimBus(int busGroup, int ownerKey)
{
	if (!m_busOwnerForGroup.contains(busGroup))
		m_busOwnerForGroup.insert(busGroup, ownerKey);
}

int BreadboardAutorouter::makeNoNetOwnerKey()
{
	return m_nextNoNetOwnerKey--;
}

int BreadboardAutorouter::ownerKeyForPin(ConnectorItem *pin) const
{
	// Netted pins share their net's key; a no-net pin gets no shared key
	// here - callers claim with a fresh sentinel so nothing may join it.
	return m_netForConnector.value(pin, INT_MIN);
}

void BreadboardAutorouter::seedBusOwnership()
{
	m_busOwnerForGroup.clear();
	m_nextNoNetOwnerKey = -2;
	m_netForConnector.clear();
	for (int netIndex = 0; netIndex < m_allPartConnectorItems.count(); netIndex++)
	{
		QList<ConnectorItem *> *net = m_allPartConnectorItems.at(netIndex);
		if (net == nullptr)
			continue;
		Q_FOREACH (ConnectorItem *connectorItem, *net)
		{
			if (connectorItem == nullptr)
				continue;
			m_netForConnector.insert(connectorItem, netIndex);
			// Only a real male PART pin actually plugged into a hole claims a
			// bus at seed time. Female connectors are breadboard holes/sockets
			// (they are net members but occupy nothing themselves), and
			// connectedBreadboardHoleFor returns a female pin as itself - which
			// would wrongly claim every hosting bus before placement even runs.
			if (connectorItem->connectorType() == Connector::Female)
				continue;
			if (connectorItem->attachedToItemType() == ModelPart::Wire)
				continue;
			ConnectorItem *hole = connectedBreadboardHoleFor(connectorItem);
			if (hole == nullptr || hole->connectorType() != Connector::Female)
				continue;
			const int busGroup = busGroupFor(hole);
			const int owner = busOwner(busGroup);
			if (owner == -1)
				claimBus(busGroup, netIndex);
			else if (owner != netIndex)
				logAutoroute(QString("bus ownership seed conflict (pre-existing): bus=%1 owner=net%2 also touched by net%3 pin=%4")
								 .arg(busGroup)
								 .arg(owner)
								 .arg(netIndex)
								 .arg(connectorSummary(connectorItem)));
		}
	}

	// Pins with no net already sitting in holes (pre-placed parts) claim
	// their buses exclusively.
	Q_FOREACH (QGraphicsItem *graphicsItem, m_sketchWidget->scene()->items())
	{
		auto *connectorItem = dynamic_cast<ConnectorItem *>(graphicsItem);
		if (connectorItem == nullptr || connectorItem->attachedTo() == nullptr)
			continue;
		if (connectorItem->attachedTo()->getRatsnest() || !connectorItem->attachedTo()->isEverVisible())
			continue;
		if (connectorItem->connectorType() == Connector::Female)
			continue;
		if (connectorItem->attachedToItemType() == ModelPart::Wire)
			continue;
		if (m_netForConnector.contains(connectorItem))
			continue;
		ConnectorItem *hole = connectedBreadboardHoleFor(connectorItem);
		if (hole == nullptr)
			continue;
		const int busGroup = busGroupFor(hole);
		if (busOwner(busGroup) == -1)
			claimBus(busGroup, makeNoNetOwnerKey());
	}
	logAutoroute(QString("bus ownership seeded: ownedBuses=%1 nettedPins=%2").arg(m_busOwnerForGroup.count()).arg(m_netForConnector.count()));
}

bool BreadboardAutorouter::verifySchematicConformance(QStringList &violations, bool recordBaseline)
{
	// Independent short detector, breadboard-view only. Union bus groups
	// that are joined by real breadboard wires, then assert no resulting
	// component carries pins from two different owners (schematic nets, or a
	// no-net pin such as an unused DIP output). Deliberately does NOT walk
	// cross-layer, so it can never mistake the schematic netlist itself for
	// a short. Pre-existing owner contacts are exempt.

	// Owner key per part pin: its schematic net index, or a unique negative
	// id for a no-net pin (each no-net pin is its own singleton owner - it
	// must never share a component with any other owner). Only real MALE
	// part pins count: m_netForConnector also holds female breadboard holes
	// (net members that occupy nothing), which would produce phantom shorts.
	QHash<ConnectorItem *, int> ownerForPin;
	for (auto it = m_netForConnector.constBegin(); it != m_netForConnector.constEnd(); ++it)
	{
		ConnectorItem *pin = it.key();
		if (pin == nullptr || pin->connectorType() == Connector::Female)
			continue;
		if (pin->attachedToItemType() == ModelPart::Wire)
			continue;
		ownerForPin.insert(pin, it.value());
	}
	int nextNoNetId = -2;
	Q_FOREACH (QGraphicsItem *graphicsItem, m_sketchWidget->scene()->items())
	{
		auto *pin = dynamic_cast<ConnectorItem *>(graphicsItem);
		if (pin == nullptr || pin->attachedTo() == nullptr)
			continue;
		if (pin->attachedTo()->getRatsnest() || !pin->attachedTo()->isEverVisible())
			continue;
		if (pin->connectorType() == Connector::Female || pin->attachedToItemType() == ModelPart::Wire)
			continue;
		if (ownerForPin.contains(pin))
			continue;
		if (connectedBreadboardHoleFor(pin) != nullptr)
			ownerForPin.insert(pin, nextNoNetId--);
	}

	// Union-find over bus groups.
	QHash<int, int> parent;
	std::function<int(int)> findRoot = [&](int value) {
		int root = value;
		while (parent.value(root, root) != root) root = parent.value(root, root);
		while (parent.value(value, value) != value) {
			const int next = parent.value(value, value);
			parent[value] = root;
			value = next;
		}
		return root;
	};
	auto unite = [&](int first, int second) {
		const int firstRoot = findRoot(first);
		const int secondRoot = findRoot(second);
		if (firstRoot != secondRoot) parent[firstRoot] = secondRoot;
	};

	// Join buses connected by breadboard wires. Component bodies are NOT
	// wires, so intended part-to-part netlist connections are not unioned -
	// only breadboard copper is.
	using WireEnds = QPair<ConnectorItem *, ConnectorItem *>;
	Q_FOREACH (const WireEnds &ends, normalBreadboardWireEnds())
	{
		ConnectorItem *fromHole = connectedBreadboardHoleFor(ends.first);
		ConnectorItem *toHole = connectedBreadboardHoleFor(ends.second);
		if (fromHole == nullptr || toHole == nullptr)
			continue;
		unite(busGroupFor(fromHole), busGroupFor(toHole));
	}

	// Collect the distinct owners on each component.
	QHash<int, QList<QPair<int, ConnectorItem *> > > ownersByComponent;
	for (auto it = ownerForPin.constBegin(); it != ownerForPin.constEnd(); ++it)
	{
		ConnectorItem *hole = connectedBreadboardHoleFor(it.key());
		if (hole == nullptr)
			continue;
		ownersByComponent[findRoot(busGroupFor(hole))].append(qMakePair(it.value(), it.key()));
	}

	for (auto it = ownersByComponent.constBegin(); it != ownersByComponent.constEnd(); ++it)
	{
		const QList<QPair<int, ConnectorItem *> > &pins = it.value();
		for (int a = 0; a < pins.count(); a++)
		{
			for (int b = a + 1; b < pins.count(); b++)
			{
				if (pins.at(a).first == pins.at(b).first)
					continue;
				const QPair<int, int> contact(qMin(pins.at(a).first, pins.at(b).first),
											  qMax(pins.at(a).first, pins.at(b).first));
				if (recordBaseline)
				{
					m_preExistingNetContacts.insert(contact);
					continue;
				}
				if (m_preExistingNetContacts.contains(contact))
					continue;
				violations.append(QObject::tr("%1 and %2 are joined by breadboard wiring but belong to different schematic nets")
									  .arg(connectorSummary(pins.at(a).second))
									  .arg(connectorSummary(pins.at(b).second)));
			}
		}
	}
	return violations.isEmpty();
}

void BreadboardAutorouter::invalidateRoutingCaches()
{
	m_wireEndsCacheValid = false;
	m_normalBreadboardWireEnds.clear();
	m_busGroupForConnector.clear();
	m_busGroupCount = 0;
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

void BreadboardAutorouter::loadTuning()
{
	QSettings settings;
	m_maxLegLength = settings.value("breadboardAutorouter/leadStretchLimit", 120.0).toDouble();
	m_leadLengthWeight = settings.value("breadboardAutorouter/leadLengthWeight", 1.0).toDouble();
	m_jumperPenalty = settings.value("breadboardAutorouter/jumperPenalty", 100000.0).toDouble();
	m_leadAngleWeight = settings.value("breadboardAutorouter/leadAngleWeight", 4.0).toDouble();
	m_foldbackWeight = settings.value("breadboardAutorouter/foldbackWeight", 6.0).toDouble();
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
	m_logBuffer.append(QDateTime::currentDateTime().toString(Qt::ISODateWithMs) + " " + message);
	// Cap memory without losing the tail on a crash mid-phase.
	if (m_logBuffer.count() >= 512)
		flushAutorouteLog();
}

void BreadboardAutorouter::flushAutorouteLog() const
{
	if (m_logBuffer.isEmpty())
		return;

	QFile file(logFilePath());
	if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
		return;

	QTextStream stream(&file);
	Q_FOREACH (const QString &line, m_logBuffer)
		stream << line << '\n';
	m_logBuffer.clear();
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
		return isBreadboardHoleConnector(partConnector) ? partConnector : nullptr;

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
		if (isBreadboardHoleConnector(connectorItem))
			return connectorItem;
	}

	return nullptr;
}

ConnectorItem *BreadboardAutorouter::breadboardHoleFor(ConnectorItem *partConnector) const
{
	if (partConnector == nullptr)
		return nullptr;
	if (partConnector->connectorType() == Connector::Female)
		return isBreadboardHoleConnector(partConnector) ? partConnector : nullptr;

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
		if (!isBreadboardHoleConnector(connectorItem))
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

		// collectEqualPotential returns members in hash order, which Qt
		// randomizes per process. Anchor choice (and therefore jumper
		// layout) follows member order, so sort by stable ids to make
		// autoroute results reproducible run to run.
		std::sort(subnet.begin(), subnet.end(), [](ConnectorItem *a, ConnectorItem *b)
				  {
			if (a == nullptr || b == nullptr) return b == nullptr && a != nullptr;
			if (a->attachedToID() != b->attachedToID()) return a->attachedToID() < b->attachedToID();
			return a->connectorSharedID() < b->connectorSharedID(); });

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

// Update the progress dialog and (rate-limited) let it repaint. The router
// runs synchronously on the GUI thread, so nothing paints unless events are
// pumped - but pumping unconditionally per part or per net would thrash the
// GUI engine with repaints. ~5 pumps per second keeps the bar live for free.
void BreadboardAutorouter::reportProgress(int percent, const QString &detail)
{
	Q_EMIT setProgressValue(percent);
	if (!detail.isEmpty())
		Q_EMIT setProgressMessage2(detail);
	if (m_progressPumpTimer.isValid() && m_progressPumpTimer.elapsed() < 200)
		return;
	m_progressPumpTimer.restart();
	ProcessEventBlocker::processEvents();
}

// collectAllNets walks hash-ordered structures, so both net order and member
// order vary with Qt's per-process hash seed. Anchor selection, group order
// and difficulty tie-breaks all follow list order, so sort by stable ids to
// make autoroute results reproducible run to run.
void BreadboardAutorouter::sortCollectedNets()
{
	auto connectorLess = [](ConnectorItem *a, ConnectorItem *b) {
		if (a == nullptr || b == nullptr)
			return b == nullptr && a != nullptr;
		if (a->attachedToID() != b->attachedToID())
			return a->attachedToID() < b->attachedToID();
		return a->connectorSharedID() < b->connectorSharedID();
	};

	Q_FOREACH (QList<ConnectorItem *> *net, m_allPartConnectorItems)
	{
		if (net != nullptr)
			std::sort(net->begin(), net->end(), connectorLess);
	}

	// Net indices double as bus-ownership keys, and nets are re-collected
	// after placement (holes join the equal-potential groups). The net sort
	// key must therefore ignore members that placement adds: key on the
	// smallest real part pin, which is invariant across both collections,
	// so index i names the same schematic net before and after placement.
	auto netSortKey = [&connectorLess](QList<ConnectorItem *> *net) -> ConnectorItem * {
		if (net == nullptr)
			return nullptr;
		ConnectorItem *best = nullptr;
		Q_FOREACH (ConnectorItem *connectorItem, *net)
		{
			if (connectorItem == nullptr)
				continue;
			if (connectorItem->connectorType() == Connector::Female)
				continue;
			if (connectorItem->attachedToItemType() == ModelPart::Wire)
				continue;
			if (best == nullptr || connectorLess(connectorItem, best))
				best = connectorItem;
		}
		return best;
	};

	std::sort(m_allPartConnectorItems.begin(), m_allPartConnectorItems.end(),
			  [&connectorLess, &netSortKey](QList<ConnectorItem *> *a, QList<ConnectorItem *> *b) {
		ConnectorItem *aKey = netSortKey(a);
		ConnectorItem *bKey = netSortKey(b);
		if (aKey == nullptr || bKey == nullptr)
			return bKey == nullptr && aKey != nullptr;
		return connectorLess(aKey, bKey);
	});
}
