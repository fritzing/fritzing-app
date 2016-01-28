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


#ifndef LISTVIEW_H_
#define LISTVIEW_H_

#include <QListWidget>
#include <QMouseEvent>

#include "partsbinview.h"


class PartsBinListView : public QListWidget, public PartsBinView {
	Q_OBJECT
	public:
		PartsBinListView(ReferenceModel* referenceModel, PartsBinPaletteWidget *parent);
		~PartsBinListView();
		void setInfoView(class HtmlInfoView *);
        void removePart(const QString &moduleID);
        void removeParts();

		ModelPart *selectedModelPart();
		ItemBase *selectedItemBase();
		bool swappingEnabled();
		int position(const QString &moduleID);

		QList<QObject*> orderedChildren();
		void reloadPart(const QString & moduleID);

	public slots:
		void setSelected(int position, bool doEmit=false);
		void itemMoved(int fromIndex, int toIndex);

	protected slots:
		void showContextMenu(const QPoint& pos);

	signals:
		void informItemMoved(int fromIndex, int toIndex);
		void settingItem();

	protected:
		void doClear();
		void moveItem(int fromIndex, int toIndex);
		int itemIndexAt(const QPoint& pos, bool &trustIt);

		void mouseMoveEvent(QMouseEvent *event);
		void mousePressEvent(QMouseEvent *event);
		//void dragEnterEvent(QDragEnterEvent* event);
		void dropEvent(QDropEvent* event);
		void startDrag(Qt::DropActions supportedActions);

		int setItemAux(ModelPart * modelPart, int position = -1);

		ModelPart *itemModelPart(const QListWidgetItem *item) const;
		ItemBase *itemItemBase(const QListWidgetItem *item) const;
		const QString& itemModuleID(const QListWidgetItem *item);

		void showInfo(QListWidgetItem * item);

		bool dropMimeData(int index, const QMimeData *data, Qt::DropAction action);
		QMimeData * mimeData(const QList<QListWidgetItem *> items) const;
		QStringList mimeTypes() const;
		void loadImage(ModelPart *, QListWidgetItem * lwi, const QString & moduleID); 

	protected:
		class HtmlInfoView * m_infoView;
		QListWidgetItem * m_hoverItem;

};
#endif /* LISTVIEW_H_ */
