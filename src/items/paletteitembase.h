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

#ifndef PALETTEITEMBASE_H
#define PALETTEITEMBASE_H

#include <QGraphicsSvgItem>
#include <QGraphicsSceneMouseEvent>
#include <QSet>

#include "../model/modelpart.h"
#include "itembase.h"
#include "../utils/cursormaster.h"

class LayerKinPaletteItem;

class PaletteItemBase : public ItemBase, public CursorKeyListener
{
	Q_OBJECT

public:
	PaletteItemBase(ModelPart *, ViewLayer::ViewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu);

	void saveGeometry();
	bool itemMoved();
	virtual void saveInstanceLocation(QXmlStreamWriter &);
	void moveItem(ViewGeometry &);
	virtual void syncKinSelection(bool selected, PaletteItemBase *originator);
	virtual void syncKinMoved(QPointF offset, QPointF loc);
 	void blockItemSelectedChange(bool selected);
 	bool syncSelected();
 	QPointF syncMoved();
	void mousePressConnectorEvent(class ConnectorItem *, QGraphicsSceneMouseEvent *);
	bool acceptsMousePressConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *);
 	virtual bool setUpImage(ModelPart* modelPart, const LayerHash & viewLayers, LayerAttributes &);
	void connectedMoved(ConnectorItem * from, ConnectorItem * to, QList<ConnectorItem *> & already);
	bool collectFemaleConnectees(QSet<ItemBase *> & items);
	void collectWireConnectees(QSet<class Wire *> & wires);
    virtual bool mousePressEventK(PaletteItemBase * originalItem, QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEventK(PaletteItemBase * originalItem, QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEventK(PaletteItemBase * originalItem, QGraphicsSceneMouseEvent *event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
	void hoverEnterEvent(QGraphicsSceneHoverEvent * event );
	void hoverLeaveEvent(QGraphicsSceneHoverEvent * event );
	void hoverMoveEvent(QGraphicsSceneHoverEvent * event );
	void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
	//bool isBuriedConnectorHit(QGraphicsSceneMouseEvent *event);
	QRectF boundingRect() const;
	QRectF boundingRectWithoutLegs() const;
	QString retrieveSvg(ViewLayer::ViewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor);
	virtual bool canEditPart();
	QPainterPath shape() const;
	QPainterPath hoverShape() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	void paintHover(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	void paintSelected(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	bool collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide);
	void setProp(const QString & prop, const QString & value);
	const QCursor * getCursor(Qt::KeyboardModifiers);
	void cursorKeyEvent(Qt::KeyboardModifiers modifiers);

	/*
	// for debugging
	void setPos(const QPointF & pos);
	void setPos(double x, double y);
	*/
protected:
    static QString normalizeSvg(QString & svg, ViewLayer::ViewLayerID viewLayerID, bool blackOnly, double dpi, double & factor);

protected:
    void setUpConnectors(FSvgRenderer *, bool ignoreTerminalPoints);
	void findConnectorsUnder();
	virtual bool canFindConnectorsUnder();
	bool inRotationLocation(QPointF scenePos, Qt::KeyboardModifiers modifiers, QPointF & returnPoint);
	virtual bool freeRotationAllowed(Qt::KeyboardModifiers modifiers);
	bool freeRotationAllowed();
	void checkFreeRotation(Qt::KeyboardModifiers modifiers, QPointF scenePos);

	virtual LayerKinPaletteItem * newLayerKinPaletteItem(PaletteItemBase * chief, ModelPart *, const ViewGeometry &, long id, QMenu* itemMenu, const LayerHash &, LayerAttributes &);
    virtual void setInitialTransform(const QTransform &);
    virtual void cacheLoaded(const LayerAttributes &);

protected slots:
	void partPropertyEntry();

protected:
 	bool m_blockItemSelectedChange;
 	bool m_blockItemSelectedValue;
 	QPointF m_offset;
 	bool m_syncSelected;
 	QPointF m_syncMoved;
 	bool m_svg;
};


#endif
