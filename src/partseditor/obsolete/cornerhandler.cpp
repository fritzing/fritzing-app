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

$Revision: 6904 $:
$Author: irascibl@gmail.com $:
$Date: 2013-02-26 16:26:03 +0100 (Di, 26. Feb 2013) $

********************************************************************/

#include <QHash>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>

#include "cornerhandler.h"
#include "connectorrectangle.h"
#include "../debugdialog.h"

QHash<Qt::Corner,QPixmap> CornerHandler::pixmapHash;
double CornerHandler::Size = 4;

CornerHandler::CornerHandler(ConnectorRectangle *parent, QGraphicsItem* parentItem, Qt::Corner corner)
	: QGraphicsRectItem(parentItem)
{
	m_parent = parent;
	m_corner = corner;
	m_resizing = false;

	initPixmapHash();

	setFlag(QGraphicsItem::ItemIgnoresTransformations);
	setFlag(QGraphicsItem::ItemClipsChildrenToShape);
	setAcceptHoverEvents(true);

	doSetVisible(false);
}

void CornerHandler::doSetVisible(bool visible) {
	if(visible) {
		setBrush(QBrush(QColor::fromRgb(0,250,0)));
	} else {
		setBrush(QBrush());
	}
	m_isVisible = visible;
}

void CornerHandler::initPixmapHash() {
	if(pixmapHash.isEmpty()) {
		pixmapHash[Qt::TopLeftCorner] =
			QPixmap(":/resources/images/itemselection/cornerHandlerActiveTopLeft.png");
		pixmapHash[Qt::TopRightCorner] =
		 	QPixmap(":/resources/images/itemselection/cornerHandlerActiveTopRight.png");
		pixmapHash[Qt::BottomRightCorner] =
		 	QPixmap(":/resources/images/itemselection/cornerHandlerActiveBottomRight.png");
		pixmapHash[Qt::BottomLeftCorner] =
		 	QPixmap(":/resources/images/itemselection/cornerHandlerActiveBottomLeft.png");
	}
}

void CornerHandler::resize(const QPointF &mousePos) {
	QRectF rect = m_parent->owner()->boundingRect();

	double oldX1 = rect.x();
	double oldY1 = rect.y();
	double oldX2 = oldX1+rect.width();
	double oldY2 = oldY1+rect.height();
	double newX = mapToItem(m_parent->owner(),mousePos).x();
	double newY = mapToItem(m_parent->owner(),mousePos).y();

	switch(m_corner) {
		case Qt::TopLeftCorner:
			m_parent->resizeRect(newX,newY,oldX2,oldY2); break;
		case Qt::TopRightCorner:
			m_parent->resizeRect(oldX1,newY,newX,oldY2); break;
		case Qt::BottomRightCorner:
			m_parent->resizeRect(oldX1,oldY1,newX,newY); break;
		case Qt::BottomLeftCorner:
			m_parent->resizeRect(newX,oldY1,oldX2,newY); break;
		default: break;
	}
}

Qt::Corner CornerHandler::corner() {
	return m_corner;
}

void CornerHandler::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
	if(m_parent->isResizable() && m_resizing) {
		resize(event->pos());
		scene()->update();
	} else {
		QGraphicsRectItem::mouseMoveEvent(event);
	}
}

void CornerHandler::mousePressEvent(QGraphicsSceneMouseEvent *event) {
	if(m_isVisible) {
		if(m_parent->isResizable()) {
			m_parent->resizingStarted();
			m_resizing = true;
			setFlag(QGraphicsItem::ItemIgnoresTransformations,false);
		} else {
			QGraphicsRectItem::mousePressEvent(event);
		}
	} else {
		QGraphicsRectItem::mousePressEvent(event);
	}
}

void CornerHandler::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
	if(m_parent->isResizable() && m_resizing) {
		setFlag(QGraphicsItem::ItemIgnoresTransformations,true);
		m_resizing = false;
		m_parent->resizingFinished();
	}
	QGraphicsRectItem::mouseReleaseEvent(event);
}

void CornerHandler::hoverEnterEvent(QGraphicsSceneHoverEvent * event) {
	QGraphicsRectItem::hoverEnterEvent(event);
	if(m_isVisible && m_parent->isResizable()) {
		setCursor(QCursor(cursorForCorner(m_corner)));
	}
}

void CornerHandler::hoverLeaveEvent(QGraphicsSceneHoverEvent * event) {
	if(m_isVisible && m_parent->isResizable()) {
		unsetCursor();
	}
	QGraphicsRectItem::hoverLeaveEvent(event);
}

Qt::CursorShape CornerHandler::cursorForCorner(Qt::Corner corner) {
	Qt::CursorShape cursorShape = Qt::SizeFDiagCursor;
	switch(corner) {
		case Qt::TopLeftCorner:
		case Qt::BottomRightCorner:
			cursorShape = Qt::SizeFDiagCursor;
			break;
		case Qt::TopRightCorner:
		case Qt::BottomLeftCorner:
			cursorShape = Qt::SizeBDiagCursor;
			break;
		default: 
			throw "CornerHandler::cursorForCorner unknown corner";
	}
	return cursorShape;
}

bool CornerHandler::isBeingDragged() {
	return m_resizing;
}

void CornerHandler::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	Q_UNUSED(option);
	Q_UNUSED(widget);
	if(m_isVisible && m_parent->isResizable()) {
		doPaint(painter);
	}
}

void CornerHandler::doPaint(QPainter *painter) {
	if(m_isVisible) {
		QRectF trgRect = m_parent->handlerRect(m_corner);
		//DebugDialog::debug("handler rect ", trgRect);
		QPixmap pm = pixmapHash[m_corner];
		QRectF srcRect = QRectF(pm.rect());
		//painter->drawRect(rect);
		painter->drawPixmap(trgRect,pm,srcRect);
	}
}

void CornerHandler::doSetRect(const QRectF &newRect) {
	setFlag(QGraphicsItem::ItemIgnoresTransformations,false);
	setRect(mapFromParent(newRect).boundingRect());
}
