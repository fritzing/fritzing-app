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

#include "bus.h"
#include "busshared.h"
#include "../debugdialog.h"
#include "connectoritem.h"
#include "../model/modelpart.h"

Bus::Bus(BusShared * busShared, ModelPart * modelPart) 
	: QObject(),
	m_connectors(),
	m_subConnector(nullptr),
	m_busShared(busShared),
	m_modelPart(modelPart)
{
}

const QString & Bus::id() const noexcept {
	if (!m_busShared) return ___emptyString___;
	return m_busShared->id();
}

const QList<Connector *> & Bus::connectors() const noexcept {
	return m_connectors;
}

void Bus::addConnector(Connector * connector) {
	// the list of connectors which make up the bus
	m_connectors.append(connector);
}

ModelPart * Bus::modelPart() const noexcept {
	return m_modelPart;
}

void Bus::addSubConnector(Connector * subConnector) {
	m_subConnector = subConnector;
}

Connector * Bus::subConnector() const noexcept {
	return m_subConnector;
}
