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

// Serial ports for the WebAssembly build, through the browser's Web Serial API
// (docker/wasm/web/fritzing-web.js). QtSerialPort is not available for WebAssembly, so
// this header provides the parts of QSerialPort and QSerialPortInfo that the program
// window and the serial monitor use; src/program/wasm/QtSerialPort/ maps the QtSerialPort
// includes here. Only compiled by pri/wasm.pri.

#ifndef WEBSERIALPORT_WASM_H
#define WEBSERIALPORT_WASM_H

#include <QByteArray>
#include <QIODevice>
#include <QList>
#include <QObject>
#include <QString>

#include <functional>

class QSerialPortInfo
{
public:
	QSerialPortInfo() = default;
	explicit QSerialPortInfo(const QString & name, const QString & location = QString());

	// the ports the user has allowed this site to use
	static QList<QSerialPortInfo> availablePorts();

	QString portName() const { return m_name; }
	QString systemLocation() const { return m_location; }
	QString description() const { return QString(); }
	QString manufacturer() const { return QString(); }
	quint16 vendorIdentifier() const { return 0; }
	quint16 productIdentifier() const { return 0; }

private:
	QString m_name;
	QString m_location;
};

class QSerialPort : public QIODevice
{
	Q_OBJECT

public:
	enum BaudRate {
		Baud1200 = 1200, Baud2400 = 2400, Baud4800 = 4800, Baud9600 = 9600,
		Baud19200 = 19200, Baud38400 = 38400, Baud57600 = 57600, Baud115200 = 115200
	};
	enum DataBits { Data5 = 5, Data6 = 6, Data7 = 7, Data8 = 8 };
	enum Parity { NoParity = 0, EvenParity = 2, OddParity = 3, SpaceParity = 4, MarkParity = 5 };
	enum StopBits { OneStop = 1, OneAndHalfStop = 3, TwoStop = 2 };
	enum FlowControl { NoFlowControl, HardwareControl, SoftwareControl };
	enum SerialPortError { NoError, DeviceNotFoundError, PermissionError, OpenError, NotOpenError = 13,
	                       WriteError = 7, ReadError = 8, ResourceError = 9, UnsupportedOperationError = 10,
	                       UnknownError = 11, TimeoutError = 12 };
	Q_ENUM(SerialPortError)

	explicit QSerialPort(QObject * parent = nullptr);
	~QSerialPort() override;

	void setPortName(const QString & name);
	QString portName() const;

	// The browser opens the port asynchronously: open() succeeds right away, and a failure
	// is reported later through error(ResourceError). Settings made right after open() apply.
	bool open(OpenMode mode) override;
	void close() override;

	bool setBaudRate(qint32 baudRate);
	bool setDataBits(DataBits dataBits);
	bool setParity(Parity parity);
	bool setStopBits(StopBits stopBits);
	bool setFlowControl(FlowControl flowControl);

	bool isSequential() const override;
	qint64 bytesAvailable() const override;

	// called from the browser
	void browserOpened(const QString & errorMessage);
	void browserData(const QByteArray & data);
	void browserClosed(const QString & message);

Q_SIGNALS:
	void error(QSerialPort::SerialPortError serialPortError);
	void errorOccurred(QSerialPort::SerialPortError serialPortError);

protected:
	qint64 readData(char * data, qint64 maxSize) override;
	qint64 writeData(const char * data, qint64 maxSize) override;

private:
	void fail(const QString & message);
	void openInBrowser();

private:
	QString m_portName;
	QString m_openedName;
	QByteArray m_buffer;
	qint32 m_baudRate = 9600;
	DataBits m_dataBits = Data8;
	Parity m_parity = NoParity;
	StopBits m_stopBits = OneStop;
	FlowControl m_flowControl = NoFlowControl;
	bool m_readyReadPending = false;
};

namespace WebSerial {

// systemLocation() of the extra port list entry that asks the browser for another port
extern const QString ChoosePortLocation;

bool supported();
QSerialPortInfo choosePortEntry();

// Opens the browser's port chooser (call while handling a click). done() receives the new
// port name, or an empty string when the user closed the chooser.
void requestPort(QObject * context, const std::function<void(const QString &)> & done);

// whether flash() supports the board (named as in PlatformArduino)
bool canFlash(const QString & board);

// Uploads a compiled sketch (Intel HEX) to the board on the port
void flash(QObject * context, const QString & portName, const QString & board, const QString & hex,
           const std::function<void(const QString &)> & progress,
           const std::function<void(bool, const QString &)> & done);

// URL of the Arduino compile service (docker/wasm/compile-service, behind nginx)
QString compileServiceUrl();

}

#endif
