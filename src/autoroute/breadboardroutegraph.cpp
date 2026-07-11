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

#include "breadboardroutegraph.h"

#include <QLineF>
#include <QPointF>
#include <QStringList>
#include <QtMath>
#include <algorithm>
#include <limits>

#include "../connectors/connectoritem.h"

BreadboardRouteGraph::Options BreadboardRouteGraph::Options::fromEnvironment()
{
	Options options;
	bool ok = false;
	const double maxLength = qEnvironmentVariable("FRITZING_BB_MAX_JUMPER_LENGTH").toDouble(&ok);
	if (ok && maxLength > 0.0) options.maxJumperLength = maxLength;
	const int candidates = qEnvironmentVariable("FRITZING_BB_CANDIDATES_PER_BUS_PAIR").toInt(&ok);
	if (ok && candidates > 0) options.candidatesPerBusPair = candidates;
	const double crossing = qEnvironmentVariable("FRITZING_BB_CROSSING_PENALTY").toDouble(&ok);
	if (ok && crossing >= 0.0) options.crossingPenalty = crossing;
	const double overlap = qEnvironmentVariable("FRITZING_BB_OVERLAP_PENALTY").toDouble(&ok);
	if (ok && overlap >= 0.0) options.overlapPenalty = overlap;
	return options;
}

BreadboardRouteGraph::BreadboardRouteGraph(const QList<ConnectorItem *> & holes,
                                           const QSet<ConnectorItem *> & reservedHoles,
                                           const Options & options)
	: m_holes(holes)
	, m_reservedHoles(reservedHoles)
	, m_options(options)
{
	buildBusIndex();
	buildStaticEdges();
}

BreadboardRouteGraph::Result BreadboardRouteGraph::route(ConnectorItem * start, ConnectorItem * target) const
{
	Result result;
	if (start == nullptr || target == nullptr) {
		result.reason = "null endpoint";
		return result;
	}

	const QString startBus = busId(start);
	const QString targetBus = busId(target);
	if (startBus.isEmpty() || targetBus.isEmpty()) {
		result.reason = "endpoint has no breadboard bus";
		return result;
	}
	if (startBus == targetBus) {
		result.found = true;
		return result;
	}

	QSet<QString> settled;
	QHash<QString, BreadboardRoutingScore> best;
	QHash<QString, Edge> cameFrom;
	best.insert(startBus, BreadboardRoutingScore());

	while (true) {
		QString currentBus;
		BreadboardRoutingScore currentScore;
		bool foundCurrent = false;
		for (auto it = best.constBegin(); it != best.constEnd(); ++it) {
			if (settled.contains(it.key())) continue;
			if (!foundCurrent || it.value() < currentScore) {
				currentBus = it.key();
				currentScore = it.value();
				foundCurrent = true;
			}
		}

		if (currentBus.isEmpty()) break;
		if (currentBus == targetBus) break;
		settled.insert(currentBus);

		Q_FOREACH (const Edge & edge, m_edgesByBus.value(currentBus)) {
			if (!edgeAvailable(edge, start, target)) continue;
			BreadboardRoutingScore nextScore = currentScore;
			nextScore.jumperCount++;
			nextScore.jumperLength += edge.length;
			nextScore.congestion += edge.congestion;
			if (best.contains(edge.toBus) && !(nextScore < best.value(edge.toBus))) continue;
			best.insert(edge.toBus, nextScore);
			cameFrom.insert(edge.toBus, edge);
		}
	}

	if (!cameFrom.contains(targetBus)) {
		result.reason = QString("no route %1 -> %2").arg(startBus, targetBus);
		return result;
	}

	QString bus = targetBus;
	while (bus != startBus) {
		const Edge edge = cameFrom.value(bus);
		if (edge.fromBus.isEmpty()) {
			result.reason = QString("broken route backtrace at %1").arg(bus);
			result.segments.clear();
			return result;
		}
		Segment segment;
		segment.from = edge.fromHole;
		segment.to = edge.toHole;
		segment.cost = edge.cost;
		result.segments.prepend(segment);
		result.cost += edge.cost;
		bus = edge.fromBus;
	}

	result.found = true;
	result.score = best.value(targetBus);
	result.cost = result.score.jumperLength + result.score.congestion;
	return result;
}

QString BreadboardRouteGraph::busId(ConnectorItem * connectorItem) const
{
	if (connectorItem == nullptr) return QString();
	QString id = connectorItem->busID();
	if (!id.isEmpty()) return id;
	return fallbackBusId(connectorItem);
}

int BreadboardRouteGraph::busCount() const
{
	return m_holesByBus.count();
}

int BreadboardRouteGraph::edgeCount() const
{
	int count = 0;
	for (auto it = m_edgesByBus.constBegin(); it != m_edgesByBus.constEnd(); ++it) {
		count += it.value().count();
	}
	return count;
}

void BreadboardRouteGraph::buildBusIndex()
{
	Q_FOREACH (ConnectorItem * hole, m_holes) {
		if (hole == nullptr) continue;
		const QString id = busId(hole);
		if (id.isEmpty()) continue;
		if (!m_holesByBus[id].contains(hole)) m_holesByBus[id].append(hole);
	}
}

void BreadboardRouteGraph::buildStaticEdges()
{
	const QStringList buses = m_holesByBus.keys();
	Q_FOREACH (const QString & bus, buses) {
		m_edgesByBus.insert(bus, QList<Edge>());
	}

	for (int i = 0; i < buses.count(); i++) {
		for (int j = i + 1; j < buses.count(); j++) {
			Q_FOREACH (const Edge & edge, bestStaticJumpers(buses.at(i), buses.at(j))) {
				m_edgesByBus[edge.fromBus].append(edge);

				Edge reverse = edge;
				reverse.fromBus = edge.toBus;
				reverse.toBus = edge.fromBus;
				reverse.fromHole = edge.toHole;
				reverse.toHole = edge.fromHole;
				m_edgesByBus[reverse.fromBus].append(reverse);
			}
		}
	}
}

QList<BreadboardRouteGraph::Edge> BreadboardRouteGraph::bestStaticJumpers(const QString & fromBus, const QString & toBus) const
{
	QList<Edge> candidates;
	Q_FOREACH (ConnectorItem * fromHole, m_holesByBus.value(fromBus)) {
		Q_FOREACH (ConnectorItem * toHole, m_holesByBus.value(toBus)) {
			const double length = manhattan(fromHole, toHole);
			if (length > m_options.maxJumperLength) continue;

			Edge edge;
			edge.fromBus = fromBus;
			edge.toBus = toBus;
			edge.fromHole = fromHole;
			edge.toHole = toHole;
			edge.length = length;
			edge.congestion = segmentPenalty(fromHole, toHole);
			edge.cost = length + m_options.jumperPenalty + edge.congestion;
			candidates.append(edge);
		}
	}

	std::sort(candidates.begin(), candidates.end(), [](const Edge & a, const Edge & b) {
		if (!qFuzzyCompare(a.length + 1.0, b.length + 1.0)) return a.length < b.length;
		return a.congestion < b.congestion;
	});
	while (candidates.count() > m_options.candidatesPerBusPair) {
		candidates.removeLast();
	}
	return candidates;
}

bool BreadboardRouteGraph::edgeAvailable(const Edge & edge, ConnectorItem * start, ConnectorItem * target) const
{
	return holeAvailable(edge.fromHole, start, target)
	    && holeAvailable(edge.toHole, start, target);
}

bool BreadboardRouteGraph::holeAvailable(ConnectorItem * hole, ConnectorItem * start, ConnectorItem * target) const
{
	if (hole == nullptr) return false;
	if (hole == start || hole == target) return true;
	if (m_reservedHoles.contains(hole)) return false;
	if (hole->connectionsCount() != 0) return false;
	return true;
}

double BreadboardRouteGraph::manhattan(ConnectorItem * first, ConnectorItem * second)
{
	if (first == nullptr || second == nullptr) return std::numeric_limits<double>::max();
	const QPointF a = first->sceneAdjustedTerminalPoint(nullptr);
	const QPointF b = second->sceneAdjustedTerminalPoint(nullptr);
	return qAbs(a.x() - b.x()) + qAbs(a.y() - b.y());
}

double BreadboardRouteGraph::segmentPenalty(ConnectorItem * first, ConnectorItem * second) const
{
	if (first == nullptr || second == nullptr) return 0.0;
	const QLineF candidate(first->sceneAdjustedTerminalPoint(nullptr), second->sceneAdjustedTerminalPoint(nullptr));
	return congestionPenalty(candidate, m_options.existingSegments, m_options.crossingPenalty, m_options.overlapPenalty);
}

double BreadboardRouteGraph::congestionPenalty(const QLineF & candidate,
                                               const QList<QLineF> & existingSegments,
                                               double crossingPenalty,
                                               double overlapPenalty)
{
	double penalty = 0.0;
	Q_FOREACH (const QLineF & existing, existingSegments) {
		if (sharesEndpoint(candidate, existing)) continue;
		if (collinearOverlap(candidate, existing)) {
			penalty += overlapPenalty;
			continue;
		}
		QPointF intersection;
		if (candidate.intersects(existing, &intersection) == QLineF::BoundedIntersection) {
			penalty += crossingPenalty;
		}
	}
	return penalty;
}

bool BreadboardRouteGraph::sharesEndpoint(const QLineF & first, const QLineF & second)
{
	constexpr double tolerance = 0.5;
	return QLineF(first.p1(), second.p1()).length() <= tolerance
	    || QLineF(first.p1(), second.p2()).length() <= tolerance
	    || QLineF(first.p2(), second.p1()).length() <= tolerance
	    || QLineF(first.p2(), second.p2()).length() <= tolerance;
}

bool BreadboardRouteGraph::collinearOverlap(const QLineF & first, const QLineF & second)
{
	const QPointF a = first.p2() - first.p1();
	const QPointF b = second.p1() - first.p1();
	const QPointF c = second.p2() - first.p1();
	const double crossB = a.x() * b.y() - a.y() * b.x();
	const double crossC = a.x() * c.y() - a.y() * c.x();
	if (qAbs(crossB) > 0.5 || qAbs(crossC) > 0.5) return false;
	const QRectF firstBounds = QRectF(first.p1(), first.p2()).normalized().adjusted(-0.5, -0.5, 0.5, 0.5);
	const QRectF secondBounds = QRectF(second.p1(), second.p2()).normalized();
	return firstBounds.intersects(secondBounds);
}

QString BreadboardRouteGraph::fallbackBusId(ConnectorItem * connectorItem)
{
	if (connectorItem == nullptr) return QString();
	return QString("%1:%2")
	        .arg(connectorItem->attachedToID())
	        .arg(connectorItem->connectorSharedID());
}
