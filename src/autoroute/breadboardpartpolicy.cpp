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

#include "breadboardpartpolicy.h"
#include "breadboardtopology.h"

#include "../connectors/connectoritem.h"
#include "../items/itembase.h"
#include "../model/modelpart.h"

#include <QStringList>

namespace {
bool containsAny(const QString & text, const QStringList & needles)
{
	Q_FOREACH (const QString & needle, needles) {
		if (text.contains(needle, Qt::CaseInsensitive)) return true;
	}
	return false;
}

bool isPlaceablePin(ConnectorItem * connectorItem)
{
	if (connectorItem == nullptr) return false;
	Connector::ConnectorType connectorType = connectorItem->connectorType();
	return connectorType == Connector::Male || connectorType == Connector::Pad || connectorItem->isHybrid();
}
}

BreadboardPartPolicy::Decision BreadboardPartPolicy::classify(ItemBase * itemBase)
{
	Decision decision;
	if (itemBase == nullptr) {
		decision.reason = "null item";
		return decision;
	}

	ModelPart * modelPart = itemBase->modelPart();
	decision.title = itemBase->title();
	decision.moduleID = itemBase->moduleID();
	if (modelPart != nullptr) {
		decision.family = modelPart->family();
		decision.taxonomy = modelPart->taxonomy();
		decision.package = modelPart->properties().value("package");
	}

	if (!itemBase->isEverVisible()) {
		decision.reason = "not visible";
		return decision;
	}
	if (itemBase->getRatsnest()) {
		decision.reason = "ratsnest";
		return decision;
	}
	if (BreadboardTopology::isBreadboardItem(itemBase)) {
		decision.reason = "breadboard";
		return decision;
	}
	if (BreadboardTopology::isBreadboardDecorationItem(itemBase)) {
		decision.reason = "breadboard decoration";
		return decision;
	}
	if (itemBase->itemType() == ModelPart::Wire) {
		decision.reason = "wire";
		return decision;
	}
	if (itemBase->moveLock()) {
		decision.reason = "move locked";
		return decision;
	}

	int femaleSockets = 0;
	Q_FOREACH (ConnectorItem * connectorItem, itemBase->cachedConnectorItems()) {
		if (connectorItem == nullptr) continue;
		if (connectorItem->connectorType() == Connector::Female) {
			femaleSockets++;
			continue;
		}
		if (!isPlaceablePin(connectorItem)) continue;
		decision.placeablePins++;
		if (connectorItem->hasRubberBandLeg() || !connectorItem->legID(itemBase->viewID(), itemBase->viewLayerID()).isEmpty()) {
			decision.hasBendableLegs = true;
		}
	}

	if (decision.placeablePins <= 0) {
		if (femaleSockets > 0) {
			// Breakout boards and socketed modules with female headers: never
			// seated into a breadboard, but their sockets are legitimate wire
			// terminals - jumper them from off-board, like all breakouts.
			// (Future exception per user: dual-row male headers at breadboard
			// pitch could seat like a DIP; that variant has placeable pins
			// and does not reach this branch.)
			decision.classification = Classification::Peripheral;
			decision.reason = "female-socket breakout (jumper wiring only)";
			return decision;
		}
		decision.reason = "no placeable pins";
		return decision;
	}

	const QString descriptiveText = QString("%1 %2 %3 %4")
	        .arg(decision.title)
	        .arg(decision.family)
	        .arg(decision.taxonomy)
	        .arg(decision.package);
	const QString searchable = QString("%1 %2").arg(descriptiveText).arg(decision.moduleID);

	const bool looksLikeTrimpot = containsAny(searchable, {"trimpot", "trim pot", "trimmer", "preset"});
	if (descriptiveText.contains("potentiometer", Qt::CaseInsensitive) && !looksLikeTrimpot) {
		decision.classification = Classification::Peripheral;
		decision.reason = "panel/control potentiometer";
		return decision;
	}

	if (containsAny(descriptiveText, {"audio jack", " jack", "connector", "terminal", "switch", "button", "keypad"})) {
		decision.classification = Classification::Peripheral;
		decision.reason = "interface/control part";
		return decision;
	}

	if (containsAny(descriptiveText, {"power", "battery", "supply", "module", "breakout", "board", "sensor", "arduino"})) {
		decision.classification = Classification::Peripheral;
		decision.reason = "module/power/peripheral part";
		return decision;
	}

	if (containsAny(searchable, {"resistor", "capacitor", "diode", "led", "transistor", "ic", "dip", "header"})
	    || decision.hasBendableLegs
	    || looksLikeTrimpot) {
		decision.classification = Classification::BoardPlaceable;
		decision.reason = "breadboard-suitable component";
		return decision;
	}

	decision.classification = Classification::Peripheral;
	decision.reason = "not known breadboard-placeable";
	return decision;
}

QString BreadboardPartPolicy::classificationName(Classification classification)
{
	switch (classification) {
	case Classification::BoardPlaceable:
		return "BoardPlaceable";
	case Classification::Peripheral:
		return "Peripheral";
	case Classification::Ignore:
	default:
		return "Ignore";
	}
}
