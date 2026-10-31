/*******************************************************************

Part of the Fritzing project - https://fritzing.org
Copyright (c) 2026 Fritzing GmbH

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

#include "webserialport_wasm.h"

#include <QHash>
#include <QPointer>
#include <QTimer>

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <cstdlib>
#include <string>
#include <vector>

// The browser side is globalThis.FritzingWeb (docker/wasm/web/fritzing-web.js). Its promises
// report back through the fritzingWebSerial* functions bound at the end of this file. Those
// run inside browser callbacks, so they only queue work for the Qt event loop.

namespace {

struct Request {
	QPointer<QObject> context;
	std::function<void(const QString &)> progress;
	std::function<void(bool, const QString &)> done;
	std::function<void(const QString &)> portChosen;
};

QHash<int, Request> & requests() {
	static QHash<int, Request> requests;
	return requests;
}

int addRequest(const Request & request) {
	static int nextId = 1;
	requests().insert(nextId, request);
	return nextId++;
}

QHash<QString, QPointer<QSerialPort>> & openPorts() {
	static QHash<QString, QPointer<QSerialPort>> ports;
	return ports;
}

std::string toStd(const QString & s) {
	return s.toStdString();
}

}

EM_JS(int, fritzingWebAvailable, (), {
	return (typeof FritzingWeb !== 'undefined' && FritzingWeb.serialSupported()) ? 1 : 0;
});

EM_JS(void, fritzingWebInstallHandlers, (), {
	FritzingWeb.setHandlers({
		data: (name, bytes) => Module.fritzingWebSerialData(name, bytes),
		closed: (name, message) => Module.fritzingWebSerialClosed(name, message),
		portsChanged: () => {},
	});
});

EM_JS(char *, fritzingWebPortNames, (), {
	return stringToNewUTF8(FritzingWeb.portNames().join('\n'));
});

EM_JS(void, fritzingWebOpen, (const char * name, int baudRate, int dataBits, int stopBits, const char * parity, const char * flowControl), {
	const portName = UTF8ToString(name);
	FritzingWeb.open(portName, {
		baudRate: baudRate,
		dataBits: dataBits,
		stopBits: stopBits,
		parity: UTF8ToString(parity),
		flowControl: UTF8ToString(flowControl),
	}).then(() => Module.fritzingWebSerialOpened(portName, ''),
	        (e) => Module.fritzingWebSerialOpened(portName, String((e && e.message) || e)));
});

EM_JS(void, fritzingWebWrite, (const char * name, const char * data, int size), {
	const portName = UTF8ToString(name);
	FritzingWeb.write(portName, HEAPU8.slice(data, data + size))
		.catch((e) => Module.fritzingWebSerialClosed(portName, String((e && e.message) || e)));
});

EM_JS(void, fritzingWebClose, (const char * name), {
	FritzingWeb.close(UTF8ToString(name)).catch(() => {});
});

EM_JS(void, fritzingWebRequestPort, (int id), {
	FritzingWeb.requestPort().then((name) => Module.fritzingWebSerialPortChosen(id, name || ''),
	                               (e) => Module.fritzingWebSerialPortChosen(id, ''));
});

EM_JS(int, fritzingWebCanFlash, (const char * board), {
	return FritzingWeb.canFlash(UTF8ToString(board)) ? 1 : 0;
});

EM_JS(void, fritzingWebFlash, (int id, const char * name, const char * board, const char * hex), {
	FritzingWeb.flash(UTF8ToString(name), UTF8ToString(board), UTF8ToString(hex),
	                  (message) => Module.fritzingWebSerialProgress(id, message))
		.then(() => Module.fritzingWebSerialDone(id, true, ''),
		      (e) => Module.fritzingWebSerialDone(id, false, String((e && e.message) || e)));
});

EM_JS(char *, fritzingWebOrigin, (), {
	return stringToNewUTF8(location.origin);
});

static bool ensureBrowserSide()
{
	static int available = -1;
	if (available < 0) {
		available = fritzingWebAvailable();
		if (available) fritzingWebInstallHandlers();
	}
	return available == 1;
}

////////////////////////////////////////////////////////////////////////////////

const QString WebSerial::ChoosePortLocation("webserial:choose");

QSerialPortInfo::QSerialPortInfo(const QString & name, const QString & location)
	: m_name(name)
	, m_location(location.isEmpty() ? name : location)
{
}

QList<QSerialPortInfo> QSerialPortInfo::availablePorts()
{
	QList<QSerialPortInfo> ports;
	if (!ensureBrowserSide()) return ports;

	char * names = fritzingWebPortNames();
	for (const QString & name : QString::fromUtf8(names).split('\n', Qt::SkipEmptyParts)) {
		ports.append(QSerialPortInfo(name));
	}
	free(names);
	return ports;
}

QSerialPort::QSerialPort(QObject * parent) : QIODevice(parent)
{
}

QSerialPort::~QSerialPort()
{
	close();
}

void QSerialPort::setPortName(const QString & name)
{
	m_portName = name;
}

QString QSerialPort::portName() const
{
	return m_portName;
}

bool QSerialPort::open(OpenMode mode)
{
	if (isOpen()) return false;
	if (!ensureBrowserSide()) {
		setErrorString(tr("This browser does not support serial ports. Please use Chrome or Edge."));
		return false;
	}
	bool known = false;
	for (const QSerialPortInfo & info : QSerialPortInfo::availablePorts()) {
		known = known || info.portName() == m_portName;
	}
	if (!known) {
		setErrorString(tr("The port %1 is not available.").arg(m_portName));
		return false;
	}

	QIODevice::open(mode);
	m_buffer.clear();
	m_openedName = m_portName;
	openPorts().insert(m_openedName, this);
	// let the caller apply its settings first, as with a native port
	QTimer::singleShot(0, this, &QSerialPort::openInBrowser);
	return true;
}

void QSerialPort::openInBrowser()
{
	if (!isOpen()) return;

	const char * parity = "none";
	switch (m_parity) {
	case EvenParity: parity = "even"; break;
	case OddParity: parity = "odd"; break;
	case NoParity: break;
	default:
		fail(tr("The browser does not support mark or space parity."));
		return;
	}
	if (m_dataBits != Data7 && m_dataBits != Data8) {
		fail(tr("The browser only supports 7 or 8 data bits."));
		return;
	}
	if (m_stopBits == OneAndHalfStop) {
		fail(tr("The browser does not support 1.5 stop bits."));
		return;
	}
	if (m_flowControl == SoftwareControl) {
		fail(tr("The browser does not support XON/XOFF flow control."));
		return;
	}
	fritzingWebOpen(toStd(m_openedName).c_str(), m_baudRate, m_dataBits, m_stopBits == TwoStop ? 2 : 1,
	                parity, m_flowControl == HardwareControl ? "hardware" : "none");
}

void QSerialPort::close()
{
	if (!isOpen()) return;

	openPorts().remove(m_openedName);
	fritzingWebClose(toStd(m_openedName).c_str());
	QIODevice::close();
}

void QSerialPort::fail(const QString & message)
{
	setErrorString(message);
	close();
	Q_EMIT error(ResourceError);
	Q_EMIT errorOccurred(ResourceError);
}

void QSerialPort::browserOpened(const QString & errorMessage)
{
	if (!errorMessage.isEmpty() && isOpen()) {
		fail(errorMessage);
	}
}

void QSerialPort::browserData(const QByteArray & data)
{
	if (!isOpen()) return;

	m_buffer.append(data);
	if (!m_readyReadPending) {
		m_readyReadPending = true;
		QTimer::singleShot(0, this, [this]() {
			m_readyReadPending = false;
			if (!m_buffer.isEmpty()) Q_EMIT readyRead();
		});
	}
}

void QSerialPort::browserClosed(const QString & message)
{
	if (isOpen()) {
		fail(message);
	}
}

bool QSerialPort::setBaudRate(qint32 baudRate)
{
	m_baudRate = baudRate;
	return true;
}

bool QSerialPort::setDataBits(DataBits dataBits)
{
	m_dataBits = dataBits;
	return true;
}

bool QSerialPort::setParity(Parity parity)
{
	m_parity = parity;
	return true;
}

bool QSerialPort::setStopBits(StopBits stopBits)
{
	m_stopBits = stopBits;
	return true;
}

bool QSerialPort::setFlowControl(FlowControl flowControl)
{
	m_flowControl = flowControl;
	return true;
}

bool QSerialPort::isSequential() const
{
	return true;
}

qint64 QSerialPort::bytesAvailable() const
{
	return m_buffer.size() + QIODevice::bytesAvailable();
}

qint64 QSerialPort::readData(char * data, qint64 maxSize)
{
	qint64 count = qMin<qint64>(maxSize, m_buffer.size());
	memcpy(data, m_buffer.constData(), count);
	m_buffer.remove(0, count);
	return count;
}

qint64 QSerialPort::writeData(const char * data, qint64 maxSize)
{
	fritzingWebWrite(toStd(m_openedName).c_str(), data, static_cast<int>(maxSize));
	return maxSize;
}

////////////////////////////////////////////////////////////////////////////////

bool WebSerial::supported()
{
	return ensureBrowserSide();
}

QSerialPortInfo WebSerial::choosePortEntry()
{
	return QSerialPortInfo(QObject::tr("Choose a port..."), ChoosePortLocation);
}

void WebSerial::requestPort(QObject * context, const std::function<void(const QString &)> & done)
{
	if (!ensureBrowserSide()) {
		QTimer::singleShot(0, context, [done]() { done(QString()); });
		return;
	}
	Request request;
	request.context = context;
	request.portChosen = done;
	fritzingWebRequestPort(addRequest(request));
}

bool WebSerial::canFlash(const QString & board)
{
	return ensureBrowserSide() && fritzingWebCanFlash(toStd(board).c_str());
}

void WebSerial::flash(QObject * context, const QString & portName, const QString & board, const QString & hex,
                      const std::function<void(const QString &)> & progress,
                      const std::function<void(bool, const QString &)> & done)
{
	if (!ensureBrowserSide()) {
		QTimer::singleShot(0, context, [done]() {
			done(false, QObject::tr("This browser does not support serial ports. Please use Chrome or Edge."));
		});
		return;
	}
	// the serial monitor would hold the port
	if (QPointer<QSerialPort> port = openPorts().value(portName)) {
		port->close();
	}
	Request request;
	request.context = context;
	request.progress = progress;
	request.done = done;
	fritzingWebFlash(addRequest(request), toStd(portName).c_str(), toStd(board).c_str(), toStd(hex).c_str());
}

QString WebSerial::compileServiceUrl()
{
	char * origin = fritzingWebOrigin();
	QString url = QString::fromUtf8(origin) + "/api/compile";
	free(origin);
	return url;
}

////////////////////////////////////////////////////////////////////////////////
// called by fritzing-web.js

static void onSerialOpened(std::string name, std::string errorMessage)
{
	QPointer<QSerialPort> port = openPorts().value(QString::fromStdString(name));
	if (port) {
		QString message = QString::fromStdString(errorMessage);
		QTimer::singleShot(0, port, [port, message]() { if (port) port->browserOpened(message); });
	}
}

static void onSerialData(std::string name, emscripten::val bytes)
{
	QPointer<QSerialPort> port = openPorts().value(QString::fromStdString(name));
	if (port) {
		std::vector<uint8_t> data = emscripten::convertJSArrayToNumberVector<uint8_t>(bytes);
		port->browserData(QByteArray(reinterpret_cast<const char *>(data.data()), static_cast<qsizetype>(data.size())));
	}
}

static void onSerialClosed(std::string name, std::string message)
{
	QPointer<QSerialPort> port = openPorts().value(QString::fromStdString(name));
	if (port) {
		QString text = QString::fromStdString(message);
		QTimer::singleShot(0, port, [port, text]() { if (port) port->browserClosed(text); });
	}
}

static void onPortChosen(int id, std::string name)
{
	Request request = requests().take(id);
	if (request.context && request.portChosen) {
		QString portName = QString::fromStdString(name);
		auto portChosen = request.portChosen;
		QTimer::singleShot(0, request.context, [portChosen, portName]() { portChosen(portName); });
	}
}

static void onFlashProgress(int id, std::string message)
{
	Request request = requests().value(id);
	if (request.context && request.progress) {
		QString text = QString::fromStdString(message);
		auto progress = request.progress;
		QTimer::singleShot(0, request.context, [progress, text]() { progress(text); });
	}
}

static void onFlashDone(int id, bool ok, std::string message)
{
	Request request = requests().take(id);
	if (request.context && request.done) {
		QString text = QString::fromStdString(message);
		auto done = request.done;
		QTimer::singleShot(0, request.context, [done, ok, text]() { done(ok, text); });
	}
}

EMSCRIPTEN_BINDINGS(fritzing_webserial) {
	emscripten::function("fritzingWebSerialOpened", &onSerialOpened);
	emscripten::function("fritzingWebSerialData", &onSerialData);
	emscripten::function("fritzingWebSerialClosed", &onSerialClosed);
	emscripten::function("fritzingWebSerialPortChosen", &onPortChosen);
	emscripten::function("fritzingWebSerialProgress", &onFlashProgress);
	emscripten::function("fritzingWebSerialDone", &onFlashDone);
}
