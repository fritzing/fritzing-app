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

$Revision: 6954 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-05 10:22:00 +0200 (Fr, 05. Apr 2013) $

********************************************************************/

#ifndef CONNECTOR_H
#define CONNECTOR_H

#include <QHash>
#include <QString>
#include <QList>
#include <QXmlStreamWriter>
#include <QGraphicsScene>
#include <QSvgRenderer>
#include <QPointer>

#include "../viewlayer.h"

class Connector : public QObject
{
Q_OBJECT
public:
	enum ConnectorType {
		Male,
		Female,
		Wire,
		Pad,
		Unknown
	};

public:
	Connector(class ConnectorShared *, class ModelPart * modelPart);
	~Connector();

	Connector::ConnectorType connectorType() const;
	void addViewItem(class ConnectorItem *);
	void removeViewItem(class ConnectorItem *);
	class ConnectorShared * connectorShared();
	void connectTo(Connector *);
	void disconnectFrom(Connector *);
	void saveAsPart(QXmlStreamWriter & writer);
	const QList<Connector *> & toConnectors();
	ConnectorItem * connectorItemByViewLayerID(ViewLayer::ViewID, ViewLayer::ViewLayerID);
	ConnectorItem * connectorItem(ViewLayer::ViewID);
	bool connectionIsAllowed(Connector* that);
	const QString & connectorSharedID() const;
	const QString & connectorSharedName() const;	
	const QString & connectorSharedDescription() const;
	const QString & connectorSharedReplacedby() const;
	class ErcData * connectorSharedErcData();
	const QString & busID();
	class Bus * bus();
	void setBus(class Bus *);
	long modelIndex();
	ModelPart * modelPart();
	int connectorItemCount();
	void unprocess(ViewLayer::ViewID viewID, ViewLayer::ViewLayerID viewLayerID);
	class SvgIdLayer * fullPinInfo(ViewLayer::ViewID viewId, ViewLayer::ViewLayerID viewLayerID);
	const QList<SvgIdLayer *> svgIdLayers() const;
	QList< QPointer<class ConnectorItem> > viewItems();
	const QString & legID(ViewLayer::ViewID, ViewLayer::ViewLayerID);
	void setConnectorLocalName(const QString &);
	const QString & connectorLocalName();
	void addPin(ViewLayer::ViewID, const QString & svgId, ViewLayer::ViewLayerID, const QString & terminalId, const QString & legId, bool hybrid);

public:
	static void initNames();
	static const QString & connectorNameFromType(ConnectorType);
	static ConnectorType connectorTypeFromName(const QString & name);

protected:
	void writeLayerAttr(QXmlStreamWriter &writer, ViewLayer::ViewLayerID);
	void writeSvgIdAttr(QXmlStreamWriter &writer, ViewLayer::ViewID view, QString connId);
	void writeTerminalIdAttr(QXmlStreamWriter &writer, ViewLayer::ViewID view, QString terminalId);

protected:
	QPointer<class ConnectorShared> m_connectorShared;
	QHash< int, QPointer<class ConnectorItem> > m_connectorItems;
	QList<Connector *> m_toConnectors;
	QPointer<class ModelPart> m_modelPart;
	QPointer<class Bus> m_bus;
	QString m_connectorLocalName;

protected:
	static QHash<ConnectorType, QString> Names;
};

#endif
