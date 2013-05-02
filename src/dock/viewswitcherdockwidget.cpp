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

$Revision: 6373 $:
$Author: cohen@irascible.com $:
$Date: 2012-09-06 08:39:51 +0200 (Do, 06. Sep 2012) $

********************************************************************/

#include <QtGui>
#include <QPainter>

#include "viewswitcherdockwidget.h"
#include "../debugdialog.h"
#include "viewswitcher.h"

#ifdef Q_WS_WIN
#include "windows.h"
#endif


const double inactiveOpacity = 0.6;
const double activeOpacity = 1.0;

ViewSwitcherDockWidget::ViewSwitcherDockWidget(const QString & title, QWidget * parent)
    : FDockWidget(title, parent)
{
	m_bitmap = NULL;
	m_viewSwitcher = NULL;
	m_within = true;

	bool floatFlag = true;
	QPoint initial(10,50);

#ifdef Q_WS_MAC
	initial.setY(34);
#else
	#ifdef Q_WS_X11
		floatFlag = false;
		initial.setY(60);
	#else
		#ifdef Q_WS_WIN
			OSVERSIONINFO OSversion;	
			OSversion.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
			::GetVersionEx(&OSversion);
			//DebugDialog::debug(QString("windows os version %1 %2").arg(OSversion.dwMajorVersion).arg(OSversion.dwMinorVersion));
			if (OSversion.dwMajorVersion > 5) {
				// vista and win 7 major version is 6
                initial.setX(12);
                initial.setY(60);
			}
		#endif
	#endif
#endif

	setFloating(floatFlag);
	m_offsetFromParent.setX(initial.x());
	m_offsetFromParent.setY(initial.y());

	// connect last so they doesn't trigger during construction
	connect(this, SIGNAL(topLevelChanged(bool)), this, SLOT(topLevelChangedSlot(bool)));
	connect(toggleViewAction(), SIGNAL(triggered()), this, SLOT(savePreference()), Qt::DirectConnection);
}

ViewSwitcherDockWidget::~ViewSwitcherDockWidget() {
	if (m_bitmap) {
		delete m_bitmap;
	}
}

void ViewSwitcherDockWidget::calcWithin()
{
	QRect rw = parentWidget()->frameGeometry();
	QRect rt = this->frameGeometry();
	m_within = rw.contains(rt);
	DebugDialog::debug(QString("tabwindow docked %1").arg(m_within));
	if (m_within) {
		m_offsetFromParent = rt.topLeft() - rw.topLeft();
	}
}

void ViewSwitcherDockWidget::windowMoved(QWidget * widget) {
	if (!this->isFloating()) return;

	Q_UNUSED(widget);
	if (m_within) {
		QRect rw = parentWidget()->frameGeometry();
		this->move(rw.topLeft() + m_offsetFromParent);
	}
	else {
		calcWithin();
	}
}

bool ViewSwitcherDockWidget::event(QEvent *event)
{
	bool result = FDockWidget::event(event);
	if (isFloating()) {
		switch (event->type()) {
			case QEvent::MouseButtonRelease:
				calcWithin();
				break;
			default:
				break;
		}

	}
	return result;
}

void ViewSwitcherDockWidget::setViewSwitcher(ViewSwitcher * viewSwitcher)
{
	m_viewSwitcher = viewSwitcher;
	setTitleBarWidget(viewSwitcher);
	topLevelChangedSlotAux(isFloating());
}

void ViewSwitcherDockWidget::resizeEvent(QResizeEvent * event)
{
	FDockWidget::resizeEvent(event);
}

void ViewSwitcherDockWidget::topLevelChangedSlot(bool topLevel) {
	topLevelChangedSlotAux(topLevel);
	savePreference();
}

void ViewSwitcherDockWidget::topLevelChangedSlotAux(bool topLevel) {
	if (m_viewSwitcher == NULL) return;

	if (!topLevel) {
		this->clearMask();
		return;
	}

	this->setStyleSheet("border: 0px; margin: 0px; padding: 0px; border-radius: 0px; spacing: 0px;");
	QRect r = this->geometry();
	const QBitmap * mask = m_viewSwitcher->getMask();
	QSize vssz = mask->size();
	QSize sz = size();
	this->setMinimumSize(vssz);
	//DebugDialog::debug(QString("mask size %1 %2").arg(vssz.width()).arg(vssz.height()));
	r.setRect(r.left() + ((sz.width() - vssz.width()) / 2.0), 
				r.top() + ((sz.height() - vssz.height()) / 2.0), 
				vssz.width(), 
				vssz.height());
	this->setGeometry(r);
	this->setMask(*mask);
}

void ViewSwitcherDockWidget::setVisible(bool visible) {
	FDockWidget::setVisible(visible);
}


void ViewSwitcherDockWidget::prestorePreference() {
	disconnect(this, SIGNAL(topLevelChanged(bool)), this, SLOT(topLevelChangedSlot(bool)));
}

void ViewSwitcherDockWidget::restorePreference() {
	QSettings settings;
	QVariant value = settings.value("viewswitcher/state");
	if (!value.isNull()) {
		// override visibility setting because if you close an inactive mainwindow
		// the viewswitcher will be hidden, but that's not really the true state:
		// the viewswitcher would be visible if the window were active
		int v = value.toInt();
		//DebugDialog::debug(QString("loading viewswitcher pref %1").arg(v));
		bool vis = (v & 2) != 0;
		if ((v & 1) == 1) {
			// floating, so use m_state
			setVisible(vis);
#ifndef Q_WS_X11
			// until the dock activation bug is fixed
			setFloating(true);
#endif
		}
		else {
			setVisible(vis);
			setFloating(false);
		}
	}
	connect(this, SIGNAL(topLevelChanged(bool)), this, SLOT(topLevelChangedSlot(bool)));
}

void ViewSwitcherDockWidget::savePreference() {
	QSettings settings;
	int v = isVisible() ? 2 : 0;
	v += isFloating() ? 1 : 0;
	//DebugDialog::debug(QString("saving viewswitcher pref (1):%1").arg(v));
	settings.setValue("viewswitcher/state", v);
}

