/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2024 Fritzing GmbH

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

#ifndef SERVICELISTFETCHER_H
#define SERVICELISTFETCHER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QString>

class ServiceListFetcher : public QObject {
	Q_OBJECT
public:
	explicit ServiceListFetcher(QObject* parent = nullptr);
	void fetchServices();

signals:
	void servicesFetched(const QStringList& services);

private slots:
	void onRequestFinished();

private:
	QNetworkAccessManager* m_manager;
};

#endif // SERVICELISTFETCHER_H
