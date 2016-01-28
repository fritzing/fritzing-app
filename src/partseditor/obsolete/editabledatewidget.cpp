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


#include "editabledatewidget.h"
#include "../utils/misc.h"

EditableDateWidget::EditableDateWidget(QDate date, WaitPushUndoStack *undoStack, QWidget *parent, QString title, bool edited, bool noSpacing)
	: AbstractEditableLabelWidget(date.toString(Qt::ISODate), undoStack, parent, title, edited, noSpacing) {
	m_dateEdit = new QDateEdit(this);
	toStandardMode();
}

QString EditableDateWidget::editionText() {
	return m_dateEdit->date().toString(Qt::ISODate);
}
void EditableDateWidget::setEditionText(QString text) {
	m_dateEdit->setDate(QDate::fromString(text, Qt::ISODate));
}
QWidget* EditableDateWidget::myEditionWidget() {
	return m_dateEdit;
}

void EditableDateWidget::setEmptyTextToEdit() {
	m_dateEdit->setDate(QDate());
}
