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

#ifndef SERVICEICONFETCHER_H
#define SERVICEICONFETCHER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QMap>
#include <QPixmap>

class ServiceIconFetcher : public QObject {
	Q_OBJECT
public:
	static ServiceIconFetcher* instance();
	void fetchIcons();
	QMap<QString, QPixmap> getIcons() const;
	void waitForIcons();

signals:
	void iconsFetched();

private slots:
	void onRequestFinished();
	void fetchIcon(const QString& name, const QString& iconUrl);

private:
	explicit ServiceIconFetcher(QObject* parent = nullptr);
	static ServiceIconFetcher* m_instance;
	QNetworkAccessManager* m_manager;
	QMap<QString, QPixmap> m_icons;
	int m_pendingDownloads = 0;
};
#endif // SERVICEICONFETCHER_H
