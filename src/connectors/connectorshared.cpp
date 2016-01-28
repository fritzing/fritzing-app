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

$Revision: 6955 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-06 23:14:37 +0200 (Sa, 06. Apr 2013) $

********************************************************************/

#include "connectorshared.h"
#include "../debugdialog.h"
#include "connector.h"
#include "busshared.h"
#include "ercdata.h"

#include <QTextStream>


ConnectorShared::ConnectorShared()
{
	m_id = "";
	m_name = "";
	m_typeString = "";
	m_type = Connector::Unknown;
	m_description = "";
	m_bus = NULL;
	m_ercData = NULL;
}

ConnectorShared::ConnectorShared( const QDomElement & domElement )
{
	m_ercData = NULL;
	m_id = domElement.attribute("id", "");
	m_name = domElement.attribute("name", "");
	m_replacedby = domElement.attribute("replacedby", "");
	//DebugDialog::debug(QString("\tname:%1 id:%2").arg(m_name).arg(m_id));
	m_typeString = domElement.attribute("type", "");
	m_type = Connector::connectorTypeFromName(m_typeString);
	m_description = domElement.firstChildElement("description").text();
	QDomElement erc = domElement.firstChildElement("erc");
	if (!erc.isNull()) {
		m_ercData = new ErcData(erc);
	}

	loadPins(domElement);

	if (m_type == Connector::Unknown) {
		foreach (SvgIdLayer * svgIdLayer, m_pins.values()) {
			if (svgIdLayer->m_svgId.endsWith("pad")) {
				m_type = Connector::Pad;
				m_typeString = Connector::connectorNameFromType(m_type);
				break;
			}
		}

	}

	m_bus = NULL;
}

ConnectorShared::~ConnectorShared() {
	foreach (SvgIdLayer * svgIdLayer, m_pins.values()) {
		delete svgIdLayer;
	}
	 m_pins.clear();
	 if (m_ercData) {
		 delete m_ercData;
	 }
}


const QString & ConnectorShared::id() const {
	return m_id;
}

void ConnectorShared::setId(QString id) {
	m_id = id;
}

const QString & ConnectorShared::description() {
	return m_description;
}

void ConnectorShared::setDescription(QString description) {
	m_description = description;
}

const QString & ConnectorShared::sharedName() {
	return m_name;
}

void ConnectorShared::setSharedName(QString name) {
	m_name = name;
}

Connector::ConnectorType ConnectorShared::connectorType() {
	return m_type;
}

const QString & ConnectorShared::connectorTypeString() {
	return m_typeString;
}

void ConnectorShared::setConnectorType(QString type) {
	m_typeString = type;
	m_type = Connector::connectorTypeFromName(type);
}

void ConnectorShared::setConnectorType(Connector::ConnectorType type) {
	m_typeString = Connector::connectorNameFromType(type);
	m_type = type;
}

const QString & ConnectorShared::replacedby() const {
	return m_replacedby;
}

void ConnectorShared::setReplacedby(QString replacedby) {
	m_replacedby = replacedby;
}

const QMultiHash<ViewLayer::ViewID,SvgIdLayer*> & ConnectorShared::pins() {
	return m_pins;
}

void ConnectorShared::insertPin(ViewLayer::ViewID layer, SvgIdLayer * svgIdLayer) {
	m_pins.insert(layer, svgIdLayer);
}

void ConnectorShared::addPin(ViewLayer::ViewID viewID, const QString & svgId, ViewLayer::ViewLayerID viewLayerID, const QString & terminalId, const QString & legId, bool hybrid) {
	SvgIdLayer * svgIdLayer = new SvgIdLayer(viewID);
	svgIdLayer->m_svgViewLayerID = viewLayerID;
	svgIdLayer->m_svgId = svgId;
	svgIdLayer->m_terminalId = terminalId;
    svgIdLayer->m_hybrid = hybrid;
    svgIdLayer->m_legId = legId;
	m_pins.insert(viewID, svgIdLayer);
	// DebugDialog::debug(QString("insert a %1 %2 %3").arg(layer).arg(connectorId).arg(viewLayerID));
}

void ConnectorShared::removePins(ViewLayer::ViewID layer) {
	m_pins.remove(layer);
	if (m_pins.values(layer).size() != 0) {
		throw "ConnectorShared::removePins";
	}
}

void ConnectorShared::removePin(ViewLayer::ViewID layer, SvgIdLayer * svgIdLayer) {
	m_pins.remove(layer, svgIdLayer);
}

SvgIdLayer * ConnectorShared::fullPinInfo(ViewLayer::ViewID viewId, ViewLayer::ViewLayerID viewLayerID) {
	QList<SvgIdLayer *> svgLayers = m_pins.values(viewId);
	foreach ( SvgIdLayer * svgIdLayer, svgLayers) {
		if (svgIdLayer->m_svgViewLayerID == viewLayerID) {
			return svgIdLayer;
		}
	}

	return NULL;
}

const QString & ConnectorShared::legID(ViewLayer::ViewID viewId, ViewLayer::ViewLayerID viewLayerID) {
	QList<SvgIdLayer *> svgLayers = m_pins.values(viewId);
	foreach ( SvgIdLayer * svgIdLayer, svgLayers) {
		if (svgIdLayer->m_svgViewLayerID == viewLayerID) {
			return svgIdLayer->m_legId;
		}
	}

	return ___emptyString___;
}


void ConnectorShared::loadPins(const QDomElement & domElement) {
	//if(m_domElement == NULL) return;

	// TODO: this is view-related stuff and it would be nice if the model didn't know about it
	QDomElement viewsTag = domElement.firstChildElement("views");
	loadPin(viewsTag.firstChildElement("breadboardView"),ViewLayer::BreadboardView);
	loadPin(viewsTag.firstChildElement("schematicView"),ViewLayer::SchematicView);
	loadPin(viewsTag.firstChildElement("pcbView"),ViewLayer::PCBView);
}

void ConnectorShared::loadPin(QDomElement elem, ViewLayer::ViewID viewID) {
	QDomElement pinElem = elem.firstChildElement("p");
	while (!pinElem.isNull()) {
		//QString temp;
		//QTextStream stream(&temp);
		//pinElem.save(stream, 0);
		//stream.flush();
		QString svgId = pinElem.attribute("svgId");
		//svgId = svgId.left(svgId.lastIndexOf(QRegExp("\\d"))+1);
		QString layer = pinElem.attribute("layer");
		SvgIdLayer * svgIdLayer = new SvgIdLayer(viewID);
		svgIdLayer->m_hybrid = (pinElem.attribute("hybrid").compare("yes") == 0);
		svgIdLayer->m_legId = pinElem.attribute("legId");
		svgIdLayer->m_svgId = svgId;
		svgIdLayer->m_svgViewLayerID = ViewLayer::viewLayerIDFromXmlString(layer);

		//DebugDialog::debug(QString("svg id view layer id %1, %2").arg(svgIdLayer->m_viewLayerID).arg(layer));
		svgIdLayer->m_terminalId = pinElem.attribute("terminalId");
		//if (!svgIdLayer->m_terminalId.isEmpty()) {
		//	DebugDialog::debug("terminalid " + svgIdLayer->m_terminalId);
		//}
		m_pins.insert(viewID, svgIdLayer);
		//DebugDialog::debug(QString("insert b %1 %2 %3").arg(viewId).arg(svgId).arg(layer));

		pinElem = pinElem.nextSiblingElement("p");
	}
}

void ConnectorShared::setBus(BusShared * bus) {
	m_bus = bus;
}

BusShared * ConnectorShared::bus() {
	return m_bus;
}

const QString & ConnectorShared::busID() {
	if (m_bus == NULL) return ___emptyString___;
		return m_bus->id();
}

ErcData * ConnectorShared::ercData() {
	return m_ercData;
}

const QList<SvgIdLayer *> ConnectorShared::svgIdLayers() const
{
    return m_pins.values();
}
