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

$Revision: 6494 $:
$Author: irascibl@gmail.com $:
$Date: 2012-09-29 17:40:27 +0200 (Sa, 29. Sep 2012) $

********************************************************************/


#include <QMimeData>
#include <QDrag>
#include <QDragEnterEvent>
#include <QMessageBox>

#include "partsbinview.h"
#include "partsbinpalettewidget.h"
#include "../itemdrag.h"
#include "../utils/misc.h"

QHash<QString, QString> PartsBinView::TranslatedCategoryNames;
QHash<QString, ItemBase *> PartsBinView::ItemBaseHash;

PartsBinView::PartsBinView(ReferenceModel *referenceModel, PartsBinPaletteWidget *parent) {
	if (TranslatedCategoryNames.count() == 0) {
		TranslatedCategoryNames.insert("Basic", QObject::tr("Basic"));
		TranslatedCategoryNames.insert("Input", QObject::tr("Input"));
		TranslatedCategoryNames.insert("Output", QObject::tr("Output"));
		TranslatedCategoryNames.insert("ICs", QObject::tr("ICs"));
		TranslatedCategoryNames.insert("Power", QObject::tr("Power"));
		TranslatedCategoryNames.insert("Connection", QObject::tr("Connection"));
		TranslatedCategoryNames.insert("Microcontroller", QObject::tr("Microcontroller"));
		TranslatedCategoryNames.insert("Breadboard View", QObject::tr("Breadboard View"));
		TranslatedCategoryNames.insert("Schematic View", QObject::tr("Schematic View"));
		TranslatedCategoryNames.insert("PCB View", QObject::tr("PCB View"));
		TranslatedCategoryNames.insert("Tools",  QObject::tr("Tools"));
		TranslatedCategoryNames.insert("Shields",  QObject::tr("Shields"));
		TranslatedCategoryNames.insert("LilyPad",  QObject::tr("LilyPad"));
		TranslatedCategoryNames.insert("Other",  QObject::tr("Other"));
		TranslatedCategoryNames.insert("Sensors",  QObject::tr("Sensors"));
	}

	m_referenceModel = referenceModel;
	m_parent = parent;
}

PartsBinView::~PartsBinView() {
}	

void PartsBinView::cleanup() {
    foreach (ItemBase * itemBase, ItemBaseHash.values()) {
        delete itemBase;
    }
}

void PartsBinView::setPaletteModel(PaletteModel * model, bool clear) {
	if (clear) {
		doClear();
	}

	if (model->root() == NULL) return;

	setItemAux(model->root());
	setItem(model->root());
}

void PartsBinView::reloadParts(PaletteModel * model) {
	setPaletteModel(model, true);
}

void PartsBinView::doClear() {
	m_itemBaseHash.clear();
}

void PartsBinView::removePartReference(const QString & moduleID) {
	ItemBase * itemBase = ItemBaseHash.value(moduleID);
	if (itemBase) {
		ItemBaseHash.remove(moduleID);
		itemBase->deleteLater();
	}
}

void PartsBinView::setItem(ModelPart * modelPart) {
	QList<QObject *>::const_iterator i;
    for (i = modelPart->children().constBegin(); i != modelPart->children().constEnd(); ++i) {
		setItemAux(dynamic_cast<ModelPart *>(*i));
	}
    for (i = modelPart->children().constBegin(); i != modelPart->children().constEnd(); ++i) {
		setItem(dynamic_cast<ModelPart *>(*i));
	}
}

void PartsBinView::addPart(ModelPart * model, int position) {
	int newPosition = setItemAux(model, position);
	setSelected(newPosition);
}

void PartsBinView::mousePressOnItem(const QPoint &dragStartPos, const QString &moduleId, const QSize &size, const QPointF &dataPoint, const QPoint &hotspot) {
	if (moduleId.isEmpty()) return;
	
	m_dragStartPos = dragStartPos;

	QByteArray itemData;
	QDataStream dataStream(&itemData, QIODevice::WriteOnly);

	dataStream << moduleId << dataPoint;

	QMimeData *mimeData = new QMimeData;
	mimeData->setData("application/x-dnditemdata", itemData);
	mimeData->setData("action", "part-reordering");

	ItemDrag::setOriginator(this->m_parent);
    ItemDrag::setOriginatorIsTempBin(m_parent->isTempPartsBin());

	QDrag * drag = new QDrag(dynamic_cast<QWidget*>(this));

	drag->setMimeData(mimeData);

	/*
#ifndef Q_OS_LINUX
	// transparency doesn't seem to work for linux
	QPixmap pixmap(size);
	pixmap.fill(Qt::transparent);
	QPainter painter;
	painter.begin(&pixmap);
	QPen pen(QColor(0,0,0,127));
	pen.setStyle(Qt::DashLine);
	pen.setWidth(1);
	painter.setPen(pen);
	painter.drawRect(0,0,pixmap.width() - 1, pixmap.height() - 1);
	painter.end();
	drag->setPixmap(pixmap);
	drag->setHotSpot(hotspot);
#else
	Q_UNUSED(hotspot);
	Q_UNUSED(size);
#endif
	*/

	Q_UNUSED(hotspot);
	Q_UNUSED(size);

	// can set the pixmap, but can't hide it
	//QPixmap * pixmap = pitem->pixmap();
	//if (pixmap != NULL) {
		//drag.setPixmap(*pixmap);
		//drag.setHotSpot(mts.toPoint() - pitem->pos().toPoint());
	//

	// setDragCursor doesn't seem to help
	//drag.setDragCursor(*pitem->pixmap(), Qt::MoveAction);
	//drag.setDragCursor(*pitem->pixmap(), Qt::CopyAction);
	//drag.setDragCursor(*pitem->pixmap(), Qt::LinkAction);
	//drag.setDragCursor(*pitem->pixmap(), Qt::IgnoreAction);


	drag->exec();

	/*if (drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction) == Qt::MoveAction) {
	}
	else {
	}*/

	ItemDrag::dragIsDone();
}

void PartsBinView::dragMoveEnterEventAux(QDragMoveEvent* event) {
	// Only accept if it's an icon-reordering request
	const QMimeData* m = event->mimeData();
	QStringList formats = m->formats();
	if (formats.contains("action") && (m->data("action") == "part-reordering")) {
		event->acceptProposedAction();
	}
}

void PartsBinView::dropEventAux(QDropEvent* event, bool justAppend) {
	bool trustResult;
	int toIndex;

	if(justAppend) {
		toIndex = -1;
		trustResult = true;
	} else {
		toIndex = itemIndexAt(event->pos(), trustResult);
	}

	if(!trustResult) return;

	if(event->source() == dynamic_cast<QWidget*>(this)) {
		int fromIndex = itemIndexAt(m_dragStartPos, trustResult);
		if(trustResult && fromIndex != toIndex) {
			moveItem(fromIndex,toIndex);
		}
	} else {
		QByteArray itemData = event->mimeData()->data("application/x-dnditemdata");
		QDataStream dataStream(&itemData, QIODevice::ReadOnly);

		QString moduleID;
		QPointF offset;
		dataStream >> moduleID >> offset;

		ModelPart * mp = m_referenceModel->retrieveModelPart(moduleID);
		m_parent->copyFilesToContrib(mp, ItemDrag::originator());
		if(mp) {
			if(m_parent->contains(moduleID)) {
				QMessageBox::information(NULL, QObject::tr("Part already in bin"), QObject::tr("The part that you have just added,\nis already there, we won't add it again, right?"));
			} else {
				m_parent->addPart(mp,toIndex);
				m_parent->setDirty();
			}
		}
	}
	event->acceptProposedAction();
}

bool PartsBinView::contains(const QString &moduleID) {
	return m_itemBaseHash.contains(moduleID);
}

void PartsBinView::setInfoViewOnHover(bool infoViewOnHover) {
	m_infoViewOnHover = infoViewOnHover;
}
