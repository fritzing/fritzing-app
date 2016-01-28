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

$Revision: 6980 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-22 01:45:43 +0200 (Mo, 22. Apr 2013) $

********************************************************************/

#ifndef LED_H
#define LED_H

#include <QRectF>
#include <QPainterPath>
#include <QPixmap>
#include <QVariant>

#include "capacitor.h"

class LED : public Capacitor 
{
	Q_OBJECT

public:
	// after calling this constructor if you want to render the loaded svg (either from model or from file), MUST call <renderImage>
	LED(ModelPart *, ViewLayer::ViewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel);
	~LED();

	QString retrieveSvg(ViewLayer::ViewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor);
	bool hasCustomSVG();
	bool canEditPart();
	PluralType isPlural();
	void addedToScene(bool temporary);
	void setProp(const QString & prop, const QString & value);
 	bool setUpImage(ModelPart* modelPart, const LayerHash & viewLayers, LayerAttributes &);
	const QString & title();
    ViewLayer::ViewID useViewIDForPixmap(ViewLayer::ViewID, bool swappingEnabled);

protected:
	void setColor(const QString & color);
	void slamColor(QDomElement & element, const QString & colorString);
	QString getColorSVG(const QString & color, ViewLayer::ViewLayerID);

protected:
	QString m_title;

};

#endif
