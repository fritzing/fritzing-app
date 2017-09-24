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

$Revision: 6954 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-05 10:22:00 +0200 (Fr, 05. Apr 2013) $

********************************************************************/


#ifdef _MSC_VER 
#pragma warning(push) 
#pragma warning(disable:4503)
#pragma warning(disable:4100)			// disable scary-looking compiler warnings in Boost library
#pragma warning(disable:4181)	
#endif

#include <boost/config.hpp>
#include <boost/graph/transitive_closure.hpp>
#include <boost/graph/prim_minimum_spanning_tree.hpp>
// #include <boost/graph/kolmogorov_max_flow.hpp>  // kolmogorov_max_flow is probably more efficient, but it doesn't compile
#include <boost/graph/edmonds_karp_max_flow.hpp>
#include <boost/graph/adjacency_list.hpp>


#ifdef _MSC_VER 
#pragma warning(pop)					// restore warning state
#endif

#include "graphutils.h"
#include "../fsvgrenderer.h"
#include "../items/wire.h"
#include "../items/jumperitem.h"
#include "../sketch/sketchwidget.h"
#include "../debugdialog.h"


void ConnectorEdge::setHeadTail(int h, int t) {
	head = h;
	tail = t;
}

ConnectorEdge * makeConnectorEdge(QList<ConnectorEdge *> & edges, ConnectorItem * ci, ConnectorItem * cj, int weight, Wire * wire) 
{
	ConnectorEdge * connectorEdge = new ConnectorEdge;
	connectorEdge->c0 = ci;
	connectorEdge->c1 = cj;
	connectorEdge->weight = weight;
	connectorEdge->wire = wire;
	connectorEdge->visible = true;
	edges.append(connectorEdge);
	return connectorEdge;
}

static int LastVertex = 0;

#define appendVertIf(ci, vertices, verts) {	\
	int ix = vertices.value(ci, -1);		\
	if (ix == -1) {							\
		ix = vertices.count();				\
		vertices.insert(ci, ix);			\
		verts.append(add_vertex(g));		\
	}										\
	LastVertex = ix;						\
}

void GraphUtils::minCut(QList<ConnectorItem *> & connectorItems, QList<SketchWidget *> & foreignSketchWidgets, ConnectorItem * source, ConnectorItem * sink, QList<ConnectorEdge *> & minCut) 
{
	// this helped:  http://boost.2283326.n4.nabble.com/graph-edmund-karp-max-flow-vs-kolmogorov-max-flow-color-map-td2565611.html
	
	using namespace boost;

	typedef adjacency_list_traits < vecS, vecS, directedS > Traits;
    typedef property < vertex_color_t, boost::default_color_type > COLOR;
	typedef property < vertex_index_t, long, COLOR > VERTEX;
    typedef property < edge_reverse_t, Traits::edge_descriptor > REVERSE;
    typedef property < edge_residual_capacity_t, long, REVERSE > RESIDUAL;
	typedef property < edge_capacity_t, long, RESIDUAL > EDGE;
	typedef adjacency_list < listS, vecS, directedS, VERTEX, EDGE > Graph;

	Graph g;

	property_map < Graph, edge_capacity_t >::type capacity = get(edge_capacity, g);
	property_map < Graph, edge_residual_capacity_t >::type residual_capacity = get(edge_residual_capacity, g);
	property_map < Graph, edge_reverse_t >::type reverse = get(edge_reverse, g);

	property_map < Graph, vertex_color_t >::type color = get(vertex_color, g);
	property_map < Graph, vertex_index_t >::type index = get(vertex_index, g);

	Traits::vertex_descriptor s, t;

	QList<Wire *> visitedWires;
	QList<int> indexes;
	QHash<ConnectorItem *, int> vertices;
	QList<ConnectorEdge *> edges;
	QVector<Traits::vertex_descriptor> verts;

	vertices.insert(source, 0);
	vertices.insert(sink, 1);
	verts.append(s = add_vertex(g));
	verts.append(t = add_vertex(g));

	foreach (ConnectorItem * connectorItem, connectorItems) {
		//connectorItem->debugInfo("input");
		if (connectorItem->attachedToItemType() == ModelPart::Wire) {
			Wire * wire = qobject_cast<Wire *>(connectorItem->attachedTo());
			if (visitedWires.contains(wire)) continue;

			QList<Wire *> wires;
			QList<ConnectorItem *> ends;
			wire->collectChained(wires, ends);
			visitedWires.append(wires);
			if (ends.count() < 2) continue;

			foreach (ConnectorItem * end, ends) {
				appendVertIf(end, vertices, verts);
			}

			for (int i = 0; i < ends.count(); i++) {
				ConnectorItem * end = ends[i];
				for (int j = i + 1; j < ends.count(); j++) {
					ConnectorEdge * connectorEdge = makeConnectorEdge(edges, end, ends[j], 1000, wire);
					connectorEdge->setHeadTail(vertices.value(connectorEdge->c0), vertices.value(connectorEdge->c1));
				}
			}
			continue;
		}

		if (connectorItem->connectorType() == Connector::Female) {
			appendVertIf(connectorItem, vertices, verts);
			int ix = LastVertex;
			foreach (ConnectorItem * toConnectorItem, connectorItem->connectedToItems()) {
				if (toConnectorItem->attachedToItemType() == ModelPart::Wire) {
					// deal with the wire
					continue;
				}

				appendVertIf(toConnectorItem, vertices, verts);
				int jx = LastVertex;
				ConnectorEdge * connectorEdge = makeConnectorEdge(edges, connectorItem, toConnectorItem, 1000, NULL);
				connectorEdge->setHeadTail(ix, jx);
			}
		}
	}

	// don't forget to add edges from bus connections
	QList<ConnectorItem *> originalKeys = vertices.keys();

	// make cross-view connections
	QList<ConnectorEdge *> foreignEdges;
	foreach (ConnectorEdge * ce, edges) {
		if (ce->wire && !ce->wire->isEverVisible()) {
			ce->visible = false;
			foreach (SketchWidget * foreignSketchWidget, foreignSketchWidgets) {
				ItemBase * foreignItemBase = foreignSketchWidget->findItem(ce->wire->id());
				if (foreignItemBase && foreignItemBase->isEverVisible()) {
					ConnectorItem * fc0 = foreignSketchWidget->findConnectorItem(ce->c0);
					ConnectorItem * fc1 = foreignSketchWidget->findConnectorItem(ce->c1);
					if (fc0 && fc1) {
						appendVertIf(fc0, vertices, verts);
						int ix = LastVertex;
						appendVertIf(fc1, vertices, verts);
						int jx = LastVertex;
						ConnectorEdge * fce = makeConnectorEdge(foreignEdges, fc0, fc1, 1, qobject_cast<Wire *>(foreignItemBase));
						fce->setHeadTail(ix, jx);
						fce = makeConnectorEdge(foreignEdges, ce->c0, fc0, 1000, NULL);
						fce->setHeadTail(ce->head, ix);
						fce = makeConnectorEdge(foreignEdges, ce->c1, fc1, 1000, NULL);
						fce->setHeadTail(ce->tail, jx);
					}
					else {
						ce->c0->debugInfo("missing foreign connector");
					}
					break;
				}
			}
		}
		if (ce->wire) continue;

		if (!ce->c0->attachedTo()->isEverVisible()) {
			ce->visible = false;
			foreach (SketchWidget * foreignSketchWidget, foreignSketchWidgets) {
				ConnectorItem * fc0 = foreignSketchWidget->findConnectorItem(ce->c0);
				if (fc0 == NULL) {
					ce->c0->debugInfo("missing foreign connector");
					continue;
				}
				if (!fc0->attachedTo()->isEverVisible()) continue;

				ConnectorItem * fc1 = foreignSketchWidget->findConnectorItem(ce->c1);
				if (fc0 && fc1) {
					appendVertIf(fc0, vertices, verts);
					int ix = LastVertex;
					appendVertIf(fc1, vertices, verts);
					int jx = LastVertex;
					ConnectorEdge * fce = makeConnectorEdge(foreignEdges, fc0, fc1, 1, NULL);
					fce->setHeadTail(ix, jx);
					fce = makeConnectorEdge(foreignEdges, ce->c0, fc0, 1000, NULL);
					fce->setHeadTail(ce->head, ix);
					fce = makeConnectorEdge(foreignEdges, ce->c1, fc1, 1000, NULL);
					fce->setHeadTail(ce->tail, jx);

				}
				else {
					ce->c0->debugInfo("missing foreign connector");
				}
			}
		}	
	}
	
	while (originalKeys.count() > 0) {
		ConnectorItem * key = originalKeys.takeFirst();
		if (key->attachedToItemType() == ModelPart::Wire) continue;
		if (key->bus() == NULL) continue;

		int ix = vertices.value(key);
		QList<ConnectorItem *> bcis;
		key->attachedTo()->busConnectorItems(key->bus(), key, bcis);
		foreach (ConnectorItem * bci, bcis) {
			if (bci == key) continue;

			originalKeys.removeOne(bci);
			appendVertIf(bci, vertices, verts);
			int jx = LastVertex;
			ConnectorEdge * connectorEdge = makeConnectorEdge(edges, key, bci, 1000, NULL);
			connectorEdge->setHeadTail(ix, jx);
		}
	}

	edges.append(foreignEdges);

	// add cross-layer edges
	QList <ConnectorItem *> crossVisited;
	foreach (ConnectorEdge * ce, edges) {
		QList<ConnectorItem *> from;
		from << ce->c0;
		from << ce->c1;
		foreach (ConnectorItem * ci, from) {
			if (!crossVisited.contains(ci)) {
				ConnectorItem * cross = ci->getCrossLayerConnectorItem();
				if (cross == NULL) continue;

				appendVertIf(cross, vertices, verts);
				int jx = LastVertex;
				ConnectorEdge * connectorEdge = makeConnectorEdge(edges, ci, cross, 1000, NULL);
				connectorEdge->setHeadTail(vertices.value(ci), jx);
			}
		}
	}

	//foreach(ConnectorItem * connectorItem, vertices.keys()) {
	//	connectorItem->debugInfo(QString("vertex %1").arg(vertices.value(connectorItem)));
	//}

	foreach(ConnectorEdge * ce, edges) {
		if (!ce->visible) continue;

		Traits::edge_descriptor e1, e2;
		bool in1, in2;
		tie(e1, in1) = add_edge(verts[ce->head], verts[ce->tail], g);
		tie(e2, in2) = add_edge(verts[ce->tail], verts[ce->head], g);
		capacity[e2] = capacity[e1] = ce->weight;
		reverse[e1] = e2;
		reverse[e2] = e1;
		//ce->c0->debugInfo(QString("head %1").arg(ce->weight));
		//ce->c1->debugInfo("\ttail");
	}

	/*
	foreach(ConnectorEdge * ce, edges) {
		connectorItems.at(ce->head)->debugInfo(QString("%1").arg(ce->weight));
	}
	DebugDialog::debug("");	
	DebugDialog::debug("");
	foreach(ConnectorEdge * ce, edges) {
		partConnectorItems.at(ce->tail)->debugInfo(QString("%1").arg(ce->weight));
	}
	*/

	// if color_map parameter not specified, colors are not set
    long flow = edmonds_karp_max_flow(g, s, t, color_map(color)); 
	Q_UNUSED(flow);
	//DebugDialog::debug(QString("flow %1, s%2, t%3").arg(flow).arg(index(s)).arg(index(t)));
	//for (int i = 0; i < verts.count(); ++i) {
	//	DebugDialog::debug(QString("index %1 %2").arg(index(verts[i])).arg(color(verts[i])));
	//}

	typedef property_traits<property_map < Graph, vertex_color_t >::type>::value_type tColorValue;
    typedef boost::color_traits<tColorValue> tColorTraits; 
	foreach (ConnectorEdge * ce, edges) {
		bool addIt = false;
		if (ce->visible) {
			if (color(verts[ce->head]) == tColorTraits::white() && color(verts[ce->tail]) != tColorTraits::white()) {
				addIt = true;
			}
			else if (color(verts[ce->head]) != tColorTraits::white() && color(verts[ce->tail]) == tColorTraits::white()) {
				addIt = true;
			}
			if (addIt) {
				minCut << ce;
				//DebugDialog::debug(QString("edge %1 %2 w:%3").arg(ce->head).arg(ce->tail).arg(ce->weight));
			}
		}
		else {
			delete ce;
		}
	}     
}


bool GraphUtils::chooseRatsnestGraph(const QList<ConnectorItem *> * partConnectorItems, ViewGeometry::WireFlags flags, ConnectorPairHash & result) {
	using namespace boost;
	typedef adjacency_list < vecS, vecS, undirectedS, property<vertex_distance_t, double>, property < edge_weight_t, double > > Graph;
	typedef std::pair < int, int >E;

	if (partConnectorItems->count() < 2) return false;

	QList <ConnectorItem *> temp(*partConnectorItems);

	//DebugDialog::debug("__________________");
	int tix = 0;
	while (tix < temp.count()) {
		ConnectorItem * connectorItem = temp[tix++];
		//connectorItem->debugInfo("check cross");
		ConnectorItem * crossConnectorItem = connectorItem->getCrossLayerConnectorItem();
		if (crossConnectorItem) {
			// it doesn't matter which one  on which layer we remove
			// when we check equal potential both of them will be returned
			//crossConnectorItem->debugInfo("\tremove cross");
			temp.removeOne(crossConnectorItem);
		}
	}

	QList<QPointF> locs;
	foreach (ConnectorItem * connectorItem, temp) {
		locs << connectorItem->sceneAdjustedTerminalPoint(NULL);
	}

	QList < QList<ConnectorItem *> > wiredTo;

	int num_nodes = temp.count();
	int num_edges = num_nodes * (num_nodes - 1) / 2;
	E * edges = new E[num_edges];
	double * weights = new double[num_edges];
	int ix = 0;
	QVector< QVector<double> > reverseWeights(num_nodes, QVector<double>(num_nodes, 0));
	for (int i = 0; i < num_nodes; i++) {
		ConnectorItem * c1 = temp.at(i);
		//c1->debugInfo("c1");
		for (int j = i + 1; j < num_nodes; j++) {
			edges[ix].first = i;
			edges[ix].second = j;
			ConnectorItem * c2 = temp.at(j);
			if ((c1->attachedTo() == c2->attachedTo()) && (c1->bus() != NULL) && (c1->bus() == c2->bus())) {
				weights[ix++] = 0;
				continue;
			}

			bool already = false;
			bool checkWiredTo = true;
			foreach (QList<ConnectorItem *> list, wiredTo) {
				if (list.contains(c1)) {
					checkWiredTo = false;
					if (list.contains(c2)) {
						weights[ix++] = 0;
						already = true;
					}
					break;
				}
			}
			if (already) continue;

			//c2->debugInfo("\tc2");

			if (checkWiredTo) {
				QList<ConnectorItem *> cwConnectorItems;
				cwConnectorItems.append(c1);
				ConnectorItem::collectEqualPotential(cwConnectorItems, true, flags);
				wiredTo.append(cwConnectorItems);
				//foreach (ConnectorItem * cx, cwConnectorItems) {
					//cx->debugInfo("\t\tcx");
				//}
				if (cwConnectorItems.contains(c2)) {
					weights[ix++] = 0;
					continue;
				}
			}

			//DebugDialog::debug("c2 not eliminated");
			double dx = locs[i].x() - locs[j].x();
			double dy = locs[i].y() - locs[j].y();
			weights[ix++] = reverseWeights[i][j] = reverseWeights[j][i] = (dx * dx) + (dy * dy);
		}
	}

    bool retval = false;
    try {
	    Graph g(edges, edges + num_edges, weights, num_nodes);
	    property_map<Graph, edge_weight_t>::type weightmap = get(edge_weight, g);

	    std::vector < graph_traits < Graph >::vertex_descriptor > p(num_vertices(g));

	    prim_minimum_spanning_tree(g, &p[0]);

        for (std::size_t i = 0; i != p.size(); ++i) {
		    if (i == p[i]) continue;
            if (reverseWeights[(int) i][(int) p[i]] == 0) continue;

            result.insert(temp[(int) i], temp[(int) p[i]]);
	    }
        retval = true;
    }
    catch ( const std::exception& e ) {
        // catch an error in boost 1.54
        DebugDialog::debug(QString("boost spanning tree failure: %1").arg(e.what()));
    }
    catch(...) {
        DebugDialog::debug("boost spanning tree failure");
    }


	delete [] edges;
	delete [] weights;

	return retval;
}

#define add_edge_d(i, j, g) \
	add_edge(verts[i], verts[j], g); \
    //partConnectorItems[i]->debugInfo(QString("edge from %1").arg(i));
	//partConnectorItems[j]->debugInfo(QString("\tto %1").arg(j));

bool GraphUtils::scoreOneNet(QList<ConnectorItem *> & partConnectorItems, ViewGeometry::WireFlags myTrace, RoutingStatus & routingStatus) {
	using namespace boost;

	int num_nodes = partConnectorItems.count();

	typedef property < vertex_index_t, std::size_t > Index;
	typedef adjacency_list < listS, listS, directedS, Index > graph_t;
	typedef graph_traits < graph_t >::vertex_descriptor vertex_t;
	typedef graph_traits < graph_t >::edge_descriptor edge_t;

	graph_t G;
	std::vector < vertex_t > verts(num_nodes);
	for (int i = 0; i < num_nodes; ++i) {
		verts[i] = add_vertex(Index(i), G);
	}

	//std::pair<int, int> pair;
	bool gotUserConnection = false;
	for (int i = 0; i < num_nodes; i++) {
		add_edge_d(i, i, G);
		ConnectorItem * from = partConnectorItems[i];
		for (int j = i + 1; j < num_nodes; j++) {
			ConnectorItem * to = partConnectorItems[j];

			if (from->isCrossLayerConnectorItem(to)) {
				add_edge_d(i, j, G);
				add_edge_d(j, i, G);
				continue;
			}

            if (from->attachedToItemType() == ModelPart::Symbol && 
                to->attachedToItemType() == ModelPart::Symbol && 
                from->attachedTo()->isEverVisible() && 
                to->attachedTo()->isEverVisible()) 
           {
                // equipotential symbols are treated as if they were connected by wires
				add_edge_d(i, j, G);
				add_edge_d(j, i, G);
				gotUserConnection = true;
				continue;
			}

			if (to->attachedTo() != from->attachedTo()) {
				gotUserConnection = true;
				continue;
			}

			if ((to->bus() != NULL) && (to->bus() == from->bus())) {	
				add_edge_d(i, j, G);
				add_edge_d(j, i, G);
				continue;
			}

			gotUserConnection = true;
		}
	}

	if (!gotUserConnection) {
		return false;
	}



	routingStatus.m_netCount++;

	for (int i = 0; i < num_nodes; i++) {
		ConnectorItem * fromConnectorItem = partConnectorItems[i];
		if (fromConnectorItem->attachedToItemType() == ModelPart::Jumper) {
			routingStatus.m_jumperItemCount++;				
		}
		foreach (ConnectorItem * toConnectorItem, fromConnectorItem->connectedToItems()) {
            switch (toConnectorItem->attachedToItemType()) {
                case ModelPart::Wire:
                    break;
                case ModelPart::Breadboard:
                    if (toConnectorItem->attachedTo()->isEverVisible()) {
                        QList<ConnectorItem *> ends;
                        collectBreadboard(toConnectorItem, partConnectorItems, ends);
                        foreach (ConnectorItem * end, ends) {
				            if (end == fromConnectorItem) continue;

				            int j = partConnectorItems.indexOf(end);
				            if (j >= 0) {
					            add_edge_d(i, j, G);
					            add_edge_d(j, i, G);
				            }
			            }
                    }
                    continue;
            }

			Wire * wire = qobject_cast<Wire *>(toConnectorItem->attachedTo());
			if (wire == NULL) continue;

			if (!(wire->getViewGeometry().wireFlags() & myTrace)) {
				// don't add edge if the connection isn't traced with my kind of trace
				continue;
			}

			QList<Wire *> wires;
			QList<ConnectorItem *> ends;
			wire->collectChained(wires, ends);
			foreach (ConnectorItem * end, ends) {
				if (end == fromConnectorItem) continue;

				int j = partConnectorItems.indexOf(end);
				if (j >= 0) {
					add_edge_d(i, j, G);
					add_edge_d(j, i, G);
				}
			}
		}
	}

	adjacency_list <> TC;
	transitive_closure(G, TC);

	QVector<bool> check(num_nodes, true);
	bool anyMissing = false;
	for (int i = 0; i < num_nodes - 1; i++) {
		if (!check[i]) continue;

		check[i] = false;
		bool missingOne = false;
		for (int j = i + 1; j < num_nodes; j++) {
			if (!check[j]) continue;

			if (edge(i, j, TC).second) {
				check[j] = false;
			}
			else {
				// we can minimally span the set with n-1 wires, so even if multiple connections are missing from a given connector, count it as one
				anyMissing = missingOne = true;
				
				//ConnectorItem * ci = partConnectorItems.at(i);
				//ConnectorItem * cj = partConnectorItems.at(j);
				//ci->debugInfo("missing one ci");
				//cj->debugInfo("\t\tcj");
				
			}
		}
		if (missingOne) {
			routingStatus.m_connectorsLeftToRoute++;
		}
	}

	if (!anyMissing) {
		routingStatus.m_netRoutedCount++;
	}

	return true;
}

void GraphUtils::collectBreadboard(ConnectorItem * connectorItem, QList<ConnectorItem *> & partConnectorItems, QList<ConnectorItem *> & ends)
{
    QList<ConnectorItem *> itemsToGo;
    itemsToGo.append(connectorItem);
    for (int i = 0; i < itemsToGo.count(); i++) {
        ConnectorItem * candidate = itemsToGo.at(i);
        if (partConnectorItems.contains(candidate)) {
            if (!ends.contains(candidate)) {
                ends.append(candidate);
            }
            continue;
        }

        Bus * bus = candidate->bus();
        if (bus) {
            QList<ConnectorItem *> busConnectorItems;
            candidate->attachedTo()->busConnectorItems(bus, candidate, busConnectorItems);
            foreach (ConnectorItem * bci, busConnectorItems) {
                if (!itemsToGo.contains(bci)) {
                    itemsToGo.append(bci);
                }
            }
        }
        
        foreach (ConnectorItem * to, candidate->connectedToItems()) {
            if (!itemsToGo.contains(to)) {
                itemsToGo.append(to);
            }
        }
    }
}
