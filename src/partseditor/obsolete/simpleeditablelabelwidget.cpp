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



#include "simpleeditablelabelwidget.h"
#include "../debugdialog.h"

#include <QHBoxLayout>
#include <QKeyEvent>

SimpleEditableLabelWidget::SimpleEditableLabelWidget(QUndoStack *undoStack, QWidget *parent, const QString &text, bool edited)
	: QFrame(parent)
{
	setObjectName("partsBinTitle");
	m_label = new QLabel(this);
	m_lineEdit = new QLineEdit(this);
	connect(m_lineEdit,SIGNAL(editingFinished()),this,SLOT(toStandardMode()));

	QHBoxLayout *lo = new QHBoxLayout(this);
	lo->setMargin(3);
	lo->setSpacing(0);

	m_hasBeenEdited = edited;
	m_isInEditionMode = false;
	m_label->setText(text);

	m_undoStack = undoStack;
	updateUndoStackIfNecessary();

	toStandardMode(edited);
}

QString SimpleEditableLabelWidget::text() {
	return m_label->text();
}

void SimpleEditableLabelWidget::setText(const QString &text, bool markAsEdited) {
	if(m_label->text() != text) {
		m_label->setText(text);
		m_hasBeenEdited = markAsEdited;
		updateUndoStackIfNecessary();
		emit textChanged(text);
	}
}

void SimpleEditableLabelWidget::toStandardMode(bool markAsEdited) {
	setText(m_lineEdit->text(), markAsEdited);
	swapWidgets(m_label, m_lineEdit);
}

void SimpleEditableLabelWidget::toEditionMode() {
	//if(m_hasBeenEdited) {
		m_lineEdit->setText(m_label->text());
	//} else { // Remove this part of the branch if we want the lineedit to remember what was typed the last time
		//m_lineEdit->setText("");
	//}
	swapWidgets(m_lineEdit, m_label);
	m_lineEdit->setFocus();
}

void SimpleEditableLabelWidget::swapWidgets(QWidget *toShow, QWidget *toHide) {
	layout()->removeWidget(toHide);
	toHide->hide();

	layout()->addWidget(toShow);
	toShow->show();

	m_isInEditionMode = (toShow == m_lineEdit);
}

void SimpleEditableLabelWidget::swapMode() {
	if(m_isInEditionMode) {
		toStandardMode();
	} else {
		toEditionMode();
	}
}

void SimpleEditableLabelWidget::mousePressEvent(QMouseEvent *event) {
	if(!m_isInEditionMode) {
		swapMode();
	}
	QFrame::mousePressEvent(event);
}

void SimpleEditableLabelWidget::keyPressEvent(QKeyEvent *event) {
	if(m_isInEditionMode && event->key() == Qt::Key_Escape) {
		QString prevText = m_label->text();
		toStandardMode();
		setText(prevText);
	}
	QFrame::keyPressEvent(event);
}

void SimpleEditableLabelWidget::updateUndoStackIfNecessary() {
	if(m_hasBeenEdited) {
		m_undoStack->push(new QUndoCommand("Palette Widget title modified"));
	}
}
