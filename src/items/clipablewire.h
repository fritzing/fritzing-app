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

#ifndef CLIPABLEWIRE_H
#define CLIPABLEWIRE_H

#include "wire.h"

#include <QPointer>

class ClipableWire : public Wire
{

public:
	ClipableWire( ModelPart * modelPart, ViewLayer::ViewID,  const ViewGeometry & , long id, QMenu* itemMenu, bool initLabel); 
	
	void setClipEnds(bool);
	const QLineF & getPaintLine();	
	bool filterMousePressConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *);
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void hoverEnterConnectorItem(QGraphicsSceneHoverEvent * event, class ConnectorItem * item);
	void hoverLeaveConnectorItem(QGraphicsSceneHoverEvent * event, class ConnectorItem * item);
	void hoverMoveConnectorItem(QGraphicsSceneHoverEvent * event, class ConnectorItem * item);

protected:
	bool insideInnerCircle(ConnectorItem * connectorItem, QPointF localPos);
	bool insideSpoke(ClipableWire * wire, QPointF scenePos); 
	void dispatchHover(QPointF scenePos);
	void dispatchHoverAux(bool inInner, Wire * inWire);
	QPointF findIntersection(ConnectorItem * connectorItem, const QPointF & p);
	void calcClip(QPointF & p1, QPointF & p2, ConnectorItem * c1, ConnectorItem * c2);

protected:
	bool m_clipEnds;
	QLineF m_cachedLine;
	QLineF m_cachedOriginalLine;
	QGraphicsSceneMouseEvent * m_justFilteredEvent;
	QPointer<ConnectorItem> m_trackHoverItem;
	QPointer<ConnectorItem> m_trackHoverLastItem;
	QPointer<Wire> m_trackHoverLastWireItem;
};

#endif
