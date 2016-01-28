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




#include "sketchtoolbutton.h"
#include "debugdialog.h"

#include <QAction>
#include <QActionEvent>
#include <QMenu>
#include <QPaintEngine>

SketchToolButton::SketchToolButton(const QString &imageName, QWidget *parent, QAction* defaultAction)
	: QToolButton(parent)
{
    m_imageName = imageName;			// nice to have for debugging
	setupIcons(imageName);

	//DebugDialog::debug(QString("%1 %2 %3 %4 %5 %6 %7").arg(imageName)
		//.arg(m_enabledImage.width()).arg(m_enabledImage.height())
		//.arg(m_disabledImage.width()).arg(m_disabledImage.height())
		//.arg(m_pressedImage.width()).arg(m_pressedImage.height()));

	if(defaultAction) {
		setDefaultAction(defaultAction);
		setText(defaultAction->text());
	}
}

SketchToolButton::SketchToolButton(const QString &imageName, QWidget *parent, QList<QAction*> menuActions)
	: QToolButton(parent)
{
	m_imageName = imageName;			// nice to have for debugging
	setupIcons(imageName);

	QMenu *menu = new QMenu(this);
	for(int i=0; i < menuActions.size(); i++) {
		QAction* act = menuActions[i];
		menu->addAction(act);
		if(i==0) {
			setDefaultAction(act);
		}
	}
	setMenu(menu);
	connect(menu,SIGNAL(aboutToHide()),this,SLOT(setEnabledIconAux()));
	setPopupMode(QToolButton::MenuButtonPopup);
}

void SketchToolButton::setEnabledIconAux() {
	setEnabledIcon();
}

QString SketchToolButton::imagePrefix() {
	return ":/resources/images/icons/toolbar";
}

void SketchToolButton::setImage(const QPixmap & pixmap) {
	setIcon(QIcon(pixmap));
}

void SketchToolButton::setupIcons(const QString &imageName, bool hasStates) {
    setIconSize(QSize(37,24));
	setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    AbstractStatesButton::setupIcons(imageName, hasStates);
}

void SketchToolButton::updateEnabledState() {
	bool enabled = false;
	foreach(QAction *act, actions()) {
		if(act->isEnabled()) {
			enabled = true;
			break;
		}
	}
	setEnabled(enabled);
}

void SketchToolButton::actionEvent(QActionEvent *event) {
	switch (event->type()) {
		case QEvent::ActionChanged:
			if (event->action() == defaultAction()) {
				setEnabled(defaultAction()->isEnabled()); // update button state
			}
			break;
		default:
			QToolButton::actionEvent(event);
	}
}

void SketchToolButton::mousePressEvent(QMouseEvent *event) {
	setPressedIcon();
	QToolButton::mousePressEvent(event);
}

void SketchToolButton::mouseReleaseEvent(QMouseEvent *event) {
	setEnabledIcon();
	QToolButton::mouseReleaseEvent(event);
}

void SketchToolButton::changeEvent(QEvent *event) {
	if(event->type() == QEvent::EnabledChange) {
		if(this->isEnabled()) {
			setEnabledIcon();
		} else {
			setDisabledIcon();
		}
	}
	QToolButton::changeEvent(event);
}

void SketchToolButton::enterEvent(QEvent *event) {
    QToolButton::enterEvent(event);
    emit entered();
}

void SketchToolButton::leaveEvent(QEvent *event) {
    QToolButton::leaveEvent(event);
    emit left();
}

