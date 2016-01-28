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


#include "pegraphicsitem.h"
#include "../debugdialog.h"
#include "../sketch/infographicsview.h"
#include "../items/itembase.h"

#include <QBrush>
#include <QColor>
#include <QGraphicsScene>
#include <QPainter>


static QVector<qreal> Dashes;
static const int DashLength = 3;

static bool ShiftDown = false;
static QPointF OriginalShiftPos;
static bool ShiftX = false;
static bool ShiftY = false;
static bool SpaceBarWasPressed = false;
static const double MinMouseMove = 2;
static const QColor NormalColor(0, 0, 255);
static const QColor PickColor(255, 0, 255);


////////////////////////////////////////////////


PEGraphicsItem::PEGraphicsItem(double x, double y, double w, double h, ItemBase * itemBase) : QGraphicsRectItem(x, y, w, h) {
    if (Dashes.isEmpty()) {
        Dashes << DashLength << DashLength;
    }

    m_itemBase = itemBase;
    m_pick = m_flash = false;

    m_terminalPoint = QPointF(w / 2, h / 2);
    m_dragTerminalPoint = m_showMarquee = m_showTerminalPoint = false;
	setAcceptedMouseButtons(Qt::LeftButton);
	setAcceptHoverEvents(true);
    //setFlag(QGraphicsItem::ItemIsSelectable, true );
    setHighlighted(false);
    setBrush(QBrush(NormalColor));
}

PEGraphicsItem::~PEGraphicsItem() {
    m_element.clear();
}

void PEGraphicsItem::hoverEnterEvent(QGraphicsSceneHoverEvent *) {
    m_wheelAccum = 0;
	setHighlighted(true);
}

void PEGraphicsItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *) {
	setHighlighted(false);
}

void PEGraphicsItem::wheelEvent(QGraphicsSceneWheelEvent * event) {
    //DebugDialog::debug(QString("wheel %1 %2").arg(event->delta()).arg(event->orientation()));

#ifndef Q_OS_MAC
    // qt 4.8.3: mac: event orientation is messed up at this point
    if (event->orientation() == Qt::Horizontal) return;
#endif
    if (event->delta() == 0) return;
    if ((event->modifiers() & Qt::ShiftModifier) == 0) return;


    // delta one click forward = 120; delta one click backward = -120
    
    int magDelta = qAbs(event->delta());
    int sign = event->delta() / magDelta;
    int delta = sign * qMin(magDelta, 120);
    m_wheelAccum += delta;
    if (qAbs(m_wheelAccum) < 120) return;

    m_wheelAccum = 0;

    QList<PEGraphicsItem *> items;
    foreach (QGraphicsItem * item, scene()->items(event->scenePos())) {
        PEGraphicsItem * pegi = dynamic_cast<PEGraphicsItem *>(item);
        if (pegi) items.append(pegi);
    }

    if (items.count() < 2) return;

    int ix = -1;
    int mix = -1;
    for (int i = 0; i < items.count(); i++) {
        if (items.at(i)->highlighted()) {
            ix = i;
            break;
        }
        if (items.at(i) == this) {
            mix = i;
        }
    }

    if (ix == -1) ix = mix;
    if (ix == -1) {
        // shouldn't happen
        return;
    }

    ix += sign;

    // wrap
    //while (ix < 0) {
    //    ix += items.count();
    //}
    //ix = ix % items.count();

    // don't wrap
    if (ix < 0) ix = 0;
    else if (ix >= items.count()) ix = items.count() - 1;
    
    PEGraphicsItem * theItem = items.at(ix);
    if (theItem->highlighted()) {
        theItem->flash();
    }
    else {
        theItem->setHighlighted(true);
    }
}

void PEGraphicsItem::setHighlighted(bool highlighted) {
    if (highlighted) {
        m_highlighted = true;
        setOpacity(0.4);
        foreach (QGraphicsItem * item, scene()->items()) {
            PEGraphicsItem * pegi = dynamic_cast<PEGraphicsItem *>(item);
            if (pegi == NULL) continue;
            if (pegi == this) continue;
            if (!pegi->highlighted()) continue;
             
            pegi->setHighlighted(false);
        }
        emit highlightSignal(this);
    }
    else {
        m_highlighted = false;
        setOpacity(0.001);
    }
    update();
}

bool PEGraphicsItem::highlighted() {
    return m_highlighted;
}

void PEGraphicsItem::setElement(QDomElement & el)
{
    m_element = el;
}

QDomElement & PEGraphicsItem::element() {
    return m_element;
}

void PEGraphicsItem::setOffset(QPointF p) {
    m_offset = p;
}

QPointF PEGraphicsItem::offset() {
    return m_offset;
}

void PEGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) 
{
	QGraphicsRectItem::paint(painter, option, widget);

    if (m_flash) {
        m_flash = false;
        QTimer::singleShot(40, this, SLOT(restoreColor()));
    }

	bool save = m_showTerminalPoint || m_showMarquee;

	if (save) painter->save();

    if (m_showTerminalPoint) {
        QRectF r = rect();
        QLineF l1(0, m_terminalPoint.y(), r.width(), m_terminalPoint.y());
        QLineF l2(m_terminalPoint.x(), 0, m_terminalPoint.x(), r.height());

        painter->setOpacity(1.0);
        painter->setPen(QPen(QColor(0, 0, 0), 0, Qt::SolidLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawLine(l1);
        painter->drawLine(l2);

	    painter->setPen(QPen(QColor(255, 255, 255), 0, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawLine(l1);
        painter->drawLine(l2);        
    }

	if (m_showMarquee) {
        QRectF r = rect();
		double d = qMin(r.width(), r.height()) / 16;
		r.adjust(d, d, -d, -d);

        painter->setOpacity(1.0);
        painter->setPen(QPen(QColor(0, 0, 0), 0, Qt::SolidLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(r);

	    painter->setPen(QPen(QColor(255, 255, 255), 0, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(r);
	}

	if (save) painter->restore();
}

void PEGraphicsItem::showTerminalPoint(bool show) 
{
    m_showTerminalPoint = show;
    update();
}


bool PEGraphicsItem::showingTerminalPoint() {
    return m_showTerminalPoint;
}

void PEGraphicsItem::showMarquee(bool show) {
    if (show) {
        m_showMarquee = true;
        foreach (QGraphicsItem * item, scene()->items()) {
            PEGraphicsItem * pegi = dynamic_cast<PEGraphicsItem *>(item);
            if (pegi == NULL) continue;
            if (pegi == this) continue;
            if (!pegi->showingMarquee()) continue;
             
            pegi->showMarquee(false);
		}
    }
    else {
        m_showMarquee = false;
    }
    update();
}

bool PEGraphicsItem::showingMarquee() {
    return m_showMarquee;
}

void PEGraphicsItem::setTerminalPoint(QPointF p) {
    m_pendingTerminalPoint = m_terminalPoint = p;
}

QPointF PEGraphicsItem::terminalPoint() {
    return m_terminalPoint;
}

void PEGraphicsItem::setPendingTerminalPoint(QPointF p) {
    m_pendingTerminalPoint = p;
}

QPointF PEGraphicsItem::pendingTerminalPoint() {
    return m_pendingTerminalPoint;
}

void PEGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent * event) {
    m_dragTerminalPoint = false;

	if (!m_highlighted) {
		// allows to click through to next layer
		event->ignore();
		return;
    }

	if (!event->buttons() && Qt::LeftButton) {
		event->ignore();
		return;
	}

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL && infoGraphicsView->spaceBarIsPressed()) {
		event->ignore();
		return;
	}

	bool ignore;
	emit mousePressedSignal(this, ignore);
	if (ignore) {
		event->ignore();
		return;
	}

    if (m_showMarquee) {
        QPointF p = event->pos();
	    if (event->modifiers() & Qt::ShiftModifier) {
		    ShiftDown = true;
            ShiftX = ShiftY = false;
		    OriginalShiftPos = p;
	    }

        if (qAbs(p.x() - m_terminalPoint.x()) <= MinMouseMove && qAbs(p.y() - m_terminalPoint.y()) <= MinMouseMove) {
            m_dragTerminalPoint = true;
            m_terminalPointOrigin = m_terminalPoint;
            m_dragTerminalOrigin = event->pos();
        }

		return;
    }

    event->ignore();
}

void PEGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent * event) {
    if (!m_dragTerminalPoint) return;

    if (ShiftDown && !(event->modifiers() & Qt::ShiftModifier)) {
		ShiftDown = false;
	}
	QPointF p = event->pos();
	if (ShiftDown) {
		if (ShiftX) {
			// moving along x, constrain y
			p.setY(OriginalShiftPos.y());
		}
		else if (ShiftY) {
			// moving along y, constrain x
			p.setX(OriginalShiftPos.x());
		}
        else {
            double dx = qAbs(p.x() - OriginalShiftPos.x());
            double dy = qAbs(p.y() - OriginalShiftPos.y());
            if (dx - dy > MinMouseMove) {
                ShiftX = true;
            }
            else if (dy - dx > MinMouseMove) {
                ShiftY = true;
            }
        }
	}
	else if (event->modifiers() & Qt::ShiftModifier) {
		ShiftDown = true;
        ShiftX = ShiftY = false;
        OriginalShiftPos = event->pos();
	}


    QPointF newTerminalPoint = m_terminalPointOrigin + p - m_dragTerminalOrigin;
    if (newTerminalPoint.x() < 0) newTerminalPoint.setX(0);
    else if (newTerminalPoint.x() > rect().width()) newTerminalPoint.setX(rect().width());
    if (newTerminalPoint.y() < 0) newTerminalPoint.setY(0);
    else if (newTerminalPoint.y() > rect().height()) newTerminalPoint.setY(rect().height());
    m_terminalPoint = newTerminalPoint;
    emit terminalPointMoved(this, newTerminalPoint);
    update();
}


void PEGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *) {
    if (m_dragTerminalPoint) {
        m_dragTerminalPoint = false;
        if (m_terminalPointOrigin != m_terminalPoint) {
            emit terminalPointChanged(this, m_terminalPointOrigin, m_terminalPoint);
        }
    }
    else {
		// relocate the connector
        emit mouseReleasedSignal(this);
    }
}

void PEGraphicsItem::setPickAppearance(bool pick) {
    m_pick = pick;
	setBrush(pick ? PickColor : NormalColor);
}

void PEGraphicsItem::flash() {
    m_savedOpacity = opacity();
    m_flash = true;
    setBrush(QColor(255, 255, 255));
    update();
}

void PEGraphicsItem::restoreColor() {
    setBrush(m_pick ? PickColor : NormalColor);
    setOpacity(m_savedOpacity);
    update();
}

ItemBase * PEGraphicsItem::itemBase() {
    return m_itemBase;
}
