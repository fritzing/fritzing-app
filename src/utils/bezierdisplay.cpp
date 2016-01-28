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

#include "bezierdisplay.h"
#include "../viewlayer.h"
#include "../debugdialog.h"
#include "../processeventblocker.h"
#include "graphicsutils.h"

#include <QPen>
#include <QGraphicsScene>
#include <QApplication>

BezierDisplay::BezierDisplay()
{
	m_itemL0 = m_itemL1 = NULL;
	m_itemE0 = m_itemE1 = NULL;
}

BezierDisplay::~BezierDisplay()
{
	//DebugDialog::debug("removing bezier display");
	if (m_itemL0) {
		m_itemL0->scene()->removeItem(m_itemL0);
		delete m_itemL0;
	}
	if (m_itemL1) {
		m_itemL1->scene()->removeItem(m_itemL1);
		delete m_itemL1;
	}
	//if (m_itemE0) {
	//	m_itemE0->scene()->removeItem(m_itemE0);
	//	delete m_itemE0;
	//}
	//if (m_itemE1) {
	//	m_itemE1->scene()->removeItem(m_itemE1);
	//	delete m_itemE1;
	//}
}

void BezierDisplay::initDisplay(QGraphicsItem * master, Bezier *bezier)
{
	//DebugDialog::debug("adding bezier display");
		
	static int activeColor =   0xffffff;
	static int inactiveColor = 0xb0b0b0;

	QPen pen;
	pen.setWidth(0);

	QGraphicsItem * parent = master;
	while (parent->parentItem()) {
		parent = master->parentItem();
	}

	// put the feeback on top
	double z = parent->zValue() + 100;			// (ViewLayer::getZIncrement() / 2)
	
	m_itemL0 = new QGraphicsLineItem();
	pen.setColor(QColor(bezier->drag0() ? activeColor : inactiveColor));
	m_itemL0->setPen(pen);
	m_itemL0->setPos(0, 0);
	m_itemL0->setZValue(z);
	master->scene()->addItem(m_itemL0);

	//m_itemE0 = new QGraphicsEllipseItem();
	//m_itemE0->setPen(pen);
	//m_itemE0->setPos(0, 0);
	//m_itemE0->setZValue(z);
	//master->scene()->addItem(m_itemE0);

	m_itemL1 = new QGraphicsLineItem();
	pen.setColor(QColor(bezier->drag0() == false ? activeColor : inactiveColor));
	m_itemL1->setPen(pen);
	m_itemL1->setPos(0, 0);
	m_itemL1->setZValue(z);
	master->scene()->addItem(m_itemL1);

	//m_itemE1 = new QGraphicsEllipseItem();
	//m_itemE1->setPen(pen);
	//m_itemE1->setPos(0, 0);
	//m_itemE1->setZValue(z);
	//master->scene()->addItem(m_itemE1);

	updateDisplay(master, bezier);
	ProcessEventBlocker::processEvents();
}

void BezierDisplay::updateDisplay(QGraphicsItem * master, Bezier *bezier)
{
	if (m_itemL0 == NULL) return;
	if (m_itemL1 == NULL) return;
	//if (m_itemE0 == NULL) return;
	//if (m_itemE1 == NULL) return;

	if (bezier == NULL || bezier->isEmpty()) {
		m_itemL0->setVisible(false);
		m_itemL1->setVisible(false);
		//m_itemE0->setVisible(false);
		//m_itemE1->setVisible(false);
		return;
	}

        //static double minD = 5;
        //static double radius = 6;
        //static double minDSqd = minD * minD;

	QRectF sr = master->scene()->sceneRect();
	double x1, y1, x2, y2;

	QPointF p0 = master->mapToScene(bezier->endpoint0());
	QPointF p1 = master->mapToScene(bezier->cp0());
	GraphicsUtils::liangBarskyLineClip(p0.x(), p0.y(), p1.x(), p1.y(), sr.left(), sr.right(), sr.top(), sr.bottom(), x1, y1, x2, y2);
	m_itemL0->setLine(x1, y1, x2, y2);
	//if (GraphicsUtils::distanceSqd(bezier->endpoint0(), bezier->cp0()) > minDSqd) {
		//m_itemE0->setVisible(false);
	//}
	//else {
		//QRectF r(master->mapToScene(bezier->endpoint0()), QSizeF(0,0));
		//r.adjust(-radius, -radius, radius, radius);
		//m_itemE0->setRect(r);
		//m_itemE0->setVisible(true);
	//}

	p0 = master->mapToScene(bezier->endpoint1());
	p1 = master->mapToScene(bezier->cp1());
	GraphicsUtils::liangBarskyLineClip(p0.x(), p0.y(), p1.x(), p1.y(), sr.left(), sr.right(), sr.top(), sr.bottom(), x1, y1, x2, y2);
	m_itemL1->setLine(x1, y1, x2, y2);
	//if (GraphicsUtils::distanceSqd(bezier->endpoint1(), bezier->cp1()) > minDSqd) {
	//	m_itemE1->setVisible(false);
	//}
	//else {
	//	QRectF r(master->mapToScene(bezier->endpoint1()), QSizeF(0,0));
	//	r.adjust(-radius, -radius, radius, radius);
	//	m_itemE1->setRect(r);
	//	m_itemE1->setVisible(true);
	//}

	m_itemL0->setVisible(true);
	m_itemL1->setVisible(true);
}


