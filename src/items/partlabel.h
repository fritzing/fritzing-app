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

$Revision: 6112 $:
$Author: cohen@irascible.com $:
$Date: 2012-06-28 00:18:10 +0200 (Do, 28. Jun 2012) $

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

class PartLabel : public QGraphicsSvgItem
{
	Q_OBJECT
public:
	PartLabel(class ItemBase * owner, QGraphicsItem * parent = 0 );   // itembase is not the parent
	~PartLabel();

	void setPlainText(const QString & text);
	void showLabel(bool showIt, ViewLayer *);
	QPainterPath shape() const;
	bool initialized();
	void ownerMoved(QPointF newPos);
	void setHidden(bool hide);
	bool hidden();
	void setInactive(bool inactivate);
	bool inactive();
	ViewLayer::ViewLayerID viewLayerID();
	void saveInstance(QXmlStreamWriter & streamWriter);
	void restoreLabel(QDomElement & labelGeometry, ViewLayer::ViewLayerID);
	void moveLabel(QPointF newPos, QPointF newOffset);
	class ItemBase * owner();
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
	QPointer<class ItemBase> m_owner;
	bool m_initialized;
	bool m_spaceBarWasPressed;
	bool m_doDrag;
	QPointF m_initialPosition;
	QPointF m_initialOffset;
	QPointF m_offset;
	ViewLayer::ViewLayerID m_viewLayerID;
	bool m_hidden;
	bool m_inactive;
	QMenu m_menu;
	QString m_text;
	QString m_displayText;
	QStringList m_displayKeys;
    QAction * m_tinyAct;
	QAction * m_smallAct;
	QAction * m_mediumAct;
	QAction * m_largeAct;
	QAction * m_labelAct;
	QList<QAction *> m_displayActs;
	QColor m_color;
	QFont m_font;
	QSvgRenderer * m_renderer;
};

#endif
