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

#include "servicelistfetcher.h"
#include "../debugdialog.h"

#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

ServiceListFetcher::ServiceListFetcher(QObject* parent)
	: QObject(parent), m_manager(new QNetworkAccessManager(this)) {}

void ServiceListFetcher::fetchServices() {
	QUrl url("https://service.fritzing.org/fab/list");
	QNetworkRequest request(url);
	auto reply = m_manager->get(request);
	connect(reply, &QNetworkReply::finished, this, &ServiceListFetcher::onRequestFinished);
}

void ServiceListFetcher::onRequestFinished() {
	auto *reply = qobject_cast<QNetworkReply*>(sender());
	if (!reply) return;

	QStringList services;

	if (reply->error()) {
		DebugDialog::debug(QString("Error fetching services: %1").arg(reply->errorString()));
	} else {
		auto jsonData = reply->readAll();
		auto doc = QJsonDocument::fromJson(jsonData);
		if (doc.isArray()) {
			for (const auto& value : doc.array()) {
				auto obj = value.toObject();
				QString name = obj.value("name").toString();
				services.append(name);
			}
		}
	}
	reply->deleteLater();
	emit servicesFetched(services);
}
