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

#ifndef CONNECTORSHARED_H
#define CONNECTORSHARED_H

#include <QString>
#include <QDomElement>
#include <QMultiHash>

#include "../viewlayer.h"
#include "connector.h"
#include "svgidlayer.h"

class ConnectorShared : public QObject
{
Q_OBJECT

public:
	ConnectorShared();
	ConnectorShared(const QDomElement & domElement);
	~ConnectorShared();

	const QString & id() const;
	void setId(QString id);
	const QString & description();
	void setDescription(QString description);
	const QString & sharedName();
	void setSharedName(QString name);
	const QString & connectorTypeString();
	void setConnectorType(QString type);
	void setConnectorType(Connector::ConnectorType);
	Connector::ConnectorType connectorType();
	const QString & replacedby() const;
	void setReplacedby(QString);

	const QString & legID(ViewLayer::ViewID viewId, ViewLayer::ViewLayerID viewLayerID);
	const QMultiHash<ViewLayer::ViewID,SvgIdLayer *> &pins();
	SvgIdLayer * fullPinInfo(ViewLayer::ViewID viewId, ViewLayer::ViewLayerID viewLayerID);
    const QList<SvgIdLayer *> svgIdLayers() const;
	void addPin(ViewLayer::ViewID, const QString & svgId, ViewLayer::ViewLayerID, const QString & terminalId, const QString & legId, bool hybrid);
	void insertPin(ViewLayer::ViewID layer, SvgIdLayer * svgIdLayer);
	void removePins(ViewLayer::ViewID layer);
	void removePin(ViewLayer::ViewID layer, SvgIdLayer * svgIdLayer);

	class BusShared * bus();
	void setBus(class BusShared *);
	const QString & busID();
	class ErcData * ercData();

protected:
	void loadPins(const QDomElement & domElement);
	void loadPin(QDomElement elem, ViewLayer::ViewID viewId);

	QString m_description;
	QString m_id;
	QString m_name;
	QString m_typeString;
    QString m_replacedby;
	Connector::ConnectorType m_type;
	QString m_ercType;
	class ErcData * m_ercData;
	class BusShared * m_bus;

	QMultiHash<ViewLayer::ViewID, SvgIdLayer*> m_pins;
};

static QList<ConnectorShared *> ___emptyConnectorSharedList___;

#endif
