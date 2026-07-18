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

#ifndef BREADBOARDROUTEGRAPHCORE_H
#define BREADBOARDROUTEGRAPHCORE_H

#include <QLineF>
#include <QList>
#include <QPointF>
#include <QString>
#include <QVector>

#include "breadboardroutingscore.h"

// Pure bus-graph router: plain data in (hole positions, dense bus ids),
// route plans out. No scene or Qt-graphics types so it is unit-testable
// and safe on worker threads.
//
// Static edges (hole pair, length) are built ONCE per instance; everything
// that changes while routing proceeds - blocked holes and congestion from
// already-planned segments - is supplied per route() query.
class BreadboardRouteGraphCore
{
public:
	struct Options {
		double maxJumperLength = 280.0;
		double jumperPenalty = 240.0;
		double crossingPenalty = 700.0;
		double overlapPenalty = 1200.0;
		int candidatesPerBusPair = 12;
	};

	struct Segment {
		int fromHole = -1;
		int toHole = -1;
		double cost = 0.0;
	};

	struct Result {
		bool found = false;
		double cost = 0.0;
		QVector<Segment> segments;
		BreadboardRoutingScore score;
		QString reason;
	};

	// holeBus[i] is the dense bus id of hole i (>= 0); holes with negative
	// bus ids are ignored.
	BreadboardRouteGraphCore(const QVector<QPointF> & holePositions,
							 const QVector<int> & holeBus,
							 const Options & options = Options());

	// Per-batch query state: blocked holes and per-edge congestion computed
	// once, then any number of route() calls run pure Dijkstra against it.
	// Rebuild the context whenever reserved holes or planned segments change.
	struct QueryContext {
		QVector<bool> holeBlocked;
		QVector<bool> busBlocked;
		QVector<double> edgeCongestion;
	};

	// holeBlocked[i]: hole may not be used (reserved/occupied); start and
	// target holes are always usable. busBlocked[b]: the bus is owned by a
	// foreign net (or a no-net pin) and must not be landed on or traversed -
	// unlike hole blocking there is NO endpoint exemption, because touching
	// a foreign bus electrically breaks the schematic. congestionSegments:
	// already-planned wires; crossing or overlapping them penalizes an edge.
	QueryContext prepareQuery(const QVector<bool> & holeBlocked,
							  const QVector<bool> & busBlocked,
							  const QList<QLineF> & congestionSegments) const;

	Result route(int startHole, int targetHole, const QueryContext & context) const;

	// Single-source variant: one Dijkstra from sourceHole settles every
	// reachable bus; extractRoute then reads any target in O(path length).
	// Turns entry x anchor scans (N x M Dijkstras) into M Dijkstras.
	// Note: only the SOURCE hole is exempt from blocking; targets extracted
	// later must be unblocked holes.
	struct MultiResult {
		int sourceHole = -1;
		int sourceBus = -1;
		QVector<BreadboardRoutingScore> bestByBus;
		QVector<bool> reachedByBus;
		QVector<int> cameFromEdge;
		QVector<double> edgeQueryCost;
	};
	MultiResult routeFrom(int sourceHole, const QueryContext & context) const;
	Result extractRoute(const MultiResult & multi, int targetHole) const;

	int busCount() const { return m_holesByBus.count(); }
	int edgeCount() const { return m_edges.count(); }

	static double congestionPenalty(const QLineF & candidate,
									const QList<QLineF> & existingSegments,
									double crossingPenalty,
									double overlapPenalty);

private:
	struct Edge {
		int fromBus = -1;
		int toBus = -1;
		int fromHole = -1;
		int toHole = -1;
		double length = 0.0;
	};

	void buildStaticEdges();
	static bool sharesEndpoint(const QLineF & first, const QLineF & second);
	static bool collinearOverlap(const QLineF & first, const QLineF & second);

private:
	QVector<QPointF> m_holePositions;
	QVector<int> m_holeBus;
	Options m_options;
	QVector<QVector<int> > m_holesByBus;          // bus id -> hole indices
	QVector<Edge> m_edges;
	QVector<QVector<int> > m_edgesByBus;          // bus id -> indices into m_edges
};

#endif
