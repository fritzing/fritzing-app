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

#include "focusoutcombobox.h"
#include "../debugdialog.h"

FocusOutComboBox::FocusOutComboBox(QWidget * parent) : QComboBox(parent) {
	setEditable(true);
    m_wasOut = true;
    lineEdit()->installEventFilter( this );
}

FocusOutComboBox::~FocusOutComboBox() {
}

void FocusOutComboBox::focusInEvent(QFocusEvent * e) {
    //DebugDialog::debug("focus in");
    QComboBox::focusInEvent(e);
    checkSelectAll();
}

void FocusOutComboBox::focusOutEvent(QFocusEvent * e) {
    //DebugDialog::debug("focus out");
    m_wasOut = true;
	QComboBox::focusOutEvent(e);
	QString t = this->currentText();
	QString it = this->itemText(this->currentIndex());
	if (t.compare(it) != 0) {
		int ix = findText(t);
		if (ix == -1) {
			addItem(t);
			ix = count() - 1;
		}
		setCurrentIndex(ix);
	}
}

bool FocusOutComboBox::eventFilter( QObject *target, QEvent *event ) {
    // subclassing mouseReleaseEvent doesn't seem to work so use eventfilter instead
    if( target == lineEdit() && event->type() == QEvent::MouseButtonRelease ) {
        if (m_wasOut) {
            // only select all the first time the focused lineEdit is clicked, not every time,
            // otherwise you can't move the selection point with the mouse
            checkSelectAll();
            m_wasOut = false;
        }
    }
    return false;
}

void FocusOutComboBox::checkSelectAll() {
    if(lineEdit() && !lineEdit()->hasSelectedText() && isEnabled()) {
        lineEdit()->selectAll();
    }
}
