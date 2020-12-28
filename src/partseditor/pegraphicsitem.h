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



#ifndef PEGRAPHICSITEM_H_
#define PEGRAPHICSITEM_H_

#include <QGraphicsRectItem>
#include <QDomElement>
#include <QGraphicsSceneHoverEvent>
#include <QTimer>
#include <QPointer>
#include <QTime>
#include <QBrush>

class PEGraphicsItem : public QObject, public QGraphicsRectItem
{
	Q_OBJECT
public:
	PEGraphicsItem(double x, double y, double width, double height, class ItemBase *);
	~PEGraphicsItem();

	void hoverEnterEvent(QGraphicsSceneHoverEvent *);
	void hoverLeaveEvent(QGraphicsSceneHoverEvent *);
	void wheelEvent(QGraphicsSceneWheelEvent *);
	void mousePressEvent(QGraphicsSceneMouseEvent *);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *);
	void setHighlighted(bool);
	bool highlighted();
	void setElement(QDomElement &);
	QDomElement & element();
	void setOffset(QPointF);
	QPointF offset();
	void showTerminalPoint(bool);
	bool showingTerminalPoint();
	void showMarquee(bool);
	bool showingMarquee();
	void setTerminalPoint(QPointF);
	QPointF terminalPoint();
	void setPendingTerminalPoint(QPointF);
	QPointF pendingTerminalPoint();
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	void setPickAppearance(bool);
	void flash();
	class ItemBase * itemBase();

signals:
	void highlightSignal(PEGraphicsItem *);
	void mousePressedSignal(PEGraphicsItem *, bool & ignore);
	void mouseReleasedSignal(PEGraphicsItem *);
	void terminalPointMoved(PEGraphicsItem *, QPointF);
	void terminalPointChanged(PEGraphicsItem *, QPointF before, QPointF after);

protected slots:
	void restoreColor();

protected:
	bool m_highlighted = false;
	bool m_flash = false;
	QDomElement  m_element;
	QPointF m_offset;
	bool m_showTerminalPoint = false;
	bool m_showMarquee = false;
	QPointF m_terminalPoint;
	QPointF m_pendingTerminalPoint;
	bool m_dragTerminalPoint = false;
	QPointF m_dragTerminalOrigin;
	QPointF m_terminalPointOrigin;
	bool m_drawHighlight = false;
	int m_wheelAccum = 0;
	qreal m_savedOpacity = 0;
	bool m_pick = false;
	class ItemBase * m_itemBase = nullptr;
};

#endif /* PEGRAPHICSITEM_H_ */
