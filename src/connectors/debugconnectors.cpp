/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2023 Fritzing GmbH

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

#include "debugconnectors.h"
#include "../items/itembase.h"
#include "../items/symbolpaletteitem.h"
#include "../items/virtualwire.h"
#include "../connectors/connectoritem.h"
#include "../debugdialog.h"

DebugConnectors::DebugConnectors(SketchWidget *breadboardGraphicsView, SketchWidget *schematicGraphicsView, SketchWidget *pcbGraphicsView)
	: m_breadboardGraphicsView(breadboardGraphicsView),
	  m_schematicGraphicsView(schematicGraphicsView),
	  m_pcbGraphicsView(pcbGraphicsView),
	  timer(new QTimer(this)),
	  firstCall(true)
{
	timer->setSingleShot(true);
	connect(timer, &QTimer::timeout, this, &DebugConnectors::routingCheckSlot);
	connect(m_breadboardGraphicsView, &SketchWidget::routingCheckSignal, this, &DebugConnectors::routingCheckSlot);
	connect(m_schematicGraphicsView, &SketchWidget::routingCheckSignal, this, &DebugConnectors::routingCheckSlot);
	connect(m_pcbGraphicsView, &SketchWidget::routingCheckSignal, this, &DebugConnectors::routingCheckSlot);
}

QSet<QString> DebugConnectors::getItemConnectorSet(ConnectorItem *connectorItem) {
	QSet<QString> set;
	QRegularExpression re("^(AM|VM|OM)\\d+");
	Q_FOREACH (ConnectorItem * toConnectorItem, connectorItem->connectedToItems()) {
		ItemBase * attachedToItem = toConnectorItem->attachedTo();
		VirtualWire * virtualWire = qobject_cast<VirtualWire *>(attachedToItem);
		if (virtualWire != nullptr) continue;
		if (re.match(attachedToItem->instanceTitle()).hasMatch()) continue; // Ignore Multimeter. It has no pcb connections.
		QString idString = toConnectorItem->attachedToInstanceTitle() + ":" + toConnectorItem->connectorSharedID();
		set.insert(idString);
	}
	return set;
}

QList<ItemBase *> DebugConnectors::toSortedItembases(const QList<QGraphicsItem *> &graphicsItems) {
	QList<ItemBase *> itembases;
	foreach (QGraphicsItem* item, graphicsItems) {
		ItemBase * part = dynamic_cast<ItemBase *>(item);
		if (!part) continue;
		itembases.append(part);
	}
	std::sort(itembases.begin(), itembases.end(), [](ItemBase * b1, ItemBase * b2) {
		return b1->id() < b2->id();
	});
	return itembases;
}

void DebugConnectors::collectPartsForCheck(QList<ItemBase *> &partList, QGraphicsScene *scene) {
	Q_FOREACH (QGraphicsItem * item, scene->items()) {
		ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
		if (!itemBase) continue;
		if (itemBase->layerKinChief() != itemBase) continue;

		partList.append(itemBase);
	}
}

void DebugConnectors::routingCheckSlot() {
	if(firstCall) {
	    doRoutingCheck();
	    firstCall = false;
	    return;
	}
	qint64 elapsed = lastExecution.elapsed();
	if (elapsed < minimumInterval) {
		if(!timer->isActive()){
			timer->start(minimumInterval - elapsed);
		}
	} else {
		doRoutingCheck();
	}
}

void DebugConnectors::doRoutingCheck() {
	lastExecution.restart();
	bool foundError = false;
	QHash<qint64, ItemBase *> bbID2ItemHash;
	QHash<qint64, ItemBase *> pcbID2ItemHash;
	QList<ItemBase *> bbList;
	collectPartsForCheck(bbList, m_breadboardGraphicsView->scene());
	Q_FOREACH (ItemBase * part, bbList) {
		bbID2ItemHash.insert(part->id(), part);
		//						QString("!!!!!! Duplicate breadboard part found. title:"
		//								"%1 id1: %2 id2: %3 moduleID1: %4 moduleID2: %5 "
		//								"viewIDname: 1: %6 2: %7 viewLayerIDs: 1: %8 2: %9")
		//						.arg(part->instanceTitle())
		//						.arg(firstPart->id())
		//						.arg(part->id())
		//						.arg(firstPart->moduleID())
		//						.arg(part->moduleID())
		//						.arg(firstPart->viewIDName())
		//						.arg(part->viewIDName())
		//						.arg(firstPart->viewLayerID())
		//						.arg(part->viewLayerID()));
	}

	QList<ItemBase *> pcbList;
	collectPartsForCheck(pcbList, m_pcbGraphicsView->scene());
	Q_FOREACH (ItemBase * part, pcbList) {
		if (part->viewLayerID() == ViewLayer::ViewLayerID::Silkscreen0) continue;
		if (part->viewLayerID() == ViewLayer::ViewLayerID::Silkscreen1) continue;
		if (part->viewLayerID() == ViewLayer::ViewLayerID::Board) continue;

		pcbID2ItemHash.insert(part->id(), part);
	}

	QList<ItemBase *> schList;
	collectPartsForCheck(schList, m_schematicGraphicsView->scene());
	Q_FOREACH (ItemBase* schPart, schList) {
		if (dynamic_cast<NetLabel *>(schPart) != nullptr) continue;
		if (schPart->viewLayerID() == ViewLayer::ViewLayerID::SchematicText) continue;
		ItemBase * bbPart = bbID2ItemHash.value(schPart->id());
		ItemBase * pcbPart = pcbID2ItemHash.value(schPart->id());
		if (bbPart != nullptr) {
			QHash<QString, ConnectorItem *> bbID2ConnectorHash;
			Q_FOREACH (ConnectorItem * connectorItem, bbPart->cachedConnectorItems()) {
				bbID2ConnectorHash.insert(connectorItem->connectorSharedID(), connectorItem);
			}
			Q_FOREACH (ConnectorItem * schConnectorItem, schPart->cachedConnectorItems()) {
				ConnectorItem * bbConnectorItem = bbID2ConnectorHash.value(schConnectorItem->connectorSharedID());
				if (bbConnectorItem != nullptr) {
					if (bbConnectorItem != nullptr) {
						QSet<QString> schSet = getItemConnectorSet(schConnectorItem);
						QSet<QString> bbSet = getItemConnectorSet(bbConnectorItem);
						if (schSet != bbSet) {
							QString schSetString = TextUtils::setToString(schSet);
							QString bbSetString = TextUtils::setToString(bbSet);
							DebugDialog::debug(QString("Connectors with id: %1 for item: %2 have differing QSets. Schema{%3} BB{%4}").arg(
										   schConnectorItem->connectorSharedID(),
										   schPart->instanceTitle(),
										   schSetString,
										   bbSetString));
							foundError = true;
						}
					}
				}
			}
		}
		if (pcbPart != nullptr) {
			QHash<QString, ConnectorItem *> pcbID2ConnectorHash;
			Q_FOREACH (ConnectorItem * connectorItem, pcbPart->cachedConnectorItems()) {
				pcbID2ConnectorHash.insert(connectorItem->connectorSharedID(), connectorItem);
			}
			Q_FOREACH (ConnectorItem * schConnectorItem, schPart->cachedConnectorItems()) {
				ConnectorItem * pcbConnectorItem = pcbID2ConnectorHash.value(schConnectorItem->connectorSharedID());
				if (pcbConnectorItem != nullptr) {
					if (pcbConnectorItem != nullptr) {
						QSet<QString> schSet = getItemConnectorSet(schConnectorItem);
						QSet<QString> pcbSet = getItemConnectorSet(pcbConnectorItem);
						if (schSet != pcbSet) {
							QSet<QString> setSchMinusPcb = schSet - pcbSet;
							QSet<QString> setPcbMinusSch = pcbSet - schSet;
							bool nonWireError = !setPcbMinusSch.isEmpty();
							Q_FOREACH(QString str, setSchMinusPcb) {
								if (!str.startsWith("Wire")) {
									nonWireError = true;
								} else {
									schSet -= str;
								}
							}
							if (nonWireError) {
								QString schSetString = TextUtils::setToString(schSet);
								QString pcbSetString = TextUtils::setToString(pcbSet);
								DebugDialog::debug(QString("Connectors with id: %1 for item: %2 have differing QSets. Schema{%3} PCB{%4}").arg(
											   schConnectorItem->connectorSharedID(),
											   schPart->instanceTitle(),
											   schSetString,
											   pcbSetString));
								foundError = true;
							}
						}
					}
				}
			}
		}
	}


	QColor newColor = foundError ? QColor("red") : QColor("white");
	m_breadboardGraphicsView->setBackgroundColor(newColor, false);
	m_schematicGraphicsView->setBackgroundColor(newColor, false);
	if (foundError) {
		m_pcbGraphicsView->setBackgroundColor(newColor, false);
	}
}
