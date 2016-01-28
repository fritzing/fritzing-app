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



#include <QUndoCommand>

#include "abstracteditablelabelwidget.h"
#include "../utils/misc.h"

AbstractEditableLabelWidget::AbstractEditableLabelWidget(QString text, WaitPushUndoStack *undoStack, QWidget *parent, QString title, bool edited, bool noSpacing) : QFrame(parent) {
	m_noSpacing = noSpacing;
	m_edited = edited;
	m_isInEditionMode = false;
	m_undoStack = undoStack;

	QGridLayout *layout = new QGridLayout;

	if(!title.isNull() && !title.isEmpty()) {
		m_title = new QLabel(title, this);
		m_title->setObjectName("title");
		layout->addWidget(m_title,0,0);
	} else {
		m_title = NULL;
	}

	setLayout(layout);

	m_label = new EditableLabel(text, this);
	connect(m_label,SIGNAL(editionStarted(QString)),this,SLOT(editionStarted(QString)));
	connect(this,SIGNAL(editionCompleted(QString)),m_label,SLOT(editionCompleted(QString)));

	m_acceptButton = new QPushButton(tr("Accept"), this);
	connect(m_acceptButton,SIGNAL(clicked()),this,SLOT(informEditionCompleted()));

	m_cancelButton = new QPushButton(tr("Cancel"), this);
	connect(m_cancelButton,SIGNAL(clicked()),this,SLOT(editionCanceled()));
}

QString AbstractEditableLabelWidget::text() {
	if(m_edited) {
		return m_label->text();
	} else {
		return ___emptyString___;
	}
}

void AbstractEditableLabelWidget::editionStarted(QString text) {
	setEditionText(text);
	toEditionMode();
}

void AbstractEditableLabelWidget::informEditionCompleted() {
	if(m_isInEditionMode) {
		m_undoStack->push(new QUndoCommand("Dummy parts editor command"));
		m_edited = true;
		emit editionCompleted(editionText());
		toStandardMode();
	}
}

void AbstractEditableLabelWidget::editionCanceled() {
	toStandardMode();
}

void AbstractEditableLabelWidget::toStandardMode() {
	m_isInEditionMode = false;

	hide();
	QGridLayout *layout = (QGridLayout*)this->layout();

	myEditionWidget()->hide();
	layout->removeWidget(myEditionWidget());
	m_acceptButton->hide();
	layout->removeWidget(m_acceptButton);
	m_cancelButton->hide();
	layout->removeWidget(m_cancelButton);

	m_label->show();
	layout->addWidget(m_label,1,0);

	setNoSpacing(layout);

	updateGeometry();
	show();

	emit editionFinished();
}

void AbstractEditableLabelWidget::toEditionMode() {
	m_isInEditionMode = true;

	hide();
	QGridLayout *layout = (QGridLayout*)this->layout();

	m_label->hide();
	layout->removeWidget(m_label);

	if(!m_edited) {
		setEmptyTextToEdit();
	}
	myEditionWidget()->show();
	layout->addWidget(myEditionWidget(),1,0,1,5);
	m_acceptButton->show();
	layout->addWidget(m_acceptButton,2,3);
	m_cancelButton->show();
	layout->addWidget(m_cancelButton,2,4);

	setNoSpacing(layout);

	updateGeometry();
	show();

	emit editionStarted();
}

void AbstractEditableLabelWidget::setNoSpacing(QLayout *layout) {
	if(m_noSpacing) {
		layout->setMargin(0);
		layout->setSpacing(0);
	}
}
