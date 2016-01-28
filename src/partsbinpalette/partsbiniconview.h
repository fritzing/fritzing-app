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


#ifndef ICONVIEW_H_
#define ICONVIEW_H_

#include <QFrame>
#include <QGraphicsView>

#include "partsbinview.h"
#include "../sketch/infographicsview.h"

QT_BEGIN_NAMESPACE
class QDragEnterEvent;
class QDropEvent;
QT_END_NAMESPACE

class PartsBinIconView : public InfoGraphicsView, public PartsBinView
{
	Q_OBJECT
	public:
		PartsBinIconView(ReferenceModel* referenceModel, PartsBinPaletteWidget *parent);
		void loadFromModel(class PaletteModel *);
		void setPaletteModel(class PaletteModel *model, bool clear=false);
		void addPart(ModelPart * model, int position = -1);
        void removePart(const QString &moduleID);
        void removeParts();

		bool swappingEnabled(ItemBase *);

		ModelPart *selectedModelPart();
		ItemBase *selectedItemBase();
		int selectedIndex();

		QList<QObject*> orderedChildren();
		void reloadPart(const QString & moduleID);

	protected:
		void doClear();
		void moveItem(int fromIndex, int toIndex);
		int itemIndexAt(const QPoint& pos, bool &trustIt);

		void mousePressEvent(QMouseEvent *event);
		void dragMoveEvent(QDragMoveEvent* event);
		void dropEvent(QDropEvent* event);

		int setItemAux(ModelPart *, int position = -1);

		void resizeEvent(QResizeEvent * event);
		void updateSize(QSize newSize);
		void updateSize();
		void updateSizeAux(int width);
		void setupLayout();

		bool inEmptyArea(const QPoint& pos);
		QGraphicsWidget* closestItemTo(const QPoint& pos);
		class SvgIconWidget * svgIconWidgetAt(const QPoint & pos);
		class SvgIconWidget * svgIconWidgetAt(int x, int y);
		ItemBase * loadItemBase(const QString & moduleID, ItemBase::PluralType &);

	public slots:
		void setSelected(int position, bool doEmit=false);
		void informNewSelection();
		void itemMoved(int fromIndex, int toIndex);

	protected slots:
		void showContextMenu(const QPoint& pos);

	signals:
		void informItemMoved(int fromIndex, int toIndex);
		void selectionChanged(int index);
		void settingItem();

	protected:
		LayerHash m_viewLayers;

		QGraphicsWidget *m_layouter;
		class GraphicsFlowLayout *m_layout;

		QMenu *m_itemMenu;
		bool m_noSelectionChangeEmition;
};

#endif /* ICONVIEW_H_ */
