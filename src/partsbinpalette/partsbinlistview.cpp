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

$Revision: 6934 $:
$Author: irascibl@gmail.com $:
$Date: 2013-03-22 08:35:24 +0100 (Fr, 22. Mrz 2013) $

********************************************************************/


#include <QMenu>
#include <QMimeData>

#include "../debugdialog.h"
#include "../infoview/htmlinfoview.h"
#include "../items/itembase.h"
#include "../fsvgrenderer.h"
#include "../itemdrag.h"
#include "../items/partfactory.h"
#include "../layerattributes.h"
#include "partsbinpalettewidget.h"

#include "partsbinlistview.h"

static const QColor SectionHeaderBackgroundColor(128, 128, 128);
static const QColor SectionHeaderForegroundColor(32, 32, 32);

PartsBinListView::PartsBinListView(ReferenceModel* referenceModel, PartsBinPaletteWidget *parent)
	: QListWidget((QWidget *) parent), PartsBinView(referenceModel, parent)
{
	m_infoView = NULL;
	m_hoverItem = NULL;
	m_infoViewOnHover = true;
	setMouseTracking(true);
	setSpacing(2);
	setIconSize(QSize(16,16));
	setSortingEnabled(false);

	setDragEnabled(true);
	viewport()->setAcceptDrops(true);
	setDropIndicatorShown(true);
	setDragDropMode(QAbstractItemView::DragDrop);
	setAcceptDrops(true);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(
    	this, SIGNAL(customContextMenuRequested(const QPoint&)),
    	this, SLOT(showContextMenu(const QPoint&))
    );
}

PartsBinListView::~PartsBinListView() {
}

void PartsBinListView::doClear() {
	m_hoverItem = NULL;
	PartsBinView::doClear();
	clear();
}

int PartsBinListView::setItemAux(ModelPart * modelPart, int position) {
	if (modelPart->modelPartShared() == NULL) return position;
	if (modelPart->itemType() == ModelPart::Unknown) {
		// don't want the empty root to appear in the view
		return position;
	}

	emit settingItem();
	QString moduleID = modelPart->moduleID();
	if (contains(moduleID)) {
		return position;
	}

	QListWidgetItem * lwi = new QListWidgetItem(modelPart->title());
	if (modelPart->itemType() == ModelPart::Space) {
		lwi->setBackground(QBrush(SectionHeaderBackgroundColor));
		lwi->setForeground(QBrush(SectionHeaderForegroundColor));
		lwi->setData(Qt::UserRole, 0);
		lwi->setFlags(0);
		lwi->setText("        " + TranslatedCategoryNames.value(modelPart->instanceText(), modelPart->instanceText()));
	}
	else {
		loadImage(modelPart, lwi, moduleID);
	}

	if(position > -1 && position < count()) {
		insertItem(position, lwi);
	} else {
		addItem(lwi);
		position = this->count();
	}

	return position;
	
}

void PartsBinListView::mouseMoveEvent(QMouseEvent *event) {
	if(m_infoViewOnHover) {
		QListWidgetItem * item = itemAt(event->pos());
		if (item != NULL) {
			showInfo(item);
		}
		else {
			m_hoverItem = NULL;
		}
	}

	QListWidget::mouseMoveEvent(event);
}

void PartsBinListView::showInfo(QListWidgetItem * item) {
	if (item == m_hoverItem) {
		// no change
		return;
	}

	if (m_hoverItem != NULL && m_infoView != NULL) {
		ItemBase * itemBase = itemItemBase(m_hoverItem);
		if (itemBase != NULL) {
			m_infoView->hoverLeaveItem(NULL, NULL, itemBase);
		}
	}

	if (item == NULL) {
		return;
	}

	m_hoverItem = item;
    if (m_infoView != NULL) {
	    ItemBase * itemBase = itemItemBase(item);
	    if (itemBase != NULL) {
	        m_infoView->hoverEnterItem(NULL, NULL, itemBase, swappingEnabled());
        }
    }
}


void PartsBinListView::mousePressEvent(QMouseEvent *event) {
	m_dragStartPos = event->pos();
	QListWidget::mousePressEvent(event);

	QListWidgetItem * current = currentItem();
	if (current == NULL) {
		m_hoverItem = NULL;
        if (m_infoView != NULL) m_infoView->viewItemInfo(NULL, NULL, false);
		return;
	}

	showInfo(current);
	if (m_infoView != NULL) m_infoView->viewItemInfo(NULL, itemItemBase(current), false);
}

void PartsBinListView::setInfoView(HtmlInfoView * infoView) {
	m_infoView = infoView;
}

void PartsBinListView::removePart(const QString &moduleID) {
	m_hoverItem = NULL;
	int idxToRemove = position(moduleID);
	if(idxToRemove > -1) {
		m_itemBaseHash.remove(moduleID);
		delete takeItem(idxToRemove);
	}
}

void PartsBinListView::removeParts() {
	m_hoverItem = NULL;
    m_itemBaseHash.clear();
    while (count() > 0) {
        delete takeItem(0);
    }
}

int PartsBinListView::position(const QString &moduleID) {
	for(int i=0; i < count(); i++) {
		if(itemModuleID(item(i)) == moduleID) {
			return i;
		}
	}
	return -1;
}

ItemBase * PartsBinListView::itemItemBase(const QListWidgetItem *item) const {
    return item->data(Qt::UserRole).value<ItemBase *>();
}

ModelPart *PartsBinListView::itemModelPart(const QListWidgetItem *item) const {
	ItemBase * itemBase = itemItemBase(item);
	if (itemBase == NULL) return NULL;

	return itemBase->modelPart();
}

const QString &PartsBinListView::itemModuleID(const QListWidgetItem *item) {
    ModelPart *modelPart = itemModelPart(item);
    if (modelPart == NULL) {
        // this shouldn't happen?
        return ___emptyString___;
    }

	return modelPart->moduleID();
}

ItemBase *PartsBinListView::selectedItemBase() {
	if(selectedItems().size()==1) {
		return itemItemBase(selectedItems()[0]);
	}
	return NULL;
}

ModelPart *PartsBinListView::selectedModelPart() {
	if(selectedItems().size()==1) {
		return itemModelPart(selectedItems()[0]);
	}
	return NULL;
}

bool PartsBinListView::swappingEnabled() {
	return false;
}

void PartsBinListView::setSelected(int position, bool doEmit) {
	Q_UNUSED(doEmit);
	if(position > -1 && position < count()) {
		item(position)->setSelected(true);
	} else {
		setCurrentRow(position);
	}
}

/*void PartsBinListView::dragEnterEvent(QDragEnterEvent* event) {
	dragMoveEnterEventAux(event);
}*/

void PartsBinListView::dropEvent(QDropEvent* event) {
	dropEventAux(event);
	QListWidget::dropEvent(event);
}

void PartsBinListView::startDrag(Qt::DropActions supportedActions) {
	QListWidget::startDrag(supportedActions);
	ItemDrag::dragIsDone();
}

void PartsBinListView::moveItem(int fromIndex, int toIndex) {
	itemMoved(fromIndex,toIndex);
	emit informItemMoved(fromIndex, toIndex);
}

void PartsBinListView::itemMoved(int fromIndex, int toIndex) {
	QListWidgetItem *item = takeItem(fromIndex);
	insertItem(toIndex,item);
	//setItemSelected(item,true);
	setSelected(toIndex);
}

int PartsBinListView::itemIndexAt(const QPoint& pos, bool &trustIt) {
	trustIt = true;
	QListWidgetItem *item = itemAt(pos);
	if(item) {
		return row(item);
	} else if(rect().contains(pos)) {
		trustIt = false;
		return -1;
	} else {
		return -1;
	}
}

bool PartsBinListView::dropMimeData(int index, const QMimeData *data, Qt::DropAction action) {
	Q_UNUSED(index);
	Q_UNUSED(action);
	if(data->hasFormat("action") && (data->data("action") == "part-reordering")) {
		/*QByteArray itemData = data->data("application/x-dnditemdata");
		QDataStream dataStream(&itemData, QIODevice::ReadOnly);

		QString moduleID;
		QPointF offset;
		dataStream >> moduleID >> offset;

		ModelPart *modelPart = m_partHash[moduleID];

		QModelIndex idx = model()->index(index, 0);
		model()->setData(idx, qVariantFromValue(modelPart), Qt::UserRole);*/

		return true;
	} else {
		return false;
	}
	return true;
}

QMimeData * PartsBinListView::mimeData(const QList<QListWidgetItem *> items) const {
	if (items.count()>1) {
		throw "PartsBinListView::mimeData too many items";
	}

	if(items.count()==1) {
		QListWidgetItem * item = items.at(0);
		ModelPart * modelPart = itemModelPart(item);
		QByteArray itemData;
		QDataStream dataStream(&itemData, QIODevice::WriteOnly);

		dataStream << modelPart->moduleID() << QPointF(0,0);

		QMimeData *mimeData = new QMimeData;
		mimeData->setData("application/x-dnditemdata", itemData);
		mimeData->setData("action", "part-reordering");

		return mimeData;
	} else {
		return QListWidget::mimeData(items);
	}
}

QStringList PartsBinListView::mimeTypes() const {
	QStringList list;
	list << "application/x-dnditemdata" << "action";
	return list;
}

QList<QObject*> PartsBinListView::orderedChildren() {
	QList<QObject*> result;
	for(int i=0; i < count(); i++) {
		ModelPart *mp = itemModelPart(item(i));
		if(mp) {
			result << mp;
		}
	}
	return result;
}

void PartsBinListView::showContextMenu(const QPoint& pos) {
	QListWidgetItem *it = itemAt(pos);

	QMenu *menu;
	if(it) {
		clearSelection();
		it->setSelected(true);
		menu = m_parent->partContextMenu();
	} else {
		menu = m_parent->combinedMenu();
	}
    menu->exec(mapToGlobal(pos));
}

void PartsBinListView::reloadPart(const QString & moduleID) {
	if (!contains(moduleID)) return;

	for(int i = 0; i < count(); i++) {
		QListWidgetItem * lwi = item(i);
		ItemBase * itemBase = itemItemBase(lwi);
		if (itemBase == NULL) continue;

		if (itemBase->moduleID().compare(moduleID) == 0) {
			lwi->setText(itemBase->title());
			loadImage(itemBase->modelPart(), lwi, moduleID);
			return;
		}
	}
}

void PartsBinListView::loadImage(ModelPart * modelPart, QListWidgetItem * lwi, const QString & moduleID) 
{
    ItemBase * itemBase = ItemBaseHash.value(moduleID);
    if (itemBase == NULL) {
		itemBase = PartFactory::createPart(modelPart, ViewLayer::NewTop, ViewLayer::IconView, ViewGeometry(), ItemBase::getNextID(), NULL, NULL, false);
		ItemBaseHash.insert(moduleID, itemBase);
		LayerAttributes layerAttributes;
        itemBase->initLayerAttributes(layerAttributes, ViewLayer::IconView, ViewLayer::Icon, itemBase->viewLayerPlacement(), false, false);
		FSvgRenderer * renderer = itemBase->setUpImage(modelPart, layerAttributes);
		if (renderer != NULL) {
			if (itemBase) {
				itemBase->setFilename(renderer->filename());
			}
            itemBase->setSharedRendererEx(renderer);
        }
    }
	lwi->setData(Qt::UserRole, qVariantFromValue( itemBase ) );
	QSize size(HtmlInfoView::STANDARD_ICON_IMG_WIDTH, HtmlInfoView::STANDARD_ICON_IMG_HEIGHT);
	QPixmap * pixmap = FSvgRenderer::getPixmap(itemBase->renderer(), size);
	lwi->setIcon(QIcon(*pixmap));
	delete pixmap;
	lwi->setData(Qt::UserRole + 1, itemBase->renderer()->defaultSize());

	m_itemBaseHash.insert(moduleID, itemBase);
}
