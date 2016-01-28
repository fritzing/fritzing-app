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

$Revision: 6469 $:
$Author: irascibl@gmail.com $:
$Date: 2012-09-23 00:54:30 +0200 (So, 23. Sep 2012) $

********************************************************************/

#ifndef WAITPUSHUNDOSTACK_H
#define WAITPUSHUNDOSTACK_H

#include <QUndoStack>
#include <QTimer>
#include <QMutex>
#include <QFile>
#include <QPointer>

class WaitPushUndoStack : public QUndoStack
{
    Q_OBJECT
public:
	WaitPushUndoStack(QObject * parent = 0);
	~WaitPushUndoStack();

	void waitPush(QUndoCommand *, int delayMS);
	void waitPushTemporary(QUndoCommand *, int delayMS);
    void resolveTemporary();
    void deleteTemporary();
    void deleteTimer(QTimer *);
    void addTimer(QTimer *);
	void push(QUndoCommand *);
	bool hasTimers();

#ifndef QT_NO_DEBUG
public:
	void writeUndo(const QUndoCommand *, int indent, const class BaseCommand * parent);

protected:
	QFile m_file;
#endif
   
 protected:
	void clearDeadTimers();
	void clearLiveTimers();
	void clearTimers(QList<QTimer *> &);

protected:
	QList<QTimer *> m_deadTimers;
	QList<QTimer *> m_liveTimers;
	QMutex m_mutex;
    QUndoCommand * m_temporary;
};


class CommandTimer : public QTimer
{

Q_OBJECT

public:
	CommandTimer(QUndoCommand * command, int delayMS, WaitPushUndoStack * undoStack);

protected slots:
	void timedout();

protected:
	QUndoCommand * m_command;
	QPointer<WaitPushUndoStack> m_undoStack;
};


#endif
