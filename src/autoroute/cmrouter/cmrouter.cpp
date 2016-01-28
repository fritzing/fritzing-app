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

This router is  based on the one described in Contour: A Tile-based Gridless Router
http://www.hpl.hp.com/techreports/Compaq-DEC/WRL-95-3.pdf

Plus additional ideas from An Efficient Tile-Based ECO Router with Routing Graph Reduction
http://www.cis.nctu.edu.tw/~ylli/paper/f69-li.pdf

The corner stitching code is a modified version of code from the Magic VLSI Layout Tool
http://opencircuitdesign.com/magic/

********************************************************************

$Revision: 6976 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-21 09:50:09 +0200 (So, 21. Apr 2013) $

********************************************************************/

// TODO:
//
//  run separate beginning overlap check with half keepout width?
//
//	would be nice to eliminate ratsnests as we go
//
//	think of orderings like simulated annealing or genetic algorithms
//
//	option to turn off propagation feedback?
//	remove debugging output and extra calls to processEvents
//
//	still seeing a few thin tiles going across the board: 
//		this is because the thick tiles above and below are wider than the thin tile
//
//	slide corner: if dogleg is too close to other connectors, slide it more towards the middle
//
//	bugs: 
//		why does the same routing task give different results (qSort?)
//			especially annoying in schematic view when sometimes wires flow along wires and sometimes don't, for the same routing task
//		border seems asymmetric
//		still some funny shaped routes (thin tile problem?)
//		jumper item: sometimes one end doesn't route
//		schematic view: some lines still overlap
//		split_original_wire.fz shouldn't have two jumpers
//		stepper motor example: not putting jumper item in space to the left of a connector where there is clearly open space
//				is this due to a blocking thin tile?
//		some traces violate drc
//
//		catching 1st repeat to end rip-up-and-reroute is not valid
//
//  longer route than expected:  
//		It is possible that the shortest tile route is actually longer than the shortest crow-fly route.  
//		For example, in the following case, route ABC will reach goal before ABDEF:
//                                   -------
//                                   |  A  |  
//      ------------------------------------
//      |               B                  |
//		------------------------------------
//      |       |                 |    D   |
//      |       |               --------------
//      |   C   |               |      E     |
//      |       |              -------------------
//      |       |              |       F         |
//      ------------------------------------------
//      |              GOAL                      |
//      ------------------------------------------
//		There is a way to deal with this in the following paper: http://eie507.eie.polyu.edu.hk/projects/sp-tiles-00980255.pdf
//
//

#include "cmrouter.h"
#include "../../sketch/pcbsketchwidget.h"
#include "../../debugdialog.h"
#include "../../items/virtualwire.h"
#include "../../items/tracewire.h"
#include "../../items/jumperitem.h"
#include "../../items/via.h"
#include "../../items/resizableboard.h"
#include "../../utils/graphicsutils.h"
#include "../../utils/graphutils.h"
#include "../../utils/textutils.h"
#include "../../connectors/connectoritem.h"
#include "../../items/moduleidnames.h"
#include "../../processeventblocker.h"
#include "../../svg/groundplanegenerator.h"
#include "../../svg/svgfilesplitter.h"
#include "../../fsvgrenderer.h"

#include "tile.h"
#include "tileutils.h"

#include <qmath.h>
#include <limits>
#include <QApplication>
#include <QMessageBox> 
//#include <QElapsedTimer>			// forces a dependency on qt 4.7
#include <QSettings>
#include <QCryptographicHash>

static const int MaximumProgress = 1000;
static int TileStandardWireWidth = 0;
static int TileHalfStandardWireWidth = 0;
static double StandardWireWidth = 0;
static double HalfStandardWireWidth = 0;
static const double CloseEnough = 0.5;
static const int GridEntryAlpha = 128;

//static qint64 seedNextTime = 0;
//static qint64 propagateUnitTime = 0;

static const int DefaultMaxCycles = 10;

#ifndef QT_NO_DEBUG
#define DGI(item) drawGridItem(item)
#else
#define DGI(item) Q_UNUSED(item)
#endif

static inline double dot(const QPointF & p1, const QPointF & p2)
{
	return (p1.x() * p2.x()) + (p1.y() * p2.y());
}

bool tilePointRectXLessThan(TilePointRect * tpr1, TilePointRect * tpr2)
{
	return tpr1->tilePoint.xi <= tpr2->tilePoint.xi;
}

bool tilePointRectYGreaterThan(TilePointRect * tpr1, TilePointRect * tpr2)
{
	return tpr1->tilePoint.yi >= tpr2->tilePoint.yi;
}

///////////////////////////////////////////////
//
// tile functions

static inline void infoTile(const QString & message, Tile * tile)
{
	if (tile == NULL) {
		DebugDialog::debug("infoTile: tile is NULL");
		return;
	}

	DebugDialog::debug(QString("tile:%1 lb:%2 bl:%3 tr:%4 rt%5")
		.arg((long) tile, 0, 16)
		.arg((long) tile->ti_lb, 0, 16)
		.arg((long) tile->ti_bl, 0, 16)
		.arg((long) tile->ti_tr, 0, 16)
		.arg((long) tile->ti_rt, 0, 16));

	DebugDialog::debug(QString("%1 tile:%2 l:%3 t:%4 w:%5 h:%6 type:%7 body:%8")
		.arg(message)
		.arg((long) tile, 0, 16)
		.arg(LEFT(tile))
		.arg(YMIN(tile))
		.arg(WIDTH(tile))
		.arg(HEIGHT(tile))
		.arg(TiGetType(tile))
		.arg((long) TiGetBody(tile), 0, 16)
	);
}

static inline void infoTileRect(const QString & message, const TileRect & tileRect)
{
	DebugDialog::debug(QString("%1 l:%2 t:%3 w:%4 h:%5")
		.arg(message)
		.arg(tileRect.xmini)
		.arg(tileRect.ymini)
		.arg(tileRect.xmaxi - tileRect.xmini)
		.arg(tileRect.ymaxi - tileRect.ymini)
	);
}

static inline int manhattan(TileRect & tr1, TileRect & tr2) {
	int dx =  qAbs(tr1.xmaxi - tr2.xmaxi);
	dx = qMin(qAbs(tr1.xmaxi - tr2.xmini), dx);
	dx = qMin(qAbs(tr1.xmini - tr2.xmaxi), dx);
	dx = qMin(qAbs(tr1.xmini - tr2.xmini), dx);
	int dy =  qAbs(tr1.ymaxi - tr2.ymaxi);
	dy = qMin(qAbs(tr1.ymaxi - tr2.ymini), dy);
	dy = qMin(qAbs(tr1.ymini - tr2.ymaxi), dy);
	dy = qMin(qAbs(tr1.ymini - tr2.ymini), dy);
	return dx + dy;
}

static inline GridEntry * TiGetGridEntry(Tile * tile) { return dynamic_cast<GridEntry *>(TiGetClient(tile)); }

void extendToBounds(TileRect & from, TileRect & to) {
	// bail if it already extends to or past the bounds
	if (from.xmini <= to.xmini) return;
	if (from.xmaxi >= to.xmaxi) return;
	if (from.ymini <= to.ymini) return;
	if (from.ymaxi >= to.ymaxi) return;

	int which = 0;
	int dmin = from.xmini - to.xmini;
	if (to.xmaxi - from.xmaxi < dmin) {
		which = 1;
		dmin = to.xmaxi - from.xmaxi;
	}
	if (from.ymini - to.ymini < dmin) {
		which = 2;
		dmin = from.ymini - to.ymini;
	}
	if (to.ymaxi - from.ymaxi < dmin) {
		which = 3;
		dmin = to.ymaxi - from.ymaxi;
	}
	switch(which) {
		case 0:
			from.xmini = to.xmini;
			return;
		case 1:
			from.xmaxi = to.xmaxi;
			return;
		case 2:
			from.ymini = to.ymini;
			return;
		case 3:
			from.ymaxi = to.ymaxi;
			return;
		default:
			break;
	}
}


////////////////////////////////////////////////////////////////////
//
// tile crawling functions

int checkAlready(Tile * tile, UserData userData) {
	switch (TiGetType(tile)) {
		case Tile::SPACE:		
		case Tile::SPACE2:		
		case Tile::SCHEMATICWIRESPACE:		
		case Tile::BUFFER:
			return 0;
		default:
			break;
	}

	QList<Tile *> * tiles = (QList<Tile *> *) userData;
	tiles->append(tile);
	return 0;
}

int prepDeleteTile(Tile * tile, UserData userData) {
	switch(TiGetType(tile)) {
		case Tile::DUMMYLEFT:
		case Tile::DUMMYRIGHT:
		case Tile::DUMMYTOP:
		case Tile::DUMMYBOTTOM:
			return 0;
		default:
			break;
	}

	//infoTile("prep delete", tile);
	QSet<Tile *> * tiles = (QSet<Tile *> *) userData;
	tiles->insert(tile);

	return 0;
}

////////////////////////////////////////////////////////////////////

GridEntry::GridEntry(QRectF & r, QGraphicsItem * parent) : QGraphicsRectItem(r, parent)
{
	m_drawn = false;
	setAcceptedMouseButtons(Qt::NoButton);
	setAcceptHoverEvents(false);
}

bool GridEntry::drawn() {
	return m_drawn;
}

void GridEntry::setDrawn(bool d) {
	m_drawn = d;
}

////////////////////////////////////////////////////////////////////

CMRouter::CMRouter(PCBSketchWidget * sketchWidget, ItemBase * board, bool adjustIf) : QObject()
{
	m_board = board;
	m_sketchWidget = sketchWidget;	

	m_unionPlane = m_union90Plane = NULL;

	if (m_board) {
		m_maxRect = m_board->sceneBoundingRect();
	}
	else {
		m_maxRect = m_sketchWidget->scene()->itemsBoundingRect();
        if (adjustIf) {
		    m_maxRect.adjust(-m_maxRect.width() / 2, -m_maxRect.height() / 2, m_maxRect.width() / 2, m_maxRect.height() / 2);
        }
	}

	QMatrix matrix90;
	matrix90.rotate(90);
	m_maxRect90 = matrix90.mapRect(m_maxRect);

	qrectToTile(m_maxRect, m_tileMaxRect); 

	setUpWidths(m_sketchWidget->getAutorouterTraceWidth());
}

CMRouter::~CMRouter()
{
}

Plane * CMRouter::initPlane(bool rotate90) {
	Tile * bufferTile = TiAlloc();
	TiSetType(bufferTile, Tile::BUFFER);
	TiSetBody(bufferTile, NULL);

	QRectF bufferRect(rotate90 ? m_maxRect90 : m_maxRect);

    TileRect br;
    qrectToTile(bufferRect, br);

	bufferRect.adjust(-bufferRect.width(), -bufferRect.height(), bufferRect.width(), bufferRect.height());
    //DebugDialog::debug("max rect", m_maxRect);
    //DebugDialog::debug("max rect 90", m_maxRect90);


    int l = fasterRealToTile(bufferRect.left());
    int t = fasterRealToTile(bufferRect.top());
    int r = fasterRealToTile(bufferRect.right());
    int b = fasterRealToTile(bufferRect.bottom());
    SETLEFT(bufferTile, l);
    SETYMIN(bufferTile, t);		// TILE is Math Y-axis not computer-graphic Y-axis

	Plane * thePlane = TiNewPlane(bufferTile, br.xmini, br.ymini, br.xmaxi, br.ymaxi);

    SETRIGHT(bufferTile, r);
	SETYMAX(bufferTile, b);		// TILE is Math Y-axis not computer-graphic Y-axis

	// do not use InsertTile here
	TiInsertTile(thePlane, &thePlane->maxRect, NULL, Tile::SPACE); 
    //infoTileRect("insert", thePlane->maxRect);

	return thePlane;
}

/*
void CMRouter::shortenUs(QList<QPointF> & allPoints, JSubedge * subedge)
{
	// TODO: this could be implemented recursively as a child tile space 
	//		with the goals being the sides of the U-shape and the obstacles copied in from the parent tile space
	// for now just look for a straight line
	int ix = 0;
	while (ix < allPoints.count() - 3) {
		QPointF p0 = allPoints.at(ix);
		QPointF p1 = allPoints.at(ix + 1);
		QPointF p2 = allPoints.at(ix + 2);
		QPointF p3 = allPoints.at(ix + 3);
		ix += 1;

		TileRect tileRect;
		if (p1.x() == p2.x()) {
			if ((p0.x() > p1.x() && p3.x() > p2.x()) || (p0.x() < p1.x() && p3.x() < p2.x())) {
				// opening to left or right
				bool targetGreater;
				if (p0.x() < p1.x()) {
					// opening left
					targetGreater = false;
					realsToTile(tileRect, qMax(p0.x(), p3.x()), qMin(p0.y(), p3.y()), p2.x(), qMax(p0.y(), p3.y()));
				}
				else {
					// opening right
					targetGreater = true;
					realsToTile(tileRect, p2.x(), qMin(p0.y(), p3.y()), qMin(p0.x(), p3.x()), qMax(p0.y(), p3.y()));
				}

				if (findShortcut(tileRect, true, targetGreater, subedge, allPoints, ix - 1)) {
					ix--;
				}
			}
			else {
				// not a U-shape
				continue;
			}
		}
		else if (p1.y() == p2.y()) {
			if ((p0.y() > p1.y() && p3.y() > p2.y()) || (p0.y() < p1.y() && p3.y() < p2.y())) {
				// opening to top or bottom
				bool targetGreater;
				if (p0.y() < p1.y()) {
					// opening top
					targetGreater = false;
					realsToTile(tileRect, qMin(p0.x(), p3.x()), qMax(p0.y(), p3.y()), qMax(p0.x(), p3.x()), p2.y());
				}
				else {
					// opening bottom
					targetGreater = true;
					realsToTile(tileRect, qMin(p0.x(), p3.x()), p2.y(), qMax(p0.x(), p3.x()), qMin(p0.y(), p3.y()));
				}

				if (findShortcut(tileRect, false, targetGreater, subedge, allPoints, ix - 1)) {
					ix--;
				}
			}
			else {
				// not a U-shape
				continue;
			}
		}

	}
}
*/

GridEntry * CMRouter::drawGridItem(Tile * tile)
{
    return NULL; 

	if (tile == NULL) return NULL;

	QRectF r;
	tileToQRect(tile, r);

	GridEntry * gridEntry = TiGetGridEntry(tile);
	if (gridEntry == NULL) {
		gridEntry = new GridEntry(r, NULL);
		gridEntry->setZValue(m_sketchWidget->getTopZ());
		TiSetClient(tile, gridEntry);
	}
	else {
		QRectF br = gridEntry->boundingRect();
		if (br != r) {
			gridEntry->setRect(r);
			gridEntry->setDrawn(false);
		}
	}

	if (gridEntry->drawn()) return gridEntry;

	QColor c;
	switch (TiGetType(tile)) {
		case Tile::SPACE:
			c = QColor(255, 255, 0, GridEntryAlpha);
			break;
		case Tile::SPACE2:
			c = QColor(200, 200, 0, GridEntryAlpha);
			break;
		case Tile::SOURCE:
			c = QColor(0, 255, 0, GridEntryAlpha);
			break;
		case Tile::DESTINATION:
			c = QColor(0, 0, 255, GridEntryAlpha);
			break;
		case Tile::SCHEMATICWIRESPACE:
			c = QColor(255, 192, 203, GridEntryAlpha);
			break;
		case Tile::OBSTACLE:
			c = QColor(60, 60, 60, GridEntryAlpha);
			break;
		default:
			c = QColor(255, 0, 0, GridEntryAlpha);
			break;
	}

	gridEntry->setPen(c);
	gridEntry->setBrush(QBrush(c));
	if (gridEntry->scene() == NULL) {
		m_sketchWidget->scene()->addItem(gridEntry);
	}
	gridEntry->show();
	gridEntry->setDrawn(true);
	ProcessEventBlocker::processEvents();
	return gridEntry;
}

Tile * CMRouter::addTile(NonConnectorItem * nci, Tile::TileType type, Plane * thePlane, QList<Tile *> & alreadyTiled, CMRouter::OverlapType overlapType) 
{
	QRectF r = nci->attachedTo()->mapRectToScene(nci->rect());
	TileRect tileRect;
	realsToTile(tileRect, r.left() - m_keepoutPixels, r.top() - m_keepoutPixels, r.right() + m_keepoutPixels, r.bottom() + m_keepoutPixels);
	Tile * tile = insertTile(thePlane, tileRect, alreadyTiled, nci, type, overlapType);
	DGI(tile);
	return tile;
}

void CMRouter::hideTiles() 
{
	foreach (QGraphicsItem * item, m_sketchWidget->items()) {
		GridEntry * gridEntry = dynamic_cast<GridEntry *>(item);
		if (gridEntry) gridEntry->setVisible(false);
	}
}

void CMRouter::clearPlane(Plane * thePlane) 
{
	if (thePlane == NULL) return;

	QSet<Tile *> tiles;

                    //infoTileRect("clear", thePlane->maxRect);

	TiSrArea(NULL, thePlane, &thePlane->maxRect, prepDeleteTile, &tiles);
	foreach (Tile * tile, tiles) {
		TiFree(tile);
	}

	TiFreePlane(thePlane);
}

void CMRouter::displayBadTiles(QList<Tile *> & alreadyTiled) {
	//hideTiles();
	foreach (Tile * tile, alreadyTiled) {
		TileRect tileRect;
		TiToRect(tile, &tileRect);
		displayBadTileRect(tileRect);
	}
	displayBadTileRect(m_overlappingTileRect);
}

void CMRouter::displayBadTileRect(TileRect & tileRect) {
	QRectF r;
	tileRectToQRect(tileRect, r);
	GridEntry * gridEntry = new GridEntry(r, NULL);
	gridEntry->setZValue(m_sketchWidget->getTopZ());
	QColor c(255, 0, 0, GridEntryAlpha);
	gridEntry->setPen(c);
	gridEntry->setBrush(QBrush(c));
	m_sketchWidget->scene()->addItem(gridEntry);
	gridEntry->show();
	ProcessEventBlocker::processEvents();
}


Tile * CMRouter::insertTile(Plane * thePlane, QRectF & rect, QList<Tile *> & alreadyTiled, QGraphicsItem * item, Tile::TileType tileType, CMRouter::OverlapType overlapType) 
{
	TileRect tileRect;
	qrectToTile(rect, tileRect);
	return insertTile(thePlane, tileRect, alreadyTiled, item, tileType, overlapType);
}

Tile * CMRouter::insertTile(Plane * thePlane, TileRect & tileRect, QList<Tile *> &, QGraphicsItem * item, Tile::TileType tileType, CMRouter::OverlapType overlapType) 
{
	//infoTileRect("insert tile", tileRect);
	if (tileRect.xmaxi - tileRect.xmini <= 0 || tileRect.ymaxi - tileRect.ymini <= 0) {
		DebugDialog::debug("attempting to insert zero width tile");
		return NULL;
	}

    if (tileRect.xmaxi > thePlane->maxRect.xmaxi) {
        tileRect.xmaxi = thePlane->maxRect.xmaxi;
    }
    if (tileRect.xmini < thePlane->maxRect.xmini) {
        tileRect.xmini = thePlane->maxRect.xmini;
    }

    if (tileRect.ymaxi > thePlane->maxRect.ymaxi) {
        tileRect.ymaxi = thePlane->maxRect.ymaxi;
    }
    if (tileRect.ymini < thePlane->maxRect.ymini) {
        tileRect.ymini = thePlane->maxRect.ymini;
    }

    if (tileRect.xmaxi - tileRect.xmini <= 0 || tileRect.ymaxi - tileRect.ymini <= 0) {
		return NULL;
	}

	bool gotOverlap = false;
	if (overlapType != CMRouter::IgnoreAllOverlaps) {
		//TiSrArea(NULL, thePlane, &tileRect, checkAlready, &alreadyTiled);
	}

	if (gotOverlap) {
		m_overlappingTileRect = tileRect;
		DebugDialog::debug("!!!!!!!!!!!!!!!!!!!!!!! overlaps not allowed !!!!!!!!!!!!!!!!!!!!!!");
		return NULL;
	}
	Tile * newTile = TiInsertTile(thePlane, &tileRect, item, tileType);
	insertUnion(tileRect, item, tileType);

	DGI(newTile);
	return newTile;
}

bool CMRouter::overlapsOnly(QGraphicsItem *, QList<Tile *> & alreadyTiled)
{
	bool doClip = false;
	for (int i = alreadyTiled.count() - 1;  i >= 0; i--) {
		Tile * intersectingTile = alreadyTiled.at(i);
		if (dynamic_cast<Wire *>(TiGetBody(intersectingTile)) != NULL || dynamic_cast<ConnectorItem *>(TiGetBody(intersectingTile)) != NULL) {
			doClip = true;
			continue;
		}

		alreadyTiled.removeAt(i);
	}

	return doClip;
}

bool CMRouter::allowEquipotentialOverlaps(QGraphicsItem * item, QList<Tile *> & alreadyTiled)
{
	bool collected = false;
	QList<ConnectorItem *> equipotential;
	Wire * w = dynamic_cast<Wire *>(item);
	if (w) {
		equipotential.append(w->connector0());
	}
	else {
		ConnectorItem * ci = dynamic_cast<ConnectorItem *>(item);
		equipotential.append(ci);
	}
		
	foreach (Tile * intersectingTile, alreadyTiled) {
		QGraphicsItem * bodyItem = TiGetBody(intersectingTile);
		ConnectorItem * ci = dynamic_cast<ConnectorItem *>(bodyItem);
		if (ci != NULL) {
			if (!collected) {
				ConnectorItem::collectEqualPotential(equipotential, false, ViewGeometry::NoFlag);
				collected = true;
			}
			if (!equipotential.contains(ci)) {
				// overlap not allowed
				//infoTile("intersecting", intersectingTile);
				return false;
			}
		}
		else {
			Wire * w = dynamic_cast<Wire *>(bodyItem);
			if (w == NULL) return false;

			if (!collected) {
				ConnectorItem::collectEqualPotential(equipotential, false, ViewGeometry::NoFlag);
				collected = true;
			}

			if (!equipotential.contains(w->connector0())) {
				// overlap not allowed
				//infoTile("intersecting", intersectingTile);
				return false;
			}
		}
	}

	return true;

}

void CMRouter::clearGridEntries() {
	foreach (QGraphicsItem * item, m_sketchWidget->scene()->items()) {
		GridEntry * gridEntry = dynamic_cast<GridEntry *>(item);
		if (gridEntry == NULL) continue;

		delete gridEntry;
	}
}

bool CMRouter::insideV(const QPointF & check, const QPointF & vertex)
{
	// form the V from p2

	QPointF lv(vertex.x() - 10 + vertex.y() - check.y(), check.y() + 10);
	QPointF rv(vertex.x() + 10 - vertex.y() + check.y(), check.y() + 10);

	// the rest of this from: http://www.blackpawn.com/texts/pointinpoly/default.html

	QPointF v0 = rv - vertex;
	QPointF v1 = lv - vertex;
	QPointF v2 = check - vertex;

	// Compute dot products
	double dot00 = dot(v0, v0);
	double dot01 = dot(v0, v1);
	double dot02 = dot(v0, v2);
	double dot11 = dot(v1, v1);
	double dot12 = dot(v1, v2);

	// Compute barycentric coordinates
	double invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
	double u = (dot11 * dot02 - dot01 * dot12) * invDenom;
	double v = (dot00 * dot12 - dot01 * dot02) * invDenom;

	// Check if point is in or on triangle
	return (u >= 0) && (v >= 0) && (u + v <= 1);
}

void CMRouter::insertUnion(TileRect & tileRect, QGraphicsItem *, Tile::TileType tileType) {
	if (m_unionPlane == NULL) return;
	if (tileType == Tile::SPACE) return;
	if (tileType == Tile::SPACE2) return;

	TiInsertTile(m_unionPlane, &tileRect, NULL, Tile::OBSTACLE);
	//infoTileRect("union", tileRect);

	TileRect tileRect90;
	tileRotate90(tileRect, tileRect90);

    if (tileRect90.xmaxi > m_union90Plane->maxRect.xmaxi) {
        tileRect90.xmaxi = m_union90Plane->maxRect.xmaxi;
    }
    if (tileRect90.xmini < m_union90Plane->maxRect.xmini) {
        tileRect90.xmini = m_union90Plane->maxRect.xmini;
    }

    if (tileRect90.ymaxi > m_union90Plane->maxRect.ymaxi) {
        tileRect90.ymaxi = m_union90Plane->maxRect.ymaxi;
    }
    if (tileRect90.ymini < m_union90Plane->maxRect.ymini) {
        tileRect90.ymini = m_union90Plane->maxRect.ymini;
    }

    if (tileRect90.xmaxi - tileRect90.xmini <= 0 || tileRect90.ymaxi - tileRect90.ymini <= 0) {
		return;
	}

	TiInsertTile(m_union90Plane, &tileRect90, NULL, Tile::OBSTACLE);
}

void CMRouter::drawTileRect(TileRect & tileRect, QColor & color)
{
	QRectF r;
	tileRectToQRect(tileRect, r);
	GridEntry * gridEntry = new GridEntry(r, NULL);
	gridEntry->setZValue(m_sketchWidget->getTopZ());
	gridEntry->setPen(color);
	gridEntry->setBrush(QBrush(color));
	m_sketchWidget->scene()->addItem(gridEntry);
	gridEntry->show();
	//ProcessEventBlocker::processEvents();
}

TileRect CMRouter::boardRect() {
	return m_tileMaxRect;
}

void CMRouter::setUpWidths(double width)
{
	StandardWireWidth = width;
    TileStandardWireWidth = fasterRealToTile(StandardWireWidth);						
	HalfStandardWireWidth = StandardWireWidth / 2;										
    TileHalfStandardWireWidth = fasterRealToTile(HalfStandardWireWidth);
}

void CMRouter::setKeepout(double keepout)
{
	m_keepoutPixels = keepout;
}

void CMRouter::drcClean() 
{
	clearGridEntries();
	if (m_unionPlane) {
		clearPlane(m_unionPlane);
		m_unionPlane = NULL;
	}
	if (m_union90Plane) {
		clearPlane(m_union90Plane);
		m_union90Plane = NULL;
	}
}

