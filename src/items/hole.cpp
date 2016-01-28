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

#include "hole.h"
#include "../utils/graphicsutils.h"
#include "../fsvgrenderer.h"
#include "../sketch/infographicsview.h"
#include "../svg/svgfilesplitter.h"
#include "../commands.h"
#include "../utils/textutils.h"
#include "../viewlayer.h"
#include "partlabel.h"
#include "../connectors/nonconnectoritem.h"
#include "../connectors/svgidlayer.h"

#include <QDomNodeList>
#include <QDomDocument>
#include <QDomElement>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QSpacerItem>
#include <QGroupBox>
#include <QSettings>

const double Hole::OffsetPixels = 2;

static HoleClassThing TheHoleThing;

//////////////////////////////////////////////////

Hole::Hole( ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: PaletteItem(modelPart, viewID, viewGeometry, id, itemMenu, doLabel)
{
	PaletteItem::setUpHoleSizes("hole", TheHoleThing);
}

Hole::~Hole() {
}

void Hole::setProp(const QString & prop, const QString & value) {
	if (prop.compare("hole size", Qt::CaseInsensitive) == 0) {
		setHoleSize(value, false);
		return;
	}

	PaletteItem::setProp(prop, value);
}

QString Hole::holeSize() {
	return m_holeSettings.holeSize();
}


void Hole::setHoleSize(QString newSize, bool force) {
	//DebugDialog::debug(QString("old holesize %1").arg(viewID()) + holeSize(), sceneBoundingRect());
	//foreach (QGraphicsItem * childItem, childItems()) {
	//	DebugDialog::debug(QString("   child"), childItem->sceneBoundingRect());
	//}

	if (PaletteItem::setHoleSize(newSize, force, m_holeSettings)) {
		setBoth(m_holeSettings.holeDiameter, m_holeSettings.ringThickness);
		modelPart()->setLocalProp("hole size", newSize);

		if (m_partLabel) m_partLabel->displayTextsIf();	
	}
	//DebugDialog::debug(QString("new holesize %1 ").arg(viewID()) + holeSize(), sceneBoundingRect());
	//foreach (QGraphicsItem * childItem, childItems()) {
	//	DebugDialog::debug(QString("   child"), childItem->sceneBoundingRect());
	//}
}

QRectF Hole::getRect(const QString & newSize) {
	QString s = newSize;
	QStringList sizes = getSizes(s, m_holeSettings);
	if (sizes.count() < 2) return boundingRect();

	double diameter = TextUtils::convertToInches(sizes.at(0));
	double ringThickness = TextUtils::convertToInches(sizes.at(1));
	double dim = (diameter + ringThickness + ringThickness) * GraphicsUtils::SVGDPI;
	//DebugDialog::debug(QString("get rect %1 %2").arg(newSize).arg(dim));
	return QRectF(0, 0, dim, dim);
}

void Hole::setBoth(const QString & holeDiameter, const QString & ringThickness) {
	if (this->m_viewID != ViewLayer::PCBView) return;

	ItemBase * otherLayer = setBothSvg(holeDiameter, ringThickness);

	// there's only one NonConnectorItem
	foreach (SvgIdLayer * svgIdLayer, fsvgRenderer()->setUpNonConnectors(viewLayerPlacement())) {
		if (svgIdLayer == NULL) continue;

		setBothNonConnectors(this, svgIdLayer);
		if (otherLayer != NULL) {
			setBothNonConnectors(otherLayer, svgIdLayer);
		}

		delete svgIdLayer;
	}
}

ItemBase * Hole::setBothSvg(const QString & holeDiameter, const QString & ringThickness) 
{
	QString svg = makeSvg(holeDiameter, ringThickness, m_viewLayerID, true);
	resetRenderer(svg);
	//DebugDialog::debug("both");
	//DebugDialog::debug(svg);

	QString setColor;
	QStringList noIDs;

	QString osvg;
	ItemBase * otherLayer = NULL;
	foreach (ItemBase * layerKin, m_layerKin) {
		if (layerKin->hasNonConnectors()) {
			otherLayer = layerKin;
			break;
		}
	}

	if (otherLayer) {
		osvg = makeSvg(holeDiameter, ringThickness, otherLayer->viewLayerID(), true);
		//DebugDialog::debug(osvg);
		otherLayer->resetRenderer(osvg);
	}

	//DebugDialog::debug("other");
	
	return otherLayer;
}

void Hole::setBothNonConnectors(ItemBase * itemBase, SvgIdLayer * svgIdLayer) {
	foreach (QGraphicsItem * child, itemBase->childItems()) {
		NonConnectorItem * nonConnectorItem = dynamic_cast<NonConnectorItem *>(child);
		if (nonConnectorItem == NULL) continue;

		//DebugDialog::debug(QString("hole set rect %1").arg(m_viewID), svgIdLayer->m_rect);
		nonConnectorItem->setRect(svgIdLayer->rect(viewLayerPlacement()));
		nonConnectorItem->setRadius(svgIdLayer->m_radius, svgIdLayer->m_strokeWidth);
		break;
	}
}


QString Hole::makeSvg(const QString & holeDiameter, const QString & ringThickness, ViewLayer::ViewLayerID viewLayerID, bool includeHole) 
{
	double offsetDPI = OffsetPixels / GraphicsUtils::SVGDPI;
	double hd = TextUtils::convertToInches(holeDiameter);
	double rt = TextUtils::convertToInches(ringThickness);

	double wInches = hd + rt + rt + offsetDPI + offsetDPI;
	QString svg = TextUtils::makeSVGHeader(1, GraphicsUtils::StandardFritzingDPI, wInches, wInches);

	hd *= GraphicsUtils::StandardFritzingDPI;
	rt *= GraphicsUtils::StandardFritzingDPI;
	offsetDPI *= GraphicsUtils::StandardFritzingDPI;

	QString setColor;
	if (viewLayerID == ViewLayer::Copper0) {
		setColor = ViewLayer::Copper0Color;
	}
	else if (viewLayerID == ViewLayer::Copper1) {
		setColor = ViewLayer::Copper1Color;
	}

	svg += QString("<g id='%1'>").arg(ViewLayer::viewLayerXmlNameFromID(viewLayerID));
	
	QString id = makeID();
	if (hd == 0) {
		svg += QString("<circle cx='%1' cy='%1' r='%2' fill='%3' id='%4' />")
					.arg(rt + offsetDPI)
					.arg(rt)
					.arg(setColor)
					.arg(id);
	}
	else {
		svg += QString("<circle fill='%6' cx='%1' cy='%1' r='%2' stroke-width='%3' stroke='%4' id='%5' />")
			.arg((hd / 2) + rt + offsetDPI)
			.arg((hd / 2) + (rt / 2))
			.arg(rt)
			.arg(setColor)
			.arg(id)
            .arg(rt == 0 ? "black" : "none")
            ;
        if (includeHole) {
		    svg += QString("<circle drill='0' fill='black' cx='%1' cy='%1' r='%2' stroke-width='0'  />")   // set the drill attribute to 0 for gerber translation
			    .arg((hd / 2) + rt + offsetDPI)
			    .arg(hd / 2);
        }
	}
  		
	svg += "</g></svg>";
	//DebugDialog::debug("hole svg " + svg);
	return svg;
}

QString Hole::makeID() {
	return FSvgRenderer::NonConnectorName + "0";
}

QString Hole::getProperty(const QString & key) {
	if (key.compare("hole size", Qt::CaseInsensitive) == 0) {
		return m_holeSettings.holeDiameter;
	}

	return PaletteItem::getProperty(key);
}

void Hole::addedToScene(bool temporary)
{
	if (this->scene()) {
		setHoleSize(m_holeSettings.holeSize(), true);
	}

    return PaletteItem::addedToScene(temporary);
}

bool Hole::hasCustomSVG() {
	switch (m_viewID) {
		case ViewLayer::PCBView:
			return true;
		default:
			return ItemBase::hasCustomSVG();
	}
}

ItemBase::PluralType Hole::isPlural() {
	return Plural;
}

QString Hole::retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor) 
{
	if (m_viewID != ViewLayer::PCBView || 
		(viewLayerID != ViewLayer::Copper0 && viewLayerID != ViewLayer::Copper1)) 
	{
		return PaletteItemBase::retrieveSvg(viewLayerID, svgHash, blackOnly, dpi, factor);
	}

	QStringList holeSize = m_modelPart->localProp("hole size").toString().split(",");
	if (holeSize.length() == 2) {
		QString svg = makeSvg(holeSize.at(0), holeSize.at(1), viewLayerID, false);
		if (!svg.isEmpty()) {
            return PaletteItemBase::normalizeSvg(svg, viewLayerID, blackOnly, dpi, factor);
		}
	}

	return PaletteItemBase::retrieveSvg(viewLayerID, svgHash, blackOnly, dpi, factor);
}

bool Hole::collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide) 
{
	if (prop.compare("hole size", Qt::CaseInsensitive) == 0) {
        return collectHoleSizeInfo(TheHoleThing.holeSizeValue, parent, swappingEnabled, returnProp, returnValue, returnWidget);
	}

	return PaletteItem::collectExtraInfo(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget, hide);
}

void Hole::changeHoleSize(const QString & newSize) {
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
        QRectF holeRect = getRect(holeSize());
        QRectF newHoleRect = getRect(newSize);
        infoGraphicsView->setHoleSize(this, "hole size", tr("hole size"), holeSize(), newSize, holeRect, newHoleRect, true);
	}
}

bool Hole::canEditPart() {
	return false;
}

bool Hole::hasPartNumberProperty()
{
	return false;
}

bool Hole::rotationAllowed() {
	return false;
}

bool Hole::rotation45Allowed() {
	return false;
}

bool Hole::canFindConnectorsUnder() {
	return false;
}

ViewLayer::ViewID Hole::useViewIDForPixmap(ViewLayer::ViewID vid, bool) 
{
    if (vid == ViewLayer::PCBView) {
        return ViewLayer::IconView;
    }

    return ViewLayer::UnknownView;
}

void Hole::changeUnits(bool) 
{
	QString newVal = PaletteItem::changeUnits(m_holeSettings);
	modelPart()->setLocalProp("hole size", newVal);
}

QRectF Hole::trueSceneBoundingRect() {
    QRectF r = sceneBoundingRect();
    return r.adjusted(OffsetPixels, OffsetPixels, -OffsetPixels, -OffsetPixels);
}


