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

$Revision: 6984 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-22 23:44:56 +0200 (Mo, 22. Apr 2013) $

********************************************************************/

#ifndef HOLE_H
#define HOLE_H

#include <QRectF>
#include <QPointF>
#include <QPainterPath>
#include <QPixmap>
#include <QVariant>

#include "paletteitem.h"


class Hole : public PaletteItem 
{
	Q_OBJECT

public:
	// after calling this constructor if you want to render the loaded svg (either from model or from file), MUST call <renderImage>
	Hole(ModelPart *, ViewLayer::ViewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel);
	~Hole();

	QString getProperty(const QString & key);
	void setProp(const QString & prop, const QString & value);
	void setHoleSize(QString holeSize, bool force);

	bool collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide);
	bool hasCustomSVG();
	PluralType isPlural();
	QString retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor); 
	bool canEditPart();
	void addedToScene(bool temporary);	
	bool hasPartNumberProperty();
	QString holeSize();
	bool rotationAllowed();
	bool rotation45Allowed();
	bool canFindConnectorsUnder();
    QRectF trueSceneBoundingRect();

protected slots:
	void changeHoleSize(const QString &);
	void changeUnits(bool);

protected:
	QString makeSvg(const QString & holeDiameter, const QString & ringThickness, ViewLayer::ViewLayerID, bool includeHole);
	virtual QString makeID();
	ItemBase * setBothSvg(const QString & holeDiameter, const QString & ringThickness); 
	void setBothNonConnectors(ItemBase * itemBase, SvgIdLayer * svgIdLayer);
	virtual void setBoth(const QString & holeDiameter, const QString &  thickness);
	QRectF getRect(const QString & newSize);
    ViewLayer::ViewID useViewIDForPixmap(ViewLayer::ViewID, bool swappingEnabled);

public:
	static const double OffsetPixels;
};

#endif
