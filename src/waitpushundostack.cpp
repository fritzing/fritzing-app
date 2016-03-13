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

#include "waitpushundostack.h"
#include "utils/misc.h"
#include "utils/folderutils.h"
#include "commands.h"

#include <QCoreApplication>
#include <QTextStream>

CommandTimer::CommandTimer(QUndoCommand * command, int delayMS, WaitPushUndoStack * undoStack) : QTimer()
{
	m_undoStack = undoStack;
	m_command = command;
	m_undoStack->addTimer(this);
	setSingleShot(true);
	setInterval(delayMS);
	connect(this, SIGNAL(timeout()), this, SLOT(timedout()));
	start();
}

void CommandTimer::timedout() {
    if (m_undoStack) {
	    m_undoStack->push(m_command);
	    m_undoStack->deleteTimer(this);
    }
}

/////////////////////////////////

WaitPushUndoStack::WaitPushUndoStack(QObject * parent) :
	QUndoStack(parent)
{
    m_temporary = NULL;
#ifndef QT_NO_DEBUG
    QString path = FolderUtils::getTopLevelUserDataStorePath();
    path += "/undostack.txt";

	m_file.setFileName(path);
	m_file.remove();
#endif
}

WaitPushUndoStack::~WaitPushUndoStack() {
    clearLiveTimers();
    clearDeadTimers();
}

void WaitPushUndoStack::push(QUndoCommand * cmd) 
{
#ifndef QT_NO_DEBUG
	writeUndo(cmd, 0, NULL);
#endif
    if (m_temporary == cmd) {
        m_temporary->redo();
        return;
    }
	
	QUndoStack::push(cmd);
}


void WaitPushUndoStack::waitPush(QUndoCommand * command, int delayMS) {
	clearDeadTimers();
    if (delayMS <= 0) {
        push(command);
        return;
    }

	new CommandTimer(command, delayMS, this);
}


void WaitPushUndoStack::waitPushTemporary(QUndoCommand * command, int delayMS) {
    m_temporary = command;
	waitPush(command, delayMS);
}

void WaitPushUndoStack::clearDeadTimers() {
    clearTimers(m_deadTimers);
}

void WaitPushUndoStack::clearLiveTimers() {
    clearTimers(m_liveTimers);
}

void WaitPushUndoStack::clearTimers(QList<QTimer *> & timers) {
	QMutexLocker locker(&m_mutex);
	foreach (QTimer * timer, timers) {
		delete timer;
	}
	timers.clear();
}

void WaitPushUndoStack::deleteTimer(QTimer * timer) {
	QMutexLocker locker(&m_mutex);
	m_deadTimers.append(timer);
	m_liveTimers.removeOne(timer);
}

void WaitPushUndoStack::addTimer(QTimer * timer) {
	QMutexLocker locker(&m_mutex);
	m_liveTimers.append(timer);
}

bool WaitPushUndoStack::hasTimers() {
	QMutexLocker locker(&m_mutex);
	return m_liveTimers.count() > 0;
}

void WaitPushUndoStack::resolveTemporary() {
    TemporaryCommand * tc = dynamic_cast<TemporaryCommand *>(m_temporary);
    m_temporary = NULL;
    if (tc) {
        tc->setEnabled(false);
        push(tc);
        tc->setEnabled(true);
    }
}

void WaitPushUndoStack::deleteTemporary() {
    if (m_temporary != NULL) {
        delete m_temporary;
        m_temporary = NULL;
    }
}

#ifndef QT_NO_DEBUG
void WaitPushUndoStack::writeUndo(const QUndoCommand * cmd, int indent, const BaseCommand * parent) 
{
	const BaseCommand * bcmd = dynamic_cast<const BaseCommand *>(cmd);
	QString cmdString; 
	QString indexString;
	if (bcmd == NULL) {
		cmdString = cmd->text();
	}
	else {
		cmdString = bcmd->getDebugString();
		indexString = QString::number(bcmd->index()) + " ";
	}

   	if (m_file.open(QIODevice::Append | QIODevice::Text)) {
   		QTextStream out(&m_file);
		QString indentString(indent, QChar(' '));	
		if (parent) {
			indentString += QString("(%1) ").arg(parent->index());
		}
		indentString += indexString;
		out << indentString << cmdString << "\n";
		m_file.close();
	}

	for (int i = 0; i < cmd->childCount(); i++) {
		writeUndo(cmd->child(i), indent + 4, NULL);
	}

	if (bcmd) {
		for (int i = 0; i < bcmd->subCommandCount(); i++) {
			writeUndo(bcmd->subCommand(i), indent + 4, bcmd);
		}
	}
}
#endif

