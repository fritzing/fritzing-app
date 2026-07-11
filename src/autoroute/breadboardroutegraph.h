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

#ifndef BREADBOARDROUTEGRAPH_H
#define BREADBOARDROUTEGRAPH_H

#include <QHash>
#include <QList>
#include <QLineF>
#include <QSet>
#include <QString>

#include "breadboardroutingscore.h"

class ConnectorItem;

class BreadboardRouteGraph
{
public:
	struct Options {
		double maxJumperLength = 280.0;
		double jumperPenalty = 240.0;
		double crossingPenalty = 700.0;
		double overlapPenalty = 1200.0;
		int candidatesPerBusPair = 12;
		QList<QLineF> existingSegments;

		static Options fromEnvironment();
	};

	struct Segment {
		ConnectorItem * from = nullptr;
		ConnectorItem * to = nullptr;
		double cost = 0.0;
	};

	struct Result {
		bool found = false;
		double cost = 0.0;
		QList<Segment> segments;
		BreadboardRoutingScore score;
		QString reason;
	};

	BreadboardRouteGraph(const QList<ConnectorItem *> & holes,
	                     const QSet<ConnectorItem *> & reservedHoles,
	                     const Options & options = Options());

	Result route(ConnectorItem * start, ConnectorItem * target) const;

	QString busId(ConnectorItem * connectorItem) const;
	int busCount() const;
	int edgeCount() const;
	static double congestionPenalty(const QLineF & candidate,
	                                const QList<QLineF> & existingSegments,
	                                double crossingPenalty,
	                                double overlapPenalty);

private:
	struct Edge {
		QString fromBus;
		QString toBus;
		ConnectorItem * fromHole = nullptr;
		ConnectorItem * toHole = nullptr;
		double cost = 0.0;
		double length = 0.0;
		double congestion = 0.0;
	};

	void buildBusIndex();
	void buildStaticEdges();
	QList<Edge> bestStaticJumpers(const QString & fromBus, const QString & toBus) const;
	bool edgeAvailable(const Edge & edge, ConnectorItem * start, ConnectorItem * target) const;
	bool holeAvailable(ConnectorItem * hole, ConnectorItem * start, ConnectorItem * target) const;
	static double manhattan(ConnectorItem * first, ConnectorItem * second);
	double segmentPenalty(ConnectorItem * first, ConnectorItem * second) const;
	static bool sharesEndpoint(const QLineF & first, const QLineF & second);
	static bool collinearOverlap(const QLineF & first, const QLineF & second);
	static QString fallbackBusId(ConnectorItem * connectorItem);

private:
	QList<ConnectorItem *> m_holes;
	QSet<ConnectorItem *> m_reservedHoles;
	Options m_options;
	QHash<QString, QList<ConnectorItem *> > m_holesByBus;
	QHash<QString, QList<Edge> > m_edgesByBus;
};

#endif
