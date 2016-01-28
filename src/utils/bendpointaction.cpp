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

$Revision: 6904 $:
$Author: irascibl@gmail.com $:
$Date: 2013-02-26 16:26:03 +0100 (Di, 26. Feb 2013) $

********************************************************************/

#include "bendpointaction.h"
#include "../items/itembase.h"
#include "../connectors/connectoritem.h"


BendpointAction::BendpointAction(const QString & text, QObject * parent)
	: QAction(text, parent)
{
	m_lastHoverEnterConnectorItem = NULL;
	m_lastHoverEnterItem = NULL;
}

void BendpointAction::setLastHoverEnterConnectorItem(ConnectorItem * connectorItem) {
	m_lastHoverEnterConnectorItem = connectorItem;
}

void BendpointAction::setLastHoverEnterItem(ItemBase * itemBase) {
	m_lastHoverEnterItem = itemBase;
}

void BendpointAction::setLastLocation(QPointF p) {
	m_lastLocation = p;
}

ConnectorItem * BendpointAction::lastHoverEnterConnectorItem() {
	return m_lastHoverEnterConnectorItem;
}

ItemBase * BendpointAction::lastHoverEnterItem() {
	return m_lastHoverEnterItem;
}

QPointF BendpointAction::lastLocation() {
	return m_lastLocation;
}

