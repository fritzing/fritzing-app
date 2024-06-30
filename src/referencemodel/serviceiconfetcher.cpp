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

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QTimer>
#include <QEventLoop>

#include "serviceiconfetcher.h"
#include "../debugdialog.h"

ServiceIconFetcher* ServiceIconFetcher::m_instance = nullptr;

ServiceIconFetcher* ServiceIconFetcher::instance() {
	if (!m_instance) {
		m_instance = new ServiceIconFetcher();
	}
	return m_instance;
}

ServiceIconFetcher::ServiceIconFetcher(QObject* parent)
	: QObject(parent)
{
	m_manager = new QNetworkAccessManager(this);
}

void ServiceIconFetcher::fetchIcons() {
	m_pendingDownloads = 1;
	QUrl url("https://service.fritzing.org/fab/list");
	QNetworkRequest request(url);
	auto reply = m_manager->get(request);
	connect(reply, &QNetworkReply::finished, this, &ServiceIconFetcher::onRequestFinished);
}

void ServiceIconFetcher::onRequestFinished() {
	auto *reply = qobject_cast<QNetworkReply*>(sender());
	if (!reply) return;

	if (reply->error()) {
		DebugDialog::debug(QString("Error fetching services: %1").arg(reply->errorString()));
	} else {
		auto jsonData = reply->readAll();
		auto doc = QJsonDocument::fromJson(jsonData);
		if (doc.isArray()) {
			auto array = doc.array();
			for (const auto& value : array) {
				auto obj = value.toObject();
				int id = obj.value("id").toInt();
				QString name = obj.value("name").toString();
				QString iconUrl = QString("https://service.fritzing.org/fab/icons/%1").arg(id);
				fetchIcon(name, iconUrl);
			}
		}
	}
	reply->deleteLater();

	// Decrement for the list fetch operation and check if it's time to emit iconsFetched
	if (--m_pendingDownloads == 0) {
		emit iconsFetched();
	}
}

QMap<QString, QPixmap> ServiceIconFetcher::getIcons() const {
	return m_icons;
}

void ServiceIconFetcher::fetchIcon(const QString& name, const QString& iconUrl) {
	m_pendingDownloads++;
	QUrl url(iconUrl);
	QNetworkRequest request(url);
	auto reply = m_manager->get(request);
	connect(reply, &QNetworkReply::finished, this, [this, reply, name]() {
		reply->deleteLater();
		if (reply->error()) {
			DebugDialog::debug(QString("Error fetching icon for %1 : %2").arg(name, reply->errorString()));
		} else {
			QPixmap pixmap;
			if (pixmap.loadFromData(reply->readAll())) {
				m_icons[name] = pixmap;
			} else {
				DebugDialog::debug(QString("Failed to load pixmap for %1").arg(name));
			}
		}

		if (--m_pendingDownloads == 0) {
			emit iconsFetched();
		}
	});
}

void ServiceIconFetcher::waitForIcons() {
	if (m_pendingDownloads > 0) {
		QEventLoop loop;
		QTimer timer;
		timer.setSingleShot(true);

		connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
		// Quit the loop early if all icons are fetched
		connect(this, &ServiceIconFetcher::iconsFetched, &loop, &QEventLoop::quit);

		timer.start(2000);
		loop.exec();

		if (timer.isActive()) {
			// The loop exited because all icons were fetched
			timer.stop();
		} else {
			// The loop exited due to timeout
			DebugDialog::debug("Timeout reached while waiting for icons to be fetched.");
		}
	}
}
