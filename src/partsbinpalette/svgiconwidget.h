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



#ifndef SVGICONWIDGET_H_
#define SVGICONWIDGET_H_

#include <QGraphicsWidget>
#include <QGraphicsPixmapItem>
#include <QToolTip>
#include <QPointer>
#include <QPixmap>

#include "../model/modelpart.h"
#include "../items/itembase.h"


class SvgIconPixmapItem : public QGraphicsPixmapItem {

public:
	SvgIconPixmapItem(const QPixmap & pixmap, QGraphicsItem * parent = nullptr);
	SvgIconPixmapItem(const QPixmap & pixmap, QGraphicsItem * parent, bool plural);
	void setPlural(bool);
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
protected:
	bool m_plural = false;
};

class SvgIconWidget : public QGraphicsWidget
{
	Q_OBJECT
public:
	SvgIconWidget(ModelPart *, ViewLayer::ViewID, ItemBase *, bool plural);
	~SvgIconWidget() = default;
	ItemBase * itemBase() const noexcept;
	ModelPart * modelPart() const noexcept;
	constexpr const QString &moduleID() const noexcept { return m_moduleId; }
	void setItemBase(ItemBase *, bool plural);

	static void initNames();
	static void cleanup();

protected:
	void hoverEnterEvent ( QGraphicsSceneHoverEvent * event );
	void hoverLeaveEvent ( QGraphicsSceneHoverEvent * event );
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	void setupImage(bool plural, ViewLayer::ViewID viewID);

protected:
	QPointer<ItemBase> m_itemBase;
	SvgIconPixmapItem * m_pixmapItem = nullptr;
	QString m_moduleId;
};


#endif /* SVGICONWIDGET_H_ */
