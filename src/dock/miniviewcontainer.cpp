/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2012 Fachhochschule Potsdam - http://fh-potsdam.de

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


#include <QScrollBar>
#include <QLabel>

#include "miniviewcontainer.h"
#include "../debugdialog.h"
#include "../utils/misc.h"

bool firstTime = true;

MiniViewContainer::MiniViewContainer( QWidget * parent )
	: QWidget(parent)
{
	this->setMinimumSize(32, 32);

	m_miniView = new MiniView(this);
	connect(m_miniView, SIGNAL(rectChangedSignal()), this, SLOT(updateFrame()) );
	connect(m_miniView, SIGNAL(miniViewMousePressedSignal()), this, SLOT(miniViewMousePressedSlot()) );
	m_miniView->resize(this->minimumSize());

	QBrush brush1(QColor(0,0,0));
	m_outerFrame = new MiniViewFrame(brush1, false, this);
	m_outerFrame->resize(this->minimumSize());
	//m_outerFrame->setUpdatesEnabled(false);

	QBrush brush2(QColor(128,0,0));
	m_frame = new MiniViewFrame(brush2, true, this);
	m_frame->resize(this->minimumSize());
}

void MiniViewContainer::setView(QGraphicsView * view)
{
	QGraphicsView * oldView = m_miniView->view();
	if (oldView == view) return;

	if (oldView != NULL) {
		disconnect(oldView->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(updateFrame()));
		disconnect(oldView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(updateFrame()));
		disconnect(oldView->scene(), SIGNAL(sceneRectChanged(QRectF)), this, SLOT(updateFrame()));
		disconnect(oldView, SIGNAL(resizeSignal()), this, SLOT(updateFrame()));
		disconnect(m_frame, SIGNAL(scrollChangeSignal(double, double)), oldView, SLOT(navigatorScrollChange(double, double)));
	}

	m_miniView->setView(view);
	updateFrame();

	bool succeeded = connect(view->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(updateFrame()));
	succeeded = connect(view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(updateFrame()));
	succeeded = connect(view->scene(), SIGNAL(sceneRectChanged(QRectF)), this, SLOT(updateFrame()));
	succeeded = connect(view, SIGNAL(resizeSignal()), this, SLOT(updateFrame()));
	succeeded = connect(m_frame, SIGNAL(scrollChangeSignal(double, double)), view, SLOT(navigatorScrollChange(double, double)));

	forceResize();
}

void MiniViewContainer::forceResize() {
	// force a resize on a view change because otherwise some size or sceneRect isn't updated and the navigator is off
	m_miniView->resize(this->size() / 2.0);
	m_miniView->resize(this->size());
}

void MiniViewContainer::resizeEvent ( QResizeEvent * event )
{
	QWidget::resizeEvent(event);
	m_miniView->resize(this->size());
}

void MiniViewContainer::updateFrame()
{
	QGraphicsView * view = m_miniView->view();
	if (view == NULL) return;

	QSize vSize = view->size();

	bool vVis = false;
	bool hVis = false;
	if (view->verticalScrollBar()->isVisible()) {
		vSize.setWidth(view->width() - view->verticalScrollBar()->width());
		vVis = true;
	}
	if (view->horizontalScrollBar()->isVisible()) {
		vSize.setHeight(view->height() - view->horizontalScrollBar()->height());
		hVis = true;
	}

	QPointF topLeft = view->mapToScene(QPoint(0, 0));
	QPointF bottomRight = view->mapToScene(QPoint(vSize.width(), vSize.height()));
	//DebugDialog::debug(QString("tl %1 %2; br %3 %4, vs %5 %6")
		//.arg(topLeft.x()).arg(topLeft.y()).arg(bottomRight.x()).arg(bottomRight.y())
		//.arg(vSize.width()).arg(vSize.height()) );
	QRectF sceneRect = view->sceneRect();
	QRectF miniSceneRect = m_miniView->sceneRect();

	int newW, newH, newX, newY, tw, th;

	if (hVis && vVis) {
		tw = (int) sceneRect.width();
		th = (int) sceneRect.height();

		//DebugDialog::debug(QString("tw:%1 th:%2").arg(tw).arg(th) );

		newW = (int) (miniSceneRect.width() * (bottomRight.x() - topLeft.x())  / tw);
		newX = (int) (miniSceneRect.width() * (topLeft.x() - sceneRect.x()) / tw);
		newY = (int) (miniSceneRect.height() * (topLeft.y() - sceneRect.y()) / th);
		newH = (int) (miniSceneRect.height() * (bottomRight.y() - topLeft.y())  / th);
	}
	else if (hVis) {
		int min = view->horizontalScrollBar()->minimum();
		int max = view->horizontalScrollBar()->maximum();
		int page = view->horizontalScrollBar()->pageStep();
		int value = view->horizontalScrollBar()->value();

		//DebugDialog::debug(QString("min:%1 max:%2 page:%3 value:%4").arg(min).arg(max).arg(page).arg(value) );

		newW = miniSceneRect.width() * page / (max + page - min);
		newX = miniSceneRect.width() * (value - min) / (max + page - min);

		newY = 0;
		newH = miniSceneRect.height();

	}
	else if (vVis) {
		int min = view->verticalScrollBar()->minimum();
		int max = view->verticalScrollBar()->maximum();
		int page = view->verticalScrollBar()->pageStep();
		int value = view->verticalScrollBar()->value();

		//DebugDialog::debug(QString("min:%1 max:%2 page:%3 value:%4").arg(min).arg(max).arg(page).arg(value) );

		newH = miniSceneRect.height() * page / (max + page - min);
		newY = miniSceneRect.height() * (value - min) / (max + page - min);

		newX = 0;
		newW = miniSceneRect.width();
	}
	else if ((sceneRect.width() > 0) && (sceneRect.height() > 0)) {
		newX = 0;
		newW = miniSceneRect.width();

		newY = 0;
		newH = miniSceneRect.height();
	}
	else {
		newW = this->width();
		newH = this->height();
		newX = newY = 0;
	}

	m_outerFrame->setGeometry(miniSceneRect.left(), miniSceneRect.top(), miniSceneRect.width(), miniSceneRect.height());
	m_frame->setMaxDrag(miniSceneRect.width() + miniSceneRect.left(), miniSceneRect.height() + miniSceneRect.top());
	m_frame->setMinDrag(miniSceneRect.left(), miniSceneRect.top());
	m_frame->setGeometry(newX + miniSceneRect.left(), newY + miniSceneRect.top(), newW, newH);

}

bool MiniViewContainer::eventFilter(QObject *obj, QEvent *event)
{
	switch (event->type()) {
		case QEvent::MouseButtonPress:
		case QEvent::NonClientAreaMouseButtonPress:
		case QEvent::GraphicsSceneMousePress:
			if (obj == this || isParent(this, obj)) {
				emit navigatorMousePressedSignal(this);
			}
			break;
		default:
			break;
    }

	return QObject::eventFilter(obj, event);
}

void MiniViewContainer::filterMousePress()
{
	parent()->installEventFilter(this);
	installEventFilter(this);
	m_outerFrame->installEventFilter(this);
	m_frame->installEventFilter(this);
	m_miniView->installEventFilter(this);
}

void MiniViewContainer::miniViewMousePressedSlot() {
	emit navigatorMousePressedSignal(this);
}

void MiniViewContainer::miniViewMouseEnterSlot() {
	emit navigatorMouseEnterSignal(this);
}

void MiniViewContainer::miniViewMouseLeaveSlot() {
	emit navigatorMouseLeaveSignal(this);
}

void MiniViewContainer::hideHandle(bool hide) {
	m_frame->setVisible(!hide);
	m_outerFrame->setVisible(!hide);
}

void MiniViewContainer::setTitle(const QString & title) {
	m_miniView->setTitle(title);
}

QWidget * MiniViewContainer::miniView() {
	return m_miniView;
}
/////////////////////////////////////////////

MiniViewFrame::MiniViewFrame(QBrush & brush, bool draggable, QWidget * parent)
	: QFrame(parent)

{
	m_brush = brush;
	m_pen.setBrush(m_brush);
	m_pen.setWidth(4);
	m_draggable = draggable;
}

void MiniViewFrame::paintEvent(QPaintEvent * ) {
   	QPainter painter(this);
   	painter.setPen(m_pen);
    painter.setOpacity(0.33);
    painter.drawRect(0,0, this->size().width(), this->size().height());
}

void MiniViewFrame::mousePressEvent(QMouseEvent * event) {
	if (m_draggable) {
		m_dragOffset = event->globalPos();
		m_originalPos = this->pos();
		m_inDrag = true;
	}
	else {
		QFrame::mousePressEvent(event);
	}
}

void MiniViewFrame::mouseMoveEvent(QMouseEvent * event) {
	if (m_inDrag) {
		QRect r = this->geometry();
		QPoint newPos = m_originalPos + event->globalPos() - m_dragOffset;
		if (newPos.x() < m_minDrag.x()) {
			newPos.setX(m_minDrag.x());
		}
		if (newPos.y() < m_minDrag.y()) {
			newPos.setY(m_minDrag.y());
		}
		if (newPos.x() + r.width() > m_maxDrag.x()) {
			newPos.setX(m_maxDrag.x() - r.width());
		}
		if (newPos.y() + r.height() > m_maxDrag.y()) {
			newPos.setY(m_maxDrag.y() - r.height());
		}
		r.moveTopLeft(newPos);
		if (r != this->geometry()) {
			this->setGeometry(r);
			emit scrollChangeSignal((newPos.x() - m_minDrag.x()) / (double) (m_maxDrag.x() - m_minDrag.x() - r.width()),
									(newPos.y() - m_minDrag.y()) / (double) (m_maxDrag.y() - m_minDrag.y() - r.height()) );
		}
	}
	else {
		QFrame::mouseMoveEvent(event);
	}
}

void MiniViewFrame::mouseReleaseEvent(QMouseEvent * event) {
	if (m_inDrag) {
		m_inDrag = false;
	}
	else {
		QFrame::mouseReleaseEvent(event);
	}
}

void MiniViewFrame::setMaxDrag(int x, int y) {
	m_maxDrag.setX(x);
	m_maxDrag.setY(y);
}

void MiniViewFrame::setMinDrag(int x, int y) {
	m_minDrag.setX(x);
	m_minDrag.setY(y);
}

