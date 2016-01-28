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

$Revision: 6112 $:
$Author: cohen@irascible.com $:
$Date: 2012-06-28 00:18:10 +0200 (Do, 28. Jun 2012) $

********************************************************************/

#include "eventeater.h"
#include "../debugdialog.h"


EventEater::EventEater(QObject * parent) : QObject(parent) {
}

bool EventEater::eventFilter(QObject *obj, QEvent *event)
{
	switch (event->type()) {
		case QEvent::KeyPress:
		case QEvent::KeyRelease:			
		case QEvent::MouseButtonPress:
		case QEvent::MouseButtonRelease:			
		case QEvent::MouseButtonDblClick:
		case QEvent::HoverEnter:
		case QEvent::HoverLeave:
		case QEvent::WinEventAct:
		case QEvent::GraphicsSceneHoverEnter:
		case QEvent::GraphicsSceneHoverLeave:
		case QEvent::GraphicsSceneHoverMove:
		case QEvent::GraphicsSceneMouseDoubleClick:
		case QEvent::GraphicsSceneMouseMove:
		case QEvent::GraphicsSceneMousePress:
		case QEvent::GraphicsSceneMouseRelease:
		case QEvent::NonClientAreaMouseButtonDblClick:
		case QEvent::NonClientAreaMouseButtonPress:
		case QEvent::NonClientAreaMouseButtonRelease:
		case QEvent::NonClientAreaMouseMove:
			{
				bool gotOne = false;
				foreach (QWidget * widget, m_allowedWidgets) {
					for (QObject * parentObj = obj; parentObj != NULL; parentObj = parentObj->parent()) {
						if (parentObj == widget) {
							gotOne = true;
							break;
						}
					}
				}
				if (!gotOne) {
					// filter out the event
					return true;
				}
			}
			break;
		 default:
			 break;
    }

	return QObject::eventFilter(obj, event);
}

void EventEater::allowEventsIn(QWidget * widget) {
	m_allowedWidgets.append(widget);
}
