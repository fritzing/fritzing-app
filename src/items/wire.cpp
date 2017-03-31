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

$Revision: 7000 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-29 07:24:08 +0200 (Mo, 29. Apr 2013) $

********************************************************************/

/* 

curvy To Do

	curvy to begin with? would have to vary with some function of angle and distance
		could convert control points to t values?

	turn curvature on/off per view

---------------------------------------------------------

later:

	clippable wire

	gerber

	autorouter warning in PCB view

	modify parameters (tension, unit area)?

*/

/////////////////////////////////////////////////////////////////

#include "wire.h"

#include <qmath.h>
#include <QLineF>
#include <QPen>
#include <QRadialGradient>
#include <QBrush>
#include <QPen>
#include <QGraphicsScene>
#include <QList>
#include <QGraphicsItem>
#include <QSet>
#include <QComboBox>
#include <QToolTip>
#include <QApplication>
#include <QCheckBox>
#include <QHBoxLayout>

#include "../debugdialog.h"
#include "../sketch/infographicsview.h"
#include "../connectors/connectoritem.h"
#include "../connectors/svgidlayer.h"
#include "../fsvgrenderer.h"
#include "partlabel.h"
#include "../model/modelpart.h"
#include "../utils/graphicsutils.h"
#include "../utils/textutils.h"
#include "../utils/bezier.h"
#include "../utils/bezierdisplay.h"
#include "../utils/cursormaster.h"
#include "../utils/ratsnestcolors.h"
#include "../layerattributes.h"

#include <stdlib.h>

QVector<qreal> Wire::TheDash;
QVector<qreal> RatDash;
QBrush BandedBrush(QColor(255, 255, 255));


QHash<QString, QString> Wire::colorTrans;
QStringList Wire::colorNames;
QHash<int, QString> Wire::widthTrans;
QList<int> Wire::widths;
QList<QColor> Wire::lengthColorTrans;
double Wire::STANDARD_TRACE_WIDTH;
double Wire::HALF_STANDARD_TRACE_WIDTH;
double Wire::THIN_TRACE_WIDTH;

const double DefaultHoverStrokeWidth = 4;

static Bezier UndoBezier;
static BezierDisplay * TheBezierDisplay = NULL;

////////////////////////////////////////////////////////////

bool alphaLessThan(QColor * c1, QColor * c2)
{
	return c1->alpha() < c2->alpha();
}

void debugCompare(ItemBase * it) {
    Wire * wire = dynamic_cast<Wire *>(it);
    if (wire) {
        QRectF r0 = wire->connector0()->rect();
        QRectF r1 = wire->connector1()->rect();
        if (qAbs(r0.left() - r1.left()) < 0.1 && 
            qAbs(r0.right() - r1.right()) < 0.1 && 
            qAbs(r0.top() - r1.top()) < 0.1 &&
            qAbs(r0.bottom() - r1.bottom()) < 0.1)
        {
            wire->debugInfo("zero wire");
            if (wire->viewID() == ViewLayer::PCBView) {
                DebugDialog::debug("in pcb");
            }
        }
    }
}

/////////////////////////////////////////////////////////////

WireAction::WireAction(QAction * action) : QAction(action) {
	m_wire = NULL;
	this->setText(action->text());
	this->setStatusTip(action->statusTip());
	this->setCheckable(action->isCheckable());
}

WireAction::WireAction(const QString & title, QObject * parent) : QAction(title, parent) {
	m_wire = NULL;
}

void WireAction::setWire(Wire * w) {
	m_wire = w;
}

Wire * WireAction::wire() {
	return m_wire;
}

/////////////////////////////////////////////////////////////

Wire::Wire( ModelPart * modelPart, ViewLayer::ViewID viewID,  const ViewGeometry & viewGeometry, long id, QMenu* itemMenu, bool initLabel)
	: ItemBase(modelPart, viewID, viewGeometry, id, itemMenu)
{
    m_banded = false;
    m_colorByLength = false;
	m_bezier = NULL;
	m_displayBendpointCursor = m_canHaveCurve = true;
	m_hoverStrokeWidth = DefaultHoverStrokeWidth;
	m_connector0 = m_connector1 = NULL;
	m_partLabel = initLabel ? new PartLabel(this, NULL) : NULL;
	m_canChainMultiple = false;
    setFlag(QGraphicsItem::ItemIsSelectable, true );
	m_connectorHover = NULL;
	m_opacity = 1.0;
	m_ignoreSelectionChange = false;

	//DebugDialog::debug(QString("aix line %1 %2 %3 %4").arg(this->viewGeometry().line().x1())
													//.arg(this->viewGeometry().line().y1())
													//.arg(this->viewGeometry().line().x2())
													//.arg(this->viewGeometry().line().y2()) );
	//DebugDialog::debug(QString("aix loc %1 %2").arg(this->viewGeometry().loc().x())
														//.arg(this->viewGeometry().loc().y()) );

	setPos(m_viewGeometry.loc());

	m_dragCurve = m_dragEnd = false;
}

Wire::~Wire() {
	if (m_bezier) {
		delete m_bezier;
	}
}

FSvgRenderer * Wire::setUp(ViewLayer::ViewLayerID viewLayerID, const LayerHash &  viewLayers, InfoGraphicsView * infoGraphicsView) {
	ItemBase::setViewLayerID(viewLayerID, viewLayers);
	FSvgRenderer * svgRenderer = setUpConnectors(m_modelPart, m_viewID);
	if (svgRenderer != NULL) {
        initEnds(m_viewGeometry, svgRenderer->viewBox(), infoGraphicsView);
        //debugCompare(this);
	}
	setZValue(this->z());

	return svgRenderer;
}

void Wire::saveGeometry() {
	m_viewGeometry.setSelected(this->isSelected());
	m_viewGeometry.setLine(this->line());
	m_viewGeometry.setLoc(this->pos());
	m_viewGeometry.setZ(this->zValue());
}


bool Wire::itemMoved() {
	if  (m_viewGeometry.loc() != this->pos()) return true;

	if (this->line().dx() != m_viewGeometry.line().dx()) return false;
	if (this->line().dy() != m_viewGeometry.line().dy()) return false;

	return (this->line() != m_viewGeometry.line());
}

void Wire::moveItem(ViewGeometry & viewGeometry) {
	this->setPos(viewGeometry.loc());
	this->setLine(viewGeometry.line());
}

void Wire::initEnds(const ViewGeometry & vg, QRectF defaultRect, InfoGraphicsView * infoGraphicsView) {
	double penWidth = 1;
	foreach (ConnectorItem * item, cachedConnectorItems()) {
        if (item->connectorSharedID().endsWith("0")) {
            penWidth = item->rect().width();
            m_connector0 = item;
            //item->debugInfo("connector 0");
        }
        else {
            //item->debugInfo("connector 1");
			m_connector1 = item;
		}
	}

    if (m_connector0 == NULL || m_connector1 == NULL) {
        // should never happen
		return;
	}

	if ((vg.line().length() == 0) && (vg.line().x1() == -1)) {
		this->setLine(defaultRect.left(), defaultRect.top(), defaultRect.right(), defaultRect.bottom());
	}
	else {
        this->setLine(vg.line());
	}

	setConnector0Rect();
	setConnector1Rect();
	m_viewGeometry.setLine(this->line());
	
   	QBrush brush(QColor(0, 0, 0));
	QPen pen(brush, penWidth, Qt::SolidLine, Qt::RoundCap);
	this->setPen(pen);

	m_pen.setCapStyle(Qt::RoundCap);
	m_shadowPen.setCapStyle(Qt::RoundCap);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->initWire(this, penWidth);
	}

	prepareGeometryChange();
}

void Wire::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget ) {
	if (m_hidden) return;

	ItemBase::paint(painter, option, widget);
}

void Wire::paintBody(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget ) 
{
	Q_UNUSED(option);
	Q_UNUSED(widget);

	QPainterPath painterPath;
	if (m_bezier && !m_bezier->isEmpty()) {
		QLineF line = this->line();
		painterPath.moveTo(line.p1());
		painterPath.cubicTo(m_bezier->cp0(), m_bezier->cp1(), line.p2());
		
		/*
		DebugDialog::debug(QString("c0x:%1,c0y:%2 c1x:%3,c1y:%4 p0x:%5,p0y:%6 p1x:%7,p1y:%8 px:%9,py:%10")
							.arg(m_controlPoints.at(0).x())
							.arg(m_controlPoints.at(0).y())
							.arg(m_controlPoints.at(1).x())
							.arg(m_controlPoints.at(1).y())
							.arg(m_line.p1().x())
							.arg(m_line.p1().y())
							.arg(m_line.p2().x())
							.arg(m_line.p2().y())
							.arg(pos().x())
							.arg(pos().y())

							);
		*/
	}


	painter->setOpacity(m_inactive ? m_opacity  / 2 : m_opacity);
	if (hasShadow()) {
		painter->save();
		painter->setPen(m_shadowPen);
		if (painterPath.isEmpty()) {
			QLineF line = this->line();
			painter->drawLine(line);
		}
		else {
			painter->drawPath(painterPath);
		}
		painter->restore();
	}
	   
	// DebugDialog::debug(QString("pen width %1 %2").arg(m_pen.widthF()).arg(m_viewID));

    if (m_banded) {
        QBrush brush = m_pen.brush();
        m_pen.setStyle(Qt::SolidLine);
        m_pen.setBrush(BandedBrush);
        painter->setPen(m_pen);
	    if (painterPath.isEmpty()) {
		    painter->drawLine(getPaintLine());	
	    }
	    else {
		    painter->drawPath(painterPath);
	    }
        m_pen.setBrush(brush);
        m_pen.setDashPattern(TheDash);
        m_pen.setCapStyle(Qt::FlatCap);
    }

    if (getRatsnest()) {
        m_pen.setDashPattern(RatDash);
    }

    QColor realColor = color();
    if (coloredByLength()) {
        m_pen.setColor(colorForLength());
    }

    painter->setPen(m_pen);
    if (painterPath.isEmpty()) {
		painter->drawLine(getPaintLine());	
	}
	else {
		painter->drawPath(painterPath);
	}

    if (m_banded) {
        m_pen.setStyle(Qt::SolidLine);
        m_pen.setCapStyle(Qt::RoundCap);
    }

    if (coloredByLength()) {
        m_pen.setColor(realColor);
    }
}

void Wire::paintHover(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) 
{
	Q_UNUSED(widget);
	Q_UNUSED(option);
	painter->save();
	if ((m_connectorHoverCount > 0 && !(m_dragEnd || m_dragCurve)) || m_connectorHoverCount2 > 0) {
		painter->setOpacity(.50);
		painter->fillPath(this->hoverShape(), QBrush(ConnectorHoverColor));
	}
	else {
		painter->setOpacity(HoverOpacity);
		painter->fillPath(this->hoverShape(), QBrush(HoverColor));
	}
	painter->restore();
}

QPainterPath Wire::hoverShape() const
{
	return shapeAux(m_hoverStrokeWidth);
}

QPainterPath Wire::shape() const
{
	return shapeAux(m_pen.widthF());
}

QPainterPath Wire::shapeAux(double width) const
{
	QPainterPath path;
	if (m_line == QLineF()) {
	    return path;
	}
				
	path.moveTo(m_line.p1());
	if (m_bezier == NULL || m_bezier->isEmpty()) {
		path.lineTo(m_line.p2());
	}
	else {
		path.cubicTo(m_bezier->cp0(), m_bezier->cp1(), m_line.p2());
	}
	//DebugDialog::debug(QString("using hoverstrokewidth %1 %2").arg(m_id).arg(m_hoverStrokeWidth));
	return GraphicsUtils::shapeFromPath(path, m_pen, width, false);
}

QRectF Wire::boundingRect() const
{
	if (m_pen.widthF() == 0.0) {
	    const double x1 = m_line.p1().x();
	    const double x2 = m_line.p2().x();
	    const double y1 = m_line.p1().y();
	    const double y2 = m_line.p2().y();
	    double lx = qMin(x1, x2);
	    double rx = qMax(x1, x2);
	    double ty = qMin(y1, y2);
	    double by = qMax(y1, y2);
	    return QRectF(lx, ty, rx - lx, by - ty);
	}
	return hoverShape().controlPointRect();
}

void Wire::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
	//DebugDialog::debug("checking press event");
	emit wireSplitSignal(this, event->scenePos(), this->pos(), this->line());
}

void Wire::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->setActiveWire(this);
	}

	ItemBase::contextMenuEvent(event);
}

void Wire::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	ItemBase::mousePressEvent(event);
}

void Wire::initDragCurve(QPointF scenePos) {
	if (m_bezier == NULL) {
		m_bezier = new Bezier();
	}

	UndoBezier.copy(m_bezier);

	m_dragCurve = true;
	m_dragEnd = false;

	QPointF p0 = connector0()->sceneAdjustedTerminalPoint(NULL);
	QPointF p1 = connector1()->sceneAdjustedTerminalPoint(NULL);
	if (m_bezier->isEmpty()) {
		m_bezier->initToEnds(mapFromScene(p0), mapFromScene(p1));
	}
	else {
		m_bezier->set_endpoints(mapFromScene(p0), mapFromScene(p1));
	}

	m_bezier->initControlIndex(mapFromScene(scenePos), m_pen.widthF());
	TheBezierDisplay = new BezierDisplay;
	TheBezierDisplay->initDisplay(this, m_bezier);
}

bool Wire::initNewBendpoint(QPointF scenePos, Bezier & left, Bezier & right) {
	if (m_bezier == NULL || m_bezier->isEmpty()) {
		UndoBezier.clear();
		return false;
	}

	QPointF p0 = connector0()->sceneAdjustedTerminalPoint(NULL);
	QPointF p1 = connector1()->sceneAdjustedTerminalPoint(NULL);
	m_bezier->set_endpoints(mapFromScene(p0), mapFromScene(p1));
	UndoBezier.copy(m_bezier);

	double t = m_bezier->findSplit(mapFromScene(scenePos), m_pen.widthF());
	m_bezier->split(t, left, right);
	return true;
}

void Wire::initDragEnd(ConnectorItem * connectorItem, QPointF scenePos) {
	Q_UNUSED(scenePos);
	saveGeometry();
	QLineF line = this->line();
	m_drag0 = (connectorItem == m_connector0);
    //debugInfo("setting drag end to true");
	m_dragEnd = true;
	m_dragCurve = false;
	if (m_drag0) {
		m_wireDragOrigin = line.p2();
 		//DebugDialog::debug(QString("drag near origin %1 %2").arg(m_wireDragOrigin.x()).arg(m_wireDragOrigin.y()) );
	}
	else {
		m_wireDragOrigin = line.p1();
 		//DebugDialog::debug(QString("drag far origin %1 %2").arg(m_wireDragOrigin.x()).arg(m_wireDragOrigin.y()) );
 		//DebugDialog::debug(QString("drag far other %1 %2").arg(line.p2().x()).arg(line.p2().y()) );
	}

	if (connectorItem->chained()) {
		QList<Wire *> chained;
		QList<ConnectorItem *> ends;
		collectChained(chained, ends);
		// already saved the first one
		for (int i = 1; i < chained.count(); i++) {
			chained[i]->saveGeometry();
		}
	}
}


void Wire::mouseReleaseConnectorEvent(ConnectorItem * connectorItem, QGraphicsSceneMouseEvent * event) {
	Q_UNUSED(event);
	Q_UNUSED(connectorItem);
	releaseDrag();
}

void Wire::mouseMoveConnectorEvent(ConnectorItem * connectorItem, QGraphicsSceneMouseEvent * event) {
	mouseMoveEventAux(this->mapFromItem(connectorItem, event->pos()), event->modifiers());
}

void Wire::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
	mouseMoveEventAux(event->pos(), event->modifiers());
}

void Wire::mouseMoveEventAux(QPointF eventPos, Qt::KeyboardModifiers modifiers) {
	if (m_spaceBarWasPressed) {
		return;
	}

	if (m_dragCurve) {
		prepareGeometryChange();
		dragCurve(eventPos, modifiers);
		update();
		if (TheBezierDisplay) TheBezierDisplay->updateDisplay(this, m_bezier);
		return;
	}

	if (m_dragEnd == false) {
		return;
	}

    //debugInfo("dragging wire");

	ConnectorItem * whichConnectorItem;
	ConnectorItem * otherConnectorItem;
	if (m_drag0) {
		whichConnectorItem = m_connector0;
		otherConnectorItem = m_connector1;
	}
	else {
		whichConnectorItem = m_connector1;
		otherConnectorItem = m_connector0;
	}

	if ((modifiers & Qt::ShiftModifier) != 0) {
		QPointF initialPos = mapFromScene(otherConnectorItem->sceneAdjustedTerminalPoint(NULL)); 
		bool bendpoint = isBendpoint(whichConnectorItem);
		if (bendpoint) {
			bendpoint = false;
			foreach (ConnectorItem * ci, whichConnectorItem->connectedToItems()) {
				Wire * w = qobject_cast<Wire *>(ci->attachedTo());
				ConnectorItem * oci = w->otherConnector(ci);
				QPointF otherInitialPos = mapFromScene(oci->sceneAdjustedTerminalPoint(NULL));
				QPointF p1(initialPos.x(), otherInitialPos.y());
				double d = GraphicsUtils::distanceSqd(p1, eventPos);
				if (d <= 144) {
					bendpoint = true;
					eventPos = p1;
					break;
				}
				p1.setX(otherInitialPos.x());
				p1.setY(initialPos.y());
				d = GraphicsUtils::distanceSqd(p1, eventPos);
				if (d <= 144) {
					bendpoint = true;
					eventPos = p1;
					break;
				}				
			}
		}

		if (!bendpoint) {
			eventPos = GraphicsUtils::calcConstraint(initialPos, eventPos);
		}

	}

	if (m_drag0) {
		QPointF p = this->mapToScene(eventPos);
		QGraphicsSvgItem::setPos(p.x(), p.y());
		this->setLine(0, 0, m_wireDragOrigin.x() - p.x() + m_viewGeometry.loc().x(),
							m_wireDragOrigin.y() - p.y() + m_viewGeometry.loc().y() );
		//DebugDialog::debug(QString("drag0 wdo:(%1,%2) p:(%3,%4) vg:(%5,%6) l:(%7,%8)")
		//			.arg(m_wireDragOrigin.x()).arg(m_wireDragOrigin.y())
		//			.arg(p.x()).arg(p.y())
		//			.arg(m_viewGeometry.loc().x()).arg(m_viewGeometry.loc().y())
		//			.arg(line().p2().x()).arg(line().p2().y())
		//	);
	}
	else {
		this->setLine(m_wireDragOrigin.x(), m_wireDragOrigin.y(), eventPos.x(), eventPos.y());
		//DebugDialog::debug(QString("drag1 wdo:(%1,%2) ep:(%3,%4) p:(%5,%6) l:(%7,%8)")
		//			.arg(m_wireDragOrigin.x()).arg(m_wireDragOrigin.y())
		//			.arg(eventPos.x()).arg(eventPos.y())
		//			.arg(pos().x()).arg(pos().y())
		//			.arg(line().p2().x()).arg(line().p2().y())
		//	);
	}
	setConnector1Rect();


    QSet<ConnectorItem *> allTo;
    allTo.insert(whichConnectorItem);
	foreach (ConnectorItem * toConnectorItem, whichConnectorItem->connectedToItems()) {
		Wire * chainedWire = qobject_cast<Wire *>(toConnectorItem->attachedTo());
		if (chainedWire == NULL) continue;

        allTo.insert(toConnectorItem);
        foreach (ConnectorItem * subTo, toConnectorItem->connectedToItems()) {
            allTo.insert(subTo);
        }
	}
    allTo.remove(whichConnectorItem);

    // TODO: this could all be determined once at mouse press time

	if (allTo.count() == 0) {
        // dragging one end of the wire

		// don't allow wire to connect back to something the other end is already directly connected to
        // an alternative would be to exclude all connectors in the net connected by the same kind of trace
		QList<Wire *> wires;
		QList<ConnectorItem *> ends;
		collectChained(wires, ends);

        InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
        //DebugDialog::debug("------------------------");

        QList<ConnectorItem *> exclude;
        foreach (ConnectorItem * end, ends) {
            exclude << end;
            foreach (ConnectorItem * ci, end->connectedToItems()) {
                // if there is a wire growing out of one of the excluded ends, exclude the attached end
                exclude << ci;
            }
            foreach (ConnectorItem * toConnectorItem, end->connectedToItems()) {
                if (toConnectorItem->attachedToItemType() != ModelPart::Wire) continue;

                Wire * w = qobject_cast<Wire *>(toConnectorItem->attachedTo());
                if (w->getRatsnest()) continue;
                if (!w->isTraceType(infoGraphicsView->getTraceFlag())) continue;

                //w->debugInfo("what wire");

                QList<ConnectorItem *> ends2;
                QList<Wire *> wires2;
                w->collectChained(wires2, ends2);
                exclude.append(ends2);
                foreach (ConnectorItem * e2, ends2) {
                    foreach (ConnectorItem * ci, e2->connectedToItems()) {
                        // if there is a wire growing out of one of the excluded ends, exclude that end of the wire
                        exclude << ci;
                    }
                }
                foreach (Wire * w2, wires2) {
			        exclude.append(w2->cachedConnectorItems());
		        }
            }
        }


        // but allow to restore connections at this end (collect chained above got both ends of this wire) 
		foreach (ConnectorItem * toConnectorItem, whichConnectorItem->connectedToItems()) {
			if (ends.contains(toConnectorItem)) exclude.removeAll(toConnectorItem);
		}

        //DebugDialog::debug("");
        //DebugDialog::debug("__________________");
        //foreach (ConnectorItem * end, exclude) end->debugInfo("exclude");

        ConnectorItem * originatingConnector = NULL;
		if (otherConnectorItem) {
			foreach (ConnectorItem * toConnectorItem, otherConnectorItem->connectedToItems()) {
			    if (ends.contains(toConnectorItem)) {
                    originatingConnector = toConnectorItem;
                    break;
                }
		    }
		}

		whichConnectorItem->findConnectorUnder(false, true, exclude, true, originatingConnector);
	}
    else {
        // dragging a bendpoint
        foreach (ConnectorItem * toConnectorItem, allTo) {
            Wire * chained = qobject_cast<Wire *>(toConnectorItem->attachedTo());
            if (chained) {
                chained->simpleConnectedMoved(whichConnectorItem, toConnectorItem);
            }
        }
    }
}

QRectF Wire::connector0Rect(const QLineF & line) {
    QRectF rect = m_connector0->rect();
    rect.moveTo(0 - (rect.width()  / 2.0),
                0 - (rect.height()  / 2.0) );
    return rect;
}

QRectF Wire::connector1Rect(const QLineF & line) {

    QRectF rect = m_connector1->rect();
    rect.moveTo(line.dx() - (rect.width()  / 2.0),
                line.dy() - (rect.height()  / 2.0) );
    return rect;
}

void Wire::setConnector0Rect() {
	QRectF rect = m_connector0->rect();
	rect.moveTo(0 - (rect.width()  / 2.0),
				0 - (rect.height()  / 2.0) );
	m_connector0->setRect(rect);

    //debugCompare(this);

//	QPointF p = m_connector0->mapToScene(m_connector0->rect().center());
//	m_connector0->debugInfo(QString("c0:%1 %2").arg(p.x()).arg(p.y()));
//	p = m_connector1->mapToScene(m_connector1->rect().center());
//	m_connector1->debugInfo(QString("c1:%1 %2").arg(p.x()).arg(p.y()));

}


void Wire::setConnector1Rect() {
	QRectF rect = m_connector1->rect();
	rect.moveTo(this->line().dx() - (rect.width()  / 2.0),
				this->line().dy() - (rect.height()  / 2.0) );
	m_connector1->setRect(rect);

    //debugCompare(this);

//	QPointF p = m_connector0->mapToScene(m_connector0->rect().center());
//	m_connector0->debugInfo(QString("c0:%1 %2").arg(p.x()).arg(p.y()));
//	p = m_connector1->mapToScene(m_connector1->rect().center());
//	m_connector1->debugInfo(QString("c1:%1 %2").arg(p.x()).arg(p.y()));
}

void Wire::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
	if (m_spaceBarWasPressed) {
		return;
	}

    //debugInfo("wire release drag");
	if (releaseDrag()) return;

	ItemBase::mouseReleaseEvent(event);
}

bool Wire::releaseDrag() {
	if (m_dragEnd == false && m_dragCurve == false) return false;

	if (m_dragCurve) {
		delete TheBezierDisplay;
		TheBezierDisplay = NULL;
		m_dragCurve = false;
		ungrabMouse();
		if (UndoBezier != *m_bezier) {
			emit wireChangedCurveSignal(this, &UndoBezier, m_bezier, false);
		}
		return true;
	}

    //debugInfo("clearing drag end");
	m_dragEnd = false;

	ConnectorItem * from = (m_drag0) ? m_connector0 : m_connector1;
	ConnectorItem * to = from->releaseDrag();

	QLineF newLine = this->line();
	QLineF oldLine = m_viewGeometry.line();
	QPointF oldPos = m_viewGeometry.loc();
	QPointF newPos = this->pos();
	if (newLine != oldLine || oldPos != newPos) {
		emit wireChangedSignal(this, oldLine, newLine, oldPos, newPos, from, to);
	}

	return true;
}


void Wire::saveInstanceLocation(QXmlStreamWriter & streamWriter)
{
	QLineF line = m_viewGeometry.line();
	QPointF loc = m_viewGeometry.loc();
	streamWriter.writeAttribute("x", QString::number(loc.x()));
	streamWriter.writeAttribute("y", QString::number(loc.y()));
	streamWriter.writeAttribute("x1", QString::number(line.x1()));
	streamWriter.writeAttribute("y1", QString::number(line.y1()));
	streamWriter.writeAttribute("x2", QString::number(line.x2()));
	streamWriter.writeAttribute("y2", QString::number(line.y2()));
	streamWriter.writeAttribute("wireFlags", QString::number(m_viewGeometry.flagsAsInt()));
}

void Wire::writeGeometry(QXmlStreamWriter & streamWriter) {
	ItemBase::writeGeometry(streamWriter);
	streamWriter.writeStartElement("wireExtras");
	streamWriter.writeAttribute("mils", QString::number(mils()));
	streamWriter.writeAttribute("color", m_pen.brush().color().name());
	streamWriter.writeAttribute("opacity", QString::number(m_opacity));
	streamWriter.writeAttribute("banded", m_banded ? "1" : "0");
	if (m_bezier) m_bezier->write(streamWriter);
	streamWriter.writeEndElement();
}

void Wire::setExtras(QDomElement & element, InfoGraphicsView * infoGraphicsView)
{
	if (element.isNull()) return;

	bool ok;
	double w = element.attribute("width").toDouble(&ok);
	if (ok) {
		setWireWidth(w, infoGraphicsView, infoGraphicsView->getWireStrokeWidth(this, w));
	}
	else {
		w = element.attribute("mils").toDouble(&ok);
		if (ok) {
			double wpix = GraphicsUtils::mils2pixels(w, GraphicsUtils::SVGDPI);
			setWireWidth(wpix, infoGraphicsView, infoGraphicsView->getWireStrokeWidth(this, wpix));
		}
	}

    m_banded = (element.attribute("banded", "") == "1");

	setColorFromElement(element);
	QDomElement bElement = element.firstChildElement("bezier");
	Bezier bezier = Bezier::fromElement(bElement);
	if (!bezier.isEmpty()) {
		prepareGeometryChange();
		m_bezier = new Bezier;
		m_bezier->copy(&bezier);
		QPointF p0 = connector0()->sceneAdjustedTerminalPoint(NULL);
		QPointF p1 = connector1()->sceneAdjustedTerminalPoint(NULL);
		m_bezier->set_endpoints(mapFromScene(p0), mapFromScene(p1));
	}

}

void Wire::setColorFromElement(QDomElement & element) {
	QString colorString = element.attribute("color");
	if (colorString.isNull() || colorString.isEmpty()) return;

	bool ok;
	double op = element.attribute("opacity").toDouble(&ok);
	if (!ok) {
		op = 1.0;
	}

	setColorString(colorString, op, false);
}

void Wire::hoverEnterConnectorItem(QGraphicsSceneHoverEvent * event , ConnectorItem * item) {
	m_connectorHover = item;
	ItemBase::hoverEnterConnectorItem(event, item);
}

void Wire::hoverLeaveConnectorItem(QGraphicsSceneHoverEvent * event, ConnectorItem * item) {
	m_connectorHover = NULL;
	ItemBase::hoverLeaveConnectorItem(event, item);
}

void Wire::hoverEnterEvent ( QGraphicsSceneHoverEvent * event ) {
	ItemBase::hoverEnterEvent(event);
	CursorMaster::instance()->addCursor(this, cursor());
	//DebugDialog::debug("---wire set override cursor");
	updateCursor(event->modifiers());
}

void Wire::hoverLeaveEvent ( QGraphicsSceneHoverEvent * event ) {
	ItemBase::hoverLeaveEvent(event);
	//DebugDialog::debug("------wire restore override cursor");
	CursorMaster::instance()->removeCursor(this);
}


void Wire::connectionChange(ConnectorItem * onMe, ConnectorItem * onIt, bool connect) {
	checkVisibility(onMe, onIt, connect);

	bool movable = true;
	foreach (ConnectorItem * connectedTo, m_connector0->connectedToItems()) {
		if (connectedTo->attachedToItemType() != ModelPart::Wire) {
			movable = false;
			break;
		}
	}
	if (movable) {
		foreach (ConnectorItem * connectedTo, m_connector1->connectedToItems()) {
			if (connectedTo->attachedToItemType() != ModelPart::Wire) {
				movable = false;
				break;
			}
		}
	}
}

void Wire::mouseDoubleClickConnectorEvent(ConnectorItem * connectorItem) {
	int chained = 0;
	foreach (ConnectorItem * toConnectorItem, connectorItem->connectedToItems()) {
		if (toConnectorItem->attachedToItemType() == ModelPart::Wire) {
			chained++;
		}
		else {
			return;
		}
	}


	if (chained == 1) {
		// near as I can tell, this is to eliminate the overrides from the connectorItem and then from the wire itself
		emit wireJoinSignal(this, connectorItem);
	}
}

void Wire::mousePressConnectorEvent(ConnectorItem * connectorItem, QGraphicsSceneMouseEvent * event) {
	//DebugDialog::debug("checking press connector event");

	if (m_canChainMultiple && event->modifiers() & altOrMetaModifier()) {
		// dragging a wire out of a bendpoint
		InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
		if (infoGraphicsView != NULL) {
			infoGraphicsView->mousePressConnectorEvent(connectorItem, event);
		}

		return;
	}


	connectorItem->setOverConnectorItem(NULL);
	initDragEnd(connectorItem, event->scenePos());

}

void Wire::simpleConnectedMoved(ConnectorItem * to) {
	// to is this wire, from is something else
	simpleConnectedMoved(to->firstConnectedToIsh(), to);
}

void Wire::simpleConnectedMoved(ConnectorItem * from, ConnectorItem * to)
{
	if (from == NULL) return;

    //if (from) from->debugInfo("connected moved from");
    //if (to) to->debugInfo("\tto");

	// to is this wire, from is something else
	QPointF p1, p2;
	calcNewLine(from, to, p1, p2);

	/*
	QPointF oldPos = this->pos();
	QPointF newPos = p1;
	QLineF oldLine = this->line();
	QLineF newLine(0, 0,  p2.x() - p1.x(), p2.y() - p1.y());
	if (qAbs(oldPos.x() - newPos.x()) > 1.75 ||
		qAbs(oldPos.y() - newPos.y()) > 1.75 ||
		qAbs(oldLine.x1() - newLine.x1()) > 1.75 ||
		qAbs(oldLine.x2() - newLine.x2()) > 1.75 ||
		qAbs(oldLine.y1() - newLine.y1()) > 1.75 ||
		qAbs(oldLine.y2() - newLine.y2()) > 1.75
		)
	{
		DebugDialog::debug("line changed");
		calcNewLine(from,to,p1,p2);
	}
	*/

	this->setPos(p1);
	this->setLine(0,0, p2.x() - p1.x(), p2.y() - p1.y() );
	//debugInfo(QString("set line  %1 %2, %3 %4, vis:%5").arg(p1.x()).arg(p1.y()).arg(p2.x()).arg(p2.y()).arg(isVisible()) );
	setConnector1Rect();
}

void Wire::calcNewLine(ConnectorItem * from, ConnectorItem * to, QPointF & p1, QPointF & p2) {
	// to is this wire, from is something else
	if (to == m_connector0) {
		p1 = from->sceneAdjustedTerminalPoint(to);
		ConnectorItem * otherFrom = m_connector1->firstConnectedToIsh();
		if (otherFrom == NULL) {
			p2 = m_connector1->mapToScene(m_connector1->rect().center());
		}
		else {
			p2 = otherFrom->sceneAdjustedTerminalPoint(m_connector1);
		}
	}
	else {
		p2 = from->sceneAdjustedTerminalPoint(to);
		ConnectorItem * otherFrom = m_connector0->firstConnectedToIsh();
		if (otherFrom == NULL) {
			p1 = m_connector0->mapToScene(m_connector0->rect().center());
		}
		else {
			p1 = otherFrom->sceneAdjustedTerminalPoint(m_connector0);
		}

	}
}

void Wire::connectedMoved(ConnectorItem * from, ConnectorItem * to, QList<ConnectorItem *> & already) {
    Q_UNUSED(already);

	// "from" is the connector on the part
	// "to" is the connector on the wire

    //from->debugInfo("connected moved");
    //to->debugInfo("\tconnected moved");

    if (from->connectedToItems().contains(to) || to->connectedToItems().contains(from)) {
	    simpleConnectedMoved(from, to);
    }
    else {
        //from->debugInfo("not connected");
        //to->debugInfo("\t");
    }
}


FSvgRenderer * Wire::setUpConnectors(ModelPart * modelPart, ViewLayer::ViewID viewID) 
{
	clearConnectorItemCache();

	LayerAttributes layerAttributes;
    this->initLayerAttributes(layerAttributes, viewID, m_viewLayerID, m_viewLayerPlacement, false, false);
	FSvgRenderer * renderer = ItemBase::setUpImage(modelPart, layerAttributes);
	if (renderer == NULL) {
		return NULL;
	}

	foreach (Connector * connector, modelPart->connectors().values()) {
		if (connector == NULL) continue;

		SvgIdLayer * svgIdLayer = connector->fullPinInfo(viewID, m_viewLayerID);
		if (svgIdLayer == NULL) continue;

		bool result = renderer->setUpConnector(svgIdLayer, false, viewLayerPlacement());
		if (!result) continue;

		ConnectorItem * connectorItem = newConnectorItem(connector);
		connectorItem->setRect(svgIdLayer->rect(viewLayerPlacement()));
		connectorItem->setTerminalPoint(svgIdLayer->point(viewLayerPlacement()));
		m_originalConnectorRect = svgIdLayer->rect(viewLayerPlacement());

		connectorItem->setCircular(true);
		//DebugDialog::debug(QString("terminal point %1 %2").arg(terminalPoint.x()).arg(terminalPoint.y()) );
	}

	return renderer;
}

/*
void Wire::setPos(const QPointF & pos) {
	ItemBase::setPos(pos);
}
*/


void Wire::setLineAnd(QLineF line, QPointF pos, bool useLine) {
	this->setPos(pos);
	if (useLine) this->setLine(line);

	setConnector1Rect();
}

ConnectorItem * Wire::otherConnector(ConnectorItem * oneConnector) {
	if (oneConnector == m_connector0) return m_connector1;

	return m_connector0;
}

ConnectorItem * Wire::connector0() {
	return m_connector0;
}

ConnectorItem * Wire::connector1() {
	return m_connector1;
}

void Wire::findConnectorsUnder() {
	foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
		if (connectorItem->connectionsCount() > 0) continue;  // only check free ends
		connectorItem->findConnectorUnder(true, false, ConnectorItem::emptyConnectorItemList, false, NULL);
	}
}

void Wire::collectChained(QList<Wire *> & chained, QList<ConnectorItem *> & ends ) {
	chained.append(this);
	for (int i = 0; i < chained.count(); i++) {
		Wire * wire = chained[i];
		collectChained(wire->m_connector1, chained, ends);
		collectChained(wire->m_connector0, chained, ends);
	}
}

void Wire::collectChained(ConnectorItem * connectorItem, QList<Wire *> & chained, QList<ConnectorItem *> & ends) {
	if (connectorItem == NULL) return;

	foreach (ConnectorItem * connectedToItem, connectorItem->connectedToItems()) {
		Wire * wire = qobject_cast<Wire *>(connectedToItem->attachedTo());
		if (wire == NULL) {
			if (!ends.contains(connectedToItem)) {
				ends.append(connectedToItem);
			}
			continue;
		}

		if (chained.contains(wire)) continue;
		chained.append(wire);
	}
}

void Wire::collectWires(QList<Wire *> & wires) {
	if (wires.contains(this)) return;

	wires.append(this);
	//DebugDialog::debug(QString("collecting wire %1").arg(this->id()) );
	collectWiresAux(wires, m_connector0);
	collectWiresAux(wires, m_connector1);
}

void Wire::collectWiresAux(QList<Wire *> & wires, ConnectorItem * start) {
	foreach (ConnectorItem * toConnectorItem, start->connectedToItems()) {
		if (toConnectorItem->attachedToItemType() == ModelPart::Wire) {
			qobject_cast<Wire *>(toConnectorItem->attachedTo())->collectWires(wires);
		}
	}

}

bool Wire::stickyEnabled()
{
	QList<Wire *> wires;
	QList<ConnectorItem *> ends;
	this->collectChained(wires, ends);
	foreach (ConnectorItem * connector, ends) {
		if (connector->connectionsCount() > 0) {
			return false;
		}
	}

	return true;
}

bool Wire::getTrace() {
	return m_viewGeometry.getAnyTrace();
}

bool Wire::getRouted() {
	return m_viewGeometry.getRouted();
}

void Wire::setRouted(bool routed) {
	m_viewGeometry.setRouted(routed);
}

void Wire::setRatsnest(bool ratsnest) {
	m_viewGeometry.setRatsnest(ratsnest);
}

void Wire::setAutoroutable(bool ar) {
	m_viewGeometry.setAutoroutable(ar);
}

bool Wire::getAutoroutable() {
	return m_viewGeometry.getAutoroutable();
}

void Wire::setNormal(bool normal) {
	m_viewGeometry.setNormal(normal);
}

bool Wire::getNormal() {
	return m_viewGeometry.getNormal();
}

void Wire::setColor(const QColor & color, double op) {
	m_pen.setBrush(QBrush(color));
	m_opacity = op;
	m_colorName = color.name();
	this->update();
}

void Wire::setShadowColor(QColor & color, bool restore) {
	m_shadowBrush = QBrush(color);
	m_shadowPen.setBrush(m_shadowBrush);
	m_bendpointPen.setBrush(m_shadowBrush);
	m_bendpoint2Pen.setBrush(m_shadowBrush);
    QList<ConnectorItem *> visited;
    if (restore) {
	    if (m_connector0) m_connector0->restoreColor(visited);
	    if (m_connector1) m_connector1->restoreColor(visited);
    }
	this->update();
}

const QColor & Wire::color() {
	return m_pen.brush().color();
}

void Wire::setWireWidth(double width, InfoGraphicsView * infoGraphicsView, double hoverStrokeWidth) {
	if (m_pen.widthF() == width) return;

	prepareGeometryChange();
	setPenWidth(width, infoGraphicsView, hoverStrokeWidth);
    QList<ConnectorItem *> visited;
	if (m_connector0) m_connector0->restoreColor(visited);
	if (m_connector1) m_connector1->restoreColor(visited);
	update();
}

double Wire::width() {
	return m_pen.widthF();
}

double Wire::shadowWidth() {
	return m_shadowPen.widthF();
}

double Wire::mils() {
	return 1000 * m_pen.widthF() / GraphicsUtils::SVGDPI;
}

void Wire::setColorString(QString colorName, double op, bool restore) {
	// sets a color using the name (.e. "red")
	// note: colorName is associated with a Fritzing color, not a Qt color

	QString colorString = RatsnestColors::wireColor(m_viewID, colorName);     
	if (colorString.isEmpty()) {
		colorString = colorName;
	}

	QColor c;
	c.setNamedColor(colorString);
	setColor(c, op);
	m_colorName = colorName;

	QString shadowColorString = RatsnestColors::shadowColor(m_viewID, colorName);
	if (shadowColorString.isEmpty()) {
		shadowColorString = colorString;
	}

	c.setNamedColor(shadowColorString);
	setShadowColor(c, restore);
}

QString Wire::hexString() {
	return m_pen.brush().color().name();
}

QString Wire::shadowHexString() {
	return m_shadowPen.brush().color().name();
}

QString Wire::colorString() {
	return m_colorName;
}

void Wire::initNames() {
	if (colorNames.count() > 0) return;

    TheDash.clear();
    TheDash << 10 << 8;
    RatDash.clear();
    RatDash << 2 << 2;

	widths << 8 << 12 << 16 << 24 << 32 << 48;
    int i = 0;
	widthTrans.insert(widths[i++], tr("super fine (8 mil)"));
	widthTrans.insert(widths[i++], tr("extra thin (12 mil)"));

	THIN_TRACE_WIDTH = GraphicsUtils::mils2pixels(widths[i], GraphicsUtils::SVGDPI);
	widthTrans.insert(widths[i++], tr("thin (16 mil)"));

	STANDARD_TRACE_WIDTH = GraphicsUtils::mils2pixels(widths[i], GraphicsUtils::SVGDPI);
	widthTrans.insert(widths[i++], tr("standard (24 mil)"));

	widthTrans.insert(widths[i++], tr("thick (32 mil)"));
	widthTrans.insert(widths[i++], tr("extra thick (48 mil)"));

	HALF_STANDARD_TRACE_WIDTH = STANDARD_TRACE_WIDTH / 2.0;

    // need a list because a hash table doesn't guarantee order 
    colorNames.append(tr("blue"));
	colorNames.append(tr("red"));
    colorNames.append(tr("black"));
	colorNames.append(tr("yellow"));
	colorNames.append(tr("green"));
	colorNames.append(tr("grey"));
	colorNames.append(tr("white"));
    colorNames.append(tr("orange"));
    colorNames.append(tr("ochre"));
    colorNames.append(tr("cyan"));
    colorNames.append(tr("brown"));
    colorNames.append(tr("purple"));
    colorNames.append(tr("pink"));

	// need this hash table to translate from user's language to internal color name
    colorTrans.insert(tr("blue"), "blue");
	colorTrans.insert(tr("red"), "red");
    colorTrans.insert(tr("black"), "black");
	colorTrans.insert(tr("yellow"), "yellow");
	colorTrans.insert(tr("green"), "green");
	colorTrans.insert(tr("grey"), "grey");
	colorTrans.insert(tr("white"), "white");
    colorTrans.insert(tr("orange"), "orange");
    colorTrans.insert(tr("ochre"), "ochre");
    colorTrans.insert(tr("cyan"), "cyan");
	colorTrans.insert(tr("brown"), "brown");
    colorTrans.insert(tr("purple"), "purple");
    colorTrans.insert(tr("pink"), "pink");

    // List of standard breadboard jumper colors, as seen in various kits
    lengthColorTrans.append(QColor(192, 192, 192)); // 100 mil -> silver (no insulator)
    lengthColorTrans.append(QColor(209, 0, 0)); // 200mil -> red
    lengthColorTrans.append(QColor(255, 102, 34)); // 300mil -> orange
    lengthColorTrans.append(QColor(255, 218, 33)); // 400mil -> yellow
    lengthColorTrans.append(QColor(51, 221, 0)); // 500mil -> green
    lengthColorTrans.append(QColor(17, 51, 204)); // 600mil -> blue
    lengthColorTrans.append(QColor(75, 0, 130)); // 700mil -> purple
    lengthColorTrans.append(QColor(169, 169, 169)); // 800mil -> gray
    lengthColorTrans.append(QColor(245, 245, 245)); // 900mil -> white
    lengthColorTrans.append(QColor(139, 69, 19)); // 1000mil -> brown

}

bool Wire::hasFlag(ViewGeometry::WireFlag flag)
{
	return m_viewGeometry.hasFlag(flag);
}

bool Wire::isTraceType(ViewGeometry::WireFlag flag) {
	return hasFlag(flag);
}

bool Wire::hasAnyFlag(ViewGeometry::WireFlags flags)
{
	return m_viewGeometry.hasAnyFlag(flags);
}

Wire * Wire::findTraced(ViewGeometry::WireFlags flags, QList<ConnectorItem *>  & ends) {
	QList<Wire *> chainedWires;
	this->collectChained(chainedWires, ends);
	if (ends.count() != 2) {
		DebugDialog::debug(QString("wire in jumper or trace must have two ends") );
		return NULL;
	}

	return ConnectorItem::directlyWiredTo(ends[0], ends[1], flags);
}

void Wire::setWireFlags(ViewGeometry::WireFlags wireFlags) {
	m_viewGeometry.setWireFlags(wireFlags);
}

double Wire::opacity() {
	return m_opacity;
}

void Wire::setOpacity(double opacity) {
	m_opacity = opacity;
	this->update();
}

bool Wire::draggingEnd() {
	return m_dragEnd || m_dragCurve;
}

void Wire::setCanChainMultiple(bool can) {
	m_canChainMultiple = can;
}

bool Wire::canChangeColor() {
	if (getRatsnest()) return false;
	if (!getTrace()) return true;

	return (this->m_viewID == ViewLayer::SchematicView);
}

void Wire::collectDirectWires(QList<Wire *> & wires) {
    bool firstRound = false;
	if (!wires.contains(this)) {
		wires.append(this);
        firstRound = true;
	}

    QList<ConnectorItem *> junctions;
    if (firstRound) {
        // collect up to any junction
	    collectDirectWires(m_connector0, wires, junctions);
	    collectDirectWires(m_connector1, wires, junctions);
        return;
    }

    // second round: deal with junctions
    foreach (Wire * wire, wires) {
        junctions << wire->connector0() << wire->connector1();
    }

    int ix = 0;
    while (ix < junctions.count()) {
        ConnectorItem * junction = junctions.at(ix++);

        QSet<Wire *> jwires;
        foreach (ConnectorItem * toConnectorItem, junction->connectedToItems()) {
            if (toConnectorItem->attachedToItemType() != ModelPart::Wire) break;

            Wire * w = qobject_cast<Wire *>(toConnectorItem->attachedTo());
            if (!wires.contains(w)) jwires << w;

            bool onlyWiresConnected = true;
            foreach (ConnectorItem * toToConnectorItem, toConnectorItem->connectedToItems()) {
                if (toToConnectorItem->attachedToItemType() != ModelPart::Wire) {
                    onlyWiresConnected = false;
                    break;
                }

                w = qobject_cast<Wire *>(toToConnectorItem->attachedTo());
                if (!wires.contains(w)) jwires << w;
            }
            if (!onlyWiresConnected) break;
        }

        if (jwires.count() == 1) {
            // there is a junction of > 2 wires and all wires leading to it except one are already on the delete list
            Wire * w = jwires.values().at(0);
            wires << w;
            w->collectDirectWires(w->connector0(), wires, junctions);
            w->collectDirectWires(w->connector1(), wires, junctions);
        }
    }
}


void Wire::collectDirectWires(ConnectorItem * connectorItem, QList<Wire *> & wires, QList<ConnectorItem *> & junctions) {
	if (connectorItem->connectionsCount() == 0) return;
    if (connectorItem->connectionsCount() > 1) {
        if (!junctions.contains(connectorItem)) junctions.append(connectorItem);
        return;
    }

	ConnectorItem * toConnectorItem = connectorItem->connectedToItems().at(0);
	if (toConnectorItem->attachedToItemType() != ModelPart::Wire) return;

    if (toConnectorItem->connectionsCount() != 1) {
        if (!junctions.contains(connectorItem)) junctions.append(connectorItem);
        return;
    }

	Wire * nextWire = qobject_cast<Wire *>(toConnectorItem->attachedTo());
	if (wires.contains(nextWire)) return;

	wires.append(nextWire);
	nextWire->collectDirectWires(nextWire->otherConnector(toConnectorItem), wires, junctions);
}

QVariant Wire::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSelectedChange) {
		if (m_partLabel) {
			m_partLabel->update();
		}

		if (!m_ignoreSelectionChange) {
			QList<Wire *> chained;
			QList<ConnectorItem *> ends;
			collectChained(chained, ends);
			InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
			if (infoGraphicsView) {
				infoGraphicsView->setIgnoreSelectionChangeEvents(true);
			}
			// DebugDialog::debug(QString("original wire selected %1 %2").arg(value.toBool()).arg(this->id()));
			foreach (Wire * wire, chained) {
				if (wire != this ) {
					wire->setIgnoreSelectionChange(true);
					wire->setSelected(value.toBool());
					wire->setIgnoreSelectionChange(false);
					// DebugDialog::debug(QString("wire selected %1 %2").arg(value.toBool()).arg(wire->id()));
				}
			}
			if (infoGraphicsView) {
				infoGraphicsView->setIgnoreSelectionChangeEvents(false);
			}
		}
    }
    return ItemBase::itemChange(change, value);
}

void Wire::cleanup() {
}

void Wire::getConnectedColor(ConnectorItem * connectorItem, QBrush &brush, QPen &pen, double & opacity, double & negativePenWidth, bool & negativeOffsetRect) 
{
	connectorItem->setBigDot(false);
	ItemBase::getConnectedColor(connectorItem, brush, pen, opacity, negativePenWidth, negativeOffsetRect);
	int count = 0;
	bool bendpoint = true;
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView == NULL) {
		return;
    }

	foreach (ConnectorItem * toConnectorItem, connectorItem->connectedToItems()) {
		if (toConnectorItem->attachedToItemType() == ModelPart::Wire) {
			Wire * w = qobject_cast<Wire *>(toConnectorItem->attachedTo());
			if (w->isTraceType(infoGraphicsView->getTraceFlag())) {
				count++;
			}
		}
		else {
			// for drawing a big dot on the end of a part connector in schematic view if the part is connected to more than one trace
			bendpoint = false;
			if (toConnectorItem->connectionsCount() > 1) {	
				if (infoGraphicsView->hasBigDots()) {
					int c = 0;
					foreach (ConnectorItem * totoConnectorItem, toConnectorItem->connectedToItems()) {
						if (totoConnectorItem->attachedToItemType() == ModelPart::Wire) {
							Wire * w = qobject_cast<Wire *>(totoConnectorItem->attachedTo());
							if (w && w->isTraceType(ViewGeometry::SchematicTraceFlag) && w->isTraceType(infoGraphicsView->getTraceFlag())) {
								c++;
							}
						}
					}
					if (c > 1) {
						count = 2;
						break;
					}
				}
			}

			count = 0;
			break;
		}
	}

	if (count == 0) {
		return;
	}
	
	// connectorItem is a bendpoint or connects to a multiply connected connector

	//if (!bendpoint) {
		//DebugDialog::debug(QString("big dot %1 %2 %3").arg(this->id()).arg(connectorItem->connectorSharedID()).arg(count));
	//}

	brush = m_shadowBrush;
	opacity = 1.0;
	if (count > 1) {
		// only ever reach here when drawing a connector that is connected to more than one trace
		pen = m_bendpoint2Pen;
		negativePenWidth = m_bendpoint2Width;
		negativeOffsetRect = m_negativeOffsetRect;
		connectorItem->setBigDot(true);
	}
	else {
		negativeOffsetRect = m_negativeOffsetRect;
		negativePenWidth = m_bendpointWidth;
		pen = m_bendpointPen;
	}
}

void Wire::setPenWidth(double w, InfoGraphicsView * infoGraphicsView, double hoverStrokeWidth) {
	m_hoverStrokeWidth = hoverStrokeWidth;
	//DebugDialog::debug(QString("setting hoverstrokewidth %1 %2").arg(m_id).arg(m_hoverStrokeWidth));
	m_pen.setWidthF(w);
	infoGraphicsView->getBendpointWidths(this, w, m_bendpointWidth, m_bendpoint2Width, m_negativeOffsetRect);
	m_bendpointPen.setWidthF(qAbs(m_bendpointWidth));
	m_bendpoint2Pen.setWidthF(qAbs(m_bendpoint2Width));
	m_shadowPen.setWidthF(w + 2);
}

bool Wire::connectionIsAllowed(ConnectorItem * to) {
	if (!ItemBase::connectionIsAllowed(to)) return false;

	Wire * w = qobject_cast<Wire *>(to->attachedTo());
	if (w == NULL) return true;

	if (w->getRatsnest()) return false;

	return m_viewID != ViewLayer::BreadboardView;
}

bool Wire::isGrounded() {
	return ConnectorItem::isGrounded(connector0(), connector1());
}

bool Wire::acceptsMouseDoubleClickConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *) {
	return true;
}

bool Wire::acceptsMouseMoveConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *) {
	return true;
}

bool Wire::acceptsMouseReleaseConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *) {
	return true;
}

void Wire::setIgnoreSelectionChange(bool ignore) {
	m_ignoreSelectionChange = ignore;
}

bool Wire::collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide)
{
	if (prop.compare("width", Qt::CaseInsensitive) == 0) {
		// don't display width property
        hide = true;
		return false;
	}

	if (prop.compare("color", Qt::CaseInsensitive) == 0) {
		returnProp = tr("color");
		if (canChangeColor()) {
			QComboBox * comboBox = new QComboBox(parent);
			comboBox->setEditable(false);
			comboBox->setEnabled(swappingEnabled);
			comboBox->setObjectName("infoViewComboBox");
			
			int ix = 0;
			QString englishCurrColor = colorString();
			foreach(QString transColorName, Wire::colorNames) {
				QString englishColorName = Wire::colorTrans.value(transColorName);
				bool ok = (this->m_viewID != ViewLayer::SchematicView || englishColorName.compare("white", Qt::CaseInsensitive) != 0);
				if (ok) {
					comboBox->addItem(transColorName, QVariant(englishColorName));
					if (englishColorName.compare(englishCurrColor, Qt::CaseInsensitive) == 0) {
						comboBox->setCurrentIndex(ix);
					}
					ix++;
				}
			}

			connect(comboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(colorEntry(const QString &)));

            if (this->hasShadow()) {
                QCheckBox * checkBox = new QCheckBox(tr("Banded"));
		        checkBox->setChecked(m_banded);
		        checkBox->setObjectName("infoViewCheckBox");
                connect(checkBox, SIGNAL(clicked(bool)), this, SLOT(setBandedProp(bool)));

                QFrame * frame = new QFrame(parent);
                QHBoxLayout * hboxLayout = new QHBoxLayout;
                hboxLayout->addWidget(comboBox);
                hboxLayout->addWidget(checkBox);
                frame->setLayout(hboxLayout);

                returnWidget = frame;
            }
            else {
		        returnWidget = comboBox;
            }
	
			returnValue = comboBox->currentText();
			return true;
		}
		else {
			returnWidget = NULL;
			returnValue = colorString();
			return true;
		}
	}

	return ItemBase::collectExtraInfo(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget, hide);
}

void Wire::colorEntry(const QString & text) {
	Q_UNUSED(text);

	QComboBox * comboBox = qobject_cast<QComboBox *>(sender());
	if (comboBox == NULL) return;

	QString color = comboBox->itemData(comboBox->currentIndex()).toString();

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->changeWireColor(color);
	}
}

bool Wire::hasPartLabel() {
	
	return false;
}

ItemBase::PluralType Wire::isPlural() {
	return Plural;
}

void Wire::checkVisibility(ConnectorItem * onMe, ConnectorItem * onIt, bool connect) {
	if (connect) {
		if (!onIt->attachedTo()->isVisible()) {
			this->setVisible(false);
		}
		else {
			ConnectorItem * other = otherConnector(onMe);
			foreach (ConnectorItem * toConnectorItem, other->connectedToItems()) {
				if (toConnectorItem->attachedToItemType() == ModelPart::Wire) continue;

				if (!toConnectorItem->attachedTo()->isVisible()) {
					this->setVisible(false);
					break;
				}
			}
		}
	}
}

bool Wire::canSwitchLayers() {
	return false;
}

bool Wire::hasPartNumberProperty()
{
	return false;
}

bool Wire::rotationAllowed() {
	return false;
}

bool Wire::rotation45Allowed() {
	return false;
}

void Wire::addedToScene(bool temporary) {
	ItemBase::addedToScene(temporary);

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView == NULL) return;

	infoGraphicsView->newWire(this);
}

void Wire::setConnectorDimensions(double width, double height) 
{
	setConnectorDimensionsAux(connector0(), width, height);
	setConnectorDimensionsAux(connector1(), width, height);
}

void Wire::setConnectorDimensionsAux(ConnectorItem * connectorItem, double width, double height) 
{
	QPointF p = connectorItem->rect().center();
	QRectF r(p.x() - (width / 2), p.y() - (height / 2), width, height);
	connectorItem->setRect(r);
	connectorItem->setTerminalPoint(r.center() - r.topLeft());
    //debugCompare(connectorItem->attachedTo());
}

void Wire::originalConnectorDimensions(double & width, double & height) 
{
	width = m_originalConnectorRect.width();
	height = m_originalConnectorRect.height();
}

bool Wire::isBendpoint(ConnectorItem * connectorItem) {
	return connectorItem->isBendpoint();
}

double Wire::hoverStrokeWidth() {
	return m_hoverStrokeWidth;
}

const QLineF & Wire::getPaintLine() {
	return m_line;
}

/*!
    Returns the item's line, or a null line if no line has been set.

    \sa setLine()
*/
QLineF Wire::line() const
{
    return m_line;
}

/*!
    Sets the item's line to be the given \a line.

    \sa line()
*/
void Wire::setLine(const QLineF &line)
{

    //if (line.length() < 0.5) {
    //    debugInfo("zero line");
    //}

    if (m_line == line)
        return;
    prepareGeometryChange();
    m_line = line;
    update();
}

void Wire::setLine(double x1, double y1, double x2, double y2)
{ 
	setLine(QLineF(x1, y1, x2, y2)); 
}

/*!
    Returns the item's pen, or a black solid 0-width pen if no pen has
    been set.

    \sa setPen()
*/
QPen Wire::pen() const
{
    return m_pen;
}

/*!
    Sets the item's pen to \a pen. If no pen is set, the line will be painted
    using a black solid 0-width pen.

    \sa pen()
*/
void Wire::setPen(const QPen &pen)
{
	if (pen.widthF() != m_pen.widthF()) {
		prepareGeometryChange();
	}
    m_pen = pen;
    update();
}

bool Wire::canHaveCurve() {
	return m_canHaveCurve && !getRatsnest();
}

void Wire::dragCurve(QPointF eventPos, Qt::KeyboardModifiers)
{
	m_bezier->recalc(eventPos);
}

void Wire::changeCurve(const Bezier * bezier)
{
	prepareGeometryChange();
	if (m_bezier == NULL) m_bezier = new Bezier;
	m_bezier->copy(bezier);
	update();
}

bool Wire::isCurved() {
	return (m_bezier != NULL) && !m_bezier->isEmpty();
}

const Bezier * Wire::curve() {
	return m_bezier;
}

const Bezier * Wire::undoCurve() {
	return &UndoBezier;
}

QPolygonF Wire::sceneCurve(QPointF offset) {
	QPolygonF poly;
	if (m_bezier == NULL) return poly;
	if (m_bezier->isEmpty()) return poly;

	poly.append(m_line.p1() + pos() - offset);
	poly.append(m_bezier->cp0() + pos() - offset);
	poly.append(m_bezier->cp1() + pos() - offset);
	poly.append(m_line.p2() + pos() - offset);
	return poly;
}

bool Wire::hasShadow() {
	if (getRatsnest()) return false;
	if (getTrace()) return false;
	return m_pen.widthF() != m_shadowPen.widthF();
}

void Wire::cursorKeyEvent(Qt::KeyboardModifiers modifiers)
{
	if (m_dragEnd || m_dragCurve) return;

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);;
	if (infoGraphicsView) {
		QPoint p = infoGraphicsView->mapFromGlobal(QCursor::pos());
		QPointF r = infoGraphicsView->mapToScene(p);
		// DebugDialog::debug(QString("got key event %1").arg(keyEvent->modifiers()));
		updateCursor(modifiers);
	}
}

void Wire::updateCursor(Qt::KeyboardModifiers modifiers)
{
	if (m_connectorHover) {
		return;
	}

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	bool segment = false;
	int totalConnections = 0;
	foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
		totalConnections += connectorItem->connectionsCount();
	}
	if (totalConnections == 2 && modifiers & altOrMetaModifier()) {
		segment = true;
		foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
			if (connectorItem->connectionsCount() != 1) {
				segment = false;
				break;
			}

			ConnectorItem * toConnectorItem = connectorItem->connectedToItems().at(0);
			if (toConnectorItem->attachedToItemType() != ModelPart::Wire) {
				segment = false;
				break;
			}
		}
	}
		
	if (segment) {
		// dragging a segment of wire between bounded by two other wires
		CursorMaster::instance()->addCursor(this, *CursorMaster::RubberbandCursor);
	}
	else if (totalConnections == 0) {
		// only in breadboard view
		CursorMaster::instance()->addCursor(this, *CursorMaster::MoveCursor);
	}
	else if (infoGraphicsView != NULL && infoGraphicsView->curvyWiresIndicated(modifiers)) {
		CursorMaster::instance()->addCursor(this, *CursorMaster::MakeCurveCursor);
	}
	else if (m_displayBendpointCursor) {
		CursorMaster::instance()->addCursor(this, *CursorMaster::NewBendpointCursor);
	}
}

bool Wire::canChainMultiple()
{
	return m_canChainMultiple;
}

ViewLayer::ViewID Wire::useViewIDForPixmap(ViewLayer::ViewID vid, bool) 
{
    if (vid == ViewLayer::BreadboardView) {
        return ViewLayer::IconView;
    }

    return ViewLayer::UnknownView;
}

void Wire::setDisplayBendpointCursor(bool dbc) {
	m_displayBendpointCursor = dbc;
}

bool Wire::banded() {
    return m_banded;
}

void Wire::setBanded(bool banded) {
    m_banded = banded;
	QList<Wire *> chained;
	QList<ConnectorItem *> ends;
	collectChained(chained, ends);
    foreach (Wire * w, chained) {
        w->m_banded = banded;
        w->update();
    }
}

void Wire::setBandedProp(bool banded) {
   	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->setProp(this, "banded", ItemBase::TranslatedPropertyNames.value("banded"), m_banded ? "Yes" : "No", banded  ? "Yes" : "No", true);
    }
}

void Wire::setProp(const QString & prop, const QString & value) {
	if (prop.compare("banded", Qt::CaseInsensitive) == 0) {
		setBanded(value == "Yes");
		return;
	}

	ItemBase::setProp(prop, value);
}

QColor Wire::colorForLength() {
    // No point in recoloring bent breadboard wires
    if (m_bezier) {
        return color();
    }
    qreal length = m_line.length();
    qint32 wireLength = qRound(.111 * length); // I have no idea what units this is in
    if (wireLength < 1) {
        wireLength = 1;
    }
    wireLength -= 1;
    if (wireLength < lengthColorTrans.count()) {
        return lengthColorTrans[wireLength];
    }

    return color();
}

bool Wire::coloredByLength() {
    return m_colorByLength;
}

void Wire::colorByLength(bool colorByLength) {
    m_colorByLength = colorByLength;
    update();
}

