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

#include "searchlineedit.h"
#include "../debugdialog.h"

#include <QPainter>
#include <QImage>
#include <QPixmap>

static QPixmap * SearchFieldPixmap;

SearchLineEdit::SearchLineEdit(QWidget * parent) : QLineEdit(parent)
{
    SearchFieldPixmap = NULL;
	setDecoy(true);
}

SearchLineEdit::~SearchLineEdit()
{
}

void SearchLineEdit::cleanup() {
	if (SearchFieldPixmap != NULL) {
		delete SearchFieldPixmap;
		SearchFieldPixmap = NULL;
	}

}

void SearchLineEdit::mousePressEvent(QMouseEvent * event) {
    QLineEdit::mousePressEvent(event);
    emit clicked();
}

void SearchLineEdit::paintEvent(QPaintEvent * event) {
	QLineEdit::paintEvent(event);
    if (SearchFieldPixmap == NULL) {
        SearchFieldPixmap = new QPixmap(":/resources/images/icons/searchField.png");
    }
    if (SearchFieldPixmap == NULL) return;
    if (SearchFieldPixmap->isNull()) return;

    QPainter painter(this);
    QSize sz = size();
    int x = sz.width() - SearchFieldPixmap->width() - 2;
    int y = (sz.height() - SearchFieldPixmap->height()) / 2;
    painter.drawPixmap(x, y, SearchFieldPixmap->width(), SearchFieldPixmap->height(), *SearchFieldPixmap);

}

void SearchLineEdit::enterEvent(QEvent * event) {
	QLineEdit::enterEvent(event);
	if (m_decoy) {
		setColors(QColor(0xc8, 0xc8, 0xc8), QColor(0x57, 0x57, 0x57));
	}
}

void SearchLineEdit::leaveEvent(QEvent * event) {
	QLineEdit::leaveEvent(event);
	if (m_decoy) {
		setColors(QColor(0xb3, 0xb3, 0xb3), QColor(0x57, 0x57, 0x57));
	}
}

void SearchLineEdit::setColors(const QColor & base, const QColor & text) {
	setStyleSheet(QString("background: rgb(%1,%2,%3); color: rgb(%4,%5,%6);")
		.arg(base.red()).arg(base.green()).arg(base.blue())
		.arg(text.red()).arg(text.green()).arg(text.blue()) );
}

void SearchLineEdit::setDecoy(bool d) {
	m_decoy = d;
	if (m_decoy) {
		setReadOnly(true);
		setColors(QColor(0xb3, 0xb3, 0xb3), QColor(0x57, 0x57, 0x57));
	}
	else {
		setReadOnly(false);
		setColors(QColor(0xfc, 0xfc, 0xfc), QColor(0x00, 0x00, 0x00));
	}
	setCursor(Qt::IBeamCursor);
}

bool SearchLineEdit::decoy() {
	return m_decoy;
}
