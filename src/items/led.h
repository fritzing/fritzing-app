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

#ifndef LED_H
#define LED_H

#include <QRectF>
#include <QPainterPath>
#include <QPixmap>
#include <QVariant>

#include "capacitor.h"

class LedLight : public QGraphicsEllipseItem {
public:
	LedLight(QGraphicsItem *parent = nullptr) : QGraphicsEllipseItem(parent) {
		setFlag(QGraphicsItem::ItemStacksBehindParent);
		setPen(Qt::NoPen);
	};
	~LedLight() {
	};
	void setLight(double brightness, int red, int green, int blue){
		//Only set light coming out if brightness is bigger than 0.25
		if (brightness < 0.25)
			brightness = 0.0;
		Capacitor* led = dynamic_cast<Capacitor *>(parentItem());
		if (led) {
			double radious = std::min(led->boundingRectWithoutLegs().width()/2,
									  led->boundingRectWithoutLegs().height()/2) * brightness * 4;
			prepareGeometryChange();
			setRect(-radious + led->boundingRectWithoutLegs().width()/2,
					-radious + led->boundingRectWithoutLegs().width()/2,
					radious*2, radious*2);
			QRadialGradient gradient = QRadialGradient(0.5, 0.5, 0.5);
			gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
			gradient.setColorAt(0, QColor(red, green, blue, 255));
			gradient.setColorAt(0.3, QColor(red, green, blue, 230));
			gradient.setColorAt(1, QColor(red, green, blue, 0));
			setBrush(gradient);
			this->show();
		}
	};
};

class LED : public Capacitor
{
	Q_OBJECT

	//The color that remains when the brightness is set to 0
	static constexpr double offColor = 0.3;

	//The maximum color is achived when brigtness*brigtnessMultiplier=1
	//After that, more current increases the light comming out of the LED,
	//but it does not change the color
	static constexpr double brigtnessMultiplier = 4.0;

public:
	// after calling this constructor if you want to render the loaded svg (either from model or from file), MUST call <renderImage>
	explicit LED(ModelPart *, ViewLayer::ViewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel);
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
	void setBrightness(double);
	void setBrightnessRGB(double brightnessR, double brightnessG, double brightnessB);
	void resetBrightness();

protected:
	void setColor(const QString & color);
	void slamColor(QDomElement & element, const QString & colorString);
	QString getColorSVG(const QString & color, ViewLayer::ViewLayerID);

protected:
	QString m_title;
	LedLight  * m_ledLight = nullptr;


};

#endif
