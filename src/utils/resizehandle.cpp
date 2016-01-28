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

#include "resizehandle.h"
#include "../sketch/zoomablegraphicsview.h"
#include "../debugdialog.h"
#include "../sketch/fgraphicsscene.h"

#include <QCursor>

ResizeHandle::ResizeHandle(const QPixmap &pixmap, const QCursor & cursor, bool ignoresTransforms, QGraphicsItem *parent)
: QGraphicsPixmapItem(pixmap, parent)
{
	setCursor(cursor);
	setVisible(true);
	setFlag(QGraphicsItem::ItemIgnoresTransformations, ignoresTransforms);
}

ResizeHandle::~ResizeHandle() {
}

void ResizeHandle::mousePressEvent(QGraphicsSceneMouseEvent * event) {
	event->accept();
	emit mousePressSignal(event, this);
}

void ResizeHandle::mouseMoveEvent(QGraphicsSceneMouseEvent * event) {
	event->accept();
	emit mouseMoveSignal(event, this);
}

void ResizeHandle::mouseReleaseEvent(QGraphicsSceneMouseEvent * event) {
	event->accept();
	emit mouseReleaseSignal(event, this);
}

void ResizeHandle::setResizeOffset(QPointF p) {
	m_resizeOffset = p;
}

QPointF ResizeHandle::resizeOffset()
{
	return m_resizeOffset;
}

QVariant ResizeHandle::itemChange(GraphicsItemChange change, const QVariant &value)
{
	switch (change) {
		case QGraphicsItem::ItemSceneHasChanged: 
			if (scaling()) {
				ZoomableGraphicsView *sw = dynamic_cast<ZoomableGraphicsView*>(scene()->parent());
				if (sw) {
					connect(sw, SIGNAL(zoomChanged(double)), this, SLOT(zoomChangedSlot(double)));
				}

			}
			break;
		default:
			break;
   	}

    return QGraphicsPixmapItem::itemChange(change, value);
}

void ResizeHandle::zoomChangedSlot(double scale) {
	emit zoomChangedSignal(scale);
}

double ResizeHandle::currentScale() {
	if (scaling()) {
		ZoomableGraphicsView *sw = dynamic_cast<ZoomableGraphicsView*>(scene()->parent());
		if(sw) {
			return sw->currentZoom()/100;
		}
	}
	return 1;
}

void ResizeHandle::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if(scene()) {
        FGraphicsScene * fscene = qobject_cast<FGraphicsScene *>(scene());
        if (fscene != NULL && fscene->displayHandles()) {
            QGraphicsPixmapItem::paint(painter, option, widget);
		}
	}
}

bool ResizeHandle::scaling() {
    return (this->flags() & QGraphicsItem::ItemIgnoresTransformations) && (scene() != NULL);
}

