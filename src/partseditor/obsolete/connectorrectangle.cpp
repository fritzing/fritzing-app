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
#include <QPainter>
#include <QHash>

#include "connectorrectangle.h"
#include "../sketch/zoomablegraphicsview.h"
#include "../debugdialog.h"


double ConnectorRectangle::ErrorIconSize = 6;

ConnectorRectangle::ConnectorRectangle(QGraphicsItem* owner, bool withHandlers)
: QObject()
{
	m_minWidth = m_minHeight = 0;
	m_owner = owner;
	m_firstPaint = true;

	if(withHandlers) {
		m_topLeftHandler     = new CornerHandler(this, owner, Qt::TopLeftCorner);
		m_topRightHandler    = new CornerHandler(this, owner, Qt::TopRightCorner);
		m_bottomRightHandler = new CornerHandler(this, owner, Qt::BottomRightCorner);
		m_bottomLeftHandler  = new CornerHandler(this, owner, Qt::BottomLeftCorner);

		m_cornerHandlers
			<< m_topLeftHandler << m_topRightHandler
			<< m_bottomRightHandler << m_bottomLeftHandler;

		setHandlersVisible(false);
	} else {
		m_topLeftHandler = m_topRightHandler =
		m_bottomRightHandler = m_bottomLeftHandler = NULL;
	}
}

ConnectorRectangle::~ConnectorRectangle() {
}

void ConnectorRectangle::setHandlersVisible(bool visible) {
	foreach(CornerHandler* handler, m_cornerHandlers) {
		handler->doSetVisible(visible);
	}
}

void ConnectorRectangle::resizeRect(double x1, double y1, double x2, double y2) {

	double width = x2-x1 < m_minWidth ? m_minWidth : x2-x1;
	double height = y2-y1 < m_minHeight ? m_minHeight : y2-y1;

	QRectF rect = owner()->boundingRect();
	if(width != rect.width() && height != rect.height()) {
		emit resizeSignal(x1,y1,width,height);
	}
}

bool ConnectorRectangle::isResizable() {
	bool resizable;
	emit isResizableSignal(resizable);				// must be connected via Qt::DirectConnection
	return resizable;

}

void ConnectorRectangle::paint(QPainter *painter) {
	QRectF rect = m_owner->boundingRect();

	if(m_firstPaint && rect.width() > 0 && rect.height() > 0) {
		placeHandlers();
		m_firstPaint = false;
	}

	bool beingResized = false;
	foreach(CornerHandler* handler, m_cornerHandlers) {
		if(handler->isBeingDragged()) {
			beingResized = true;
			break;
		}
	}

	if(beingResized) {
		foreach(CornerHandler* handler, m_cornerHandlers) {
			handler->doPaint(painter);
		}
	}
}

void ConnectorRectangle::resizingStarted() {
	foreach(CornerHandler* handler, m_cornerHandlers) {
		handler->doSetVisible(false);
	}
}

void ConnectorRectangle::resizingFinished() {
	foreach(CornerHandler* handler, m_cornerHandlers) {
		handler->doSetVisible(true);
		setHandlerRect(handler);
	}
}

void ConnectorRectangle::placeHandlers() {
	foreach(CornerHandler* handler, m_cornerHandlers) {
		setHandlerRect(handler);
	}
}

void ConnectorRectangle::setHandlerRect(CornerHandler* handler) {
	handler->doSetRect(handlerRect(handler->corner()));
}

QRectF ConnectorRectangle::handlerRect(Qt::Corner corner) {
	QRectF rect = m_owner->boundingRect();
	double scale = currentScale();
	//DebugDialog::debug(QString("scale %1").arg(scale));
	QPointF offset(CornerHandler::Size/scale,CornerHandler::Size/scale);
	QPointF cornerPoint;
	switch(corner) {
		case Qt::TopLeftCorner:
			cornerPoint=rect.topLeft();
			break;
		case Qt::TopRightCorner:
			cornerPoint=rect.topRight();
			break;
		case Qt::BottomRightCorner:
			cornerPoint=rect.bottomRight();
			break;
		case Qt::BottomLeftCorner:
			cornerPoint=rect.bottomLeft();
			break;
		default: 
			throw "ConnectorRectangle::handlerRect: unknown corner";
	}
	return QRectF(cornerPoint-offset,cornerPoint+offset);
}

QRectF ConnectorRectangle::errorIconRect() {
	QRectF rect = m_owner->boundingRect();
	double scale = currentScale();
	QPointF offset(ErrorIconSize/scale,ErrorIconSize/scale);
	QPointF cornerPoint = rect.topLeft()-offset;
	return QRectF(cornerPoint-offset,cornerPoint+offset);
}

double ConnectorRectangle::currentScale() {
	if(m_owner->scene()) {
		ZoomableGraphicsView *sw = dynamic_cast<ZoomableGraphicsView*>(m_owner->scene()->parent());
		if(sw) {
			return sw->currentZoom()/100;
		}
	}
	return 1;
}

QGraphicsItem *ConnectorRectangle::owner() {
	return m_owner;
}


void ConnectorRectangle::setMinSize(double minWidth, double minHeight) {
	m_minWidth = minWidth;
	m_minHeight = minHeight;
}

