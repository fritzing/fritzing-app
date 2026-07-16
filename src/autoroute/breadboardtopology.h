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

#ifndef BREADBOARDTOPOLOGY_H
#define BREADBOARDTOPOLOGY_H

#include <QHash>
#include <QList>
#include <QRectF>
#include <QSet>
#include <QString>
#include <QStringList>

#include "../connectors/connectoritem.h"

class ItemBase;
class QGraphicsItem;
class QGraphicsScene;

class BreadboardTopology
{
public:
	struct Board {
		ItemBase * item = nullptr;
		QList<ConnectorItem *> holes;
		QRectF bounds;
		int itemConnectorCount = 0;
	};

	bool discover(const QGraphicsScene * scene, const QList<QGraphicsItem *> & selectedItems);

	bool isValid() const;
	const QList<Board> & boards() const;
	QList<ItemBase *> boardItems() const;
	const QList<ConnectorItem *> & holes() const;
	const QSet<ConnectorItem *> & reservedHoles() const;
	const QRectF & bounds() const;
	int sceneConnectorCount() const;
	int itemConnectorCount() const;
	int busCount() const;
	QStringList diagnosticLines() const;

	static bool isBreadboardItem(ItemBase * itemBase);
	static bool isBreadboardDecorationItem(ItemBase * itemBase);
	static bool isTargetBreadboardHole(ConnectorItem * connectorItem);
	static bool connectorsShareBus(ConnectorItem * first, ConnectorItem * second);

private:
	void clear();
	static ItemBase * ownerChief(const ConnectorItem * connectorItem);
	static ItemBase * itemChief(QGraphicsItem * graphicsItem);
	static QString itemSummary(ItemBase * itemBase);
	static QString connectorSummary(ConnectorItem * connectorItem);
	void addBoard(ItemBase * boardItem, const QList<ConnectorItem *> & holes);
	void addHole(ConnectorItem * hole);
	void buildBusIndex();

private:
	QList<Board> m_boards;
	QList<ConnectorItem *> m_holes;
	QSet<ConnectorItem *> m_reservedHoles;
	QHash<QString, QList<ConnectorItem *> > m_busIndex;
	QRectF m_bounds;
	int m_sceneConnectorCount = 0;
	int m_itemConnectorCount = 0;
	QStringList m_diagnosticLines;
};

#endif
