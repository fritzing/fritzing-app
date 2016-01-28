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

#include "fmessagebox.h"
#include "../debugdialog.h"

bool FMessageBox::BlockMessages = false;

FMessageBox::FMessageBox(QWidget * parent) : QMessageBox(parent) {
}

int FMessageBox::exec() {
    if (BlockMessages) return QMessageBox::Cancel;

    return QMessageBox::exec();
}

QMessageBox::StandardButton FMessageBox::critical( QWidget * parent, const QString & title, const QString & text, StandardButtons buttons, StandardButton defaultButton) {
    if (BlockMessages) {
        DebugDialog::debug("critcal " + title);
        DebugDialog::debug(text);
        return QMessageBox::Cancel;
    }

    return QMessageBox::critical(parent, title, text, buttons, defaultButton); 
}

QMessageBox::StandardButton FMessageBox::information( QWidget * parent, const QString & title, const QString & text, StandardButtons buttons, StandardButton defaultButton) {
    if (BlockMessages) {
        DebugDialog::debug("information " + title);
        DebugDialog::debug(text);
        return QMessageBox::Cancel;
    }

    return QMessageBox::information(parent, title, text, buttons, defaultButton); 
}

QMessageBox::StandardButton FMessageBox::question( QWidget * parent, const QString & title, const QString & text, StandardButtons buttons, StandardButton defaultButton) {
    if (BlockMessages) {
        DebugDialog::debug("question " + title);
        DebugDialog::debug(text);
        return QMessageBox::Cancel;
    }

    return QMessageBox::question(parent, title, text, buttons, defaultButton); 
}

QMessageBox::StandardButton FMessageBox::warning( QWidget * parent, const QString & title, const QString & text, StandardButtons buttons, StandardButton defaultButton) {
    if (BlockMessages) {
        DebugDialog::debug("warning " + title);
        DebugDialog::debug(text);
        return QMessageBox::Cancel;
    }

    return QMessageBox::warning(parent, title, text, buttons, defaultButton); 
}

