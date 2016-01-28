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

$Revision: 6976 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-21 09:50:09 +0200 (So, 21. Apr 2013) $

********************************************************************/

#include "viewlayer.h"
#include "debugdialog.h"
#include <qmath.h>

double ViewLayer::zIncrement = 0.00001;  // 0.000000001;

QHash<ViewLayer::ViewLayerID, NamePair * > ViewLayer::names;
QMultiHash<ViewLayer::ViewLayerID, ViewLayer::ViewLayerID> ViewLayer::alternatives;
QMultiHash<ViewLayer::ViewLayerID, ViewLayer::ViewLayerID> ViewLayer::unconnectables;
QHash<QString, ViewLayer::ViewLayerID> ViewLayer::xmlHash;

const QString ViewLayer::Copper0Color = "#f9a435";  // "#FFCB33"; //Bottom
const QString ViewLayer::Copper1Color = "#fdde68";  // "#F28A00"; //Top
const QString ViewLayer::Copper0WireColor = "#f28a00";
const QString ViewLayer::Copper1WireColor = "#f2c600";
const QString ViewLayer::Silkscreen0Color = "#444444";
const QString ViewLayer::Silkscreen1Color = "#000000";
const QString ViewLayer::BoardColor = "#ffffff";

static LayerList CopperBottomLayers;
static LayerList CopperTopLayers;
static LayerList NonCopperLayers;  // just NonCopperLayers in pcb view

static LayerList IconViewLayerList;
static LayerList BreadboardViewLayerList;
static LayerList SchematicViewLayerList;
static LayerList PCBViewLayerList;
static LayerList EmptyLayerList;
static LayerList PCBViewFromBelowLayerList;

NamePair::NamePair(QString xml, QString display)
{
    xmlName = xml;
    displayName = display;
}

//////////////////////////////////////////////

class NameTriple {

public:
	NameTriple(const QString & _xmlName, const QString & _viewName, const QString & _naturalName) {
		m_xmlName = _xmlName;
		m_viewName = _viewName;
		m_naturalName = _naturalName;
	}

	QString & xmlName() {
		return m_xmlName;
	}

	QString & viewName() {
		return m_viewName;
	}

	QString & naturalName() {
		return m_naturalName;
	}

protected:
	QString m_xmlName;
	QString m_naturalName;
	QString m_viewName;
};

//////////////////////////////////////////////

ViewLayer::ViewLayer(ViewLayerID viewLayerID, bool visible, double initialZ)
{
    m_fromBelow = false;
	m_viewLayerID = viewLayerID;
	m_visible = visible;
	m_action = NULL;
	m_initialZFromBelow = m_initialZ = initialZ;	
    m_nextZ = 0;
	m_parentLayer = NULL;
	m_active = true;
	m_includeChildLayers = true;
}

ViewLayer::~ViewLayer() {
}

void ViewLayer::initNames() {
	if (CopperBottomLayers.isEmpty()) {
		CopperBottomLayers << ViewLayer::GroundPlane0 << ViewLayer::Copper0 << ViewLayer::Copper0Trace;
	}
	if (CopperTopLayers.isEmpty()) {
		CopperTopLayers << ViewLayer::GroundPlane1 << ViewLayer::Copper1 << ViewLayer::Copper1Trace;
	}
	if (NonCopperLayers.isEmpty()) {
		NonCopperLayers << ViewLayer::Silkscreen0 << ViewLayer::Silkscreen0Label
						<< ViewLayer::Silkscreen1 << ViewLayer::Silkscreen1Label 
						<< ViewLayer::PartImage;
	}

	if (names.count() == 0) {
		// xmlname, displayname
		names.insert(ViewLayer::Icon, new NamePair("icon", QObject::tr("Icon")));
		names.insert(ViewLayer::BreadboardBreadboard, new NamePair("breadboardbreadboard", QObject::tr("Breadboard")));
		names.insert(ViewLayer::Breadboard, new NamePair("breadboard", QObject::tr("Parts")));
		names.insert(ViewLayer::BreadboardWire,  new NamePair("breadboardWire", QObject::tr("Wires")));
		names.insert(ViewLayer::BreadboardLabel,  new NamePair("breadboardLabel", QObject::tr("Part Labels")));
		names.insert(ViewLayer::BreadboardRatsnest, new NamePair("breadboardRatsnest", QObject::tr("Ratsnest")));
		names.insert(ViewLayer::BreadboardNote,  new NamePair("breadboardNote", QObject::tr("Notes")));
		names.insert(ViewLayer::BreadboardRuler,  new NamePair("breadboardRuler", QObject::tr("Rulers")));

		names.insert(ViewLayer::SchematicFrame,  new NamePair("schematicframe", QObject::tr("Frame")));
		names.insert(ViewLayer::Schematic,  new NamePair("schematic", QObject::tr("Parts")));
		names.insert(ViewLayer::SchematicText,  new NamePair("schematicText", QObject::tr("Text")));
		names.insert(ViewLayer::SchematicWire,  new NamePair("schematicWire",QObject::tr("Ratsnest")));
		names.insert(ViewLayer::SchematicTrace,  new NamePair("schematicTrace",QObject::tr("Wires")));
		names.insert(ViewLayer::SchematicLabel,  new NamePair("schematicLabel", QObject::tr("Part Labels")));
		names.insert(ViewLayer::SchematicNote,  new NamePair("schematicNote", QObject::tr("Notes")));
		names.insert(ViewLayer::SchematicRuler,  new NamePair("schematicRuler", QObject::tr("Rulers")));

		names.insert(ViewLayer::Board,  new NamePair("board", QObject::tr("Board")));
		names.insert(ViewLayer::Silkscreen1,  new NamePair("silkscreen", QObject::tr("Silkscreen Top")));			// really should be silkscreen1
		names.insert(ViewLayer::Silkscreen1Label,  new NamePair("silkscreenLabel", QObject::tr("Silkscreen Top (Part Labels)")));
		names.insert(ViewLayer::GroundPlane0,  new NamePair("groundplane", QObject::tr("Copper Fill Bottom")));
		names.insert(ViewLayer::Copper0,  new NamePair("copper0", QObject::tr("Copper Bottom")));
		names.insert(ViewLayer::Copper0Trace,  new NamePair("copper0trace", QObject::tr("Copper Bottom Trace")));
		names.insert(ViewLayer::GroundPlane1,  new NamePair("groundplane1", QObject::tr("Copper Fill Top")));
		names.insert(ViewLayer::Copper1,  new NamePair("copper1", QObject::tr("Copper Top")));
		names.insert(ViewLayer::Copper1Trace,  new NamePair("copper1trace", QObject::tr("Copper Top Trace")));
		names.insert(ViewLayer::PcbRatsnest, new NamePair("ratsnest", QObject::tr("Ratsnest")));
		names.insert(ViewLayer::Silkscreen0,  new NamePair("silkscreen0", QObject::tr("Silkscreen Bottom")));
		names.insert(ViewLayer::Silkscreen0Label,  new NamePair("silkscreen0Label", QObject::tr("Silkscreen Bottom (Part Labels)")));
		//names.insert(ViewLayer::Soldermask,  new NamePair("soldermask",  QObject::tr("Solder mask")));
		//names.insert(ViewLayer::Outline,  new NamePair("outline",  QObject::tr("Outline")));
		//names.insert(ViewLayer::Keepout, new NamePair("keepout", QObject::tr("Keep out")));
		names.insert(ViewLayer::PartImage, new NamePair("partimage", QObject::tr("Part Image")));
		names.insert(ViewLayer::PcbNote,  new NamePair("pcbNote", QObject::tr("Notes")));
		names.insert(ViewLayer::PcbRuler,  new NamePair("pcbRuler", QObject::tr("Rulers")));

		foreach (ViewLayerID key, names.keys()) {
			xmlHash.insert(names.value(key)->xmlName, key);
		}

		names.insert(ViewLayer::UnknownLayer,  new NamePair("unknown", QObject::tr("Unknown Layer")));

		alternatives.insert(ViewLayer::Copper0, ViewLayer::Copper1);
		alternatives.insert(ViewLayer::Copper1, ViewLayer::Copper0);

        LayerList l0, l1;
        l0 << ViewLayer::Copper0 << ViewLayer::Copper0Trace << ViewLayer::GroundPlane0;
        l1 << ViewLayer::Copper1 << ViewLayer::Copper1Trace << ViewLayer::GroundPlane1;
        foreach (ViewLayer::ViewLayerID viewLayerID0, l0) {
            foreach (ViewLayer::ViewLayerID viewLayerID1, l1) {
                unconnectables.insert(viewLayerID0, viewLayerID1);
                unconnectables.insert(viewLayerID1, viewLayerID0);
            }
        }
	}

	if (ViewIDNames.count() == 0) {
		ViewIDNames.insert(ViewLayer::IconView, new NameTriple("iconView", QObject::tr("icon view"), "icon"));
		ViewIDNames.insert(ViewLayer::BreadboardView, new NameTriple("breadboardView", QObject::tr("breadboard view"), "breadboard"));
		ViewIDNames.insert(ViewLayer::SchematicView, new NameTriple("schematicView", QObject::tr("schematic view"), "schematic"));
		ViewIDNames.insert(ViewLayer::PCBView, new NameTriple("pcbView", QObject::tr("pcb view"), "pcb"));
	}

	if (BreadboardViewLayerList.count() == 0) {
		IconViewLayerList << ViewLayer::Icon;
		BreadboardViewLayerList << ViewLayer::BreadboardBreadboard << ViewLayer::Breadboard 
			<< ViewLayer::BreadboardWire << ViewLayer::BreadboardLabel 
			<< ViewLayer::BreadboardRatsnest 
			<< ViewLayer::BreadboardNote << ViewLayer::BreadboardRuler;
		SchematicViewLayerList << ViewLayer::SchematicFrame << ViewLayer::Schematic
			<< ViewLayer::SchematicText << ViewLayer::SchematicWire 
			<< ViewLayer::SchematicTrace << ViewLayer::SchematicLabel 
			<< ViewLayer::SchematicNote <<  ViewLayer::SchematicRuler;
		PCBViewLayerList << ViewLayer::Board 
            << ViewLayer::GroundPlane0 
			<< ViewLayer::Silkscreen0 << ViewLayer::Silkscreen0Label
			<< ViewLayer::Copper0  << ViewLayer::Copper0Trace 
            << ViewLayer::GroundPlane1 
			<< ViewLayer::Copper1 << ViewLayer::Copper1Trace 
			<< ViewLayer::PcbRatsnest 
			<< ViewLayer::Silkscreen1 << ViewLayer::Silkscreen1Label 
			<< ViewLayer::PartImage 
			<< ViewLayer::PcbNote << ViewLayer::PcbRuler;
		PCBViewFromBelowLayerList << ViewLayer::Board 
            << ViewLayer::GroundPlane1 
			<< ViewLayer::Silkscreen1 << ViewLayer::Silkscreen1Label 
			<< ViewLayer::Copper1 << ViewLayer::Copper1Trace 
            << ViewLayer::GroundPlane0 
			<< ViewLayer::Copper0 << ViewLayer::Copper0Trace 
			<< ViewLayer::PcbRatsnest 
			<< ViewLayer::Silkscreen0 << ViewLayer::Silkscreen0Label
			<< ViewLayer::PartImage 
			<< ViewLayer::PcbNote << ViewLayer::PcbRuler;
	}
}

QString & ViewLayer::displayName() {
	return names[m_viewLayerID]->displayName;
}

void ViewLayer::setAction(QAction * action) {
	m_action = action;
}

QAction* ViewLayer::action() {
	return m_action;
}

bool ViewLayer::visible() {
	return m_visible;
}

void ViewLayer::setVisible(bool visible) {
	m_visible = visible;
	if (m_action) {
		m_action->setChecked(visible);
	}
}

double ViewLayer::nextZ() {
	double temp = m_nextZ + (m_fromBelow ? m_initialZFromBelow : m_initialZ);
	m_nextZ += zIncrement;
	return temp;
}

ViewLayer::ViewLayerID ViewLayer::viewLayerIDFromXmlString(const QString & viewLayerName) {
	return xmlHash.value(viewLayerName, ViewLayer::UnknownLayer);
}

ViewLayer::ViewLayerID ViewLayer::viewLayerID() {
	return m_viewLayerID;
}

double ViewLayer::incrementZ(double z) {
	return (z + zIncrement);
}

double ViewLayer::getZIncrement() {
	return zIncrement;
}

const QString & ViewLayer::viewLayerNameFromID(ViewLayerID viewLayerID) {
	NamePair * sp = names.value(viewLayerID);
	if (sp == NULL) return ___emptyString___;

	return sp->displayName;
}

const QString & ViewLayer::viewLayerXmlNameFromID(ViewLayerID viewLayerID) {
	NamePair * sp = names.value(viewLayerID);
	if (sp == NULL) return ___emptyString___;

	return sp->xmlName;
}

ViewLayer * ViewLayer::parentLayer() {
	return m_parentLayer;
}

void ViewLayer::setParentLayer(ViewLayer * parent) {
	m_parentLayer = parent;
	if (parent) {
		parent->m_childLayers.append(this);
	}
}

const QList<ViewLayer *> & ViewLayer::childLayers() {
	return m_childLayers;
}

bool ViewLayer::alreadyInLayer(double z) {
    if (m_fromBelow) {
	    return (z >= m_initialZFromBelow && z <= m_initialZFromBelow + m_nextZ);
    }

	return (z >= m_initialZ && z <= m_initialZ + m_nextZ);
}

void ViewLayer::cleanup() {
	foreach (NamePair * sp, names.values()) {
		delete sp;
	}
	names.clear();

	foreach (NameTriple * nameTriple, ViewIDNames) {
		delete nameTriple;
	}
	ViewIDNames.clear();
}

void ViewLayer::resetNextZ(double z) {
	m_nextZ = z - floor(z);
}

LayerList ViewLayer::findAlternativeLayers(ViewLayer::ViewLayerID id)
{
	LayerList alts = alternatives.values(id);
	return alts;
}

bool ViewLayer::canConnect(ViewLayer::ViewLayerID v1, ViewLayer::ViewLayerID v2) {
	if (v1 == v2) return true;

 	LayerList uncs = unconnectables.values(v1);
	return (!uncs.contains(v2));
}

bool ViewLayer::isActive() {
	return m_active;
}

void ViewLayer::setActive(bool a) {
	m_active = a;
}

ViewLayer::ViewLayerPlacement ViewLayer::specFromID(ViewLayer::ViewLayerID viewLayerID) 
{
	switch (viewLayerID) {
		case Copper1:
		case Copper1Trace:
		case GroundPlane1:
			return ViewLayer::NewTop;
		default:
			return ViewLayer::NewBottom;
	}
}

bool ViewLayer::isCopperLayer(ViewLayer::ViewLayerID viewLayerID) {
	if (CopperTopLayers.contains(viewLayerID)) return true;
	if (CopperBottomLayers.contains(viewLayerID)) return true;
	
	return false;
}


const LayerList & ViewLayer::copperLayers(ViewLayer::ViewLayerPlacement viewLayerPlacement) 
{
	if (viewLayerPlacement == ViewLayer::NewTop) return CopperTopLayers;
	
	return CopperBottomLayers;
}

const LayerList & ViewLayer::maskLayers(ViewLayer::ViewLayerPlacement viewLayerPlacement) {
	static LayerList bottom;
	static LayerList top;
	if (bottom.isEmpty()) {
		bottom << ViewLayer::Copper0;
	}
	if (top.isEmpty()) {
		top << ViewLayer::Copper1;
	}
	if (viewLayerPlacement == ViewLayer::NewTop) return top;
	
	return bottom;
}

const LayerList & ViewLayer::silkLayers(ViewLayer::ViewLayerPlacement viewLayerPlacement) {
	static LayerList bottom;
	static LayerList top;
	if (bottom.isEmpty()) {
		bottom << ViewLayer::Silkscreen0 << ViewLayer::Silkscreen0Label;
	}
	if (top.isEmpty()) {
		top << ViewLayer::Silkscreen1 << ViewLayer::Silkscreen1Label;
	}
	if (viewLayerPlacement == ViewLayer::NewTop) return top;
	
	return bottom;
}

const LayerList & ViewLayer::outlineLayers() {
	static LayerList layerList;
	if (layerList.isEmpty()) {
		layerList << ViewLayer::Board;
	}
	
	return layerList;
}

const LayerList & ViewLayer::drillLayers() {
	static LayerList layerList;
	if (layerList.isEmpty()) {
		layerList << ViewLayer::Copper0 << ViewLayer::Copper1;
	}
	
	return layerList;
}

const LayerList & ViewLayer::silkLayers() {
	static LayerList layerList;
	if (layerList.isEmpty()) {
		layerList << ViewLayer::Silkscreen0 << ViewLayer::Silkscreen1;
	}
	
	return layerList;
}

const LayerList & ViewLayer::topLayers() {
	static LayerList layerList;
	if (layerList.isEmpty()) {
		layerList << ViewLayer::Copper1 << ViewLayer::Copper1Trace << ViewLayer::Silkscreen1 << ViewLayer::Silkscreen1Label << ViewLayer::GroundPlane1;
	}
	
	return layerList;
}

const LayerList & ViewLayer::bottomLayers() {
	static LayerList layerList;
	if (layerList.isEmpty()) {
		layerList << ViewLayer::Copper0 << ViewLayer::Copper0Trace << ViewLayer::Silkscreen0 << ViewLayer::Silkscreen0Label << ViewLayer::GroundPlane0;
	}
	
	return layerList;
}





bool ViewLayer::isNonCopperLayer(ViewLayer::ViewLayerID viewLayerID) {
	// for pcb view layers only
	return NonCopperLayers.contains(viewLayerID);
}


bool ViewLayer::includeChildLayers() {
	return m_includeChildLayers;
} 

void ViewLayer::setIncludeChildLayers(bool incl) {
	m_includeChildLayers = incl;
}

/////////////////////////////////

QHash <ViewLayer::ViewID, NameTriple * > ViewLayer::ViewIDNames;

QString & ViewLayer::viewIDName(ViewLayer::ViewID viewID) {
	if (viewID < 0 || viewID >= ViewLayer::ViewCount) {
		throw "ViewLayer::viewIDName bad identifier";
	}

	return ViewIDNames[viewID]->viewName();
}

QString & ViewLayer::viewIDXmlName(ViewLayer::ViewID viewID) {
	if (viewID < 0 || viewID >= ViewLayer::ViewCount) {
		throw "ViewLayer::viewIDXmlName bad identifier";
	}

	return ViewIDNames[viewID]->xmlName();
}

QString & ViewLayer::viewIDNaturalName(ViewLayer::ViewID viewID) {
	if (viewID < 0 || viewID >= ViewLayer::ViewCount) {
		throw "ViewLayer::viewIDNaturalName bad identifier";
	}

	return ViewIDNames[viewID]->naturalName();
}

ViewLayer::ViewID ViewLayer::idFromXmlName(const QString & name) {
	foreach (ViewID id, ViewIDNames.keys()) {
		NameTriple * nameTriple = ViewIDNames.value(id);
		if (name.compare(nameTriple->xmlName()) == 0) return id;
	}

	return UnknownView;
}

const LayerList & ViewLayer::layersForView(ViewLayer::ViewID viewID) {
	switch(viewID) {
		case IconView:
			return IconViewLayerList;
		case BreadboardView:
			return BreadboardViewLayerList;
		case SchematicView:
			return SchematicViewLayerList;
		case PCBView:
			return PCBViewLayerList;
		default:
			return EmptyLayerList;
	}
}

const LayerList & ViewLayer::layersForViewFromBelow(ViewLayer::ViewID viewID) {
	switch(viewID) {
		case IconView:
			return IconViewLayerList;
		case BreadboardView:
			return BreadboardViewLayerList;
		case SchematicView:
			return SchematicViewLayerList;
		case PCBView:
			return PCBViewFromBelowLayerList;
		default:
			return EmptyLayerList;
	}
}

bool ViewLayer::viewHasLayer(ViewLayer::ViewID viewID, ViewLayer::ViewLayerID viewLayerID) {
	return layersForView(viewID).contains(viewLayerID);
}


QDomElement ViewLayer::getConnectorPElement(const QDomElement & element, ViewLayer::ViewID viewID)
{
    QString viewName = ViewLayer::viewIDXmlName(viewID);
    QDomElement views = element.firstChildElement("views");
    QDomElement view = views.firstChildElement(viewName);
    return view.firstChildElement("p");
}

bool ViewLayer::getConnectorSvgIDs(QDomElement & element, ViewLayer::ViewID viewID, QString & id, QString & terminalID) {
    QDomElement p = getConnectorPElement(element, viewID);
    if (p.isNull()) {
		return false;
	}

    id = p.attribute("svgId");
    if (id.isEmpty()) {
		return false;
	}

    terminalID = p.attribute("terminalId");
    return true;
}

bool ViewLayer::fromBelow() {
    return m_fromBelow;
}

void ViewLayer::setFromBelow(bool fromBelow) {
    m_fromBelow = fromBelow;
}

void ViewLayer::setInitialZFromBelow(double initialZ) {
    m_initialZFromBelow = initialZ;
}

double ViewLayer::getZFromBelow(double currentZ, bool fromBelow) {
    double frac = currentZ - m_initialZ;
    if (qAbs(frac) > 1) {
        frac = currentZ - m_initialZFromBelow;
    }
    return frac + (fromBelow ? m_initialZFromBelow : m_initialZ);
}

