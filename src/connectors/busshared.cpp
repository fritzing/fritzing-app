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

$Revision: 6309 $:
$Author: cohen@irascible.com $:
$Date: 2012-08-17 11:16:43 +0200 (Fr, 17. Aug 2012) $

********************************************************************/

#include "busshared.h"
#include "connectorshared.h"
#include "../debugdialog.h"
#include "connectoritem.h"
#include "../utils/textutils.h"

BusShared::BusShared(const QString & id)
{
	m_id = id;
}

BusShared::BusShared(const QDomElement & busElement, const QHash<QString, QPointer<ConnectorShared> > & connectorHash)
{
	m_id = busElement.attribute("id");
	
	QDomElement connector = busElement.firstChildElement("nodeMember");
	while (!connector.isNull()) {
		initConnector(connector, connectorHash);		
		connector = connector.nextSiblingElement("nodeMember");
	}
}

void BusShared::initConnector(QDomElement & connector, const QHash<QString, QPointer<ConnectorShared> > & connectorHash)
{
	QString id = connector.attribute("connectorId");
	if (id.isNull()) return;
	if (id.isEmpty()) return;
				
	ConnectorShared * connectorShared = connectorHash.value(id);
	if (connectorShared == NULL) {
        QDomDocument document = connector.ownerDocument();
        DebugDialog::debug(QString("no connector is found for bus nodeMember %1 in %2")
                           .arg(id)
                           .arg(TextUtils::elementToString(document.documentElement())));
		return;
	}
		
	m_connectors.append(connectorShared);
	connectorShared->setBus(this);
}

void BusShared::addConnectorShared(ConnectorShared * connectorShared) 
{
	m_connectors.append(connectorShared);
	connectorShared->setBus(this);
}

const QString & BusShared::id() const {
	return m_id;
}


const QList<ConnectorShared *> & BusShared::connectors() {
	return m_connectors;
}



	
