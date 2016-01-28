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

$Revision: 6912 $:
$Author: irascibl@gmail.com $:
$Date: 2013-03-09 08:18:59 +0100 (Sa, 09. Mrz 2013) $

********************************************************************/

#include <QGraphicsScene>
#include <QPoint>
#include <QSet>
#include <QSvgWidget>

#include "partsbiniconview.h"
#include "graphicsflowlayout.h"
#include "../items/paletteitem.h"
#include "../debugdialog.h"
#include "svgiconwidget.h"
#include "../model/palettemodel.h"
#include "../items/partfactory.h"
#include "partsbinpalettewidget.h"

#define ICON_SPACING 5

PartsBinIconView::PartsBinIconView(ReferenceModel* referenceModel, PartsBinPaletteWidget *parent)
    : InfoGraphicsView((QWidget*)parent), PartsBinView(referenceModel, parent)
{
	setAcceptWheelEvents(false);
    setFrameStyle(QFrame::Raised | QFrame::StyledPanel);
	setAlignment(Qt::AlignLeft | Qt::AlignTop);
	setAcceptDrops(true);

    QGraphicsScene* scene = new QGraphicsScene(this);
    this->setScene(scene);

    m_layouter = NULL;
    m_layout = NULL;
    setupLayout();

    //connect(scene, SIGNAL(selectionChanged()), this, SLOT(informNewSelection()));
    //connect(scene, SIGNAL(changed(const QList<QRectF>&)), this, SLOT(informNewSelection()));

    m_noSelectionChangeEmition = false;
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(
    	this, SIGNAL(customContextMenuRequested(const QPoint&)),
    	this, SLOT(showContextMenu(const QPoint&))
    );
}

void PartsBinIconView::setupLayout() {
	// TODO Mariano: smells like leak, but if this two lines are uncommented there's a crash
	// Also tried to iterate through layout children, but didn't work
	// Mariano: see under doClear--deleting m_layouter automatically deletes m_layout

	//delete m_layouter;
	//delete m_layout;


    m_layouter = new QGraphicsWidget;
    m_layout = new GraphicsFlowLayout(m_layouter, ICON_SPACING);
    scene()->addItem(m_layouter);
}

void PartsBinIconView::updateSize(QSize newSize) {
	updateSizeAux(newSize.width());
}

void PartsBinIconView::updateSize() {
	updateSizeAux(width());
}

void PartsBinIconView::updateSizeAux(int width) {
	setSceneRect(0, 0, width, m_layout->heightForWidth(width));
}

void PartsBinIconView::resizeEvent(QResizeEvent * event) {
	InfoGraphicsView::resizeEvent(event);
	updateSize(event->size());
}

void PartsBinIconView::mousePressEvent(QMouseEvent *event) {
	SvgIconWidget* icon = svgIconWidgetAt(event->pos());
	if (icon == NULL || event->button() != Qt::LeftButton) {
		QGraphicsView::mousePressEvent(event);
		if (icon == NULL) {
			viewItemInfo(NULL);
		}
	} else {
		if (icon != NULL) {
			QList<QGraphicsItem *> items = scene()->selectedItems();
			for (int i = 0; i < items.count(); i++) {
				// not sure why clearSelection doesn't do the update, but whatever...
				items[i]->setSelected(false);
				items[i]->update();
			}
			icon->setSelected(true);
			icon->update();

			QPointF mts = this->mapToScene(event->pos());
			QString moduleID = icon->moduleID();
			QPoint hotspot = (mts.toPoint()-icon->pos().toPoint());

			viewItemInfo(icon->itemBase());

			mousePressOnItem(event->pos(), moduleID, icon->rect().size().toSize(), (mts - icon->pos()), hotspot );
		}
		else {
			viewItemInfo(NULL);
		}
	}
	informNewSelection();
}

void PartsBinIconView::doClear() {
	PartsBinView::doClear();
	m_layout->clear();
	delete m_layouter;			// deleting layouter deletes layout
	delete scene();				// deleting scene deletes QGraphicsItems
	setScene(new QGraphicsScene(this));
	setupLayout();
}

void PartsBinIconView::addPart(ModelPart * model, int position) {
	PartsBinView::addPart(model, position);
	updateSize();
}

void PartsBinIconView::removePart(const QString &moduleID) {
    SvgIconWidget *itemToRemove = NULL;
    int position = 0;
    foreach(QGraphicsItem *gIt, m_layouter->childItems()) {
        SvgIconWidget *it = dynamic_cast<SvgIconWidget*>(gIt);
        if(it && it->moduleID() == moduleID) {
            itemToRemove = it;
            break;
        } else {
            position++;
        }
    }
    if(itemToRemove) {
        m_itemBaseHash.remove(moduleID);
        itemToRemove->setParentItem(NULL);
        m_noSelectionChangeEmition = true;
        m_layout->removeItem(itemToRemove);
        delete itemToRemove;
    }

    setSelected(position, true);
    updateSize();
}

void PartsBinIconView::removeParts() {
    QList<SvgIconWidget *> itemsToRemove;
    foreach(QGraphicsItem *gIt, m_layouter->childItems()) {
        SvgIconWidget *it = dynamic_cast<SvgIconWidget*>(gIt);
        if(it) {
            itemsToRemove.append(it);
        }
    }
    m_itemBaseHash.clear();

    foreach (SvgIconWidget * itemToRemove, itemsToRemove) {
        m_noSelectionChangeEmition = true;
        itemToRemove->setParentItem(NULL);
        m_layout->removeItem(itemToRemove);
        delete itemToRemove;
    }

    updateSize();
}

int PartsBinIconView::setItemAux(ModelPart * modelPart, int position) {
	if (!modelPart || modelPart->itemType() == ModelPart::Unknown) {
		// don't want the empty root item to appear in the view
		return position;
	}

	emit settingItem();
	QString moduleID = modelPart->moduleID();
	if (contains(moduleID)) {
		return position;
	}
	
	SvgIconWidget* svgicon = NULL;
	if (modelPart->itemType() != ModelPart::Space) {
		ItemBase::PluralType plural;
		ItemBase * itemBase = loadItemBase(moduleID, plural);
        svgicon = new SvgIconWidget(modelPart, ViewLayer::IconView, itemBase, plural == ItemBase::Plural);
	}
	else {
		svgicon = new SvgIconWidget(modelPart, ViewLayer::IconView, NULL, false);
	}


	if(position > -1) {
		m_layout->insertItem(position, svgicon);
	} else {
		m_layout->addItem(svgicon);
		position = m_layout->count() - 1;
	}


	return position;
}

void PartsBinIconView::setPaletteModel(PaletteModel *model, bool clear) {
	PartsBinView::setPaletteModel(model, clear);
	updateSize();
}

void PartsBinIconView::loadFromModel(PaletteModel * model) {
	ModelPart* root = model->root();
	QList<QObject *>::const_iterator i;
    for (i = root->children().constBegin(); i != root->children().constEnd(); ++i) {
		ModelPart* mp = qobject_cast<ModelPart *>(*i);
		if (mp == NULL) continue;

		QDomElement instance = mp->instanceDomElement();
		if (instance.isNull()) continue;

		QDomElement views = instance.firstChildElement("views");
		if (views.isNull()) continue;

		QString name = ViewLayer::viewIDXmlName(ViewLayer::IconView);
		QDomElement view = views.firstChildElement(name);
		if (view.isNull()) continue;

		QDomElement geometry = view.firstChildElement("geometry");
		if (geometry.isNull()) continue;

		setItemAux(mp);
	}
}

ModelPart *PartsBinIconView::selectedModelPart() {
	SvgIconWidget *icon = dynamic_cast<SvgIconWidget *>(selectedAux());
	if(icon) {
		return icon->modelPart();
	} else {
		return NULL;
	}
}

ItemBase *PartsBinIconView::selectedItemBase() {
	SvgIconWidget *icon = dynamic_cast<SvgIconWidget *>(selectedAux());
	if(icon) {
		return icon->itemBase();
	} else {
		return NULL;
	}
}

void PartsBinIconView::setSelected(int position, bool doEmit) {
	QGraphicsLayoutItem *glIt = m_layout->itemAt(position);
	if(SvgIconWidget *item = dynamic_cast<SvgIconWidget*>(glIt)) {
		m_noSelectionChangeEmition = true;
		scene()->clearSelection();
		m_noSelectionChangeEmition = !doEmit;
		item->setSelected(true);
	}
}

bool PartsBinIconView::swappingEnabled(ItemBase * itemBase) {
	Q_UNUSED(itemBase);
	return false;
}

int PartsBinIconView::selectedIndex() {
	int idx = 0;
	foreach(QGraphicsItem *it, scene()->items()) {
		SvgIconWidget *icon = dynamic_cast<SvgIconWidget*>(it);
		if(icon) {
			if(icon->isSelected()) {
				return idx;
			} else {
				idx++;
			}
		}
	}
	return -1;
}

void PartsBinIconView::informNewSelection() {
	if(!m_noSelectionChangeEmition) {
		emit selectionChanged(selectedIndex());
	} else {
		m_noSelectionChangeEmition = false;
	}
}

void PartsBinIconView::dragMoveEvent(QDragMoveEvent* event) {
	dragMoveEnterEventAux(event);
}

void PartsBinIconView::dropEvent(QDropEvent* event) {
	dropEventAux(event);
}

void PartsBinIconView::moveItem(int fromIndex, int toIndex) {
	itemMoved(fromIndex,toIndex);
	emit informItemMoved(fromIndex, toIndex);
}

void PartsBinIconView::itemMoved(int fromIndex, int toIndex) {
	QGraphicsLayoutItem *item = m_layout->itemAt(fromIndex);
	m_layout->removeItem(item);
	m_layout->insertItem(toIndex,item);
	updateSize();
	setSelected(toIndex,/*doEmit*/false);
}


int PartsBinIconView::itemIndexAt(const QPoint& pos, bool &trustIt) {
	trustIt = true;
	SvgIconWidget *item = svgIconWidgetAt(pos);
	if(item) {
		int foundIdx = m_layout->indexOf(item);
		/*if(foundIdx != -1) { // no trouble finding it
			return foundIdx;
		} //else: something weird happened, the item exists,
		  //      but it should have and index > -1
		*/

		if(foundIdx == -1) { // no indicator shown
			trustIt = false;
		}
		return foundIdx;
	}
	if(!inEmptyArea(pos)) {
		// let's see if moving around the point we can find a real item
		return m_layout->indexOf(closestItemTo(pos));
	} else { // it's beyond the icons area, so -1 is OK
		return -1;
	}
}

bool PartsBinIconView::inEmptyArea(const QPoint& pos) {
    // used only by internal drag and drop
	if(m_layout->count() == 0) {
		return true;
	} else {
		QGraphicsLayoutItem *lastItem = m_layout->itemAt(m_layout->count()-1);
		QPointF itemBottomRightPoint =
			lastItem->graphicsItem()->mapToScene(lastItem->contentsRect().bottomRight());

		return itemBottomRightPoint.y() < pos.y()
			|| ( itemBottomRightPoint.y() >= pos.y()
				&& itemBottomRightPoint.x() < pos.x()
				);
	}
}

QGraphicsWidget* PartsBinIconView::closestItemTo(const QPoint& pos) {
	QPointF realPos = mapToScene(pos);
	SvgIconWidget *item = NULL;
    this -> setObjectName("partsIcon");
	if((item = svgIconWidgetAt(realPos.x()+ICON_SPACING,realPos.y()+ICON_SPACING))) {
		return item;
	}
	if((item = svgIconWidgetAt(realPos.x()-ICON_SPACING,realPos.y()+ICON_SPACING))) {
		return item;
	}
	if((item = svgIconWidgetAt(realPos.x()+ICON_SPACING,realPos.y()-ICON_SPACING))) {
		return item;
	}
	if((item = svgIconWidgetAt(realPos.x()-ICON_SPACING,realPos.y()-ICON_SPACING))) {
		return item;
	}
	return NULL;
}

QList<QObject*> PartsBinIconView::orderedChildren() {
	QList<QObject*> result;

	for(int i=0; i < m_layout->count(); i++) {
		SvgIconWidget *it = dynamic_cast<SvgIconWidget*>(m_layout->itemAt(i));
		if(it) {
			result << it->modelPart();
        }
	}
	return result;
}

void PartsBinIconView::showContextMenu(const QPoint& pos) {
	SvgIconWidget *it = svgIconWidgetAt(pos);

	QMenu *menu;
	if(it) {
		scene()->clearSelection();
		it->setSelected(true);
		menu = m_parent->partContextMenu();
	} else {
		menu = m_parent->combinedMenu();
	}
    menu->exec(mapToGlobal(pos));
}

SvgIconWidget * PartsBinIconView::svgIconWidgetAt(int x, int y) {
	return svgIconWidgetAt(QPoint(x, y));
}

SvgIconWidget * PartsBinIconView::svgIconWidgetAt(const QPoint & pos) {
	QGraphicsItem * item = itemAt(pos);
	while (item != NULL) {
		SvgIconWidget * svgIconWidget = dynamic_cast<SvgIconWidget *>(item);
		if (svgIconWidget != NULL) {
			return svgIconWidget;
		}

		item = item->parentItem();
	}

	return NULL;
}

void PartsBinIconView::reloadPart(const QString & moduleID) {
	if (!contains(moduleID)) return;

	for (int i = 0; i < m_layout->count(); i++) {
		SvgIconWidget *it = dynamic_cast<SvgIconWidget*>(m_layout->itemAt(i));
		if (it == NULL) continue;

		if (it->itemBase()->moduleID().compare(moduleID) != 0) continue;

		ItemBase::PluralType plural;
		ItemBase * itemBase = loadItemBase(moduleID, plural);

		it->setItemBase(itemBase, plural);
		return;
	}
}

ItemBase * PartsBinIconView::loadItemBase(const QString & moduleID, ItemBase::PluralType & plural) {
	ItemBase * itemBase = ItemBaseHash.value(moduleID);
	ModelPart * modelPart = m_referenceModel->retrieveModelPart(moduleID);
    if (itemBase == NULL) {
		itemBase = PartFactory::createPart(modelPart, ViewLayer::NewTop, ViewLayer::IconView, ViewGeometry(), ItemBase::getNextID(), NULL, NULL, false);
		ItemBaseHash.insert(moduleID, itemBase);
	}
	m_itemBaseHash.insert(moduleID, itemBase);

    plural = itemBase->isPlural();
	if (plural == ItemBase::NotSure) {
		QHash<QString,QString> properties = modelPart->properties();
		QString family = properties.value("family", "").toLower();
		foreach (QString key, properties.keys()) {
			QStringList values = m_referenceModel->propValues(family, key, true);
			if (values.length() > 1) {
				plural = ItemBase::Plural;
				break;
			}
		}
	}

	return itemBase;
}
