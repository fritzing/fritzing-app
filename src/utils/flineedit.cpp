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

#include "flineedit.h"

FLineEdit::FLineEdit(QWidget * parent) : QLineEdit(parent)
{
	editingFinishedSlot();
	connect(this, SIGNAL(editingFinished()), this, SLOT(editingFinishedSlot()));
}

FLineEdit::~FLineEdit()
{
}

void FLineEdit::editingFinishedSlot() {
	m_readOnly = true;
	emit editable(false);
	setCursor(Qt::IBeamCursor);
}

void FLineEdit::mousePressEvent ( QMouseEvent * event ) {
	if (m_readOnly) {
		m_readOnly = false;
		emit editable(true);
	}

	QLineEdit::mousePressEvent(event);
}

void FLineEdit::enterEvent(QEvent * event) {
	QLineEdit::enterEvent(event);
	if (m_readOnly) {
		emit mouseEnter();
	}
}

void FLineEdit::leaveEvent(QEvent * event) {
	QLineEdit::leaveEvent(event);
	if (m_readOnly) {
		emit mouseLeave();
	}
}

