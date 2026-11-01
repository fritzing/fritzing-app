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

#include "processeventblocker.h"
#include <QApplication>
#include <QEventLoop>
#ifdef Q_OS_WASM
#include <QElapsedTimer>
#include <QTimer>

// The browser build is single-threaded: the page only repaints and takes input once
// control goes back to the browser, which processEvents() alone does not do. Long
// operations call processEvents() regularly, so at most every 50 ms also wait in a short
// event loop, which suspends (Asyncify) and lets the browser run.
static void yieldToBrowser() {
	static QElapsedTimer sinceLastYield;
	if (sinceLastYield.isValid() && sinceLastYield.elapsed() < 50) return;

	// a 0 ms timer would already be due, so the loop would quit without waiting
	QEventLoop loop;
	QTimer::singleShot(1, &loop, &QEventLoop::quit);
	loop.exec();
	sinceLastYield.restart();
}
#endif

ProcessEventBlocker * ProcessEventBlocker::m_singleton = new ProcessEventBlocker();

ProcessEventBlocker::ProcessEventBlocker()
{
	m_count = 0;
}

void ProcessEventBlocker::processEvents() {
	m_singleton->_processEvents();
}

void ProcessEventBlocker::processEvents(int maxTime) {
	m_singleton->_processEvents(maxTime);
}

bool ProcessEventBlocker::isProcessing() {
	return m_singleton->_isProcessing();
}

void ProcessEventBlocker::block() {
	return m_singleton->_inc(1);
}

void ProcessEventBlocker::unblock() {
	return m_singleton->_inc(-1);
}

void ProcessEventBlocker::_processEvents() {
	m_mutex.lock();
	m_count++;
	m_mutex.unlock();
	QApplication::processEvents();
#ifdef Q_OS_WASM
	yieldToBrowser();
#endif
	m_mutex.lock();
	m_count--;
	m_mutex.unlock();
}

void ProcessEventBlocker::_processEvents(int maxTime) {
	m_mutex.lock();
	m_count++;
	m_mutex.unlock();
	QApplication::processEvents(QEventLoop::AllEvents, maxTime);
#ifdef Q_OS_WASM
	yieldToBrowser();
#endif
	m_mutex.lock();
	m_count--;
	m_mutex.unlock();
}

bool ProcessEventBlocker::_isProcessing() {
	m_mutex.lock();
	bool result = m_count > 0;
	m_mutex.unlock();
	return result;
}

void ProcessEventBlocker::_inc(int i) {
	m_mutex.lock();
	m_count += i;
	m_mutex.unlock();
}
