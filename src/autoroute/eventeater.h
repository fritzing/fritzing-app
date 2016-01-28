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

#ifndef EVENTEATER_H
#define EVENTEATER_H

#include <QObject>
#include <QEvent>
#include <QWidget>
#include <QList>

class EventEater : public QObject
{
Q_OBJECT

public:
	EventEater(QObject * parent = 0);

	void allowEventsIn(QWidget *);

 protected:
     bool eventFilter(QObject *obj, QEvent *event);

protected:
	QList<QWidget *> m_allowedWidgets;
};

#endif
