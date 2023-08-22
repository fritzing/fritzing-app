/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2019 Fritzing

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

QCursor * CursorMaster::BendpointCursor = nullptr;
QCursor * CursorMaster::NewBendpointCursor = nullptr;
QCursor * CursorMaster::MakeWireCursor = nullptr;
QCursor * CursorMaster::MakeCurveCursor = nullptr;
QCursor * CursorMaster::RubberbandCursor = nullptr;
QCursor * CursorMaster::MoveCursor = nullptr;
QCursor * CursorMaster::BendlegCursor = nullptr;
QCursor * CursorMaster::RotateCursor = nullptr;
QCursor * CursorMaster::ScaleCursor = nullptr;

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
	Q_FOREACH (QCursor ** cursor, Cursors) {
		delete *cursor;
	}
	Cursors.clear();
}

void CursorMaster::initCursors()
{
	if (BendpointCursor == nullptr) {
		//timer.setSingleShot(true);
		//timer.setInterval(0);
		//connect(&timer, SIGNAL(timeout()), &TheCursorMaster, SLOT(moveCursor()));

		QStringList names;

		names << ":resources/images/cursor/bendpoint.png"
			  << ":resources/images/cursor/new_bendpoint.png"
			  << ":resources/images/cursor/make_wire.png"
			  << ":resources/images/cursor/curve.png"
			  << ":resources/images/cursor/rubberband_move.png"
			  << ":resources/images/cursor/part_move.png"
			  << ":resources/images/cursor/bendleg.png"
			  << ":resources/images/cursor/rotate.png"
			  << ":resources/images/cursor/scale.png";

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
			QPixmap pixmap(names.at(i));
			*Cursors.at(i) = new QCursor(pixmap, 0, 0);
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

	if (object == nullptr) return;


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
	//DebugDialog::debug(QString("adding cursor %1").arg((long) object));
}

void CursorMaster::removeCursor(QObject * object)
{
	if (object == nullptr) return;

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
		auto *keyEvent = static_cast<QKeyEvent*>(event);
		Q_FOREACH (QObject * listener, Listeners) {
			if (listener != nullptr) {
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
