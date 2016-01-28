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

#include "autoclosemessagebox.h"
#include "../debugdialog.h"
#include "../mainwindow/mainwindow.h"

static const int Interval = 30;
static const int Steps = 7;
static const int Wait = 100;

AutoCloseMessageBox::AutoCloseMessageBox( QWidget * parent ) 
	: QLabel(parent)
{
	setWordWrap(true);
}

void AutoCloseMessageBox::setStartPos(int x, int y) {
	m_startX = x;
	m_startY = y;
	QRect r = this->geometry();
	r.moveTo(x, y);
	setGeometry(r);
}

void AutoCloseMessageBox::setEndPos(int x, int y) {
	m_endX = x;
	m_endY = y;
}

void AutoCloseMessageBox::start() {
	m_counter = Steps;
	m_animationTimer.setInterval(Interval);
	m_animationTimer.setSingleShot(false);
	connect(&m_animationTimer, SIGNAL(timeout()), this, SLOT(moveOut()));
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    m_animationTimer.setTimerType(Qt::PreciseTimer);
#endif
    m_movingState = MovingOut;
	m_animationTimer.start();
	show();
}

void AutoCloseMessageBox::mousePressEvent(QMouseEvent * event) {
	Q_UNUSED(event);

	if (m_movingState != MovingBack) {
		m_animationTimer.stop();
		if (m_movingState == MovingOut) {
			disconnect(&m_animationTimer, SIGNAL(timeout()), this, SLOT(moveOut()));
		}
		else if (m_movingState == Waiting) {
			disconnect(&m_animationTimer, SIGNAL(timeout()), this, SLOT(wait()));
		}

		prepMoveBack();
	}
}


void AutoCloseMessageBox::moveOut() {
	if (--m_counter == 0) {
		m_animationTimer.stop();
		disconnect(&m_animationTimer, SIGNAL(timeout()), this, SLOT(moveOut()));
		m_movingState = Waiting;
		QRect r = geometry();
		r.moveTo(m_endX, m_endY);
		setGeometry(r);
		m_counter = Wait;
		connect(&m_animationTimer, SIGNAL(timeout()), this, SLOT(wait()));
		m_animationTimer.start();
		return;
	}

	QRect r = geometry();
	int dx = (m_endX - r.x()) / m_counter;
	int dy = (m_endY - r.y()) / m_counter;
	r.translate(dx, dy);
	setGeometry(r);
}


void AutoCloseMessageBox::moveBack() {
	if (--m_counter == 0) {
		m_animationTimer.stop();
		disconnect(&m_animationTimer, SIGNAL(timeout()), this, SLOT(moveOut()));
		QRect r = geometry();
		r.moveTo(m_startX, m_startY);
		setGeometry(r);
		this->deleteLater();
		return;
	}

	QRect r = geometry();
	int dx = (m_startX - r.x()) / m_counter;
	int dy = (m_startY - r.y()) / m_counter;
	r.translate(dx, dy);
	setGeometry(r);
}

void AutoCloseMessageBox::wait() {
	if (--m_counter == 0) {
		m_animationTimer.stop();
		disconnect(&m_animationTimer, SIGNAL(timeout()), this, SLOT(wait()));
		prepMoveBack();
	}
}

void AutoCloseMessageBox::prepMoveBack() {
	m_counter = Steps;
	m_movingState = MovingBack;
	connect(&m_animationTimer, SIGNAL(timeout()), this, SLOT(moveBack()));
	m_animationTimer.start();
}

void AutoCloseMessageBox::showMessage(QWidget *window, const QString &message) 
{
	MainWindow * mainWindow = qobject_cast<MainWindow *>(window);
	if (mainWindow == NULL) return;

	QStatusBar * statusBar = mainWindow->realStatusBar();
	if (statusBar == NULL) return;

	AutoCloseMessageBox * acmb = new AutoCloseMessageBox(mainWindow);
	acmb->setText(message);
	QRect dest = statusBar->geometry(); // toolbar->geometry();
	QRect r = mainWindow->geometry();
	acmb->setFixedSize(QSize(dest.width(), dest.height()));
	QPoint p(dest.x(), dest.y());
	p = statusBar->parentWidget()->mapTo(mainWindow, p);
	acmb->setStartPos(p.x(), r.height());
	acmb->setEndPos(p.x(), p.y());
	acmb->start();
}


