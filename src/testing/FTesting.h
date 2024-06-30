/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2022 Fritzing GmbH

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

#ifndef FTESTING_H
#define FTESTING_H

#include <memory>

#pragma once

#if __has_include(<optional>)

#include <optional>
namespace stdx {
	using std::optional;
	using std::nullopt_t;
	using std::in_place_t;
	using std::bad_optional_access;
	using std::nullopt;
	using std::in_place;
	using std::make_optional;
}

#elif __has_include(<experimental/optional>)

#include <experimental/optional>
namespace stdx {
	using std::experimental::optional;
	using std::experimental::nullopt_t;
	using std::experimental::in_place_t;
	using std::experimental::bad_optional_access;
	using std::experimental::nullopt;
	using std::experimental::in_place;
	using std::experimental::make_optional;
}

#else
	#error "an implementation of optional is required!"
#endif

#include "FProbe.h"
#include "FTestingServer.h"


#include <QString>
#include <QVariant>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMutex>
#include <QThread>

class FTestingServerThread : public QThread
{
	Q_OBJECT

public:
	FTestingServerThread(qintptr socketDescriptor, QObject *parent);

	void run();
	void setDone();

Q_SIGNALS:
	void error(QTcpSocket::SocketError socketError);

protected:
	void writeResponse(QTcpSocket *, int code, const QString & codeString, const QString & mimeType, const QString & message);

protected:
	int m_socketDescriptor = 0;
	bool m_done = false;

protected:
	static QMutex m_busy;

};

class FTesting : public QObject {
    Q_OBJECT

private:
		FTesting();
public:
		~FTesting();

	static std::shared_ptr<FTesting> getInstance();

	void init();

	bool enabled();

	void addProbe(FProbe * probe);

	void removeProbe(std::string name);

	stdx::optional<QVariant> readProbe(std::string name);

	void writeProbe(std::string name, QVariant param);

public Q_SLOTS:
	void newConnection(qintptr socketDescriptor);

protected:
	void initServer();
	int m_portNumber = 17999;
	FTestingServer * m_server = nullptr;

private:
	bool m_initialized = false;
	std::map<std::string, FProbe *> m_probeMap;
};

#endif
