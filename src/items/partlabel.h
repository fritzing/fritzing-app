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

#ifndef PARTLABEL_H
#define PARTLABEL_H

#include <QGraphicsSvgItem>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>
#include <QXmlStreamWriter>
#include <QDomElement>
#include <QTextDocument>
#include <QKeyEvent>
#include <QPointer>
#include <QTimer>
#include <QMenu>

#include "../viewlayer.h"

class ItemBase;
class PartLabel : public QGraphicsSvgItem
{
	Q_OBJECT
public:
	PartLabel(ItemBase * owner, QGraphicsItem * parent = 0 );   // itembase is not the parent
	~PartLabel();

	void setPlainText(const QString & text);
	void showLabel(bool showIt, ViewLayer *);
	QPainterPath shape() const;
	constexpr bool initialized() const noexcept { return m_initialized; }
	void ownerMoved(QPointF newPos);
	void setHidden(bool hide);
	constexpr bool hidden() const noexcept { return m_hidden; }
	void setInactive(bool inactivate);
	constexpr bool inactive() const noexcept { return m_inactive; }
	constexpr ViewLayer::ViewLayerID viewLayerID() const noexcept { return m_viewLayerID; }
	void saveInstance(QXmlStreamWriter & streamWriter);
	void restoreLabel(QDomElement & labelGeometry, ViewLayer::ViewLayerID);
	void moveLabel(QPointF newPos, QPointF newOffset);
	ItemBase * owner();
	void rotateFlipLabel(double degrees, Qt::Orientations orientation);
	QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant & value);
	void ownerSelected(bool selected);
	void displayTexts();
	void displayTextsIf();
	QString makeSvg(bool blackOnly, double dpi, double printerScale, bool includeTransform);
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	void setFontPointSize(double pointSize);

protected:
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
	void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
	void transformLabel(QTransform currTransf);
	void setUpText();
	void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
	void initMenu();
	void partLabelEdit();
	void setFontSize(int action);
	void rotateFlip(int action);
	void setLabelDisplay(const QString & key);
	void setHiddenOrInactive();
	void partLabelHide();
	void resetSvg();
	QString makeSvgAux(bool blackOnly, double dpi, double printerScale, double & w, double & h);

protected:
	QPointer<ItemBase> m_owner;
	bool m_initialized = false;
	bool m_spaceBarWasPressed = false;
	bool m_doDrag = false;
	QPointF m_initialPosition;
	QPointF m_initialOffset;
	QPointF m_offset;
	ViewLayer::ViewLayerID m_viewLayerID = ViewLayer::UnknownLayer;
	bool m_hidden = false;
	bool m_inactive = false;
	QMenu m_menu;
	QString m_text;
	QString m_displayText;
	QStringList m_displayKeys;
	QAction * m_tinyAct = nullptr;
	QAction * m_smallAct = nullptr;
	QAction * m_mediumAct = nullptr;
	QAction * m_largeAct = nullptr;
	QAction * m_labelAct = nullptr;
	QList<QAction *> m_displayActs;
	QColor m_color;
	QFont m_font;
	QSvgRenderer * m_renderer = nullptr;
};

#endif
