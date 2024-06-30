/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2019 Fritzing

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

********************************************************************/

#ifndef LAYERKINPALETTEITEM_H
#define LAYERKINPALETTEITEM_H

#include "paletteitembase.h"
#include <QVariant>

class LayerKinPaletteItem : public PaletteItemBase
{
	Q_OBJECT

public:
	explicit LayerKinPaletteItem(PaletteItemBase * chief, ModelPart *, ViewLayer::ViewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu);
	void setOffset(double x, double y);
	ItemBase * layerKinChief();
	bool ok();
	void setHidden(bool hidden);
	void setInactive(bool inactivate);
	void clearModelPart();
	void initLKPI(LayerAttributes &, const LayerHash &viewLayers);
	bool isSticky();
	bool isBaseSticky();
	void setSticky(bool);
	void addSticky(ItemBase *, bool stickem);
	ItemBase * stickingTo();
	QList<QPointer <ItemBase> > stickyList();
	bool alreadySticking(ItemBase * itemBase);
	bool stickyEnabled();
	void resetID();
	QString retrieveSvg(ViewLayer::ViewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor);
	bool isSwappable();
	void setSwappable(bool);
	bool inRotation();
	void setInRotation(bool);

protected:
	QVariant itemChange(GraphicsItemChange change, const QVariant &value);
	void updateConnections(bool includeRatsnest, QList<ConnectorItem *> & already);
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
	ConnectorItem* newConnectorItem(class Connector *connector);

protected:
	QPointer<PaletteItemBase> m_layerKinChief;
	bool m_ok;
	bool m_passMouseEvents;
};

struct TextThing {
	int minX;
	int maxX;
	int minY;
	int maxY;
	QRectF newRect;
	QTransform flipMatrix;
};

class SchematicTextLayerKinPaletteItem : public LayerKinPaletteItem
{
	Q_OBJECT

public:
	SchematicTextLayerKinPaletteItem(PaletteItemBase * chief, ModelPart *, ViewLayer::ViewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu);

	QString getTransformedSvg(const QString & svgToTransform, double & rotation);
	void transformItem(const QTransform &, bool includeRatsnest);
	void clearTextThings();
	void setTransform2(const QTransform &);

protected:
	QString rotate(const QString & svg, bool isFlipped);
	QString flipTextSvg(const QString & textSvg);
	void positionTexts(QList<QDomElement> & texts);
	void initTextThings();
	void setInitialTransform(const QTransform &);
	void cacheLoaded(const LayerAttributes &);

protected:
	bool m_flipped;
	QList<TextThing> m_textThings;
};


#endif
