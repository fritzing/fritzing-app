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

#include "expandinglabel.h"

ExpandingLabel::ExpandingLabel(QWidget *parent, int minSize) : QTextEdit(parent) {
	setMinimumWidth(minSize);
	setReadOnly(true);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void ExpandingLabel::setLabelText(const QString& theText) {
	document()->setHtml(theText);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	setAlignment(Qt::AlignCenter);
	setContextMenuPolicy(Qt::NoContextMenu);
}

void ExpandingLabel::allTextVisible() {
	QTextDocument *doc = document();
	doc->setTextWidth(width());
	int height = doc->documentLayout()->documentSize().toSize().height();
	setFixedHeight(height);
}

void ExpandingLabel::mouseMoveEvent(QMouseEvent * event) {
	QAbstractScrollArea::mouseMoveEvent(event);
}

void ExpandingLabel::mousePressEvent(QMouseEvent *event) {
	emit mousePressSignal(event);
	QAbstractScrollArea::mousePressEvent(event);
}

void ExpandingLabel::mouseReleaseEvent(QMouseEvent *event) {
	emit mouseReleaseSignal(event);
	QAbstractScrollArea::mouseReleaseEvent(event);
}

