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

$Revision: 6912 $:
$Author: irascibl@gmail.com $:
$Date: 2013-03-09 08:18:59 +0100 (Sa, 09. Mrz 2013) $

********************************************************************/

#include "clipablewire.h"
#include "../connectors/connectoritem.h"
#include "../model/modelpart.h"
#include "../utils/graphicsutils.h"

#include <qmath.h>
#include <QDebug>

static double connectorRectClipInset = 0.5;

/////////////////////////////////////////////////////////

#define CONVEX

/* ======= Crossings algorithm ============================================ */

// from: http://tog.acm.org/GraphicsGems//gemsiv/ptpoly_haines/ptinpoly.c

#define XCOORD	0
#define YCOORD	1

/* Shoot a test ray along +X axis.  The strategy, from MacMartin, is to
 * compare vertex Y values to the testing point's Y and quickly discard
 * edges which are entirely to one side of the test ray.
 *
 * Input 2D polygon _pgon_ with _numverts_ number of vertices and test point
 * _point_, returns 1 if inside, 0 if outside.	WINDING and CONVEX can be
 * defined for this test.
 */
int CrossingsTest( double pgon[][2], int numverts, double point[2] )
{
#ifdef	WINDING
register int	crossings ;
#endif
register int	j, yflag0, yflag1, inside_flag, xflag0 ;
register double ty, tx, *vtx0, *vtx1 ;
#ifdef	CONVEX
register int	line_flag ;
#endif

    tx = point[XCOORD] ;
    ty = point[YCOORD] ;

    vtx0 = pgon[numverts-1] ;
    /* get test bit for above/below X axis */
    yflag0 = ( vtx0[YCOORD] >= ty ) ;
    vtx1 = pgon[0] ;

#ifdef	WINDING
    crossings = 0 ;
#else
    inside_flag = 0 ;
#endif
#ifdef	CONVEX
    line_flag = 0 ;
#endif
    for ( j = numverts+1 ; --j ; ) {

	yflag1 = ( vtx1[YCOORD] >= ty ) ;
	/* check if endpoints straddle (are on opposite sides) of X axis
	 * (i.e. the Y's differ); if so, +X ray could intersect this edge.
	 */
	if ( yflag0 != yflag1 ) {
	    xflag0 = ( vtx0[XCOORD] >= tx ) ;
	    /* check if endpoints are on same side of the Y axis (i.e. X's
	     * are the same); if so, it's easy to test if edge hits or misses.
	     */
	    if ( xflag0 == ( vtx1[XCOORD] >= tx ) ) {

		/* if edge's X values both right of the point, must hit */
#ifdef	WINDING
		if ( xflag0 ) crossings += ( yflag0 ? -1 : 1 ) ;
#else
		if ( xflag0 ) inside_flag = !inside_flag ;
#endif
	    } else {
		/* compute intersection of pgon segment with +X ray, note
		 * if >= point's X; if so, the ray hits it.
		 */
		if ( (vtx1[XCOORD] - (vtx1[YCOORD]-ty)*
		     ( vtx0[XCOORD]-vtx1[XCOORD])/(vtx0[YCOORD]-vtx1[YCOORD])) >= tx ) {
#ifdef	WINDING
		    crossings += ( yflag0 ? -1 : 1 ) ;
#else
		    inside_flag = !inside_flag ;
#endif
		}
	    }
#ifdef	CONVEX
	    /* if this is second edge hit, then done testing */
	    if ( line_flag ) goto Exit ;

	    /* note that one edge has been hit by the ray's line */
	    line_flag = true;
#endif
	}

	/* move to next pair of vertices, retaining info as possible */
	yflag0 = yflag1 ;
	vtx0 = vtx1 ;
	vtx1 += 2 ;
    }
#ifdef	CONVEX
    Exit: ;
#endif
#ifdef	WINDING
    /* test if crossings is not zero */
    inside_flag = (crossings != 0) ;
#endif

    return( inside_flag ) ;
}


/////////////////////////////////////////////////////////

ClipableWire::ClipableWire( ModelPart * modelPart, ViewLayer::ViewID viewID,  const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool initLabel ) 
	: Wire(modelPart, viewID,  viewGeometry,  id, itemMenu, initLabel)
{
	m_clipEnds = false;
	m_trackHoverItem = NULL;
	m_justFilteredEvent = NULL;
	m_cachedOriginalLine.setPoints(QPointF(-99999,-99999), QPointF(-99999,-99999));
}

const QLineF & ClipableWire::getPaintLine() {	
	if (!m_clipEnds) {
		return Wire::getPaintLine();
	}

	QLineF originalLine = this->line();
    //qDebug() << "line" << originalLine.p1() + pos() << originalLine.p2() + pos() << pos();
	if (m_cachedOriginalLine == originalLine) {
        // qDebug() << "  cachedline a" << m_cachedLine.p1() + pos() << m_cachedLine.p2() + pos() << pos();
        return m_cachedLine;
	}

	int t0c = 0;
	ConnectorItem* to0 = NULL;
	foreach (ConnectorItem * toConnectorItem, m_connector0->connectedToItems()) {
		if (toConnectorItem->attachedToItemType() != ModelPart::Wire) {
			to0 = toConnectorItem;
			break;
		}
		t0c++;
	}

	int t1c = 0;
	ConnectorItem* to1 = NULL;
	foreach (ConnectorItem * toConnectorItem, m_connector1->connectedToItems()) {
		if (toConnectorItem->attachedToItemType() != ModelPart::Wire) {
			to1 = toConnectorItem;
			break;
		}
		t1c++;
	}

	if ((to0 == NULL && to1 == NULL) ||		// no need to clip an unconnected wire
		(to0 == NULL && t0c == 0) ||		// dragging out a wire, no need to clip
		(to1 == NULL && t1c == 0) ||		// dragging out a wire, no need to clip
		m_dragEnd)							// dragging out a wire, no need to clip
	{
		return Wire::getPaintLine();
	}

    QPointF p1 = originalLine.p1();
    QPointF p2 = originalLine.p2();

    //if (this->viewID() == ViewLayer::PCBView) {
    //    qDebug() << "before" << "p1:" <<  p1 << "p2:" << p2 << "pos:" << pos() << "pp1:" << pos() + p1 << "pp2:" << pos() + p2;
    //    m_connector0->debugInfo("  c0");
    //    m_connector1->debugInfo("  c1");
    //    if (to0) to0->debugInfo("  to0");
    //    if (to1) to1->debugInfo("  to1");
    //}

    calcClip(p1, p2, to0, to1);

    //if (this->viewID() == ViewLayer::PCBView) {
    //    qDebug() << "after" << p1 << p2 << this->pos();
    //    if (to0) to0->debugInfo("  to0");
    //    if (to1) to1->debugInfo("  to1");
    //}

	m_cachedOriginalLine = originalLine;
	m_cachedLine.setPoints(p1, p2);
    //qDebug() << "  cachedline b" << m_cachedLine.p1() + pos() << m_cachedLine.p2() + pos() << pos();
    return m_cachedLine;

}

void ClipableWire::setClipEnds(bool clipEnds ) {
	if (m_clipEnds != clipEnds) {
		prepareGeometryChange();
		m_clipEnds = clipEnds;
	}
}

void ClipableWire::calcClip(QPointF & p1, QPointF & p2, ConnectorItem * c1, ConnectorItem * c2) {

	if (c1 != NULL && c2 != NULL && c1->isEffectivelyCircular() && c2->isEffectivelyCircular()) {
        //qDebug() << "clause 1" << p1 << p2 << c1->calcClipRadius() + (m_pen.width() / 2.0) << c2->calcClipRadius() + (m_pen.width() / 2.0);
        //c1->debugInfo("  c1");
        //c2->debugInfo("  c2");
		GraphicsUtils::shortenLine(p1, p2, c1->calcClipRadius() + (m_pen.width() / 2.0), c2->calcClipRadius() + (m_pen.width() / 2.0));
		return;
	}

	if (c1 != NULL && c1->isEffectivelyCircular()) {
        //qDebug() << "clause 2" << p1 << p2 << c1->calcClipRadius() + (m_pen.width() / 2.0) << 0;
        //c1->debugInfo("  c1");
        GraphicsUtils::shortenLine(p1, p2, c1->calcClipRadius() + (m_pen.width() / 2.0), 0);
		p2 = findIntersection(c2, p2);
		return;
	}

	if (c2 != NULL && c2->isEffectivelyCircular()) {
        //qDebug() << "clause 3" << p1 << p2 << 0 << c2->calcClipRadius() + (m_pen.width() / 2.0);
        //c2->debugInfo("  c2");
        GraphicsUtils::shortenLine(p1, p2, 0, c2->calcClipRadius() + (m_pen.width() / 2.0));
		p1 = findIntersection(c1, p1);
		return;
	}

    //qDebug() << "clause 4" << p1 << p2;
    //if (c1) c1->debugInfo("  c1");
    //if (c2) c2->debugInfo("  c2");

	p1 = findIntersection(c1, p1);
	p2 = findIntersection(c2, p2);
}

QPointF ClipableWire::findIntersection(ConnectorItem * connectorItem, const QPointF & p)
{
	if (connectorItem == NULL) return p;

	QRectF r = connectorItem->rect();
	r.adjust(connectorRectClipInset, connectorRectClipInset, -connectorRectClipInset, -connectorRectClipInset);	// inset it a little bit so the wire touches
	QPolygonF poly = this->mapFromScene(connectorItem->mapToScene(r));
	QLineF l1 = this->line();
	int count = poly.count();
	for (int i = 0; i < count; i++) {
		QLineF l2(poly[i], poly[(i + 1) % count]);
		QPointF intersectingPoint;
		if (l1.intersect(l2, &intersectingPoint) == QLineF::BoundedIntersection) {
			return intersectingPoint;
		}
	}

	return this->mapFromScene(connectorItem->mapToScene(r.center()));
}

bool ClipableWire::filterMousePressConnectorEvent(ConnectorItem * connectorItem, QGraphicsSceneMouseEvent * event) {
	m_justFilteredEvent = NULL;

	if (!m_clipEnds) return false;
	if (m_viewID != ViewLayer::PCBView) return false;

	ConnectorItem * to = NULL;
	foreach (ConnectorItem * toConnectorItem, connectorItem->connectedToItems()) {
		if (toConnectorItem->attachedToItemType() != ModelPart::Wire) {
			to = toConnectorItem;
			break;
		}
	}
	if (to == NULL) return false;

	if (insideInnerCircle(to, event->scenePos()) || !insideSpoke(this, event->scenePos())) {
		m_justFilteredEvent = event;
		event->ignore();
		return true;
	}

	return false;
}

void ClipableWire::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if ((long) event == (long) m_justFilteredEvent) {
		event->ignore();
		return;
	}

	Wire::mousePressEvent(event);
}

void ClipableWire::hoverEnterConnectorItem(QGraphicsSceneHoverEvent * event , ConnectorItem * item) {

	//Wire::hoverEnterConnectorItem(event, item);

	// track mouse move events for hover redirecting

	ConnectorItem * to = NULL;
	foreach (ConnectorItem * toConnectorItem, item->connectedToItems()) {
		if (toConnectorItem->attachedToItemType() != ModelPart::Wire) {
			to = toConnectorItem;
			break;
		}
	}

	if (to) {
		m_trackHoverItem = to;
		m_trackHoverLastWireItem = NULL;
		m_trackHoverLastItem = NULL;
		dispatchHover(event->scenePos());
	}
}

void ClipableWire::hoverLeaveConnectorItem(QGraphicsSceneHoverEvent * event, ConnectorItem * item) {
	Q_UNUSED(item);
	Q_UNUSED(event);
	dispatchHoverAux(false, NULL);
	m_trackHoverItem = NULL;
	//Wire::hoverLeaveConnectorItem(event, item);
}

void ClipableWire::hoverMoveConnectorItem(QGraphicsSceneHoverEvent * event, ConnectorItem * item) {
	if (m_trackHoverItem) {
		dispatchHover(event->scenePos());
	}

	Wire::hoverMoveConnectorItem(event, item);
}

void ClipableWire::dispatchHover(QPointF scenePos) {
	bool inInner = false;
	ClipableWire * inWire = NULL;
	if (insideInnerCircle(m_trackHoverItem, scenePos)) {
		//DebugDialog::debug("got inner circle");
		inInner = true;
	}
	else {
		foreach (ConnectorItem * toConnectorItem, m_trackHoverItem->connectedToItems()) {
			if (toConnectorItem->attachedToItemType() != ModelPart::Wire) continue;

			ClipableWire * w = dynamic_cast<ClipableWire *>(toConnectorItem->attachedTo());
			if (w == NULL) continue;
			if (w->getRatsnest()) continue;									// is there a better way to check this?

			if (insideSpoke(w, scenePos)) {
				//DebugDialog::debug("got inside spoke");
				inWire = w;
				break;
			}
		}
	}

	dispatchHoverAux(inInner, inWire);
}

void ClipableWire::dispatchHoverAux(bool inInner, Wire * inWire)
{
	if (m_trackHoverItem == NULL) return;

	if (inInner) {
		if (m_trackHoverLastItem == m_trackHoverItem) {
			// no change
			return;
		}
		if (m_trackHoverLastWireItem) {
			((ItemBase *) m_trackHoverLastWireItem)->hoverLeaveConnectorItem();
			m_trackHoverLastWireItem = NULL;
		}

		m_trackHoverLastItem = m_trackHoverItem;
		m_trackHoverLastItem->setHoverColor();
		m_trackHoverLastItem->attachedTo()->hoverEnterConnectorItem();
	}
	else if (inWire) {
		if (m_trackHoverLastWireItem == inWire) {
			// no change
			return;
		}
		if (m_trackHoverLastItem) {
            QList<ConnectorItem *> visited;
			m_trackHoverLastItem->restoreColor(visited);
			m_trackHoverLastItem->attachedTo()->hoverLeaveConnectorItem();
			m_trackHoverLastItem = NULL;
		}
		if (m_trackHoverLastWireItem) {
			((ItemBase *) m_trackHoverLastWireItem)->hoverLeaveConnectorItem();
		}
		m_trackHoverLastWireItem = inWire;
		((ItemBase *) m_trackHoverLastWireItem)->hoverEnterConnectorItem();
	}
	else {
		//DebugDialog::debug("got none");
		if (m_trackHoverLastItem != NULL) {
            QList<ConnectorItem *> visited;
			m_trackHoverLastItem->restoreColor(visited);
			m_trackHoverLastItem->attachedTo()->hoverLeaveConnectorItem();
			m_trackHoverLastItem = NULL;
		}
		if (m_trackHoverLastWireItem != NULL) {
			((ItemBase *) m_trackHoverLastWireItem)->hoverLeaveConnectorItem();
			m_trackHoverLastWireItem = NULL;
		}
	}
}

bool ClipableWire::insideInnerCircle(ConnectorItem * connectorItem, QPointF scenePos) 
{
	QPointF localPos = connectorItem->mapFromScene(scenePos);
	double rad = connectorItem->radius();
	if (rad <= 0) return false;

	rad -= ((connectorItem->strokeWidth() / 2.0));			// shrink it a little bit

	QRectF r = connectorItem->rect();
	QPointF c = r.center();
	if ( (localPos.x() - c.x()) * (localPos.x() - c.x()) + (localPos.y() - c.y()) * (localPos.y() - c.y())  < rad * rad ) {
		// inside the inner circle
		return true;
	}

	return false;
}

bool ClipableWire::insideSpoke(ClipableWire * wire, QPointF scenePos) 
{
	QLineF l = wire->line();
	QLineF normal = l.normalVector();
	normal.setLength(wire->width() / 2.0);
	double parallelogram[4][2];
	parallelogram[0][XCOORD] = normal.p2().x();
	parallelogram[0][YCOORD] = normal.p2().y();
	parallelogram[1][XCOORD] = normal.p1().x() - normal.dx();
	parallelogram[1][YCOORD] = normal.p1().y() - normal.dy();
	parallelogram[2][XCOORD] = parallelogram[1][XCOORD] + l.dx();
	parallelogram[2][YCOORD] = parallelogram[1][YCOORD] + l.dy();
	parallelogram[3][XCOORD] = parallelogram[0][XCOORD] + l.dx();
	parallelogram[3][YCOORD] = parallelogram[0][YCOORD] + l.dy();
	QPointF mp = wire->mapFromScene(scenePos);
	double point[2];
	point[XCOORD] = mp.x();
	point[YCOORD] = mp.y();
	if (CrossingsTest(parallelogram, 4, point)) {
		return true;
	}

	return false;
}
