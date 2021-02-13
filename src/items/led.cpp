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
	if (this->scene()) {
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
	setBrightness(1-offColor);
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
void LED::setBrightness(double brightness){
	QString errorStr;
	int errorLine;
	int errorColumn;
	QDomDocument domDocument;
	if (!domDocument.setContent( BreadboardSvg.value(m_filename), &errorStr, &errorLine, &errorColumn)) {
		return;
	}

	//get the color of the LED
	QString colorString;
	QString color = prop("color");
	foreach (PropertyDef * propertyDef, m_propertyDefs.keys()) {
		if (propertyDef->name.compare("color") == 0) {
			colorString = propertyDef->adjuncts.value(color, "");
			break;
		}
	}
	if (colorString.isEmpty()) return;

	int red = colorString.mid(1,2).toInt(nullptr, 16);
	int green = colorString.mid(3,2).toInt(nullptr, 16);
	int blue = colorString.mid(5,2).toInt(nullptr, 16);

	if (brightness > 1) brightness = 1;
	if (brightness < 0) brightness = 0;

	//Find the new color values
	red = offColor*red + brightness*red;
	green = offColor*green + brightness*green;
	blue = offColor*blue + brightness*blue;
	if(red > 255) red = 255;
	if(green > 255) red = 255;
	if(blue > 255) red = 255;

	QString newColorStr = QString("#%1%2%3")
			.arg(red, 2, 16)
			.arg(green, 2, 16)
			.arg(blue, 2, 16);
	newColorStr.replace(' ', '0');

	//Change the color of the LED
	QDomElement root = domDocument.documentElement();
	slamColor(root, newColorStr);
	reloadRenderer(domDocument.toString(),true);
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
