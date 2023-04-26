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

#include "led.h"
#include "../utils/graphicsutils.h"
#include "../utils/textutils.h"
#include "../fsvgrenderer.h"
#include "../sketch/infographicsview.h"
#include "../svg/svgfilesplitter.h"
#include "../commands.h"
#include "../layerattributes.h"
#include "moduleidnames.h"
#include "partlabel.h"

static QHash<QString, QString> BreadboardSvg;
static QHash<QString, QString> IconSvg;

LED::LED( ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: Capacitor(modelPart, viewID, viewGeometry, id, itemMenu, doLabel)
{
}

LED::~LED() {
	if (m_ledLight) {
		delete m_ledLight;
		m_ledLight = nullptr;
	}
}

QString LED::retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor)
{
	switch (viewLayerID) {
	case ViewLayer::Breadboard:
	case ViewLayer::Icon:
		break;
	default:
		return Capacitor::retrieveSvg(viewLayerID, svgHash, blackOnly, dpi, factor);
	}

	QString svg = getColorSVG(prop("color"), viewLayerID);
	if (svg.isEmpty()) return "";

	return PaletteItemBase::normalizeSvg(svg, viewLayerID, blackOnly, dpi, factor);
}

void LED::addedToScene(bool temporary)
{
	//Check if this an RGB led
	QString rgbString = this->getProperty("rgb");
	if (this->scene() && rgbString.isEmpty()) {
		//Set the color for the LED, does not apply if is is an RGB led
		setColor(prop("color"));
	}

	return Capacitor::addedToScene(temporary);
}

bool LED::hasCustomSVG() {
	switch (m_viewID) {
	case ViewLayer::BreadboardView:
	case ViewLayer::IconView:
		return true;
	default:
		return ItemBase::hasCustomSVG();
	}
}

bool LED::canEditPart() {
	return true;
}

ItemBase::PluralType LED::isPlural() {
	return Plural;
}

void LED::setProp(const QString & prop, const QString & value)
{
	Capacitor::setProp(prop, value);

	if (prop.compare("color") == 0) {
		setColor(value);
	}
}

void LED::slamColor(QDomElement & element, const QString & colorString)
{
	QString id = element.attribute("id");
	if (id.startsWith("color_")) {
		element.setAttribute("fill", colorString);
	}

	QDomElement child = element.firstChildElement();
	while (!child.isNull()) {
		slamColor(child, colorString);
		child = child.nextSiblingElement();
	}
}

void LED::setColor(const QString & color)
{
	switch (m_viewLayerID) {
	case ViewLayer::Breadboard:
	case ViewLayer::Icon:
		break;
	default:
		return;
	}

	reloadRenderer(getColorSVG(color, m_viewLayerID),true);

	QString title = modelPart()->modelPartShared()->title();  // bypass any local title by going to modelPartShared
	if (title.startsWith("red", Qt::CaseInsensitive)) {
		title.remove(0, 3);
		QStringList strings = color.split(" ");
		modelPart()->setLocalTitle(strings.at(0) + title);
	}
	else if (title.endsWith("blue", Qt::CaseInsensitive)) {
		title.chop(4);
		QStringList strings = color.split(" ");
		modelPart()->setLocalTitle(title + strings.at(0));
	}
}

/**
 * Resets the brightness of an LED to its original specification.
 * @brief Restores the original brightness of an LED.
 */
void LED::resetBrightness() {
	setBrightness((1-offColor)/brigtnessMultiplier);
	if(m_ledLight && this->viewID()==ViewLayer::ViewID::BreadboardView) {
		m_ledLight->hide();
	}
}

/**
 * Changes the brightness of an LED based on a brightness parameter.
 * The color is the specfied in the LED file and it can  get 30% brighter than that
 * color if brightness is equal to 1. A brightness of 0 reduces scales the color to
 * 30% of the original color (offColor).
 *
 * @brief Changes the brightness of an LED.
 * @param[in] brightness The brightness value, its range is from 0 (off) to 1 (on)
 */
void LED::setBrightness(double brightness) {
	QString errorStr;
	int errorLine;
	int errorColumn;
	QDomDocument domDocument;
	if (!domDocument.setContent( BreadboardSvg.value(m_filename), &errorStr, &errorLine, &errorColumn)) {
		return;
	}

	//get the color of the LED
	QString colorString;
	QString rgbString = this->getProperty("rgb");
	if (rgbString.isEmpty()) {
		//If it is just one LED, use color property
		QString color = prop("color");
		foreach (PropertyDef * propertyDef, m_propertyDefs.keys()) {
			if (propertyDef->name.compare("color") == 0) {
				colorString = propertyDef->adjuncts.value(color, "");
				break;
			}
		}
		if (colorString.isEmpty()) return;
	} else {
		// It is an RGB led, set color to grey
		colorString = "#E6E6E6";
	}

	int red = colorString.mid(1,2).toInt(nullptr, 16);
	int green = colorString.mid(3,2).toInt(nullptr, 16);
	int blue = colorString.mid(5,2).toInt(nullptr, 16);

	if (brightness > 1) brightness = 1;
	if (brightness < 0) brightness = 0;

	//Find the new color values
	//The color achives maximum intensity when brightness = 1/brigtnessMultiplier
	double brightnessColor = brightness * brigtnessMultiplier;
	if (brightnessColor > 1) brightnessColor = 1;
	red = offColor*red + brightnessColor*red;
	green = offColor*green + brightnessColor*green;
	blue = offColor*blue + brightnessColor*blue;
	if(red > 255) red = 255;
	if(green > 255) green = 255;
	if(blue > 255) blue = 255;

	QString newColorStr = QString("#%1%2%3")
			.arg(red, 2, 16)
			.arg(green, 2, 16)
			.arg(blue, 2, 16);
	newColorStr.replace(' ', '0');

	//Change the color of the LED
	QDomElement root = domDocument.documentElement();
	slamColor(root, newColorStr);
	reloadRenderer(domDocument.toString(),true);

	//Add light comming out of the LED in BreadboardView
	if(this->viewID()==ViewLayer::ViewID::BreadboardView) {
		if(!m_ledLight) {
			m_ledLight = new LedLight(this);
		}
		m_ledLight->setLight(brightness, red, green, blue);
	}
}

/**
 * Changes the brightness of an RGB LED based on 3 brightness parameters.
 * A brightness of 0 reduces scales the color to 30% of the color.
 *
 * @brief Changes the brightness of an RGB LED.
 * @param[in] brightness The brightness values, its range is from 0 (off) to 1 (on)
 */
void LED::setBrightnessRGB(double brightnessR, double brightnessG, double brightnessB) {
	QString errorStr;
	int errorLine;
	int errorColumn;
	QDomDocument domDocument;
	if (!domDocument.setContent( BreadboardSvg.value(m_filename), &errorStr, &errorLine, &errorColumn)) {
		return;
	}

	//The color achives maximum intensity when brightness = 0.25
	int red = 40 + (brightnessR  * brigtnessMultiplier * 200.0);
	int green = 40 + (brightnessG * brigtnessMultiplier * 200.0);
	int blue = 40 + (brightnessB * brigtnessMultiplier * 200.0);
	double brightness = std::max({brightnessR, brightnessG, brightnessB});

	int maxColor = 230; //Do not saturate the colors
	if (red > 255) red = maxColor;
	if (red < 0) red = 0;
	if (green > 255) green = maxColor;
	if (green < 0) green = 0;
	if (blue > 255) blue = maxColor;
	if (blue < 0) blue = 0;

	QString newColorStr = QString("#%1%2%3")
			.arg(red, 2, 16)
			.arg(green, 2, 16)
			.arg(blue, 2, 16);
	newColorStr.replace(' ', '0');

	//Change the color of the LED
	QDomElement root = domDocument.documentElement();
	slamColor(root, newColorStr);
	reloadRenderer(domDocument.toString(),true);

	//Add light comming out of the LED in BreadboardView
	if(this->viewID()==ViewLayer::ViewID::BreadboardView) {
		if(!m_ledLight) {
			m_ledLight = new LedLight(this);
		}
		m_ledLight->setLight(brightness, red, green, blue);
	}
}

QString LED::getColorSVG(const QString & color, ViewLayer::ViewLayerID viewLayerID)
{
	QString errorStr;
	int errorLine;
	int errorColumn;
	QDomDocument domDocument;
	if (!domDocument.setContent(viewLayerID == ViewLayer::Breadboard ? BreadboardSvg.value(m_filename) : IconSvg.value(m_filename), &errorStr, &errorLine, &errorColumn)) {
		return "";
	}

	QString colorString;
	foreach (PropertyDef * propertyDef, m_propertyDefs.keys()) {
		if (propertyDef->name.compare("color") == 0) {
			colorString = propertyDef->adjuncts.value(color, "");
			break;
		}
	}

	if (colorString.isEmpty()) return "";

	QDomElement root = domDocument.documentElement();
	slamColor(root, colorString);
	return domDocument.toString();
}

bool LED::setUpImage(ModelPart * modelPart, const LayerHash & viewLayers, LayerAttributes & layerAttributes)
{
	bool result = Capacitor::setUpImage(modelPart, viewLayers, layerAttributes);
	if (layerAttributes.viewLayerID == ViewLayer::Breadboard && BreadboardSvg.value(m_filename).isEmpty() && result) {
		BreadboardSvg.insert(m_filename, QString(layerAttributes.loaded()));
	}
	else if (layerAttributes.viewLayerID == ViewLayer::Icon && IconSvg.value(m_filename).isEmpty() && result) {
		IconSvg.insert(m_filename, QString(layerAttributes.loaded()));
	}
	return result;
}

const QString & LED::title() {
	m_title = prop("color") + " LED";
	return m_title;
}

ViewLayer::ViewID LED::useViewIDForPixmap(ViewLayer::ViewID vid, bool swappingEnabled) {
	if (swappingEnabled && vid == ViewLayer::BreadboardView) {
		return vid;
	}

	return ItemBase::useViewIDForPixmap(vid, swappingEnabled);
}

QHash<QString, QString> LED::prepareProps(ModelPart * modelPart, bool wantDebug, QStringList & keys)
{
	QHash<QString, QString> props = ItemBase::prepareProps(modelPart, wantDebug, keys);

	// ensure color and other properties are after family;
	if (keys.removeOne("color")) keys.insert(1, "color");
	if (keys.removeOne("current")) keys.insert(2, "current");

	return props;
}
