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

$Revision: 6998 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-28 13:51:10 +0200 (So, 28. Apr 2013) $

********************************************************************/

/*

rubberBand TODO:

	* show connectors under

	* undo moves & connections

	* adjust position so that connectorItem is in the center of connected-to connectorItem

	* save and load

	* alt/meta/ctrl to drag out a wire

	* rubberBand drag

	* update connections needs to be smarter (has to to with connecting to wires)
		look again at attachedMoved()

	* arrow key moves

	* hover: trigger the usual part hover highlight

	* drag selection should work as normal

	* rubberBand drag when part is stretched between two or more parts, some not being dragged correctly

	* if a part is locked, disable dragging the leg

	* fzp  has "legId" (and someday? "maxlength" or is that in the svg) in <p> element
		put the leg definition as a line in the svg, with connectorNleg
		then on loading, remove the leg, and change the viewbox height and width
			this is tricky.  Better if the leg extends outside the viewbox, then can easily tell which end is draggable
		then draw the leg as now

	* figure out how to make the connector longer or its clickable area bigger, or if you click on the wire within a few pixels..
		since it's easy to grab, no need for some kind of fast disconnect

	* hover color makes a mess when dragging leg

	* put legItem back into connector item

	* export: retrieve svg must remove the rubberBand <line> element
	
	* make it a polygon instead of a line

	* what to do when line length is zero

	* renderToSVG: make sure sceneBoundingRect is including legs

	* delete/undo delete

	* clean up pixel turds

	* rotate/flip
		do not disconnect
		should transform around center of the itemBase with no legs

	* leg cursor feedback

	* move behavior: what to do when dragging a leg: bendpoints

	* complex bent leg fails after 2nd rotate

	* rotate/flip undo failure

	* when itembase is rotated leg or bendpoint drag behavior is screwed up

	* rotate target is not correct

	* bendpoint redo (after adding multiple bendpoints) is failing

	* subclass leg connectoritem?  not for the moment

	* remove bendpoint: right click, double click

	* bendpoints: shift-90 degree?

	* copy/paste
		connected and not

	* bad crash when swapping back to unrubberBand.  probably some kind of boundingRect issue...

	* crash: swappable, swappable, undo, redo

	* swapping parts with rubberBand legs, can assume pins will always line up (unless legs can have diffent max distances)
		* no-no
		* no-yes
		* yes-no
		* yes-yes
			
	* rotate rubberBand, swap rubberBand, undo: crash of the item being undone.  it's a prepareGeometryChange() bug

	* swapping when original is rotated

	crash swapping 3v battery for 4.8 when 3v is rotated 45
					
	* click selection behavior should be as if selecting the part
		click on leg should select part

	* update bug when a rubberBand part has all legs connected and the part is dragged
		within a particular region, the part body stops updating--
		but the legs follow the phantom part until the part jumps into position

	* export: resistors and other custom generated parts with legs (retrieve svg)

	curve: undo/redo
		at mouse release curve is killed

	* curve: save/load

	* curve: copy/paste
		
	* curve:export
		
	* curve: make straight function

	* curve: fix connector indicator

	* curve: fix connector click region

	* curve: connector region is not following when dragging connector
	
	when dragging to breadboard from parts bin, don't get final alignment to breadboard

	survival in parts editor

	swapping: keep bends?
	
	bendpoints: align to grid? 

	resistor: leg y-coordinate is slightly off 
	
	parts to modify
		** LEDs (obsolete 5 colors and 15 SMD versions)
			maintain color when switching from obsolete
		** RGB LEDs
		** resistors
		** tantalum caps
		** electrolytic caps
		** ceramic caps
		** diodes
		** tilt sensor
		** temperature sensor
		** light sensor
		** reed switch
		** 2aa battery
		** 4aaa battery
		** 9v battery
		** stepper motors
		** servo
		** dc motor
		** piezo
		** loudspeaker
		** mic
		** solenoid
		** peltier
		** distance sensor
		** transistors
		** FETs
		** voltage regulator
		** resonator
		** inductor
		** crystal

		<path
			style="fill:none;fill-rule:evenodd;stroke:#c8ab37;stroke-width:3.54330707;stroke-linecap:round;stroke-linejoin:miter;stroke-miterlimit:4;stroke-dasharray:none;stroke-opacity:1"
			d="M 256.20728,390.8502 C 256.20728,390.8502 256.20728,381.58878 256.20728,381.58878"
			id="FourWire3End" />


-------------------------------------------------

rubberBand drag with snap-disconnect after a certain length is reached

parts editor support

*/

///////////////////////////////////////////////////////

#include "connectoritem.h"

#include <QBrush>
#include <QPen>
#include <QColor>
#include <limits>
#include <QSet>
#include <QToolTip>
#include <QBitmap>
#include <QApplication>
#include <qmath.h>

#include "../sketch/infographicsview.h"
#include "../debugdialog.h"
#include "bus.h"
#include "../items/wire.h"
#include "../items/virtualwire.h"
#include "../model/modelpart.h"
#include "../utils/graphicsutils.h"
#include "../utils/graphutils.h"
#include "../utils/textutils.h"
#include "../utils/ratsnestcolors.h"
#include "../utils/bezier.h"
#include "../utils/bezierdisplay.h"
#include "../utils/cursormaster.h"
#include "ercdata.h"

/////////////////////////////////////////////////////////

static Bezier UndoBezier;
static BezierDisplay * TheBezierDisplay = NULL;

static const double StandardLegConnectorDrawEnabledLength = 5;		// pixels
static const double StandardLegConnectorDetectLength = 9;			// pixels

QList<ConnectorItem *> ConnectorItem::m_equalPotentialDisplayItems;

const QList<ConnectorItem *> ConnectorItem::emptyConnectorItemList;

static double MAX_DOUBLE = std::numeric_limits<double>::max();

bool wireLessThan(ConnectorItem * c1, ConnectorItem * c2)
{
	if (c1->connectorType() == c2->connectorType()) {
		// if they're the same type return the topmost
		return c1->zValue() > c2->zValue();
	}
	if (c1->connectorType() == Connector::Female) {
		// choose the female first
		return true;
	}
	if (c2->connectorType() == Connector::Female) {
		// choose the female first
		return false;
	}
	if (c1->connectorType() == Connector::Male) {
		// choose the male first
		return true;
	}
	if (c2->connectorType() == Connector::Male) {
		// choose the male first
		return false;
	}
	if (c1->connectorType() == Connector::Pad) {
		// choose the pad first
		return true;
	}
	if (c2->connectorType() == Connector::Pad) {
		// choose the pad first
		return false;
	}

	// Connector::Wire last
	return c1->zValue() > c2->zValue();
}

QColor addColor(QColor & color, int offset)
{
    QColor rgb = color.toRgb();
	rgb.setRgb(qMax(0, qMin(rgb.red() + offset, 255)), qMax(0, qMin(rgb.green() + offset, 255)),qMax(0, qMin(rgb.blue() + offset, 255)));

    // convert back to same color spec as original color
    return rgb.convertTo(color.spec());
}

/////////////////////////////////////////////////////////////

ConnectorItemAction::ConnectorItemAction(QAction * action) : QAction(action) {
	m_connectorItem = NULL;
	this->setText(action->text());
	this->setStatusTip(action->statusTip());
	this->setCheckable(action->isCheckable());
}

ConnectorItemAction::ConnectorItemAction(const QString & title, QObject * parent) : QAction(title, parent) {
	m_connectorItem = NULL;
}

void ConnectorItemAction::setConnectorItem(ConnectorItem * c) {
	m_connectorItem = c;
}

ConnectorItem * ConnectorItemAction::connectorItem() {
	return m_connectorItem;
}

/////////////////////////////////////////////////////////

ConnectorItem::ConnectorItem( Connector * connector, ItemBase * attachedTo )
	: NonConnectorItem(attachedTo)
{
	// initialize m_connectorT, otherwise will trigger qWarning("QLine::unitVector: New line does not have unit length");
	// TODO: figure out why paint is being called with m_connectorT not initialized
	m_groundFillSeed = false;
	m_connectorDetectT = m_connectorDrawT = 0;
	m_draggingCurve = m_draggingLeg = m_rubberBandLeg = m_bigDot = m_hybrid = false;
	m_hoverEnterSpaceBarWasPressed = m_spaceBarWasPressed = false;
	m_overConnectorItem = NULL;
	m_connectorHovering = false;
	m_connector = connector;
	if (connector != NULL) {
		connector->addViewItem(this);
	}
    setAcceptHoverEvents(true);
    this->setCursor((attachedTo && attachedTo->itemType() == ModelPart::Wire) ? *CursorMaster::BendpointCursor : *CursorMaster::MakeWireCursor);

	//DebugDialog::debug(QString("%1 attached to %2")
			//.arg(this->connector()->connectorShared()->id())
			//.arg(attachedTo->modelPartShared()->title()) );
}

ConnectorItem::~ConnectorItem() {
	m_equalPotentialDisplayItems.removeOne(this);
	//DebugDialog::debug(QString("deleting connectorItem %1").arg((long) this, 0, 16));
	foreach (ConnectorItem * connectorItem, m_connectedTo) {
		if (connectorItem != NULL) {
			//DebugDialog::debug(QString("temp remove %1 %2").arg(this->attachedToID()).arg(connectorItem->attachedToID()));
			connectorItem->tempRemove(this, this->attachedToID() != connectorItem->attachedToID());
		}
	}
	if (this->connector() != NULL) {
		this->connector()->removeViewItem(this);
	}
	clearCurves();
}

void ConnectorItem::hoverEnterEvent ( QGraphicsSceneHoverEvent * event ) {

    //debugInfo("connector hoverEnter");
	/*
	QRectF sbr = this->sceneBoundingRect();
	QPointF p = event->scenePos();

	debugInfo(QString("hover %1, %2 %3 %4 %5, %6 %7")
		.arg((long) this, 0, 16)
		.arg(sbr.left())
		.arg(sbr.top())
		.arg(sbr.width())
		.arg(sbr.height())
		.arg(p.x())
		.arg(p.y())
		);
	*/

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL && infoGraphicsView->spaceBarIsPressed()) {
		m_hoverEnterSpaceBarWasPressed = true;
		event->ignore();
		return;
	}

	//DebugDialog::debug("---CI set override cursor");
	CursorMaster::instance()->addCursor(this, cursor());
	bool setDefaultCursor = true;
	m_hoverEnterSpaceBarWasPressed = false;
	setHoverColor();
	if (infoGraphicsView != NULL) {
		infoGraphicsView->hoverEnterConnectorItem(event, this);
		if (m_rubberBandLeg) {
			updateLegCursor(event->pos(), event->modifiers());
			setDefaultCursor = false;
		}
	}
	if (this->m_attachedTo != NULL) {
		if (this->attachedToItemType() == ModelPart::Wire) {
			updateWireCursor(event->modifiers());
			setDefaultCursor = false;
		}
		m_attachedTo->hoverEnterConnectorItem(event, this);
	}

	if (setDefaultCursor) CursorMaster::instance()->addCursor(this, *CursorMaster::MakeWireCursor);
}

void ConnectorItem::hoverLeaveEvent ( QGraphicsSceneHoverEvent * event ) {
	if (m_hoverEnterSpaceBarWasPressed) {
		event->ignore();
		return;
	}

    QList<ConnectorItem *> visited;
	restoreColor(visited);
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->hoverLeaveConnectorItem(event, this);
	}

	CursorMaster::instance()->removeCursor(this);

	if (this->m_attachedTo != NULL) {
		m_attachedTo->hoverLeaveConnectorItem(event, this);
	}

	//DebugDialog::debug("------CI restore override cursor");
	CursorMaster::instance()->removeCursor(this);
}

void ConnectorItem::hoverMoveEvent ( QGraphicsSceneHoverEvent * event ) {
	if (m_hoverEnterSpaceBarWasPressed) {
		event->ignore();
		return;
	}

	if (this->m_attachedTo != NULL) {
		m_attachedTo->hoverMoveConnectorItem(event, this);
	}

	if (m_rubberBandLeg) {
		updateLegCursor(event->pos(), event->modifiers());
	}
}

Connector * ConnectorItem::connector() {
	return m_connector;
}

void ConnectorItem::clearConnectorHover() {
	m_connectorHovering = false;
}

void ConnectorItem::connectorHover(ItemBase * itemBase, bool hovering) {
	m_connectorHovering = hovering;
	if (hovering) {
		setHoverColor();			// could make this light up buses as well
	}
	else {
        QList<ConnectorItem *> visited;
		restoreColor(visited);
	}
	if (this->m_attachedTo != NULL) {
		m_attachedTo->connectorHover(this, itemBase, hovering);
	}
}

bool ConnectorItem::connectorHovering() {
	return m_connectorHovering;
}

void ConnectorItem::connectTo(ConnectorItem * connected) {
	if (m_connectedTo.contains(connected)) return;

	m_connectedTo.append(connected);
	//DebugDialog::debug(QString("connect to cc:%4 this:%1 to:%2 %3").arg((long) this, 0, 16).arg((long) connected, 0, 16).arg(connected->attachedTo()->modelPartShared()->title()).arg(m_connectedTo.count()) );
    QList<ConnectorItem *> visited;
	restoreColor(visited);
	if (m_attachedTo != NULL) {
		m_attachedTo->connectionChange(this, connected, true);
	}
}

ConnectorItem * ConnectorItem::removeConnection(ItemBase * itemBase) {
    QList<ConnectorItem *> visited;
	for (int i = 0; i < m_connectedTo.count(); i++) {
		if (m_connectedTo[i]->attachedTo() == itemBase) {
			ConnectorItem * removed = m_connectedTo[i];
			m_connectedTo.removeAt(i);
			if (m_attachedTo != NULL) {
				m_attachedTo->connectionChange(this, removed, false);
			}
			restoreColor(visited);
			DebugDialog::debug(QString("remove from:%1 to:%2 count%3")
				.arg((long) this, 0, 16)
				.arg(itemBase->modelPartShared()->title())
				.arg(m_connectedTo.count()) );
			return removed;
		}
	}

	return NULL;
}

void ConnectorItem::removeConnection(ConnectorItem * connectedItem, bool emitChange) {
	if (connectedItem == NULL) return;

	m_connectedTo.removeOne(connectedItem);
    QList<ConnectorItem *> visited;
	restoreColor(visited);
	if (emitChange) {
		m_attachedTo->connectionChange(this, connectedItem, false);
	}
}

void ConnectorItem::tempConnectTo(ConnectorItem * item, bool applyColor) {
	if (!m_connectedTo.contains(item)) m_connectedTo.append(item);

	if(applyColor) {
        QList<ConnectorItem *> visited;
        restoreColor(visited);
    }
}

void ConnectorItem::tempRemove(ConnectorItem * item, bool applyColor) {
	m_connectedTo.removeOne(item);

	if(applyColor) {    
        QList<ConnectorItem *> visited;
        restoreColor(visited);
    }
}

void ConnectorItem::restoreColor(QList<ConnectorItem *> & visited) 
{
    if (visited.contains(this)) return;
    visited.append(this);

	if (!attachedTo()->isEverVisible()) return;

    QList<ConnectorItem *> connectorItems;
    connectorItems.append(this);
    collectEqualPotential(connectorItems, true, getSkipFlags());
    visited.append(connectorItems);
    QSet<ItemBase *> attachedTo;
    foreach (ConnectorItem * connectorItem, connectorItems) {
        if (connectorItem->isEverVisible()) {
            if (connectorItem->attachedToItemType() != ModelPart::Wire) {
                attachedTo.insert(connectorItem->attachedTo()->layerKinChief());
            }
        }
    }

    foreach (ConnectorItem * connectorItem, connectorItems) {
        if (connectorItem->isEverVisible()) {
	        //QString how;
	        if (attachedTo.count() <= 1) {
		        if (connectorItem->connectorType() == Connector::Female) {
                    if (connectorItem->connectionsCount() > 0) connectorItem->setUnconnectedColor();
			        else connectorItem->setNormalColor();
			        //how = "normal";	
		        }
		        else {
			        connectorItem->setUnconnectedColor();
			        //how = "unconnected";
		        }
	        }
	        else {
		        connectorItem->setConnectedColor();
		        //how = "connected";
	        }
        }
    }


	/*
	DebugDialog::debug(QString("restore color dobus:%1 bccount:%2 docross:%3 cid:'%4' '%5' id:%6 '%7' vid:%8 vlid:%9 %10")
		.arg(doBuses)
		.arg(busConnectionCount)
		.arg(doCross)
		.arg(this->connectorSharedID())
		.arg(this->connectorSharedName())
		.arg(this->attachedToID())
		.arg(this->attachedToInstanceTitle())
		.arg(this->attachedToViewID())
		.arg(this->attachedToViewLayerID())
		.arg(how)
	);

	*/
}

void ConnectorItem::setConnectedColor() {
	if (m_attachedTo == NULL) return;

	QBrush brush;
	QPen pen;
	m_attachedTo->getConnectedColor(this, brush, pen, m_opacity, m_negativePenWidth, m_negativeOffsetRect);
	//DebugDialog::debug(QString("set connected %1 %2").arg(attachedToID()).arg(pen->width()));
	setColorAux(brush, pen, true);
}

void ConnectorItem::setNormalColor() {
	if (m_attachedTo == NULL) return;

	QBrush brush;
	QPen pen;
	m_attachedTo->getNormalColor(this, brush, pen, m_opacity, m_negativePenWidth, m_negativeOffsetRect);
	//DebugDialog::debug(QString("set normal %1 %2").arg(attachedToID()).arg(pen->width()));
	setColorAux(brush, pen, false);
}

void ConnectorItem::setUnconnectedColor() {
	if (m_attachedTo == NULL) return;

	QBrush brush;
	QPen pen;
	//DebugDialog::debug(QString("set unconnected %1").arg(attachedToID()) );
	m_attachedTo->getUnconnectedColor(this, brush, pen, m_opacity, m_negativePenWidth, m_negativeOffsetRect);
	setColorAux(brush, pen, true);
}

void ConnectorItem::setHoverColor() {
	if (m_attachedTo == NULL) return;

	QBrush brush;
	QPen pen;
	m_attachedTo->getHoverColor(this, brush, pen, m_opacity, m_negativePenWidth, m_negativeOffsetRect);
	setColorAux(brush, pen, true);
}

void ConnectorItem::setColorAux(const QBrush & brush, const QPen & pen, bool paint) {
    //debugInfo(QString("setColorAux %1 %2").arg(brush.color().name()).arg(pen.color().name()));
	m_paint = paint;
	this->setBrush(brush);
	this->setPen(pen);
	update();
}

void ConnectorItem::setColorAux(const QColor &color, bool paint) {
    setColorAux(QBrush(color), QPen(color), paint);
}

void ConnectorItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {

	//DebugDialog::debug("in connectorItem mouseReleaseEvent");
	clearEqualPotentialDisplay();

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);

	if (m_rubberBandLeg && m_draggingLeg) {
		m_draggingLeg = false;

		if (m_insertBendpointPossible) {
			// didn't move far enough; bail
			return;
		}

		if (m_moveCount == 0) { 
			// never moved
			return;
		}

		ConnectorItem * to = releaseDrag();

		if (m_draggingCurve) {
			m_draggingCurve = false;
			if (TheBezierDisplay) {
				delete TheBezierDisplay;
				TheBezierDisplay = NULL;
			}

			if (infoGraphicsView != NULL) {
				infoGraphicsView->prepLegCurveChange(this, m_draggingLegIndex, &UndoBezier, m_legCurves.at(m_draggingLegIndex), false);
			}

			return;
		}

		if (m_oldPolygon.count() < m_legPolygon.count()) {
			// we inserted a bendpoint
			InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
			if (infoGraphicsView != NULL) {
				infoGraphicsView->prepLegBendpointChange(this, m_oldPolygon.count(), m_legPolygon.count(), m_draggingLegIndex, m_legPolygon.at(m_draggingLegIndex), 
									&UndoBezier, m_legCurves.at(m_draggingLegIndex -1), m_legCurves.at(m_draggingLegIndex), false);
			}
			return;
		}

		bool changeConnections = m_draggingLegIndex == m_legPolygon.count() - 1;
		if (to != NULL && changeConnections) {
			// center endpoint in the target connectorItem
			reposition(to->sceneAdjustedTerminalPoint(NULL), m_draggingLegIndex);
		}
		if (infoGraphicsView != NULL) {
			infoGraphicsView->prepLegBendpointMove(this, m_draggingLegIndex, mapToScene(m_oldPolygon.at(m_draggingLegIndex)), mapToScene(m_legPolygon.at(m_draggingLegIndex)), to, changeConnections);
		}
		return;
	}

	if (this->m_attachedTo != NULL && m_attachedTo->acceptsMouseReleaseConnectorEvent(this, event)) {
		m_attachedTo->mouseReleaseConnectorEvent(this, event);
		return;
	}

	QGraphicsRectItem::mouseReleaseEvent(event);
}

void ConnectorItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
	if (m_rubberBandLeg) {
		int bendpointIndex;
		CursorLocation cursorLocation = findLocation(event->pos(), bendpointIndex);
		switch (cursorLocation) {
			case InBendpoint:
				if (bendpointIndex < m_legPolygon.count() - 1) {
					removeBendpoint(bendpointIndex);
				}
				break;
			case InSegment:
				insertBendpoint(event->pos(), bendpointIndex);
				break;
			default:
				break;
		}

		return;
	}

	if (this->m_attachedTo != NULL && m_attachedTo->acceptsMouseDoubleClickConnectorEvent(this, event)) {
		m_attachedTo->mouseDoubleClickConnectorEvent(this);
		return;
	}

	QGraphicsRectItem::mouseDoubleClickEvent(event);
}

void ConnectorItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
	m_moveCount++;
	if (m_rubberBandLeg && m_draggingLeg) {
		if (m_draggingCurve) {
			Bezier * bezier = m_legCurves.at(m_draggingLegIndex);
			if (bezier != NULL && !bezier->isEmpty()) {
				prepareGeometryChange();
				bezier->recalc(event->pos());
				calcConnectorEnd();
				update();
				if (TheBezierDisplay) TheBezierDisplay->updateDisplay(this, bezier);
				return;
			}
		}

		QPointF currentPos = event->scenePos();
        QPointF buttonDownPos = event->buttonDownScenePos(Qt::LeftButton);

		if (m_insertBendpointPossible) {
			if (qSqrt(GraphicsUtils::distanceSqd(currentPos, buttonDownPos)) >= QApplication::startDragDistance()) {
				insertBendpointAux(m_holdPos, m_draggingLegIndex);
				m_insertBendpointPossible = false;
			}
			else {
				return;
			}
		}

		if (event->modifiers() & Qt::ShiftModifier && m_draggingLegIndex > 0 && m_draggingLegIndex < m_legPolygon.count() - 1) {
			bool bendpoint = false;
			QPointF initialPos = mapToScene(m_legPolygon.at(m_draggingLegIndex - 1)); 
			QPointF otherInitialPos = mapToScene(m_legPolygon.at(m_draggingLegIndex + 1));
			QPointF p1(initialPos.x(), otherInitialPos.y());
			double d = GraphicsUtils::distanceSqd(p1, currentPos);
			if (d <= 144) {
				bendpoint = true;
				currentPos = p1;
			}
			else {
				p1.setX(otherInitialPos.x());
				p1.setY(initialPos.y());
				d = GraphicsUtils::distanceSqd(p1, currentPos);
				if (d <= 144) {
					bendpoint = true;
					currentPos = p1;
				}				
			}

			if (!bendpoint) {
				currentPos = GraphicsUtils::calcConstraint(initialPos, currentPos);
			}
		}

		reposition(m_holdPos + currentPos - buttonDownPos, m_draggingLegIndex);

		QList<ConnectorItem *> exclude;
		findConnectorUnder(true, true, exclude, true, this);

		return;
	}

	if (this->m_attachedTo != NULL && m_attachedTo->acceptsMouseMoveConnectorEvent(this, event)) {
		m_attachedTo->mouseMoveConnectorEvent(this, event);
		return;
	}

	QGraphicsRectItem::mouseMoveEvent(event);
}

void ConnectorItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
	m_draggingCurve = m_draggingLeg = false;
	m_moveCount = 0;

	if (event->button() != Qt::LeftButton) {
		QGraphicsRectItem::mousePressEvent(event);
		return;
	}

	if (m_attachedTo->filterMousePressConnectorEvent(this, event)) {
		event->ignore();
		return;
	}

	clearEqualPotentialDisplay();

	InfoGraphicsView *infographics = InfoGraphicsView::getInfoGraphicsView(this);
	if (infographics != NULL && infographics->spaceBarIsPressed()) {
		event->ignore();
		return;
	}

	m_equalPotentialDisplayItems.append(this);
	collectEqualPotential(m_equalPotentialDisplayItems, true, ViewGeometry::NoFlag);
	//m_equalPotentialDisplayItems.removeAt(0);									// not sure whether to leave the clicked one in or out of the list
	QList<ConnectorItem *> visited;
    //DebugDialog::debug("_______________________");
    foreach (ConnectorItem * connectorItem, m_equalPotentialDisplayItems) {
		connectorItem->showEqualPotential(true, visited);
        //connectorItem->debugInfo("display eqp");
	}
	
	if (m_rubberBandLeg && this->m_attachedTo != NULL && m_attachedTo->acceptsMousePressLegEvent(this, event)) {
		if (legMousePressEvent(event)) return;
	}

	if (this->m_attachedTo != NULL && m_attachedTo->acceptsMousePressConnectorEvent(this, event)) {
		m_attachedTo->mousePressConnectorEvent(this, event);
		return;
	}

	QGraphicsRectItem::mousePressEvent(event);
}

int ConnectorItem::connectionsCount() {
	return m_connectedTo.count();
}

void ConnectorItem::attachedMoved(bool includeRatsnest, QList<ConnectorItem *> & already) {
	//DebugDialog::debug("attached moved");
    if (!this->isVisible()) return;

    QSet<ConnectorItem *> allTo;
    allTo.insert(this);
	foreach (ConnectorItem * toConnectorItem, m_connectedTo) {
        allTo.insert(toConnectorItem);
        foreach (ConnectorItem * subTo, toConnectorItem->connectedToItems()) {
            allTo.insert(subTo);
        }
	}
    allTo.remove(this);
    foreach (ConnectorItem * toConnectorItem, allTo) {
        ItemBase * itemBase = toConnectorItem->attachedTo();
		if (itemBase == NULL) continue;
		if (!itemBase->isVisible()) {
			//this->debugInfo("continue");
			//itemBase->debugInfo("    ");
			continue;
		}
        if (itemBase->getRatsnest() && !includeRatsnest) {
            continue;
        }

        toConnectorItem->attachedTo()->connectedMoved(this, toConnectorItem, already);
    }
}

ConnectorItem * ConnectorItem::firstConnectedToIsh() {
	if (m_connectedTo.count() <= 0) return NULL;

	foreach (ConnectorItem * connectorItem, m_connectedTo) {
		if (connectorItem->attachedTo()->getRatsnest()) continue;	
		if (!connectorItem->isVisible()) continue;

        return connectorItem;
	}

	// TODO: not sure whether to return invisible connectors
	// TODO: get rid of this function?

	foreach (ConnectorItem * connectorItem, m_connectedTo) {
		if (connectorItem->attachedTo()->getRatsnest()) continue;
		
		return connectorItem;
	}

	return NULL;
}

void ConnectorItem::setTerminalPoint(QPointF p) {
	m_terminalPoint = p;
}

QPointF ConnectorItem::terminalPoint() {
	return m_terminalPoint;
}

QPointF ConnectorItem::adjustedTerminalPoint() {
	if (m_legPolygon.count() < 2) {
		return m_terminalPoint + this->rect().topLeft();
	}

	return m_legPolygon.last();
}

QPointF ConnectorItem::sceneAdjustedTerminalPoint(ConnectorItem * connectee) {

	if ((connectee != NULL) && !m_circular && !m_shape.isEmpty() && (connectee->attachedToItemType() == ModelPart::Wire)) {
		Wire * wire = qobject_cast<Wire *>(connectee->attachedTo());
		if ((wire != NULL) && !wire->getRatsnest()) {
			QPointF anchor = wire->otherConnector(connectee)->sceneAdjustedTerminalPoint(NULL);
			double newX = 0, newY = 0, newDistance = MAX_DOUBLE;
			int count = m_shape.elementCount();

			QPointF prev;
			for (int i = 0; i < count; i++) {
				QPainterPath::Element el = m_shape.elementAt(i);
				if (el.isMoveTo()) {
					prev = this->mapToScene(QPointF(el));
				}
				else {
					QPointF current = this->mapToScene(QPointF(el));
					double candidateX, candidateY, candidateDistance;
					bool atEndpoint;
					GraphicsUtils::distanceFromLine(anchor.x(), anchor.y(), prev.x(), prev.y(), current.x(), current.y(), 
										candidateX, candidateY, candidateDistance, atEndpoint);
					if (candidateDistance < newDistance) {
						newX = candidateX;
						newY = candidateY;
						newDistance = candidateDistance;
						//DebugDialog::debug(QString("anchor:%1,%2; new:%3,%4; %5").arg(anchor.x()).arg(anchor.y()).arg(newX).arg(newY).arg(newDistance));
					}

					prev = current;
				}
			}

			//DebugDialog::debug(QString("anchor:%1,%2; new:%3,%4; %5\n\n").arg(anchor.x()).arg(anchor.y()).arg(newX).arg(newY).arg(newDistance));
			return QPointF(newX, newY);
		}
	}

	return this->mapToScene(adjustedTerminalPoint());
}

bool ConnectorItem::connectedTo(ConnectorItem * connectorItem) {
	return this->m_connectedTo.contains(connectorItem);
}

const QList< QPointer<ConnectorItem> > & ConnectorItem::connectedToItems() {
	return m_connectedTo;
}

void ConnectorItem::setHidden(bool hide) {
	m_hidden = hide;

	setHiddenOrInactive();
}

void ConnectorItem::setHybrid(bool h) {
	m_hybrid = h;
	setHiddenOrInactive();
}

bool ConnectorItem::isHybrid() {
	return m_hybrid;
}


void ConnectorItem::setBigDot(bool bd) {
	m_bigDot = bd;
	//if (bd) {
	//	this->debugInfo("big dot");
	//}
}

bool ConnectorItem::isBigDot() {
	return m_bigDot;
}

void ConnectorItem::setInactive(bool inactivate) {
	m_inactive = inactivate;
	setHiddenOrInactive();
}

void ConnectorItem::setHiddenOrInactive() {
	if (m_hidden || m_inactive || m_hybrid || m_layerHidden) {
		this->setAcceptedMouseButtons(Qt::NoButton);
		this->unsetCursor();
		setAcceptHoverEvents(false);
	}
	else {
		this->setAcceptedMouseButtons(ALLMOUSEBUTTONS);
		this->setCursor(attachedToItemType() == ModelPart::Wire ? *CursorMaster::BendpointCursor : *CursorMaster::MakeWireCursor);
		setAcceptHoverEvents(true);
	}
	this->update();
}

ConnectorItem * ConnectorItem::overConnectorItem() {
	return m_overConnectorItem;
}

void ConnectorItem::setOverConnectorItem(ConnectorItem * connectorItem) {
	m_overConnectorItem = connectorItem;
}


const QString & ConnectorItem::connectorSharedID() {
	if (m_connector == NULL) return ___emptyString___;

	return m_connector->connectorSharedID();
}

const QString & ConnectorItem::connectorSharedReplacedby() {
	if (m_connector == NULL) return ___emptyString___;

	return m_connector->connectorSharedReplacedby();
}

ErcData * ConnectorItem::connectorSharedErcData() {
	if (m_connector == NULL) return NULL;

	return m_connector->connectorSharedErcData();
}

const QString & ConnectorItem::connectorSharedName() {
	if (m_connector == NULL) return ___emptyString___;

	return m_connector->connectorSharedName();
}

const QString & ConnectorItem::connectorSharedDescription() {
	if (m_connector == NULL) return ___emptyString___;

	return m_connector->connectorSharedDescription();
}

const QString & ConnectorItem::busID() {
	if (m_connector == NULL) return ___emptyString___;

	return m_connector->busID();
}

ModelPartShared * ConnectorItem::modelPartShared() {
	if (m_attachedTo == NULL) return NULL;

	return m_attachedTo->modelPartShared();
}

ModelPart * ConnectorItem::modelPart() {
	if (m_attachedTo == NULL) return NULL;

	return m_attachedTo->modelPart();
}

Bus * ConnectorItem::bus() {
	if (m_connector == NULL) return NULL;

	return m_connector->bus();
}

ViewLayer::ViewLayerID ConnectorItem::attachedToViewLayerID() {
	if (m_attachedTo == NULL) return ViewLayer::UnknownLayer;

	return m_attachedTo->viewLayerID();
}

ViewLayer::ViewLayerPlacement ConnectorItem::attachedToViewLayerPlacement() {
	if (m_attachedTo == NULL) return ViewLayer::UnknownPlacement;

	return m_attachedTo->viewLayerPlacement();
}

ViewLayer::ViewID ConnectorItem::attachedToViewID() {
	if (m_attachedTo == NULL) return ViewLayer::UnknownView;

	return m_attachedTo->viewID();
}

Connector::ConnectorType ConnectorItem::connectorType() {
	if (m_connector == NULL) return Connector::Unknown;

	return m_connector->connectorType();
}

bool ConnectorItem::chained() {
	foreach (ConnectorItem * toConnectorItem, m_connectedTo) {
		if (toConnectorItem->attachedToItemType() == ModelPart::Wire) {
			return true;
		}
	}

	return false;
}

void ConnectorItem::saveInstance(QXmlStreamWriter & writer) {
	if (m_connectedTo.count() <= 0 && !m_rubberBandLeg && !m_groundFillSeed) {
		// no need to save if there's no connection
		return;
	}

	writer.writeStartElement("connector");
	writer.writeAttribute("connectorId", connectorSharedID());
	writer.writeAttribute("layer", ViewLayer::viewLayerXmlNameFromID(attachedToViewLayerID()));
	if (m_groundFillSeed) {
		writer.writeAttribute("groundFillSeed", "true");
	}

	writer.writeStartElement("geometry");
	QPointF p = this->pos();
	writer.writeAttribute("x", QString::number(p.x()));
	writer.writeAttribute("y", QString::number(p.y()));
	writer.writeEndElement();

	if (m_rubberBandLeg && m_legPolygon.count() > 1) {
		writer.writeStartElement("leg");
		for (int i = 0; i < m_legPolygon.count(); i++) {
			QPointF p = m_legPolygon.at(i);
			writer.writeStartElement("point");
			writer.writeAttribute("x", QString::number(p.x()));
			writer.writeAttribute("y", QString::number(p.y()));
			writer.writeEndElement();

			Bezier * bezier = m_legCurves.at(i);
			if (bezier == NULL) {
				writer.writeStartElement("bezier");
				writer.writeEndElement();
			}
			else {
				bezier->write(writer);
			}
		}
		writer.writeEndElement();
	}

	if (m_connectedTo.count() > 0) {
		writer.writeStartElement("connects");
		foreach (ConnectorItem * connectorItem, this->m_connectedTo) {
			if (connectorItem->attachedTo()->getRatsnest()) continue;

			connectorItem->writeConnector(writer, "connect");
		}
		writer.writeEndElement();
	}

	writeOtherElements(writer);

	writer.writeEndElement();
}


void ConnectorItem::writeConnector(QXmlStreamWriter & writer, const QString & elementName)
{
	//DebugDialog::debug(QString("write connector %1").arg(this->attachedToID()));
	writer.writeStartElement(elementName);
	writer.writeAttribute("connectorId", connectorSharedID());
	writer.writeAttribute("modelIndex", QString::number(connector()->modelIndex()));
	writer.writeAttribute("layer", ViewLayer::viewLayerXmlNameFromID(attachedToViewLayerID()));
	writer.writeEndElement();
}

void ConnectorItem::writeOtherElements(QXmlStreamWriter & writer) {
	Q_UNUSED(writer);
}

bool ConnectorItem::wiredTo(ConnectorItem * target, ViewGeometry::WireFlags skipFlags) {
	QList<ConnectorItem *> connectorItems;
	connectorItems.append(this);
	collectEqualPotential(connectorItems, true, skipFlags);
	return connectorItems.contains(target);
}

Wire * ConnectorItem::directlyWiredTo(ConnectorItem * source, ConnectorItem * target, ViewGeometry::WireFlags flags) {
	 QList<ConnectorItem *> visited;
	 return directlyWiredToAux(source, target, flags, visited);
}

Wire * ConnectorItem::directlyWiredToAux(ConnectorItem * source, ConnectorItem * target, ViewGeometry::WireFlags flags, QList<ConnectorItem *> & visited) {
	if (visited.contains(source)) return NULL;

	QList<ConnectorItem *> equals;
	equals << source;

	ConnectorItem * cross = source->getCrossLayerConnectorItem();
	if (cross) {
		if (!visited.contains(cross)) {
			equals << cross;
		}
	}

	visited.append(equals);

	foreach (ConnectorItem * fromItem, equals) {
		foreach (ConnectorItem * toConnectorItem, fromItem->m_connectedTo) {
			ItemBase * toItem = toConnectorItem->attachedTo();
			if (toItem == NULL) {
				continue;			// shouldn't happen
			}

			if (toItem->itemType() != ModelPart::Wire) continue;

			Wire * wire = qobject_cast<Wire *>(toItem);
			if (!wire->hasAnyFlag(flags)) continue;

			ConnectorItem * otherEnd = wire->otherConnector(toConnectorItem);
			bool isChained = false;
			foreach (ConnectorItem * otherConnectorItem, otherEnd->m_connectedTo) {
				if (target == otherConnectorItem) {
					return wire;
				}
				if (target->getCrossLayerConnectorItem() == otherConnectorItem) {
					return wire;
				}
				if (otherConnectorItem->attachedToItemType() == ModelPart::Wire) {
					//DebugDialog::debug(QString("wired from %1 to %2").arg(wire->id()).arg(otherConnectorItem->attachedToID()));
					isChained = true;
				}
			}

			if (isChained) {
				if (ConnectorItem::directlyWiredToAux(otherEnd, target, flags, visited)) {
					return wire;
				}
			}
		}
	}

    return NULL;
}

bool ConnectorItem::isConnectedToPart() {

	QList<ConnectorItem *> tempItems;
	tempItems << this;

	ConnectorItem * thisCrossConnectorItem = this->getCrossLayerConnectorItem();
	QList<ConnectorItem *> busConnectedItems;
	Bus * b = bus();
	if (b != NULL) {
		attachedTo()->busConnectorItems(b, this, busConnectedItems);
	}

	for (int i = 0; i < tempItems.count(); i++) {
		ConnectorItem * connectorItem = tempItems[i];

		if ((connectorItem != this) && 
			(connectorItem != thisCrossConnectorItem) && 
			!busConnectedItems.contains(connectorItem)) 
		{
			switch (connectorItem->attachedToItemType()) {
				case ModelPart::Symbol:
				case ModelPart::SchematicSubpart:
				case ModelPart::Jumper:
				case ModelPart::Part:
				case ModelPart::CopperFill:
				case ModelPart::Board:
				case ModelPart::Breadboard:
				case ModelPart::ResizableBoard:
				case ModelPart::Via:
					if (connectorItem->attachedTo()->isEverVisible()) {
						return true;
					}
					break;
				default:
					break;
			}
		}

		ConnectorItem * crossConnectorItem = connectorItem->getCrossLayerConnectorItem();
		if (crossConnectorItem != NULL) {
			if (!tempItems.contains(crossConnectorItem)) {
				tempItems.append(crossConnectorItem);
			}
		}

		foreach (ConnectorItem * cto, connectorItem->connectedToItems()) {
			if (tempItems.contains(cto)) continue;

			tempItems.append(cto);
		}

		Bus * bus = connectorItem->bus();
		if (bus != NULL) {
			QList<ConnectorItem *> busConnectedItems;
			connectorItem->attachedTo()->busConnectorItems(bus, connectorItem, busConnectedItems);
			foreach (ConnectorItem * busConnectedItem, busConnectedItems) {
				if (!tempItems.contains(busConnectedItem)) {
					tempItems.append(busConnectedItem);
				}
			}
		}
	}

	return false;
}

void ConnectorItem::collectEqualPotential(QList<ConnectorItem *> & connectorItems, bool crossLayers, ViewGeometry::WireFlags skipFlags) {
	// collects all the connectors at the same potential
	// allows direct connections or wired connections

	//DebugDialog::debug("__________________");

	QList<ConnectorItem *> tempItems = connectorItems;
	connectorItems.clear();

	for (int i = 0; i < tempItems.count(); i++) {
		ConnectorItem * connectorItem = tempItems[i];
		//connectorItem->debugInfo("testing eqp");

		Wire * fromWire = (connectorItem->attachedToItemType() == ModelPart::Wire) ? qobject_cast<Wire *>(connectorItem->attachedTo()) : NULL;
		if (fromWire != NULL) {
			if (fromWire->hasAnyFlag(skipFlags)) {
				// don't add this kind of wire
				continue;
			}
		}
		else {
			if (crossLayers) {
				ConnectorItem * crossConnectorItem = connectorItem->getCrossLayerConnectorItem();
				if (crossConnectorItem != NULL) {
					if (!tempItems.contains(crossConnectorItem)) {
						tempItems.append(crossConnectorItem);
					}
				}
			}
		}

		// this one's a keeper
		connectorItems.append(connectorItem);
        //connectorItem->debugInfo("collect");

		foreach (ConnectorItem * cto, connectorItem->connectedToItems()) {
			if (tempItems.contains(cto)) continue;

			if ((skipFlags & ViewGeometry::NormalFlag) && (fromWire == NULL) && (cto->attachedToItemType() != ModelPart::Wire)) {
				// direct (part-to-part) connections not allowed
				continue;
			}

			tempItems.append(cto);
		}

		Bus * bus = connectorItem->bus();
		if (bus != NULL) {
			QList<ConnectorItem *> busConnectedItems;
			connectorItem->attachedTo()->busConnectorItems(bus, connectorItem, busConnectedItems);
#ifndef QT_NO_DEBUG
			if (connectorItem->attachedToItemType() == ModelPart::Wire && busConnectedItems.count() != 2) {
				connectorItem->debugInfo("bus is missing");
				//busConnectedItems.clear();
				//connectorItem->attachedTo()->busConnectorItems(bus, busConnectedItems);
			}
#endif
			foreach (ConnectorItem * busConnectedItem, busConnectedItems) {
				if (!tempItems.contains(busConnectedItem)) {
					tempItems.append(busConnectedItem);
				}
			}
		}
	}
}

void ConnectorItem::collectParts(QList<ConnectorItem *> & connectorItems, QList<ConnectorItem *> & partsConnectors, bool includeSymbols, ViewLayer::ViewLayerPlacement viewLayerPlacement)
{
	if (connectorItems.count() == 0) return;

	//DebugDialog::debug("___________________________");
	switch (viewLayerPlacement) {
		case ViewLayer::NewTop:
		case ViewLayer::NewBottom:
		case ViewLayer::NewTopAndBottom:
			break;
		default:
			DebugDialog::debug(QString("collect parts unknown spec %1").arg(viewLayerPlacement));
			viewLayerPlacement = ViewLayer::NewTopAndBottom;
			break;
	}
	
	foreach (ConnectorItem * connectorItem, connectorItems) {
		if (connectorItem->isHybrid()) {
			continue;
		}

		ItemBase * candidate = connectorItem->attachedTo();
		switch (candidate->itemType()) {
			case ModelPart::Symbol:
			case ModelPart::SchematicSubpart:
				if (!includeSymbols) break;
			case ModelPart::Jumper:
			case ModelPart::Part:
			case ModelPart::CopperFill:
			case ModelPart::Board:
			case ModelPart::ResizableBoard:
			case ModelPart::Via:
				collectPart(connectorItem, partsConnectors, viewLayerPlacement);
				break;
			default:
				break;
		}
	}
}

void ConnectorItem::collectPart(ConnectorItem * connectorItem, QList<ConnectorItem *> & partsConnectors, ViewLayer::ViewLayerPlacement viewLayerPlacement) {
	if (partsConnectors.contains(connectorItem)) return;
				
	ConnectorItem * crossConnectorItem = connectorItem->getCrossLayerConnectorItem();
	if (crossConnectorItem != NULL) {
		if (partsConnectors.contains(crossConnectorItem)) {
			return;
		}
		
		if (viewLayerPlacement == ViewLayer::NewTopAndBottom) {
			partsConnectors.append(crossConnectorItem);
			
			/*
			DebugDialog::debug(QString("collecting both: %1 %2 %3 %4")
				.arg(crossConnectorItem->attachedToID())
				.arg(crossConnectorItem->connectorSharedID())
				.arg(crossConnectorItem->attachedToViewLayerID())
				.arg((long)crossConnectorItem->attachedTo(), 0, 16) );
			*/
				
		}
		else if (viewLayerPlacement == ViewLayer::NewTop) {
			if (connectorItem->attachedToViewLayerID() == ViewLayer::Copper1) {
			}
			else {
				connectorItem = crossConnectorItem;
			}
		}
		else if (viewLayerPlacement == ViewLayer::NewBottom) {
			if (connectorItem->attachedToViewLayerID() == ViewLayer::Copper0) {
			}
			else {
				connectorItem = crossConnectorItem;
			}
		}
	}

	/*
	DebugDialog::debug(QString("collecting part: %1 %2 %3 %4")
		.arg(connectorItem->attachedToID())
		.arg(connectorItem->connectorSharedID())
		.arg(connectorItem->attachedToViewLayerID())
		.arg((long) connectorItem->attachedTo(), 0, 16) );
	*/	

	partsConnectors.append(connectorItem);
}

void ConnectorItem::updateTooltip() {
	if (attachedToItemType() != ModelPart::Wire) {
		QString name = connectorSharedName();
        bool isInt = false;
        name.toInt(&isInt);
		QString descr = connectorSharedDescription();
        if (name.compare(descr, Qt::CaseInsensitive) == 0) {
            descr = "";
        }
        else {
            descr = ":" + descr;
        }
        QString id = connectorSharedID();
        int ix = IntegerFinder.indexIn(id);
        if (ix < 0 || isInt) {
            id = "";
        }
        else {
            if (attachedTo()->modelPart()->hasZeroConnector()) {
                id = QString::number(IntegerFinder.cap(0).toInt() + 1);
            }
            else {
                id = IntegerFinder.cap(0);
            }
            if (!id.isEmpty()) {
                id = " <span style='color:#909090;'>(" + id + ")</span>";
            }
        }
       
        QString tt = QString("<b>%2</b>%3%1<br /><span style='font-size:small;'>%4</span>")
                .arg(id)
                .arg(name)
                .arg(descr)
                .arg(attachedToTitle());
		setToolTip(tt);
		return;
	}

	QList<ConnectorItem *> connectors;
	foreach(ConnectorItem * toConnectorItem, m_connectedTo) {
        if (toConnectorItem->attachedToItemType() != ModelPart::Wire) {
			connectors.append(toConnectorItem);
		}
	}

	if (connectors.count() == 0) {
		setToolTip("");
		return;
	}


	QString connections = QString("<ul style='margin-left:0;padding-left:0;'>");
	foreach(ConnectorItem * connectorItem, connectors) {
		connections += QString("<li style='margin-left:0;padding-left:0;'>") + "<b>" + connectorItem->attachedTo()->label() + "</b> " + connectorItem->connectorSharedName() + "</li>";
	}
	connections += "</ul>";

    setToolTip(ItemBase::ITEMBASE_FONT_PREFIX + connections + ItemBase::ITEMBASE_FONT_SUFFIX);

}

void ConnectorItem::clearConnector() {
	m_connector = NULL;
}


bool ConnectorItem::connectionIsAllowed(ConnectorItem * other) {
	if (!connector()->connectionIsAllowed(other->connector())) return false;
	if (!m_attachedTo->connectionIsAllowed(other)) return false;
	foreach (ConnectorItem * toConnectorItem, connectedToItems()) {
		if (!toConnectorItem->attachedTo()->connectionIsAllowed(other)) {
			return false;
		}
	}

	return true;
}

void ConnectorItem::showEqualPotential(bool show, QList<ConnectorItem *> & visited) {
	if (!show) {
		restoreColor(visited);
		return;
	}

	QBrush brush;
	QPen pen;
	m_attachedTo->getEqualPotentialColor(this, brush, pen, m_opacity, m_negativePenWidth, m_negativeOffsetRect);
	//DebugDialog::debug(QString("set eqp %1 %2 %3").arg(attachedToID()).arg(pen->width()).arg(pen->color().name()));
	setColorAux(brush, pen, true);

}

void ConnectorItem::clearEqualPotentialDisplay() {
	//DebugDialog::debug(QString("clear eqp3"));
    QList<ConnectorItem *> visited;
	foreach (ConnectorItem * connectorItem, m_equalPotentialDisplayItems) {
		connectorItem->restoreColor(visited);
	}
	m_equalPotentialDisplayItems.clear();
}

bool ConnectorItem::isEverVisible() {
	return m_attachedTo->isEverVisible();
}

bool ConnectorItem::isGrounded(ConnectorItem * c1, ConnectorItem * c2) {
	QList<ConnectorItem *> connectorItems;
	if (c1 != NULL) {
		connectorItems.append(c1);
	}
	if (c2 != NULL) {
		connectorItems.append(c2);
	}
	collectEqualPotential(connectorItems, true, ViewGeometry::NoFlag);

	foreach (ConnectorItem * end, connectorItems) {
		if (end->isGrounded()) return true;

	}

	return false;
}

bool ConnectorItem::isGrounded() {
	QString name = connectorSharedName();
	return ((name.compare("gnd", Qt::CaseInsensitive) == 0) || 
			// (name.compare("-", Qt::CaseInsensitive) == 0) || 
			(name.compare("vss", Qt::CaseInsensitive) == 0) || 
			(name.compare("ground", Qt::CaseInsensitive) == 0)
			);
}

ConnectorItem * ConnectorItem::getCrossLayerConnectorItem() {
	if (m_connector == NULL) return NULL;
	if (m_attachedTo == NULL) return NULL;
	if (m_attachedTo->viewID() != ViewLayer::PCBView) return NULL;

	ViewLayer::ViewLayerID viewLayerID = attachedToViewLayerID();
	if (viewLayerID == ViewLayer::Copper0) {
		return m_connector->connectorItemByViewLayerID(this->attachedToViewID(), ViewLayer::Copper1);
	}
	if (viewLayerID == ViewLayer::Copper1) {
		return m_connector->connectorItemByViewLayerID(this->attachedToViewID(), ViewLayer::Copper0);
	}

	return NULL;
}

bool ConnectorItem::isInLayers(ViewLayer::ViewLayerPlacement viewLayerPlacement) {
	return ViewLayer::copperLayers(viewLayerPlacement).contains(attachedToViewLayerID());
}

bool ConnectorItem::isCrossLayerConnectorItem(ConnectorItem * candidate) {
	if (candidate == NULL) return false;

	ConnectorItem * cross = getCrossLayerConnectorItem();
	return cross == candidate;
}

bool ConnectorItem::isCrossLayerFrom(ConnectorItem * candidate) {
	return !ViewLayer::canConnect(this->attachedToViewLayerID(), candidate->attachedToViewLayerID());
}


bool isGrey(QColor color) {
	if (qAbs(color.red() - color.green()) > 16) return false;
	if (qAbs(color.red() - color.blue()) > 16) return false;
	if (qAbs(color.green() - color.blue()) > 16) return false;
	if (color.red() < 0x60) return false;
	if (color.red() > 0xA0) return false;
	return true;
}

void ConnectorItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) 
{
	if (m_hybrid) return;
	if (doNotPaint()) return;

	if (m_legPolygon.count() > 1) {
		paintLeg(painter);
		return;
	}

	if (m_effectively == EffectivelyUnknown) {
		if (!m_circular && m_shape.isEmpty()) {
			if (this->attachedTo()->viewID() == ViewLayer::PCBView) {
				QRectF r = rect();
				if (qAbs(r.width() - r.height()) < 0.01) m_effectively = EffectivelyCircular;
				else m_effectively = EffectivelyRectangular;
			}
		}
	}

	NonConnectorItem::paint(painter, option, widget);
}

void ConnectorItem::paintLeg(QPainter * painter) 
{
	QPen lpen = legPen();
	painter->setPen(lpen);

	bool hasCurves = false;
	foreach (Bezier * bezier, m_legCurves) {
		if (bezier != NULL && !bezier->isEmpty()) {
			hasCurves = true;
			break;
		}
	}

	// draw the leg first
	paintLeg(painter, hasCurves);

	if (m_legPolygon.count() > 2) {
		// draw bendpoint indicators
		double halfWidth = lpen.widthF() / 2;
		painter->setPen(Qt::NoPen);
		QColor c =  addColor(m_legColor, (qGray(m_legColor.rgb()) < 64) ? 80 : -64);
		painter->setBrush(c);
		for (int i = 1; i < m_legPolygon.count() - 1; i++) {
			painter->drawEllipse(m_legPolygon.at(i), halfWidth, halfWidth); 
		}
		painter->setBrush(Qt::NoBrush);
	}

	if (m_attachedTo->inHover()) {
		// hover highlight
		lpen.setColor((qGray(m_legColor.rgb()) < 48) ? QColor(255, 255, 255) : QColor(0, 0, 0));
		painter->setOpacity(ItemBase::HoverOpacity);
		painter->setPen(lpen);

		paintLeg(painter, hasCurves);	
	}

	// now draw the connector
	Bezier * bezier = m_legCurves.at(m_legCurves.count() - 2);
	bool connectorIsCurved = (bezier != NULL && !bezier->isEmpty());
	Bezier left, right;
	QPainterPath path;
	if (connectorIsCurved) {
		bezier->split(m_connectorDrawT, left, right);
		path.moveTo(right.endpoint0());
		path.cubicTo(right.cp0(), right.cp1(), right.endpoint1());
	}

	if (!isGrey(m_legColor)) {
		// draw an undercolor so the connectorColor will be visible on top of the leg color
		lpen.setColor(0x8c8c8c);			// TODO: don't hardcode color
		painter->setOpacity(1);
		painter->setPen(lpen);
		if (connectorIsCurved) {
			painter->drawPath(path);
		}
		else {
			painter->drawLine(m_connectorDrawEnd, m_legPolygon.last());		
		}
	}

	QPen pen = this->pen();
	pen.setWidthF(m_legStrokeWidth);
	pen.setCapStyle(Qt::RoundCap);
	painter->setOpacity(m_opacity);
	painter->setPen(pen);
	if (connectorIsCurved) {
		painter->drawPath(path);
	}
	else {
		painter->drawLine(m_connectorDrawEnd, m_legPolygon.last());		
	}
}

void ConnectorItem::paintLeg(QPainter * painter, bool hasCurves) 
{
	if (hasCurves) {
		for (int i = 0; i < m_legPolygon.count() -1; i++) {
			Bezier * bezier = m_legCurves.at(i);
			if (bezier && !bezier->isEmpty()) {
				QPainterPath path;
				path.moveTo(m_legPolygon.at(i));
				path.cubicTo(bezier->cp0(), bezier->cp1(), m_legPolygon.at(i + 1));
				painter->drawPath(path);
			}
			else {
				painter->drawLine(m_legPolygon.at(i), m_legPolygon.at(i + 1));
			}
		}
	}
	else {
		painter->drawPolyline(m_legPolygon);
	}
}


ConnectorItem * ConnectorItem::chooseFromSpec(ViewLayer::ViewLayerPlacement viewLayerPlacement) {
	ConnectorItem * crossConnectorItem = getCrossLayerConnectorItem();
	if (crossConnectorItem == NULL) return this;

	ViewLayer::ViewLayerID basis = ViewLayer::Copper0;
	switch (viewLayerPlacement) {
		case ViewLayer::NewTop:
			basis = ViewLayer::Copper1;
			break;
		case ViewLayer::NewBottom:
			basis = ViewLayer::Copper0;
			break;
		default:
			DebugDialog::debug(QString("unusual viewLayerPlacement %1").arg(viewLayerPlacement));
			basis = ViewLayer::Copper0;
			break;
	}

	if (this->attachedToViewLayerID() == basis) {
		return this;
	}
	if (crossConnectorItem->attachedToViewLayerID() == basis) {
		return crossConnectorItem;	
	}
	return this;
}

bool ConnectorItem::connectedToWires() {
	foreach (ConnectorItem * toConnectorItem, connectedToItems()) {
		if (toConnectorItem->attachedToItemType() == ModelPart::Wire) {
			return true;
		}
	}

	ConnectorItem * crossConnectorItem = getCrossLayerConnectorItem();
	if (crossConnectorItem == NULL) return false;

	foreach (ConnectorItem * toConnectorItem, crossConnectorItem->connectedToItems()) {
		if (toConnectorItem->attachedToItemType() == ModelPart::Wire) {
			return true;
		}
	}

	return false;
}

void ConnectorItem::displayRatsnest(QList<ConnectorItem *> & partConnectorItems, ViewGeometry::WireFlags myFlag) {
	bool formerColorWasNamed = false;
	bool gotFormerColor = false;
	QColor formerColor;

	VirtualWire * vw = NULL;
	foreach (ConnectorItem * fromConnectorItem, partConnectorItems) {
		foreach (ConnectorItem * toConnectorItem, fromConnectorItem->connectedToItems()) {
			vw = qobject_cast<VirtualWire *>(toConnectorItem->attachedTo());
			if (vw != NULL) break;
		}
		if (vw != NULL) break;
	}

	if (vw != NULL) {
		formerColorWasNamed = vw->colorWasNamed();
		formerColor = vw->color();
		gotFormerColor = true;
		clearRatsnestDisplay(partConnectorItems);
	}

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView == NULL) return;

	if (partConnectorItems.count() < 2) return;

	QStringList connectorNames;
	ConnectorItem::collectConnectorNames(partConnectorItems, connectorNames);
	QColor color;
	bool colorWasNamed = RatsnestColors::findConnectorColor(connectorNames, color);
	if (!colorWasNamed) {
		if (!formerColorWasNamed && gotFormerColor) {
			color = formerColor;
		}
		else {
			infoGraphicsView->getRatsnestColor(color);
		}
	}

	ConnectorPairHash result;
	GraphUtils::chooseRatsnestGraph(&partConnectorItems, (ViewGeometry::RatsnestFlag | ViewGeometry::NormalFlag | ViewGeometry::PCBTraceFlag | ViewGeometry::SchematicTraceFlag) ^ myFlag, result);

	foreach (ConnectorItem * key, result.uniqueKeys()) {
		foreach (ConnectorItem * value, result.values(key)) {
			VirtualWire * vw = infoGraphicsView->makeOneRatsnestWire(key, value, false, color, false);
			if (vw) {
				vw->setColorWasNamed(colorWasNamed);
			}
		}
	}
}

void ConnectorItem::clearRatsnestDisplay(QList<ConnectorItem *> & connectorItems) {

	QSet<VirtualWire *> ratsnests;
	foreach (ConnectorItem * fromConnectorItem, connectorItems) {
		if (fromConnectorItem == NULL) continue;

		foreach (ConnectorItem * toConnectorItem, fromConnectorItem->connectedToItems()) {
			VirtualWire * vw = qobject_cast<VirtualWire *>(toConnectorItem->attachedTo());
			if (vw != NULL) {
				ratsnests.insert(vw);
			}
		}
	}

	foreach (VirtualWire * vw, ratsnests.values()) {
		ConnectorItem * c1 = vw->connector0()->firstConnectedToIsh();
		if (c1 != NULL) {
			vw->connector0()->tempRemove(c1, false);
			c1->tempRemove(vw->connector0(), false);
		}

		ConnectorItem * c2 = vw->connector1()->firstConnectedToIsh();
		if (c2 != NULL) {
			vw->connector1()->tempRemove(c2, false);
			c2->tempRemove(vw->connector1(), false);
		}

		//vw->debugInfo("removing rat 1");
		vw->scene()->removeItem(vw);
		delete vw;
	}
}


void ConnectorItem::collectConnectorNames(QList<ConnectorItem *> & connectorItems, QStringList & connectorNames) 
{
	foreach(ConnectorItem * connectorItem, connectorItems) {
		if (!connectorNames.contains(connectorItem->connectorSharedName())) {
			connectorNames.append(connectorItem->connectorSharedName());
			//DebugDialog::debug("name " + connectorItem->connectorSharedName());
		}
	}
}

double ConnectorItem::calcClipRadius() {
	if (m_circular) {
		return radius() - (strokeWidth() / 2.0);
	}

	if (m_effectively == EffectivelyCircular) {
		double rad = rect().width() / 2;
		return rad - (rad / 5);
	}

	return 0;
}

bool ConnectorItem::isEffectivelyCircular() {
	return m_circular || m_effectively == EffectivelyCircular;
}

void ConnectorItem::debugInfo(const QString & msg) 
{

#ifndef QT_NO_DEBUG
    QPointF p = sceneAdjustedTerminalPoint(NULL);
	QString s = QString("%1 cid:%2 cname:%3 title:%4 id:%5 type:%6 inst:%7 vlid:%8 vid:%9 spec:%10 flg:%11 hy:%12 bus:%13 r:%14 sw:%15 pos:(%16 %17)")
			.arg(msg)
			.arg(this->connectorSharedID())
			.arg(this->connectorSharedName())
			.arg(this->attachedToTitle())
			.arg(this->attachedToID())
			.arg(this->attachedToItemType())
			.arg(this->attachedToInstanceTitle())
			.arg(this->attachedToViewLayerID())
			.arg(this->attachedToViewID())
			.arg(this->attachedToViewLayerPlacement())
			.arg(this->attachedTo()->wireFlags()) 
			.arg(this->m_hybrid)
			.arg((long) this->bus(), 0, 16)
            .arg(this->m_radius)
            .arg(this->m_strokeWidth)
            .arg(p.x())
            .arg(p.y())
            ;
	//s.replace(" ", "_");
	DebugDialog::debug(s);
#else
	Q_UNUSED(msg);
#endif
}

double ConnectorItem::minDimension() {
	QRectF r = this->boundingRect();
	return qMin(r.width(), r.height());
}

ConnectorItem * ConnectorItem::findConnectorUnder(bool useTerminalPoint, bool allowAlready, const QList<ConnectorItem *> & exclude, bool displayDragTooltip, ConnectorItem * other)
{
	QList<QGraphicsItem *> items = useTerminalPoint
		? this->scene()->items(this->sceneAdjustedTerminalPoint(NULL))
		: this->scene()->items(mapToScene(this->rect()));			// only wires use rect
	QList<ConnectorItem *> candidates;
	// for the moment, take the topmost ConnectorItem that doesn't belong to me
	foreach (QGraphicsItem * item, items) {
		ConnectorItem * connectorItemUnder = dynamic_cast<ConnectorItem *>(item);
		if (connectorItemUnder == NULL) continue;
		if (connectorItemUnder->connector() == NULL) continue;			// shouldn't happen
		if (attachedTo()->childItems().contains(connectorItemUnder)) continue;		// don't use own connectors
		if (!this->connectionIsAllowed(connectorItemUnder)) {
			continue;
		}
		if (!allowAlready) {
			if (connectorItemUnder->connectedToItems().contains(this)) {
				continue;		// already connected
			}
		}
		if (exclude.contains(connectorItemUnder)) continue;


		candidates.append(connectorItemUnder);
	}

	ConnectorItem * candidate = NULL;
	if (candidates.count() == 1) {
		candidate = candidates[0];
	}
	else if (candidates.count() > 0) {
		qSort(candidates.begin(), candidates.end(), wireLessThan);
		candidate = candidates[0];
	}

	if (m_overConnectorItem != NULL && candidate != m_overConnectorItem) {
		m_overConnectorItem->connectorHover(NULL, false);
	}
	if (candidate != NULL && candidate != m_overConnectorItem) {
		candidate->connectorHover(NULL, true);
	}

	m_overConnectorItem = candidate;

	if (candidate == NULL) {
		if (this->connectorHovering()) {
			this->connectorHover(NULL, false);
		}
	}
	else {
		if (!this->connectorHovering()) {
			this->connectorHover(NULL, true);
		}
	}

	if (displayDragTooltip) {
		displayTooltip(m_overConnectorItem, other);
	}

	return m_overConnectorItem;
}

void ConnectorItem::displayTooltip(ConnectorItem * ci, ConnectorItem * other)
{
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView == NULL) return;

	// Activate tooltip for destination connector. based on a patch submitted by bryant.mairs
	QString text;
	if (ci && ci->connectorHovering()) {
		if (other) {
			text = QString("%1: %2\n%3: %4")
						.arg(other->attachedToInstanceTitle())
						.arg(other->connectorSharedName())
						.arg(ci->attachedToInstanceTitle())
						.arg(ci->connectorSharedName());
		}
		else {
			text = QString("%1: %2").arg(ci->attachedToInstanceTitle()).arg(ci->connectorSharedName());
		}
	}
	else {
		if (other) {
			text = QString("%1: %2")
				.arg(other->attachedToInstanceTitle())
				.arg(other->connectorSharedName());
		}
	}
    // Now use Qt's tooltip functionality to display our tooltip.
    // The tooltip text is first cleared as only a change in tooltip
    // text will update its position.
    // A rect is generated to smooth out position updates.
    // NOTE: Increasing this rect will cause the tooltip to disappear
    // and not reappear until another pixel move after the move that
    // disabled it.
    QPoint sp = QCursor::pos();
    QToolTip::showText(sp, "", infoGraphicsView);
	if (!text.isEmpty()) {
		QPoint q = infoGraphicsView->mapFromGlobal(sp);
		QRect r(q.x(), q.y(), 1, 1);
		QToolTip::showText(sp, text, infoGraphicsView, r);
	}
}

ConnectorItem * ConnectorItem::releaseDrag() {
	ConnectorItem * result = m_overConnectorItem;
	if (m_overConnectorItem != NULL) {
		m_overConnectorItem->connectorHover(NULL, false);

		// clean up
		setOverConnectorItem(NULL);
		clearConnectorHover();    
        QList<ConnectorItem *> visited;
		restoreColor(visited);
	}
	attachedTo()->clearConnectorHover();
	return result;
}

void ConnectorItem::rotateLeg(const QPolygonF & poly, bool active) 
{
	resetLeg(poly, false, active, "rotate");
}

void ConnectorItem::resetLeg(const QPolygonF & poly, bool relative, bool active, const QString & why) 
{
	if (!m_rubberBandLeg) return;

	ConnectorItem * target = NULL;
	foreach (ConnectorItem * connectorItem, this->m_connectedTo) {
		if (connectorItem->connectorType() == Connector::Female) {
			target = connectorItem;
			break;
		}
	}

	if (target == NULL) {
		setLeg(poly, relative, why);
		return;
	}

	if (!active) {
		repositionTarget();
		return;
	}

	if (why.compare("swap") == 0) {
		setLeg(poly, relative, why);
		repositionTarget();
		return;
	}

	//DebugDialog::debug("connectorItem prepareGeometryChange 1");
	prepareGeometryChange();
	QPointF sceneNewLast = target->sceneAdjustedTerminalPoint(NULL);
	QPointF sceneOldLast = poly.last();

	for (int i = 1; i < m_legPolygon.count(); i++) {
		m_legPolygon.replace(i, mapFromScene(poly.at(i) - sceneOldLast + sceneNewLast));
	}

	calcConnectorEnd();
	update();
}

void ConnectorItem::setLeg(const QPolygonF & poly, bool relative, const QString & why) 
{
	Q_UNUSED(why);

	if (!m_rubberBandLeg) return;

	repoly(poly, relative);
	update();
}

const QPolygonF & ConnectorItem::leg() {
	static QPolygonF emptyPoly;

	if (!m_rubberBandLeg) return emptyPoly;

	return m_legPolygon;
}

bool ConnectorItem::isDraggingLeg() {
	return m_draggingLeg;
}

QString ConnectorItem::makeLegSvg(QPointF offset, double dpi, double printerScale, bool blackOnly) {
	if (!m_rubberBandLeg) return "";

	QString data("M");
	QPointF p = m_legPolygon.at(0);
	data += TextUtils::pointToSvgString(mapToScene(p), offset, dpi, printerScale);
	for (int i = 1; i < m_legPolygon.count(); i++) {
		QPointF p = m_legPolygon.at(i);
		Bezier * bezier = m_legCurves.at(i - 1);
		if (bezier == NULL || bezier->isEmpty()) {
			data += "L";
			data += TextUtils::pointToSvgString(mapToScene(p), offset, dpi, printerScale);
		}
		else {
			data += "C";
			data += TextUtils::pointToSvgString(mapToScene(bezier->cp0()), offset, dpi, printerScale);
			data += " ";
			data += TextUtils::pointToSvgString(mapToScene(bezier->cp1()), offset, dpi, printerScale);
			data += " ";
			data += TextUtils::pointToSvgString(mapToScene(p), offset, dpi, printerScale);
		}
	}

	QString path = QString("<path stroke='%1' stroke-width='%2' stroke-linecap='round' stroke-linejoin='round' fill='none' d='%3' />\n")
						.arg(blackOnly ? "black" :  m_legColor.name())
						.arg(m_legStrokeWidth * dpi / printerScale)
						.arg(data);
	return path;
}

QPolygonF ConnectorItem::sceneAdjustedLeg() {
	if (!m_rubberBandLeg) return QPolygonF();

	QPolygonF poly;
	foreach (QPointF p, m_legPolygon) {
		poly.append(mapToScene(p));
	}

	return poly;
}

void ConnectorItem::prepareToStretch(bool activeStretch) {
	m_activeStretch = activeStretch;
	m_oldPolygon = sceneAdjustedLeg();
}

void ConnectorItem::stretchBy(QPointF howMuch) {
	if (!m_rubberBandLeg) return;

	Q_UNUSED(howMuch);

	resetLeg(m_oldPolygon, false, m_activeStretch, "move");

	// if update isn't called here then legs repaint, but body doesn't
	// so you get a weird "headless" effect
	m_attachedTo->update();
}

void ConnectorItem::stretchDone(QPolygonF & oldLeg, QPolygonF & newLeg, bool & active) {
	oldLeg = m_oldPolygon;
	newLeg = sceneAdjustedLeg();
	active = m_activeStretch;
}

void ConnectorItem::moveDone(int & index0, QPointF & oldPos0, QPointF & newPos0, int & index1, QPointF & oldPos1, QPointF & newPos1) {
	index0 = (m_activeStretch) ? 1 : m_legPolygon.count() - 1;
	oldPos0 = m_oldPolygon.at(index0);
	newPos0 = mapToScene(m_legPolygon.at(index0));
	index1 = m_legPolygon.count() - 1;
	oldPos1 = m_oldPolygon.at(index1);
	newPos1 = mapToScene(m_legPolygon.at(index1));
}

QRectF ConnectorItem::boundingRect() const
{
	if (m_legPolygon.count() < 2) return NonConnectorItem::boundingRect();

	return shape().controlPointRect();
}

QPainterPath ConnectorItem::hoverShape() const
{
	return shapeAux(2 * m_legStrokeWidth);
}

QPainterPath ConnectorItem::shape() const
{
	return shapeAux(m_legStrokeWidth);
}

QPainterPath ConnectorItem::shapeAux(double width) const
{
	if (m_legPolygon.count() < 2) return NonConnectorItem::shape();

	QPainterPath path;
	path.moveTo(m_legPolygon.at(0));
	for (int i = 1; i < m_legPolygon.count(); i++) {
		Bezier * bezier = m_legCurves.at(i - 1);
		if (bezier != NULL && !bezier->isEmpty()) {
			path.cubicTo(bezier->cp0(), bezier->cp1(), m_legPolygon.at(i));
		}
		else {
			path.lineTo(m_legPolygon.at(i));
		}
	}
	
	QPen pen = legPen();
	
	return GraphicsUtils::shapeFromPath(path, pen, width, false);
}

void ConnectorItem::repositionTarget()
{
	// this connector is connected to another part which is being dragged
	foreach (ConnectorItem * connectorItem, this->m_connectedTo) {
		if (connectorItem->connectorType() == Connector::Female) {
			reposition(connectorItem->sceneAdjustedTerminalPoint(NULL), m_legPolygon.count() - 1);
			break;
		}
	}
}

void ConnectorItem::reposition(QPointF sceneDestPos, int draggingIndex)
{
	//DebugDialog::debug("connectorItem prepareGeometryChange 2");
	prepareGeometryChange();
	//foreach (QPointF p, m_legPolygon) DebugDialog::debug(QString("point b %1 %2").arg(p.x()).arg(p.y()));
	QPointF dest = mapFromScene(sceneDestPos);
	m_legPolygon.replace(draggingIndex, dest);
	//foreach (QPointF p, m_legPolygon) DebugDialog::debug(QString("point a %1 %2").arg(p.x()).arg(p.y()));
	calcConnectorEnd();
}

void ConnectorItem::repoly(const QPolygonF & poly, bool relative)
{
	//DebugDialog::debug("connectorItem prepareGeometryChange 3");
	prepareGeometryChange();

	//foreach (QPointF p, m_legPolygon) DebugDialog::debug(QString("point b %1 %2").arg(p.x()).arg(p.y()));
	m_legPolygon.clear();
	clearCurves();

	foreach (QPointF p, poly) {
		m_legPolygon.append(relative ? p : mapFromScene(p));
		m_legCurves.append(NULL);
	}
	//foreach (QPointF p, m_legPolygon) DebugDialog::debug(QString("point a %1 %2").arg(p.x()).arg(p.y()));
	calcConnectorEnd();
}

void ConnectorItem::calcConnectorEnd()
{
	if (m_legPolygon.count() < 2) {
		m_connectorDrawEnd = m_connectorDetectEnd = QPointF(0,0);
		return;
	}

	QPointF p1 = m_legPolygon.last();
	QPointF p0 = m_legPolygon.at(m_legPolygon.count() - 2);
	double dx = p1.x() - p0.x();
	double dy = p1.y() - p0.y();
	double lineLen = qSqrt((dx * dx) + (dy * dy));
	double drawlen = qMax(0.5, qMin(lineLen, StandardLegConnectorDrawEnabledLength));
	double detectlen = qMax(0.5, qMin(lineLen, StandardLegConnectorDetectLength));

	Bezier * bezier = m_legCurves.at(m_legCurves.count() - 2);
	if (bezier == NULL || bezier->isEmpty()) {
		m_connectorDrawEnd = QPointF(p1 - QPointF(dx * drawlen / lineLen, dy * drawlen / lineLen));
		m_connectorDetectEnd = QPointF(p1 - QPointF(dx * detectlen / lineLen, dy * detectlen / lineLen));
		return;
	}

	bezier->set_endpoints(p0, p1);
	m_connectorDetectT = m_connectorDrawT = 0;
	double blen = bezier->computeCubicCurveLength(1.0, 24);
	if (blen < StandardLegConnectorDetectLength) {
		return;
	}

	m_connectorDetectT = findT(bezier, blen, StandardLegConnectorDetectLength);
	m_connectorDrawT = findT(bezier, blen, StandardLegConnectorDrawEnabledLength);
}

double ConnectorItem::findT(Bezier * bezier, double blen, double length)
{
	// use binary search to find a value for t
	double tmax = 1.0;
	double tmin = 0;
	double t = 1.0 - (length / blen);
	while (true) {
		double l = bezier->computeCubicCurveLength(t, 24);
		if (qAbs(blen - length - l) < .0001) {
			return t;
		}

		if (blen - length - l > 0) {
			// too short
			tmin = t;
			t = (t + tmax) / 2;
		}
		else {
			// too long
			tmax = t;
			t = (t + tmin) / 2;
		}
	}
	return t;
}

const QString & ConnectorItem::legID(ViewLayer::ViewID viewID, ViewLayer::ViewLayerID viewLayerID) {
	if (m_connector) return m_connector->legID(viewID, viewLayerID);

	return ___emptyString___;
}

void ConnectorItem::setRubberBandLeg(QColor color, double strokeWidth, QLineF parentLine) {
	// assumes this is only called once, when the connector is first set up

	/*
	this->debugInfo(QString("set rubber band leg %1 %2 %3 %4")
		.arg(parentLine.p1().x())
		.arg(parentLine.p1().y())
		.arg(parentLine.p2().x())
		.arg(parentLine.p2().y())
		);
	*/

	m_rubberBandLeg = true;
	setFlag(QGraphicsItem::ItemIsMovable, true);
	setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
	setAcceptedMouseButtons(ALLMOUSEBUTTONS);

	// p1 is always the start point closest to the body. 
	setPos(parentLine.p1());
	m_legPolygon.append(QPointF(0,0));
	m_legPolygon.append(parentLine.p2() - parentLine.p1());
	m_legCurves.append(NULL);
	m_legCurves.append(NULL);
	m_legStrokeWidth = strokeWidth;
	m_legColor = color;
	reposition(m_attachedTo->mapToScene(parentLine.p2()), 1);

	this->setCircular(false);
}

bool ConnectorItem::hasRubberBandLeg() const {
	return m_rubberBandLeg;
}

void ConnectorItem::killRubberBandLeg() {
	// this is a hack; see the caller for explanation
	prepareGeometryChange();
	m_rubberBandLeg = false;
	m_legPolygon.clear();
	clearCurves();
}

QPen ConnectorItem::legPen() const
{
	if (!m_rubberBandLeg) return QPen();

	QPen pen;
	pen.setWidthF(m_legStrokeWidth);
	pen.setColor(m_legColor);
	pen.setCapStyle(Qt::RoundCap);
	pen.setJoinStyle(Qt::RoundJoin);
	return pen;
}

bool ConnectorItem::legMousePressEvent(QGraphicsSceneMouseEvent *event) {
	m_insertBendpointPossible = false;

	if (attachedTo()->moveLock()) {
		event->ignore();
		return true;
	}

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->prepLegSelection(this->attachedTo());
	}

	int bendpointIndex;
	CursorLocation cursorLocation = findLocation(event->pos(), bendpointIndex);
	switch (cursorLocation) {

		case InConnector:
			if (event->modifiers() & altOrMetaModifier()) return false;

			m_holdPos = mapToScene(m_legPolygon.last());
			m_draggingLeg = true;
			m_draggingLegIndex = m_legPolygon.count() - 1;
			m_oldPolygon = m_legPolygon;
			QGraphicsRectItem::mousePressEvent(event);
			return true;

		case InSegment:
			if (curvyWiresIndicated(event->modifiers())) {
				m_draggingLegIndex = bendpointIndex - 1;
				Bezier * bezier = m_legCurves.at(m_draggingLegIndex);
				if (bezier == NULL) {
					bezier = new Bezier();
					m_legCurves.replace(m_draggingLegIndex, bezier);
				}

				UndoBezier.copy(bezier);
				if (bezier->isEmpty()) {
					QPointF p0 = m_legPolygon.at(m_draggingLegIndex);
					QPointF p1 = m_legPolygon.at(m_draggingLegIndex + 1);
					bezier->initToEnds(p0, p1);
				}

				bezier->initControlIndex(event->pos(), m_legStrokeWidth);
				m_draggingCurve = m_draggingLeg = true;
				TheBezierDisplay = new BezierDisplay;
				TheBezierDisplay->initDisplay(this, bezier);
				return true;
			}
			else {
				m_insertBendpointPossible = true;
			}
			// must continue on to InBendpoint

		case InBendpoint:
			m_draggingLegIndex = bendpointIndex;
			m_holdPos = event->scenePos();
			m_oldPolygon = m_legPolygon;
			m_draggingLeg = true;
			QGraphicsRectItem::mousePressEvent(event);
			return true;

		case InOrigin:
		case InNotFound:
		default:
			event->ignore();
			return true;
	}
}

ConnectorItem::CursorLocation ConnectorItem::findLocation(QPointF location, int & bendpointIndex) {
	QPainterPath path;
	Bezier * bezier = m_legCurves.at(m_legCurves.count() - 2);
	if (bezier == NULL || bezier->isEmpty()) {
		path.moveTo(m_connectorDetectEnd);
		path.lineTo(m_legPolygon.last());
	}
	else {
		Bezier left, right;
		bezier->split(m_connectorDetectT, left, right);
		path.moveTo(right.endpoint0());
		path.cubicTo(right.cp0(), right.cp1(), right.endpoint1());
	}

	QPen pen = legPen();
	path = GraphicsUtils::shapeFromPath(path, pen, m_legStrokeWidth, false);
	if (path.contains(location)) {
		return InConnector;
	}

	double wSqd = 4 * m_legStrokeWidth * m_legStrokeWidth;			// hover distance
	for (int i = 0; i < m_legPolygon.count() - 1; i++) {
		QPainterPath path;
		path.moveTo(m_legPolygon.at(i));
		Bezier * bezier = m_legCurves.at(i);
		if (bezier != NULL && !bezier->isEmpty()) {
			path.cubicTo(bezier->cp0(), bezier->cp1(), m_legPolygon.at(i + 1));
		}
		else {
			path.lineTo(m_legPolygon.at(i + 1));
		}
		path = GraphicsUtils::shapeFromPath(path, pen, m_legStrokeWidth, false);
		if (path.contains(location)) {
			double d = GraphicsUtils::distanceSqd(m_legPolygon.at(i), location);
			if (d <= wSqd) {
				bendpointIndex = i;
				if (i == 0) {
					return InOrigin;
				}

				return InBendpoint;
			}
			else {
				d = GraphicsUtils::distanceSqd(m_legPolygon.at(i + 1), location);
				if (d <= wSqd) {
					bendpointIndex = i + 1;
					return InBendpoint;
				}
			}

			bendpointIndex = i + 1;
			return InSegment;
		}
	}

	return InNotFound;
}

void ConnectorItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
	if (m_hidden || m_inactive || m_hybrid) {
		event->ignore();
		return;
	}

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->setActiveConnectorItem(this);
	}

	if ((acceptedMouseButtons() & Qt::RightButton) == 0) {
		event->ignore();
		return;
	}

	if (!m_rubberBandLeg) {
		event->ignore();
		return;
	}

	int bendpointIndex;
	CursorLocation cursorLocation = findLocation(event->pos(), bendpointIndex);
	switch (cursorLocation) {
		case InSegment:
		{
			QMenu menu;
			QAction * addAction = menu.addAction(tr("Add bendpoint"));
			addAction->setData(1);
			Bezier * bezier = m_legCurves.at(bendpointIndex - 1);
			if (bezier != NULL && !bezier->isEmpty()) {
				QAction * straightenAction = menu.addAction(tr("Straighten curve"));
				straightenAction->setData(2);
			}
			QAction *selectedAction = menu.exec(event->screenPos());
			if (selectedAction) {
				if (selectedAction->data().toInt() == 1) {
					insertBendpoint(event->pos(), bendpointIndex);
				}
				else if (selectedAction->data().toInt() == 2) {
					InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
					if (infoGraphicsView != NULL) {
						Bezier newBezier;
						infoGraphicsView->prepLegCurveChange(this, bendpointIndex - 1,bezier, &newBezier, true);
					}
				}
			}
		}
		return;

		case InBendpoint:
			if (bendpointIndex < m_legPolygon.count() - 1) {
				QMenu menu;
				menu.addAction(tr("Remove bendpoint"));
				QAction *selectedAction = menu.exec(event->screenPos());
				if (selectedAction) {
					removeBendpoint(bendpointIndex);
				}
				return;
			}

		default:
			break;
	}

	event->ignore();
	return;
}

void ConnectorItem::insertBendpoint(QPointF p, int bendpointIndex) 
{
	prepareGeometryChange();

	m_oldPolygon = m_legPolygon;
	insertBendpointAux(p, bendpointIndex);
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->prepLegBendpointChange(this, m_oldPolygon.count(), m_legPolygon.count(), bendpointIndex, p, 
							&UndoBezier, m_legCurves.at(bendpointIndex - 1), m_legCurves.at(bendpointIndex), false);
	}
	calcConnectorEnd();
	update();
}

Bezier * ConnectorItem::insertBendpointAux(QPointF p, int bendpointIndex)
{
	UndoBezier.clear();
	m_legPolygon.insert(bendpointIndex, p);
	m_legCurves.insert(bendpointIndex, NULL);
	Bezier * bezier = m_legCurves.at(bendpointIndex - 1);
	if (bezier == NULL || bezier->isEmpty()) return NULL;

	QPointF p0 = m_legPolygon.at(bendpointIndex - 1);
	QPointF p1 = m_legPolygon.at(bendpointIndex + 1);
	bezier->set_endpoints(p0, p1);
	UndoBezier.copy(bezier);

	double t = bezier->findSplit(p, m_legStrokeWidth);
	Bezier left, right;
	bezier->split(t, left, right);
	replaceBezier(bendpointIndex - 1, &left);
	replaceBezier(bendpointIndex, &right);
	return NULL;
}

void ConnectorItem::removeBendpoint(int bendpointIndex) 
{
	prepareGeometryChange();

	Bezier b0, b1;
	b0.copy(m_legCurves.at(bendpointIndex - 1));
	QPointF p0 = m_legPolygon.at(bendpointIndex - 1);
	QPointF p1 = m_legPolygon.at(bendpointIndex);
	b0.set_endpoints(p0, p1);
	b1.copy(m_legCurves.at(bendpointIndex));
	p0 = m_legPolygon.at(bendpointIndex);
	p1 = m_legPolygon.at(bendpointIndex + 1);
	b0.set_endpoints(p0, p1);

	m_oldPolygon = m_legPolygon;
	QPointF p = m_legPolygon.at(bendpointIndex);
	m_legPolygon.remove(bendpointIndex);

	Bezier b2 = b0.join(&b1);
	replaceBezier(bendpointIndex - 1, &b2);
	
	Bezier * bezier = m_legCurves.at(bendpointIndex);
	m_legCurves.remove(bendpointIndex);
	if (bezier) delete bezier;

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->prepLegBendpointChange(this, m_oldPolygon.count(), m_legPolygon.count(), bendpointIndex, p, &b0, &b1, &b2, false);
	}


	calcConnectorEnd();
	update();
}

void ConnectorItem::clearCurves()
{
	foreach (Bezier * bezier, m_legCurves) {
		if (bezier != NULL) delete bezier;
	}
	m_legCurves.clear();
}

void ConnectorItem::changeLegCurve(int index, const Bezier *newBezier)
{
	prepareGeometryChange();

	replaceBezier(index, newBezier);
	calcConnectorEnd();
	update();
}

void ConnectorItem::addLegBendpoint(int index, QPointF p, const Bezier * bezierLeft, const Bezier * bezierRight)
{
	prepareGeometryChange();

	m_legPolygon.insert(index, p);
	m_legCurves.insert(index, NULL);
	replaceBezier(index - 1, bezierLeft);
	replaceBezier(index, bezierRight);
	calcConnectorEnd();
	update();

}

void ConnectorItem::removeLegBendpoint(int index, const Bezier * newBezier)
{
	prepareGeometryChange();

	m_legPolygon.remove(index);
	Bezier * bezier = m_legCurves.at(index);
	if (bezier) delete bezier;
	m_legCurves.remove(index);
	replaceBezier(index - 1, newBezier);
	calcConnectorEnd();
	update();
}

void ConnectorItem::moveLegBendpoint(int index, QPointF p)
{
	m_legPolygon.replace(index, mapFromScene(p));
	calcConnectorEnd();
	update();
}

const QVector<Bezier *> & ConnectorItem::beziers()
{
	return m_legCurves;
}

void ConnectorItem::replaceBezier(int index, const Bezier * newBezier)
{
	Bezier * bezier = m_legCurves.at(index);
	if (bezier == NULL && newBezier == NULL) {
	}
	else if (bezier && newBezier) {
		bezier->copy(newBezier);
	}
	else if (newBezier) {
		bezier = new Bezier;
		m_legCurves.replace(index , bezier);
		bezier->copy(newBezier);
	}
	else if (bezier) {
		bezier->clear();
	}
}

void ConnectorItem::cursorKeyEvent(Qt::KeyboardModifiers modifiers)
{
	if (m_rubberBandLeg) {
		if (m_draggingLeg) return;

		InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);;
		if (infoGraphicsView) {
			QPoint p = infoGraphicsView->mapFromGlobal(QCursor::pos());
			QPointF r = infoGraphicsView->mapToScene(p);
			// DebugDialog::debug(QString("got key event %1").arg(keyEvent->modifiers()));
			updateLegCursor(mapFromScene(r), modifiers);
		}
	}
	else if (attachedToItemType() == ModelPart::Wire) {
		updateWireCursor(modifiers);
	}
}

void ConnectorItem::updateWireCursor(Qt::KeyboardModifiers modifiers)
{
	//DebugDialog::debug("uwc");
	QCursor cursor = *CursorMaster::BendpointCursor;
	if (isBendpoint()) {
		//DebugDialog::debug("uwc bend");
		if (modifiers & altOrMetaModifier()) {
			//DebugDialog::debug("uwc alt");
			Wire * wire = qobject_cast<Wire *>(attachedTo());
			if (wire != NULL && wire->canChainMultiple()) {
				//DebugDialog::debug("uwc make wire");
				cursor = *CursorMaster::MakeWireCursor;
			}
		}
	}

	CursorMaster::instance()->addCursor(this, cursor);
}

void ConnectorItem::updateLegCursor(QPointF p, Qt::KeyboardModifiers modifiers)
{
	int bendpointIndex;
	CursorLocation cursorLocation = findLocation(p, bendpointIndex);
	QCursor cursor;
	switch (cursorLocation) {
		case InOrigin:
			cursor = *attachedTo()->getCursor(modifiers);
			break;
		case InBendpoint:
			cursor = *CursorMaster::BendpointCursor;
			break;
		case InSegment:
			cursor = curvyWiresIndicated(modifiers) ? *CursorMaster::MakeCurveCursor : *CursorMaster::NewBendpointCursor;
			break;
		case InConnector:
			cursor = (modifiers & altOrMetaModifier()) ? *CursorMaster::MakeWireCursor : *CursorMaster::BendlegCursor;
			break;
		default:
			cursor = Qt::ArrowCursor;
			break;
	}
	CursorMaster::instance()->addCursor(this, cursor);
}

bool ConnectorItem::curvyWiresIndicated(Qt::KeyboardModifiers modifiers)
{
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView == NULL) return true;

	return infoGraphicsView->curvyWiresIndicated(modifiers);
}

bool ConnectorItem::isBendpoint()
{
	if (connectionsCount() == 0) return false;

	foreach (ConnectorItem * ci, connectedToItems()) {
		if (ci->attachedToItemType() != ModelPart::Wire) {
			return false;
		}
	}

	return true;
}

void ConnectorItem::setConnectorLocalName(const QString & name) 
{
	if (m_connector) {
		m_connector->setConnectorLocalName(name);
	}
}

bool ConnectorItem::isGroundFillSeed() 
{
	return m_groundFillSeed;
}

void ConnectorItem::setGroundFillSeed(bool seed) 
{
	m_groundFillSeed = seed;
	ConnectorItem * cross = this->getCrossLayerConnectorItem();
	if (cross) cross->m_groundFillSeed = seed;
}

ViewGeometry::WireFlags ConnectorItem::getSkipFlags() {
    InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
    if (infoGraphicsView == NULL) return ViewGeometry::RatsnestFlag;

    return (ViewGeometry::RatsnestFlag | ViewGeometry::NormalFlag | ViewGeometry::PCBTraceFlag | ViewGeometry::SchematicTraceFlag) ^ infoGraphicsView->getTraceFlag();
}
