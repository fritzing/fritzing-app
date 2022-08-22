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

#include "fgraphicsscene.h"
#include "../items/paletteitembase.h"
#include "../items/wire.h"
#include "../connectors/connectoritem.h"
#include "../sketch/infographicsview.h"

#include <QToolTip>

FGraphicsScene::FGraphicsScene( QObject * parent) : QGraphicsScene(parent)
{
	m_displayHandles = true;
	//setItemIndexMethod(QGraphicsScene::NoIndex);
}

void FGraphicsScene::helpEvent(QGraphicsSceneHelpEvent *helpEvent)
{
	// TODO: how do we get a QTransform?
	QList<QGraphicsItem*> itemList = this->items(helpEvent->scenePos(), Qt::IntersectsItemShape, Qt::DescendingOrder, QTransform());

	bool found = false;
	QString text;
	QPoint point;
	QGraphicsItem * item = nullptr;

	//Find connectors first
	for (int i = 0; i < itemList.size(); i++) {
		item = itemList.at(i);
		ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase == NULL) {
			ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(item);
			if (connectorItem && !connectorItem->isHybrid()) {
				connectorItem->updateTooltip();
				found = true;
				break;
			}
		}
	}

	//If there are not connectors, find parts
	if (!found){
		for (int i = 0; i < itemList.size(); i++) {
			item = itemList.at(i);
			ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
			if (itemBase) {
				itemBase->updateTooltip();
				break;
			}
		}
	}

	if(!item) return;

	if (!item->toolTip().isEmpty()) {
		text = item->toolTip();
		point = helpEvent->screenPos();
	}

	// Show or hide the tooltip
	QToolTip::showText(point, text);
}

void FGraphicsScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *contextMenuEvent)
{
	m_lastContextMenuPos = contextMenuEvent->scenePos();
	QGraphicsScene::contextMenuEvent(contextMenuEvent);
}

QPointF FGraphicsScene::lastContextMenuPos() {
	return m_lastContextMenuPos;
}

void FGraphicsScene::setDisplayHandles(bool displayHandles) {
	m_displayHandles = displayHandles;
}

bool FGraphicsScene::displayHandles() {
	return m_displayHandles;
}

QList<ItemBase *> FGraphicsScene::lockedSelectedItems() {
	QList<ItemBase *> items;
	foreach (QGraphicsItem * gitem,  this->selectedItems()) {
		ItemBase *itemBase = dynamic_cast<ItemBase *>(gitem);
		if (itemBase == NULL) continue;
		if (itemBase->moveLock()) {
			items.append(itemBase);
		}
	}
	return items;
}
