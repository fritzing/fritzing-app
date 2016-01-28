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

$Revision: 6141 $:
$Author: cohen@irascible.com $:
$Date: 2012-07-04 21:20:05 +0200 (Mi, 04. Jul 2012) $

********************************************************************/

#include "cursormaster.h"
#include "../debugdialog.h"


#include <QApplication>
#include <QBitmap>
#include <QString>
#include <QKeyEvent>
#include <QEvent>
#include <QGraphicsItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsSceneMouseEvent>
#include <QTimer>

QCursor * CursorMaster::BendpointCursor = NULL;
QCursor * CursorMaster::NewBendpointCursor = NULL;
QCursor * CursorMaster::MakeWireCursor = NULL;
QCursor * CursorMaster::MakeCurveCursor = NULL;
QCursor * CursorMaster::RubberbandCursor = NULL;
QCursor * CursorMaster::MoveCursor = NULL;
QCursor * CursorMaster::BendlegCursor = NULL;
QCursor * CursorMaster::RotateCursor = NULL;
QCursor * CursorMaster::ScaleCursor = NULL;

//static QTimer timer;

CursorMaster CursorMaster::TheCursorMaster;
static QList<QObject *> Listeners;

static QHash<QGraphicsScene *, QGraphicsPixmapItem *> CursorItems;
static QList<QCursor **> Cursors;


CursorMaster::CursorMaster() : QObject()
{
    m_blocked = false;
}

void CursorMaster::cleanup() {
    foreach (QCursor ** cursor, Cursors) {
        delete *cursor;
    }
    Cursors.clear();
}

void CursorMaster::initCursors()
{
	if (BendpointCursor == NULL) {
		//timer.setSingleShot(true);
		//timer.setInterval(0);
		//connect(&timer, SIGNAL(timeout()), &TheCursorMaster, SLOT(moveCursor()));

		QStringList names;
		QStringList masks;

		names << ":resources/images/cursor/bendpoint.bmp" 
			<< ":resources/images/cursor/new_bendpoint.bmp"
			<< ":resources/images/cursor/make_wire.bmp"
			<< ":resources/images/cursor/curve.bmp"
			<< ":resources/images/cursor/rubberband_move.bmp"
			<< ":resources/images/cursor/part_move.bmp"
			<< ":resources/images/cursor/bendleg.bmp"
			<< ":resources/images/cursor/rotate.bmp"
			<< ":resources/images/cursor/scale.bmp";

		masks << ":resources/images/cursor/bendpoint_mask.bmp"
			<< ":resources/images/cursor/new_bendpoint_mask.bmp"
			<< ":resources/images/cursor/make_wire_mask.bmp"
			<< ":resources/images/cursor/curve_mask.bmp"
			<< ":resources/images/cursor/rubberband_move_mask.bmp"
			<< ":resources/images/cursor/part_move_mask.bmp"
			<< ":resources/images/cursor/bendleg_mask.bmp"
			<< ":resources/images/cursor/rotate_mask.bmp"
			<< ":resources/images/cursor/scale_mask.bmp";

		Cursors << &BendpointCursor
			<< &NewBendpointCursor
			<< &MakeWireCursor
			<< &MakeCurveCursor
			<< &RubberbandCursor
			<< &MoveCursor
			<< &BendlegCursor
			<< &RotateCursor
			<< &ScaleCursor;

		for (int i = 0; i < Cursors.count(); i++) {
			QBitmap bitmap1(names.at(i));
			QBitmap bitmap1m(masks.at(i));
            *Cursors.at(i) = new QCursor(bitmap1, bitmap1m, 0, 0);
		}

		QApplication::instance()->installEventFilter(instance());
	}
}

CursorMaster * CursorMaster::instance()
{
	return &TheCursorMaster;
}

void CursorMaster::addCursor(QObject * object, const QCursor & cursor)
{
    if (m_blocked) return;

	if (object == NULL) return;


	/*
	QGraphicsItem * item = dynamic_cast<QGraphicsItem *>(object);
	if (item == NULL) return;

	QGraphicsScene * scene = item->scene();
	if (scene == NULL) return;

	QGraphicsView * view = dynamic_cast<QGraphicsView *>(scene->parent());
	if (view == NULL) return;

	QGraphicsPixmapItem * pixmapItem = CursorItems.value(scene, NULL);
	if (pixmapItem == NULL) {
		pixmapItem = new QGraphicsPixmapItem();
		pixmapItem->setZValue(10000);			// always on top
		pixmapItem->setVisible(true);
		pixmapItem->setAcceptedMouseButtons(0);
		pixmapItem->setAcceptDrops(false);
		pixmapItem->setAcceptTouchEvents(false);
		pixmapItem->setAcceptHoverEvents(false);
		pixmapItem->setEnabled(false);
		pixmapItem->setFlags(QGraphicsItem::ItemIgnoresTransformations);
		CursorItems.insert(scene, pixmapItem);
		scene->addItem(pixmapItem);
		scene->installEventFilter(this);
	}

	pixmapItem->setPixmap(*cursor.mask());
	pixmapItem->setPos(view->mapToScene(view->mapFromGlobal(QCursor::pos())) + cursor.hotSpot());
	*/

	if (Listeners.contains(object)) {
		if (Listeners.first() != object) {
			Listeners.removeOne(object);
			Listeners.push_front(object);
		}
		//DebugDialog::debug(QString("changing cursor %1").arg((long) object));
		QApplication::changeOverrideCursor(cursor);
		return;
	}

	Listeners.push_front(object);
	connect(object, SIGNAL(destroyed(QObject *)), this, SLOT(deleteCursor(QObject *)));
	QApplication::setOverrideCursor(cursor);
	//DebugDialog::debug(QString("addding cursor %1").arg((long) object));
}

void CursorMaster::removeCursor(QObject * object)
{
	if (object == NULL) return;

	if (Listeners.contains(object)) {
		disconnect(object, SIGNAL(destroyed(QObject *)), this, SLOT(deleteCursor(QObject *)));
		Listeners.removeOne(object);
		QApplication::restoreOverrideCursor();
		//DebugDialog::debug(QString("removing cursor %1").arg((long) object));
	}


	/*
	QGraphicsItem * item = dynamic_cast<QGraphicsItem *>(object);
	if (item == NULL) return;

	QGraphicsScene * scene = item->scene();
	if (scene == NULL) return;

	QGraphicsPixmapItem * pixmapItem = CursorItems.value(scene, NULL);
	if (pixmapItem == NULL) return;

	pixmapItem->hide();
	*/
}

void CursorMaster::deleteCursor(QObject * object)
{
	removeCursor(object);
}

bool CursorMaster::eventFilter(QObject * object, QEvent * event)
{
	Q_UNUSED(object);
	//QGraphicsScene * scene = NULL;

	switch (event->type()) {
		case QEvent::KeyPress:
		case QEvent::KeyRelease:
			//scene = dynamic_cast<QGraphicsScene *>(object);
			//DebugDialog::debug(QString("event filter %1").arg(object->metaObject()->className()));
			//if (scene) 
			{
				QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
				foreach (QObject * listener, Listeners) {
					if (listener) {
						dynamic_cast<CursorKeyListener *>(listener)->cursorKeyEvent(keyEvent->modifiers());
						break;
					}
				}
			}
			break;
/*
		case QEvent::GraphicsSceneMouseMove:
			
			scene = dynamic_cast<QGraphicsScene *>(object);
			if (scene) {
				QGraphicsPixmapItem * pixmapItem = CursorItems.value(scene, NULL);
				if (pixmapItem) {
					timer.setProperty("loc", ((QGraphicsSceneMouseEvent *) event)->scenePos());
					timer.setUserData(1, (QObjectUserData *) pixmapItem);
				}
			}
			break;

		case QEvent::Leave:
			scene = dynamic_cast<QGraphicsScene *>(object);
			if (scene) {
				QGraphicsPixmapItem * pixmapItem = CursorItems.value(scene, NULL);
				if (pixmapItem) {
					//DebugDialog::debug("pos", ((QGraphicsSceneMouseEvent *) event)->scenePos());
					pixmapItem->hide();
				}
			}
			break;
*/
		default:
			break;
	}
	

	return false;
}

void CursorMaster::moveCursor() {
	//QObject * t = sender();
	//if (t == NULL) return;

	//QPointF p = t->property("loc").toPointF();
	//QGraphicsPixmapItem * item = (QGraphicsPixmapItem *) t->userData(1);

	//DebugDialog::debug("move", p);
	//item->setPos(p);   // + cursor->hotspot
	//item->show();
}

void CursorMaster::block() {
    m_blocked = true;
}

void CursorMaster::unblock() {
    m_blocked = false;
}
