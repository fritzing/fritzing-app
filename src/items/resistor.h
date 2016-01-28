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

#ifndef RESISTOR_H
#define RESISTOR_H

#include <QRectF>
#include <QPainterPath>
#include <QPixmap>
#include <QVariant>

#include "capacitor.h"

class Resistor : public Capacitor 
{
	Q_OBJECT

public:
	// after calling this constructor if you want to render the loaded svg (either from model or from file), MUST call <renderImage>
	Resistor(ModelPart *, ViewLayer::ViewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel);
	~Resistor();

	QString retrieveSvg(ViewLayer::ViewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor);
	bool collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide);
	QString getProperty(const QString & key);
	void setResistance(QString resistance, QString pinSpacing, bool force);
	QString resistance();
	QString pinSpacing();
    const QString & title();
	bool hasCustomSVG();
	bool canEditPart();
	PluralType isPlural();
	void addedToScene(bool temporary);
	void setProp(const QString & prop, const QString & value);
 	bool setUpImage(ModelPart* modelPart, const LayerHash & viewLayers, LayerAttributes &);

protected:
	QString makeSvg(const QString & ohms, ViewLayer::ViewLayerID viewLayerID);
	void updateResistances(QString r);
	ConnectorItem* newConnectorItem(class Connector *connector);
	ConnectorItem* newConnectorItem(ItemBase * layerkin, Connector *connector);
	QStringList collectValues(const QString & family, const QString & prop, QString & value);
	void setBands(QDomElement & element, int firstband, int secondband, int thirdband, int multiplier, const QString & tolerance);
    ViewLayer::ViewID useViewIDForPixmap(ViewLayer::ViewID, bool swappingEnabled);

public slots:
	void resistanceEntry(const QString & text);

public:
	static double toOhms(const QString & ohmsString, void * data);

protected:
	QString m_ohms;
	QString m_pinSpacing;
	QString m_title;
	bool m_changingPinSpacing;
	QString m_iconSvgFile;
	QString m_breadboardSvgFile;
};

#endif
