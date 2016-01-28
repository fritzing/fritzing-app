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

#include <QTimer>

#include "fdockwidget.h"
#include "../debugdialog.h"

FDockWidget::FDockWidget( const QString & title, QWidget * parent)
	: QDockWidget(title, parent)
{
	m_state = false;
	setObjectName(title.trimmed().toLower().remove(" "));
	setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
}

void FDockWidget::saveState() {
	m_state = this->isFloating() && this->isVisible();
	//DebugDialog::debug(QString("dock save state %1 %2").arg(windowTitle()).arg(m_state));

}

void FDockWidget::restoreState() {
	//DebugDialog::debug(QString("dock restore state %1 %2").arg(windowTitle()).arg(m_state));
	if (m_state) {
		this->setVisible(true);
	}
}

bool FDockWidget::event(QEvent * e) {
	switch (e->type()) {
		case QEvent::WindowActivate:
			emit dockChangeActivationSignal(true, this);
			break;
		case QEvent::WindowDeactivate:
			emit dockChangeActivationSignal(false, this);
			break;
		default:
			break;
	}
	return QDockWidget::event(e);
}

void FDockWidget::moveEvent(QMoveEvent *event) {
	QDockWidget::moveEvent(event);
	emit positionChanged();
}

