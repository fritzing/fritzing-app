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

$Revision: 6994 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-27 14:25:24 +0200 (Sa, 27. Apr 2013) $

********************************************************************/

#ifndef JUMPERITEM_H
#define JUMPERITEM_H

#include "paletteitem.h"

class JumperItem : public PaletteItem
{
	Q_OBJECT

public:
	JumperItem( ModelPart * modelPart, ViewLayer::ViewID,  const ViewGeometry & , long id, QMenu* itemMenu, bool doLabel); 
	~JumperItem();

    QPainterPath shape() const;
    QPainterPath hoverShape() const;
 	bool setUpImage(ModelPart* modelPart, const LayerHash & viewLayers, LayerAttributes &);
	void saveParams();
	void getParams(QPointF & pos, QPointF & c0, QPointF & c1);
	void resize(QPointF pos, QPointF c0, QPointF c1);
	void resize(QPointF p0, QPointF p1);   
	QSizeF footprintSize();
	QString retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor);
	bool getAutoroutable();
	void setAutoroutable(bool);
	class ConnectorItem * connector0();
	class ConnectorItem * connector1();
	bool hasCustomSVG();
	bool inDrag();
	void loadLayerKin( const LayerHash & viewLayers, ViewLayer::ViewLayerPlacement);
	PluralType isPlural();
	void addedToScene(bool temporary);
	void rotateItem(double degrees, bool includeRatsnest);
	void calcRotation(QTransform & rotation, QPointF center, ViewGeometry &);
	QPointF dragOffset();
	void saveInstanceLocation(QXmlStreamWriter & streamWriter);
	bool hasPartNumberProperty();
	QRectF boundingRect() const;
	bool mousePressEventK(PaletteItemBase * originalItem, QGraphicsSceneMouseEvent *);
	void mouseMoveEventK(PaletteItemBase * originalItem, QGraphicsSceneMouseEvent *);
	void mouseReleaseEventK(PaletteItemBase * originalItem, QGraphicsSceneMouseEvent *);

protected:
	void resize();
	QString makeSvg(ViewLayer::ViewLayerID);
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
	void resizeAux(double r0x, double r0y, double r1x, double r1y);
	void rotateEnds(QTransform & rotation, QPointF & tc0, QPointF & tc1); 
	QPointF calcPos(QPointF p0, QPointF p1);
	void initialResize(ViewLayer::ViewID);
	void paintHover(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	void paintSelected(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    ViewLayer::ViewID useViewIDForPixmap(ViewLayer::ViewID, bool swappingEnabled);

signals:
	void alignMe(JumperItem *, QPointF & p); 

protected:
	QPointer<ConnectorItem> m_dragItem;
	QPointer<ConnectorItem> m_connector0;
	QPointer<ConnectorItem> m_connector1;
	QPointer<ConnectorItem> m_otherItem;
	QPointF m_dragStartScenePos;
	QPointF m_dragStartThisPos;
	QPointF m_dragStartConnectorPos;
	QPointF m_dragStartCenterPos;
	QPointF m_otherPos;
	QPointF m_connectorTL;
	QPointF m_connectorBR;
	QPointF m_itemPos;
	QPointF m_itemC0;
	QPointF m_itemC1;
    QPointer<PaletteItemBase> m_originalClickItem;
};

#endif
