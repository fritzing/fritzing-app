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

#include "breadboardtopology.h"

#include <QGraphicsItem>
#include <QGraphicsScene>

#include "../items/itembase.h"
#include "../items/moduleidnames.h"

namespace {
constexpr int MinimumBreadboardHoleCount = 10;
}

bool BreadboardTopology::discover(QGraphicsScene * scene, const QList<QGraphicsItem *> & selectedItems)
{
	clear();
	if (scene == nullptr) return false;

	QSet<ItemBase *> explicitTargets;
	Q_FOREACH (QGraphicsItem * selectedItem, selectedItems) {
		ItemBase * chief = itemChief(selectedItem);
		if (chief == nullptr) continue;
		if (isBreadboardItem(chief)) {
			explicitTargets.insert(chief);
			m_diagnosticLines << QString("selected topology target: %1").arg(itemSummary(chief));
		}
	}

	QHash<ItemBase *, QList<ConnectorItem *> > holesByOwner;
	QHash<ItemBase *, int> connectorCountsByOwner;
	Q_FOREACH (QGraphicsItem * graphicsItem, scene->items()) {
		auto * connectorItem = dynamic_cast<ConnectorItem *>(graphicsItem);
		if (connectorItem == nullptr) continue;
		m_sceneConnectorCount++;

		ItemBase * owner = ownerChief(connectorItem);
		if (owner == nullptr) continue;
		connectorCountsByOwner[owner]++;

		if (!isTargetBreadboardHole(connectorItem)) continue;
		holesByOwner[owner].append(connectorItem);
	}

	QSet<ItemBase *> acceptedOwners;
	Q_FOREACH (ItemBase * target, explicitTargets) {
		QList<ConnectorItem *> holes = holesByOwner.value(target);
		if (holes.count() >= MinimumBreadboardHoleCount) {
			addBoard(target, holes);
			acceptedOwners.insert(target);
		}
		else {
			m_diagnosticLines << QString("selected topology target rejected: holes=%1 owner=%2")
			                     .arg(holes.count())
			                     .arg(itemSummary(target));
		}
	}

	if (m_boards.isEmpty()) {
		ItemBase * bestOwner = nullptr;
		int bestHoleCount = 0;
		for (auto it = holesByOwner.constBegin(); it != holesByOwner.constEnd(); ++it) {
			ItemBase * owner = it.key();
			int holeCount = it.value().count();
			m_diagnosticLines << QString("topology owner candidate: holes=%1 connectors=%2 owner=%3")
			                     .arg(holeCount)
			                     .arg(connectorCountsByOwner.value(owner))
			                     .arg(itemSummary(owner));
			if (holeCount > bestHoleCount) {
				bestOwner = owner;
				bestHoleCount = holeCount;
			}
		}

		if (bestOwner != nullptr && bestHoleCount >= MinimumBreadboardHoleCount) {
			addBoard(bestOwner, holesByOwner.value(bestOwner));
			acceptedOwners.insert(bestOwner);
			m_diagnosticLines << QString("topology selected largest connector owner: holes=%1 owner=%2")
			                     .arg(bestHoleCount)
			                     .arg(itemSummary(bestOwner));
		}
	}

	for (int boardIndex = 0; boardIndex < m_boards.count(); boardIndex++) {
		Board & board = m_boards[boardIndex];
		board.itemConnectorCount = connectorCountsByOwner.value(board.item);
		m_itemConnectorCount += board.itemConnectorCount;
	}

	buildBusIndex();
	m_diagnosticLines << QString("topology summary: boards=%1 holes=%2 reserved=%3 buses=%4 bounds=[%5,%6 %7x%8] sceneConnectors=%9 itemConnectors=%10")
	                     .arg(m_boards.count())
	                     .arg(m_holes.count())
	                     .arg(m_reservedHoles.count())
	                     .arg(m_busIndex.count())
	                     .arg(m_bounds.x()).arg(m_bounds.y()).arg(m_bounds.width()).arg(m_bounds.height())
	                     .arg(m_sceneConnectorCount)
	                     .arg(m_itemConnectorCount);
	return isValid();
}

bool BreadboardTopology::isValid() const
{
	return !m_boards.isEmpty() && !m_holes.isEmpty() && !m_bounds.isEmpty();
}

const QList<BreadboardTopology::Board> & BreadboardTopology::boards() const
{
	return m_boards;
}

QList<ItemBase *> BreadboardTopology::boardItems() const
{
	QList<ItemBase *> items;
	Q_FOREACH (const Board & board, m_boards) {
		if (board.item != nullptr && !items.contains(board.item)) items.append(board.item);
	}
	return items;
}

const QList<ConnectorItem *> & BreadboardTopology::holes() const
{
	return m_holes;
}

const QSet<ConnectorItem *> & BreadboardTopology::reservedHoles() const
{
	return m_reservedHoles;
}

const QRectF & BreadboardTopology::bounds() const
{
	return m_bounds;
}

int BreadboardTopology::sceneConnectorCount() const
{
	return m_sceneConnectorCount;
}

int BreadboardTopology::itemConnectorCount() const
{
	return m_itemConnectorCount;
}

int BreadboardTopology::busCount() const
{
	return m_busIndex.count();
}

QStringList BreadboardTopology::diagnosticLines() const
{
	QStringList lines = m_diagnosticLines;
	for (auto it = m_busIndex.constBegin(); it != m_busIndex.constEnd(); ++it) {
		QList<ConnectorItem *> busHoles = it.value();
		if (busHoles.isEmpty()) continue;
		lines << QString("topology bus: id=%1 holes=%2 sample=%3")
		         .arg(it.key())
		         .arg(busHoles.count())
		         .arg(connectorSummary(busHoles.first()));
	}
	return lines;
}

bool BreadboardTopology::isBreadboardItem(ItemBase * itemBase)
{
	if (itemBase == nullptr) return false;

	QString moduleID = itemBase->moduleID();
	if (moduleID == ModuleIDNames::BreadboardModuleIDName) return true;
	if (moduleID == ModuleIDNames::FullPlusBreadboardModuleIDName) return true;
	if (moduleID == ModuleIDNames::PerfboardModuleIDName) return true;
	if (moduleID == ModuleIDNames::StripboardModuleIDName) return true;
	if (moduleID == ModuleIDNames::Stripboard2ModuleIDName) return true;
	if (moduleID.endsWith(ModuleIDNames::BreadboardModuleIDName, Qt::CaseInsensitive)) return true;
	if (moduleID.compare("HalfBreadboardModuleID", Qt::CaseInsensitive) == 0) return true;
	if (moduleID.compare("HalfMinusBreadboardModuleID", Qt::CaseInsensitive) == 0) return true;
	if (moduleID.compare("MiniBreadboardModuleID", Qt::CaseInsensitive) == 0) return true;
	if (moduleID.compare("TinyBreadboardModuleID", Qt::CaseInsensitive) == 0) return true;

	return false;
}

bool BreadboardTopology::isBreadboardDecorationItem(ItemBase * itemBase)
{
	if (itemBase == nullptr) return false;

	QString moduleID = itemBase->moduleID();
	return moduleID == ModuleIDNames::BreadboardLogoTextModuleIDName
	    || moduleID == ModuleIDNames::BreadboardLogoImageModuleIDName;
}

bool BreadboardTopology::isTargetBreadboardHole(ConnectorItem * connectorItem)
{
	if (connectorItem == nullptr) return false;
	return connectorItem->connectorType() == Connector::Female || connectorItem->bus() != nullptr;
}

bool BreadboardTopology::connectorsShareBus(ConnectorItem * first, ConnectorItem * second)
{
	if (first == nullptr || second == nullptr) return false;
	if (first == second) return true;

	ItemBase * firstItem = first->attachedTo();
	ItemBase * secondItem = second->attachedTo();
	if (firstItem == nullptr || secondItem == nullptr) return false;

	QList<ConnectorItem *> firstBusHoles;
	if (firstItem->busConnectorItems(first, firstBusHoles) && firstBusHoles.contains(second)) return true;

	QList<ConnectorItem *> secondBusHoles;
	if (secondItem->busConnectorItems(second, secondBusHoles) && secondBusHoles.contains(first)) return true;

	return false;
}

void BreadboardTopology::clear()
{
	m_boards.clear();
	m_holes.clear();
	m_reservedHoles.clear();
	m_busIndex.clear();
	m_bounds = QRectF();
	m_sceneConnectorCount = 0;
	m_itemConnectorCount = 0;
	m_diagnosticLines.clear();
}

ItemBase * BreadboardTopology::ownerChief(ConnectorItem * connectorItem)
{
	if (connectorItem == nullptr) return nullptr;
	ItemBase * owner = connectorItem->attachedTo();
	return owner == nullptr ? nullptr : owner->layerKinChief();
}

ItemBase * BreadboardTopology::itemChief(QGraphicsItem * graphicsItem)
{
	auto * itemBase = dynamic_cast<ItemBase *>(graphicsItem);
	return itemBase == nullptr ? nullptr : itemBase->layerKinChief();
}

QString BreadboardTopology::itemSummary(ItemBase * itemBase)
{
	if (itemBase == nullptr) return QString("<null item>");
	return QString("%1 id=%2 module=%3")
	        .arg(itemBase->title())
	        .arg(itemBase->id())
	        .arg(itemBase->moduleID());
}

QString BreadboardTopology::connectorSummary(ConnectorItem * connectorItem)
{
	if (connectorItem == nullptr) return QString("<null connector>");
	QPointF p = connectorItem->sceneAdjustedTerminalPoint(nullptr);
	return QString("%1:%2 bus=%3 at=(%4,%5)")
	        .arg(connectorItem->attachedToTitle())
	        .arg(connectorItem->connectorSharedID())
	        .arg(connectorItem->busID())
	        .arg(p.x())
	        .arg(p.y());
}

void BreadboardTopology::addBoard(ItemBase * boardItem, const QList<ConnectorItem *> & holes)
{
	if (boardItem == nullptr || holes.isEmpty()) return;

	Board board;
	board.item = boardItem;
	Q_FOREACH (ConnectorItem * hole, holes) {
		if (hole == nullptr) continue;
		board.holes.append(hole);
		addHole(hole);

		QPointF p = hole->sceneAdjustedTerminalPoint(nullptr);
		QRectF pointRect(p, QSizeF(1.0, 1.0));
		board.bounds = board.bounds.isNull() ? pointRect : board.bounds | pointRect;
	}

	if (board.holes.isEmpty()) return;
	board.bounds = board.bounds.adjusted(-24.0, -24.0, 24.0, 24.0);
	m_bounds = m_bounds.isNull() ? board.bounds : m_bounds | board.bounds;
	m_boards.append(board);
}

void BreadboardTopology::addHole(ConnectorItem * hole)
{
	if (hole == nullptr) return;
	if (!m_holes.contains(hole)) m_holes.append(hole);
	if (hole->connectionsCount() > 0) m_reservedHoles.insert(hole);
}

void BreadboardTopology::buildBusIndex()
{
	QSet<ConnectorItem *> visited;
	Q_FOREACH (ConnectorItem * hole, m_holes) {
		if (hole == nullptr || visited.contains(hole)) continue;

		QList<ConnectorItem *> busHoles;
		ItemBase * owner = hole->attachedTo();
		if (owner != nullptr) owner->busConnectorItems(hole, busHoles);
		if (busHoles.isEmpty()) busHoles.append(hole);

		QList<ConnectorItem *> filtered;
		Q_FOREACH (ConnectorItem * candidate, busHoles) {
			if (candidate == nullptr) continue;
			if (!m_holes.contains(candidate)) continue;
			if (!filtered.contains(candidate)) filtered.append(candidate);
			visited.insert(candidate);
		}

		if (filtered.isEmpty()) continue;
		QString key = hole->busID();
		if (key.isEmpty()) key = QString("%1:%2").arg(hole->attachedToID()).arg(hole->connectorSharedID());
		m_busIndex.insert(key, filtered);
	}
}
