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

#include "bus.h"
#include "busshared.h"
#include "../debugdialog.h"
#include "connectoritem.h"
#include "../model/modelpart.h"

Bus::Bus(BusShared * busShared, ModelPart * modelPart) : QObject()
{
	m_busShared = busShared;
	m_modelPart = modelPart;
}

const QString & Bus::id() const {
	if (m_busShared == NULL) return ___emptyString___;

	return m_busShared->id();
}

const QList<Connector *> & Bus::connectors() const {
	return m_connectors;
}

void Bus::addConnector(Connector * connector) {
	// the list of connectors which make up the bus
	m_connectors.append(connector);
}

ModelPart * Bus::modelPart() {
	return m_modelPart;
}

void Bus::addSubConnector(Connector * subConnector) {
    m_subConnector = subConnector;
}

Connector * Bus::subConnector() const {
	return m_subConnector;
}