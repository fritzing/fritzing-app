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

$Revision: 6924 $:
$Author: irascibl@gmail.com $:
$Date: 2013-03-10 16:58:25 +0100 (So, 10. Mrz 2013) $

********************************************************************/

#include "graphicsflowlayout.h"
#include "../utils/misc.h"

static const int SpaceBefore = 5;
static const int SpaceAfter = 3;

GraphicsFlowLayout::GraphicsFlowLayout(QGraphicsLayoutItem *parent, int spacing)
	: QGraphicsLinearLayout(parent)
{
	setSpacing(spacing);
}

void GraphicsFlowLayout::widgetEvent(QEvent * e) {
	Q_UNUSED(e)
}

void GraphicsFlowLayout::setGeometry(const QRectF &rect) {
	QGraphicsLinearLayout::setGeometry(rect);
	doLayout(rect);
}

int GraphicsFlowLayout::heightForWidth(int width) {
	int height = doLayout(QRectF(0, 0, width, 0));
	return height;
}


int GraphicsFlowLayout::doLayout(const QRectF &rect) {
	double x = rect.x();
	double y = rect.y();
	double lineHeight = 0;

	for(int i=0; i < count(); i++) {
		QGraphicsLayoutItem* item = itemAt(i);
		int nextX = x + item->preferredSize().width() + spacing();

		if (item->sizePolicy().horizontalPolicy() == QSizePolicy::Expanding) { 
			int myY = y + lineHeight + spacing() + SpaceBefore;
			QRectF r(QPoint(rect.x(), myY), item->preferredSize());
			item->setGeometry(r);
			x = rect.x();
			y = myY + item->preferredSize().height() + spacing() + SpaceAfter;
			continue;
		}
		
		if (nextX - spacing() > rect.right() && lineHeight > 0) {
			x = rect.x();
			y = y + lineHeight + spacing();
			nextX = x + item->preferredSize().width() + spacing();
			lineHeight = 0;
		}
		item->setGeometry(QRectF(QPoint(x, y), item->preferredSize()));

		x = nextX;
		// item->preferredSize().height() returns qreal, armel compiler complains
		lineHeight = qMax(lineHeight, item->preferredSize().height());
	}

	m_lastWidth = rect.width();
	return y + lineHeight - rect.y();
}


void GraphicsFlowLayout::clear() {
	QList<QGraphicsLayoutItem*> itemsToRemove;
	for(int i=0; i < count(); i++) {
		itemsToRemove << itemAt(i);
	}

	foreach(QGraphicsLayoutItem* iToR, itemsToRemove) {
		removeItem(iToR);
	}
}

int GraphicsFlowLayout::indexOf(const QGraphicsLayoutItem *item) {
	for(int i=0; i < count(); i++) {
		if(itemAt(i) == item) return i;
	}
	return -1;
}
