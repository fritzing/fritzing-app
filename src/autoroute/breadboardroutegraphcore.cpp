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

#include "breadboardroutegraphcore.h"

#include <QRectF>
#include <algorithm>
#include <limits>

namespace
{
	inline double manhattan(const QPointF & a, const QPointF & b)
	{
		return qAbs(a.x() - b.x()) + qAbs(a.y() - b.y());
	}
}

BreadboardRouteGraphCore::BreadboardRouteGraphCore(const QVector<QPointF> & holePositions,
												   const QVector<int> & holeBus,
												   const Options & options)
	: m_holePositions(holePositions)
	, m_holeBus(holeBus)
	, m_options(options)
{
	int busCount = 0;
	Q_FOREACH (int bus, m_holeBus)
		busCount = qMax(busCount, bus + 1);
	m_holesByBus.resize(busCount);
	for (int hole = 0; hole < m_holeBus.count() && hole < m_holePositions.count(); hole++)
	{
		const int bus = m_holeBus.at(hole);
		if (bus >= 0)
			m_holesByBus[bus].append(hole);
	}
	buildStaticEdges();
}

void BreadboardRouteGraphCore::buildStaticEdges()
{
	const int busCount = m_holesByBus.count();
	m_edgesByBus.resize(busCount);

	// Bounding box per bus lets distant bus pairs be rejected in O(1)
	// before any hole-pair work.
	QVector<QRectF> busBounds(busCount);
	for (int bus = 0; bus < busCount; bus++)
	{
		QRectF bounds;
		Q_FOREACH (int hole, m_holesByBus.at(bus))
		{
			const QRectF point(m_holePositions.at(hole), QSizeF(0.0, 0.0));
			bounds = bounds.isNull() ? point : bounds | point;
		}
		busBounds[bus] = bounds;
	}
	auto boxManhattanGap = [](const QRectF & a, const QRectF & b) {
		const double dx = qMax(0.0, qMax(b.left() - a.right(), a.left() - b.right()));
		const double dy = qMax(0.0, qMax(b.top() - a.bottom(), a.top() - b.bottom()));
		return dx + dy;
	};

	QVector<Edge> pairCandidates;
	for (int fromBus = 0; fromBus < busCount; fromBus++)
	{
		for (int toBus = fromBus + 1; toBus < busCount; toBus++)
		{
			if (boxManhattanGap(busBounds.at(fromBus), busBounds.at(toBus)) > m_options.maxJumperLength)
				continue;

			pairCandidates.clear();
			Q_FOREACH (int fromHole, m_holesByBus.at(fromBus))
			{
				const QPointF fromPos = m_holePositions.at(fromHole);
				Q_FOREACH (int toHole, m_holesByBus.at(toBus))
				{
					const double length = manhattan(fromPos, m_holePositions.at(toHole));
					if (length > m_options.maxJumperLength)
						continue;
					Edge edge;
					edge.fromBus = fromBus;
					edge.toBus = toBus;
					edge.fromHole = fromHole;
					edge.toHole = toHole;
					edge.length = length;
					pairCandidates.append(edge);
				}
			}
			if (pairCandidates.isEmpty())
				continue;

			// Shortest candidates first; deterministic tie-break on indices.
			std::sort(pairCandidates.begin(), pairCandidates.end(), [](const Edge & a, const Edge & b) {
				if (!qFuzzyCompare(a.length + 1.0, b.length + 1.0)) return a.length < b.length;
				if (a.fromHole != b.fromHole) return a.fromHole < b.fromHole;
				return a.toHole < b.toHole;
			});
			const int keep = qMin(pairCandidates.count(), m_options.candidatesPerBusPair);
			for (int i = 0; i < keep; i++)
			{
				const Edge & edge = pairCandidates.at(i);
				m_edgesByBus[fromBus].append(m_edges.count());
				m_edges.append(edge);

				Edge reverse = edge;
				reverse.fromBus = edge.toBus;
				reverse.toBus = edge.fromBus;
				reverse.fromHole = edge.toHole;
				reverse.toHole = edge.fromHole;
				m_edgesByBus[toBus].append(m_edges.count());
				m_edges.append(reverse);
			}
		}
	}
}

BreadboardRouteGraphCore::QueryContext BreadboardRouteGraphCore::prepareQuery(const QVector<bool> & holeBlocked,
																			  const QVector<bool> & busBlocked,
																			  const QList<QLineF> & congestionSegments) const
{
	QueryContext context;
	context.holeBlocked = holeBlocked;
	context.busBlocked = busBlocked;
	context.edgeCongestion = QVector<double>(m_edges.count(), 0.0);

	// Bounding boxes let most (edge, segment) pairs be rejected without the
	// full intersection tests.
	QVector<QRectF> segmentBounds(congestionSegments.count());
	for (int i = 0; i < congestionSegments.count(); i++)
	{
		const QLineF & segment = congestionSegments.at(i);
		segmentBounds[i] = QRectF(segment.p1(), segment.p2()).normalized().adjusted(-1.0, -1.0, 1.0, 1.0);
	}

	for (int edgeIndex = 0; edgeIndex < m_edges.count(); edgeIndex++)
	{
		const Edge & edge = m_edges.at(edgeIndex);
		const QLineF candidate(m_holePositions.at(edge.fromHole), m_holePositions.at(edge.toHole));
		const QRectF candidateBounds = QRectF(candidate.p1(), candidate.p2()).normalized().adjusted(-1.0, -1.0, 1.0, 1.0);
		double congestion = 0.0;
		for (int i = 0; i < congestionSegments.count(); i++)
		{
			if (!candidateBounds.intersects(segmentBounds.at(i)))
				continue;
			congestion += congestionPenalty(candidate, {congestionSegments.at(i)},
											m_options.crossingPenalty, m_options.overlapPenalty);
		}
		context.edgeCongestion[edgeIndex] = congestion;
	}
	return context;
}

BreadboardRouteGraphCore::MultiResult BreadboardRouteGraphCore::routeFrom(int sourceHole,
																		  const QueryContext & context) const
{
	MultiResult multi;
	if (sourceHole < 0 || sourceHole >= m_holeBus.count())
		return multi;
	const int sourceBus = m_holeBus.at(sourceHole);
	if (sourceBus < 0)
		return multi;

	// A source on a foreign-owned bus is a caller error: routing from it
	// would already imply a schematic-breaking connection.
	if (sourceBus < context.busBlocked.count() && context.busBlocked.at(sourceBus))
		return multi;

	multi.sourceHole = sourceHole;
	multi.sourceBus = sourceBus;
	const int busCount = m_holesByBus.count();
	multi.bestByBus = QVector<BreadboardRoutingScore>(busCount);
	multi.reachedByBus = QVector<bool>(busCount, false);
	multi.cameFromEdge = QVector<int>(busCount, -1);
	multi.edgeQueryCost = QVector<double>(busCount, 0.0);
	multi.reachedByBus[sourceBus] = true;

	auto holeAvailable = [&](int hole) {
		if (hole == sourceHole)
			return true;
		return hole >= 0 && hole < context.holeBlocked.count() ? !context.holeBlocked.at(hole) : true;
	};

	QVector<bool> settled(busCount, false);
	while (true)
	{
		int currentBus = -1;
		for (int bus = 0; bus < busCount; bus++)
		{
			if (!multi.reachedByBus.at(bus) || settled.at(bus))
				continue;
			if (currentBus < 0 || multi.bestByBus.at(bus) < multi.bestByBus.at(currentBus))
				currentBus = bus;
		}
		if (currentBus < 0)
			break;
		settled[currentBus] = true;

		Q_FOREACH (int edgeIndex, m_edgesByBus.at(currentBus))
		{
			const Edge & edge = m_edges.at(edgeIndex);
			// No endpoint exemption for buses: landing on or passing through
			// a foreign-owned bus electrically breaks the schematic.
			if (edge.toBus < context.busBlocked.count() && context.busBlocked.at(edge.toBus))
				continue;
			if (!holeAvailable(edge.fromHole) || !holeAvailable(edge.toHole))
				continue;

			const double congestion = context.edgeCongestion.value(edgeIndex, 0.0);

			BreadboardRoutingScore nextScore = multi.bestByBus.at(currentBus);
			nextScore.jumperCount++;
			nextScore.jumperLength += edge.length;
			nextScore.congestion += congestion;
			if (multi.reachedByBus.at(edge.toBus) && !(nextScore < multi.bestByBus.at(edge.toBus)))
				continue;
			multi.bestByBus[edge.toBus] = nextScore;
			multi.reachedByBus[edge.toBus] = true;
			multi.cameFromEdge[edge.toBus] = edgeIndex;
			multi.edgeQueryCost[edge.toBus] = edge.length + m_options.jumperPenalty + congestion;
			settled[edge.toBus] = false;
		}
	}
	return multi;
}

BreadboardRouteGraphCore::Result BreadboardRouteGraphCore::extractRoute(const MultiResult & multi,
																		int targetHole) const
{
	Result result;
	if (multi.sourceBus < 0)
	{
		result.reason = "invalid source";
		return result;
	}
	if (targetHole < 0 || targetHole >= m_holeBus.count())
	{
		result.reason = "endpoint out of range";
		return result;
	}
	const int targetBus = m_holeBus.at(targetHole);
	if (targetBus < 0)
	{
		result.reason = "endpoint has no breadboard bus";
		return result;
	}
	if (targetBus == multi.sourceBus)
	{
		result.found = true;
		return result;
	}
	if (!multi.reachedByBus.at(targetBus) || multi.cameFromEdge.at(targetBus) < 0)
	{
		result.reason = QString("no route bus%1 -> bus%2").arg(multi.sourceBus).arg(targetBus);
		return result;
	}

	int bus = targetBus;
	while (bus != multi.sourceBus)
	{
		const int edgeIndex = multi.cameFromEdge.at(bus);
		if (edgeIndex < 0)
		{
			result.reason = QString("broken route backtrace at bus%1").arg(bus);
			result.segments.clear();
			return result;
		}
		const Edge & edge = m_edges.at(edgeIndex);
		Segment segment;
		segment.fromHole = edge.fromHole;
		segment.toHole = edge.toHole;
		segment.cost = multi.edgeQueryCost.at(bus);
		result.segments.prepend(segment);
		result.cost += segment.cost;
		bus = edge.fromBus;
	}

	result.found = true;
	result.score = multi.bestByBus.at(targetBus);
	result.cost = result.score.jumperLength + result.score.congestion;
	return result;
}

BreadboardRouteGraphCore::Result BreadboardRouteGraphCore::route(int startHole, int targetHole,
																 const QueryContext & context) const
{
	Result result;
	if (startHole < 0 || startHole >= m_holeBus.count()
		|| targetHole < 0 || targetHole >= m_holeBus.count())
	{
		result.reason = "endpoint out of range";
		return result;
	}
	if (m_holeBus.at(startHole) < 0 || m_holeBus.at(targetHole) < 0)
	{
		result.reason = "endpoint has no breadboard bus";
		return result;
	}

	// Pairwise routing rides the single-source machinery with the target
	// hole additionally exempted from blocking (matching original
	// semantics where both endpoints were always usable).
	QueryContext exempted = context;
	if (targetHole < exempted.holeBlocked.count())
		exempted.holeBlocked[targetHole] = false;
	const MultiResult multi = routeFrom(startHole, exempted);
	return extractRoute(multi, targetHole);
}

double BreadboardRouteGraphCore::congestionPenalty(const QLineF & candidate,
												   const QList<QLineF> & existingSegments,
												   double crossingPenalty,
												   double overlapPenalty)
{
	double penalty = 0.0;
	Q_FOREACH (const QLineF & existing, existingSegments)
	{
		if (sharesEndpoint(candidate, existing))
			continue;
		if (collinearOverlap(candidate, existing))
		{
			penalty += overlapPenalty;
			continue;
		}
		QPointF intersection;
		if (candidate.intersects(existing, &intersection) == QLineF::BoundedIntersection)
			penalty += crossingPenalty;
	}
	return penalty;
}

bool BreadboardRouteGraphCore::sharesEndpoint(const QLineF & first, const QLineF & second)
{
	constexpr double tolerance = 0.5;
	return QLineF(first.p1(), second.p1()).length() <= tolerance
		|| QLineF(first.p1(), second.p2()).length() <= tolerance
		|| QLineF(first.p2(), second.p1()).length() <= tolerance
		|| QLineF(first.p2(), second.p2()).length() <= tolerance;
}

bool BreadboardRouteGraphCore::collinearOverlap(const QLineF & first, const QLineF & second)
{
	const QPointF a = first.p2() - first.p1();
	const QPointF b = second.p1() - first.p1();
	const QPointF c = second.p2() - first.p1();
	const double crossB = a.x() * b.y() - a.y() * b.x();
	const double crossC = a.x() * c.y() - a.y() * c.x();
	if (qAbs(crossB) > 0.5 || qAbs(crossC) > 0.5) return false;
	// Inflate BOTH boxes: axis-aligned segments make zero-area rects, and Qt
	// treats empty rects as never intersecting (this silently disabled the
	// overlap penalty for horizontal/vertical wires in the original code).
	const QRectF firstBounds = QRectF(first.p1(), first.p2()).normalized().adjusted(-0.5, -0.5, 0.5, 0.5);
	const QRectF secondBounds = QRectF(second.p1(), second.p2()).normalized().adjusted(-0.25, -0.25, 0.25, 0.25);
	return firstBounds.intersects(secondBounds);
}
