/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2026 Fritzing

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

// Stand-in for QSerialPortInfo on platforms without the QtSerialPort module
// (the WebAssembly build). It never reports any ports, so the port menus stay empty.

#ifndef SERIALPORTINFOSTUB_H_
#define SERIALPORTINFOSTUB_H_

#include <QList>
#include <QString>

class QSerialPortInfo
{
public:
	static QList<QSerialPortInfo> availablePorts() {
		return {};
	}
	QString portName() const {
		return {};
	}
	QString systemLocation() const {
		return {};
	}
};

#endif
