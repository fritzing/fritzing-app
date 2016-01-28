/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2016 Fritzing

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

********************************************************************

$Revision: 6145 $:
$Author: cohen@irascible.com $:
$Date: 2012-07-05 19:17:11 +0200 (Do, 05. Jul 2012) $

********************************************************************/

#ifndef GRAPHUTILS_H
#define GRAPHUTILS_H

#include "../connectors/connectoritem.h"
#include "../routingstatus.h"

struct ConnectorEdge {
	int head;
	int tail;
	int weight;
	ConnectorItem * c0;
	ConnectorItem * c1;
	class Wire * wire;
	bool visible;

	void setHeadTail(int head, int tail);
};

class GraphUtils
{

public:
	static bool chooseRatsnestGraph(const QList<ConnectorItem *> * equipotentials, ViewGeometry::WireFlags, ConnectorPairHash & result);
	static bool scoreOneNet(QList<ConnectorItem *> & partConnectorItems, ViewGeometry::WireFlags, RoutingStatus & routingStatus);
	static void minCut(QList<ConnectorItem *> & connectorItems, QList<class SketchWidget *> & foreighSketchWidgets, ConnectorItem * source, ConnectorItem * sink, QList<ConnectorEdge *> & cutSet); 

protected:
    static void collectBreadboard(ConnectorItem * connectorItem, QList<ConnectorItem *> & partConnectorItems, QList<ConnectorItem *> & ends);

};

#endif
