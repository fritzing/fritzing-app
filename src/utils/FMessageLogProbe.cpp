/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2023 Fritzing GmbH

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

#include "FMessageLogProbe.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>


FMessageLogProbe::FMessageLogProbe() : FProbe("FMessageLog") {}

QVariant FMessageLogProbe::read() {
    QList<QPair<QString, QString>> messages = FMessageBox::getLoggedMessages();
    QJsonArray jsonMessages;

    for (const QPair<QString, QString> &message : messages) {
	QJsonObject messageObj;
	messageObj["title"] = message.first;
	messageObj["text"] = message.second;
	jsonMessages.append(messageObj);
    }

    QJsonDocument doc(jsonMessages);
    QString jsonString = doc.toJson(QJsonDocument::Indented);
    return jsonString;
}
