/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2014 Fachhochschule Potsdam - http://fh-potsdam.de

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

********************************************************************

$Revision: 6904 $:
$Author: irascibl@gmail.com $:
$Date: 2013-02-26 16:26:03 +0100 (Di, 26. Feb 2013) $

********************************************************************/

#include "partschecker.h"
#include "version.h"
#include "../debugdialog.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QSettings>
#include <QTimer>
#include <QDebug>

static const char * s_KeepGoingName = "keepGoing";
static const QString s_lastDateName("lastPartsDateChecked");
static const int s_delayTime = 50;  // milliseconds

PartsChecker::PartsChecker(QObject *parent) : QObject(parent)
{

}

void PartsChecker::start() {
    stop();

    DebugDialog::debug("http check new parts");

    QString urlString = QString("https://api.github.com/repos/fritzing/fritzing-parts/commits?since=%1").arg(lastDate());
    startState(READING_COMMITS, urlString);
}

void PartsChecker::stop() {
    m_listsLock.lock();
    m_shas.clear();
    m_fileNames.clear();
    m_listsLock.unlock();

    // don't need to delete here since it's done in networkFinished()
    m_managersLock.lock();
    foreach (QNetworkAccessManager * networkAccessManager, m_networkAccessManagers) {
        networkAccessManager->setProperty(s_KeepGoingName, false);
    }
    m_networkAccessManagers.clear();
    m_managersLock.unlock();
}

void PartsChecker::requestFinished(QNetworkReply * networkReply) {
    int responseCode = networkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (responseCode == 200) {
        QByteArray byteArray = networkReply->readAll();
        qDebug() << "json " << byteArray.size();
        QJsonParseError error;
        QJsonDocument jsonDocument = QJsonDocument::fromJson(byteArray, &error);
        if (error.error != QJsonParseError::NoError) {
            DebugDialog::debug(QString("parts check json error %1").arg(networkReply->errorString()));
            emit jsonError(error.errorString());
        }
        else {
            bool keepGoing = networkReply->manager()->property(s_KeepGoingName).toBool();
            switch (m_state) {
            case READING_COMMITS:
                readCommits(keepGoing, jsonDocument);
                break;
            case READING_ONE_COMMIT:
                readCommit(keepGoing, jsonDocument);
                break;
            case READING_FILE:
                readFile(keepGoing, jsonDocument);
                break;
            }
        }
    }
    else {
        DebugDialog::debug(QString("parts check http error %1").arg(networkReply->errorString()));
        emit httpError(networkReply->errorString());
    }

    QMutexLocker locker(&m_managersLock);
    m_networkAccessManagers.removeOne(networkReply->manager());
    networkReply->manager()->deleteLater();
    networkReply->deleteLater();
}

void PartsChecker::startState(PartsChecker::State state, const QString & urlString) {
    m_state = state;
    QUrl url(urlString);

    QNetworkAccessManager * networkAccessManager = new QNetworkAccessManager(this);
    networkAccessManager->setProperty(s_KeepGoingName, true);
    connect(networkAccessManager, SIGNAL(finished(QNetworkReply *)), this, SLOT(requestFinished(QNetworkReply *)));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Fritzing");
    request.setRawHeader("Authorization", "token 978f2558308b4c40c539e53110a115040d79abdf");  // c9c112967704dcb639d7603dc29395dc0152d6a5

    m_managersLock.lock();
    m_networkAccessManagers.append(networkAccessManager);
    m_managersLock.unlock();

    networkAccessManager->get(request);
}

void PartsChecker::readSha() {
    QString sha;
    int fileNameCount = 0;
    int shaCount = 0;
    m_listsLock.lock();
    if (m_shas.count() > 0) {
        sha = m_shas.values().first();
        m_shas.remove(sha);
    }
    shaCount = m_shas.count();
    fileNameCount = m_fileNames.count();
    m_listsLock.unlock();
    if (sha.isEmpty()) {
        if (fileNameCount > 0) {
            QTimer::singleShot(s_delayTime, Qt::CoarseTimer, this, SLOT(readFile()));
        }
        return;
    }

    DebugDialog::debug("reading sha " + sha + " files:" + QString::number(fileNameCount) + " shas:" + QString::number(shaCount));
    QString urlString = QString("https://api.github.com/repos/fritzing/fritzing-parts/commits/%1").arg(sha);
    startState(READING_ONE_COMMIT, urlString);
}

void PartsChecker::readFile() {
    QString fileName;

    m_listsLock.lock();
    if (m_fileNames.count() > 0) {
        fileName = m_fileNames.values().first();
        m_fileNames.remove(fileName);
    }
    m_listsLock.unlock();

    if (!fileName.isEmpty()) {
        DebugDialog::debug("reading file " + fileName);
        QString urlString = QString("https://api.github.com/repos/fritzing/fritzing-parts/contents/%1").arg(fileName);
        startState(READING_FILE, urlString);
    }
}

QString PartsChecker::lastDate() {
    QSettings settings;
    QString lastDate = settings.value(s_lastDateName).toString();
    if (lastDate.isEmpty()) {
        lastDate = Version::fullDate();
        settings.setValue(s_lastDateName, lastDate);
    }
    return lastDate;
}


void PartsChecker::readCommits(bool keepGoing, const QJsonDocument & jsonDocument) {
    if (!jsonDocument.isArray()) {
        emit jsonError("Commit array expected");
        return;
    }

    int shaCount = 0;
    m_listsLock.lock();
    foreach (const QJsonValue & jsonValue, jsonDocument.array()) {
        QJsonObject jsonObject = jsonValue.toObject();
        if (!jsonObject.isEmpty()) {
            QString sha = jsonObject["sha"].toString();
            if (!sha.isEmpty()) {
                m_shas.insert(sha);
            }
        }
    }
    shaCount = m_shas.count();
    m_listsLock.unlock();
    if (keepGoing && shaCount > 0) {
        // give github some delay
        QTimer::singleShot(s_delayTime, Qt::CoarseTimer, this, SLOT(readSha()));
    }
}

void PartsChecker::readCommit(bool keepGoing, const QJsonDocument & jsonDocument)
{
    if (!jsonDocument.isObject()) {
        emit jsonError("No commit array expected");
        return;
    }

    QJsonValue filesValue = jsonDocument.object()["files"];
    if (!filesValue.isArray()) {
        emit jsonError("Files array expected");
        return;
    }

    m_listsLock.lock();
    foreach (const QJsonValue & jsonValue, filesValue.toArray()) {
        QJsonObject jsonObject = jsonValue.toObject();
        if (!jsonObject.isEmpty()) {
            QString fileName = jsonObject["filename"].toString();
            QString status = jsonObject["status"].toString();
            if (status == "modified" || status == "added") {
                if (fileName.endsWith(".svg") || fileName.endsWith("fzp") || fileName.endsWith("fzb")) {
                    m_fileNames.insert(fileName);
                }
            }
            else if (status == "renamed") {
                if (fileName.endsWith(".svg") || fileName.endsWith("fzp") || fileName.endsWith("fzb")) {
                    m_fileNames.insert(fileName);
                }
            }
            else {
                qDebug() << "status" << status;
                QJsonDocument tempDocument(jsonObject);
                qDebug() << tempDocument.toJson();
            }
        }
    }
    m_listsLock.unlock();
    if (keepGoing) {
        QTimer::singleShot(s_delayTime, Qt::CoarseTimer, this, SLOT(readSha()));
    }
}

void PartsChecker::readFile(bool keepGoing, const QJsonDocument & jsonDocument)
{
    if (!jsonDocument.isObject()) {
        emit jsonError("No file array expected");
    }

    QJsonValue content = jsonDocument.object()["content"];
    QJsonValue path = jsonDocument.object()["path"];
    if (!content.isString() && !path.isString()) {
        emit jsonError("Unexpected contents");
    }
    else {
        QString contentString = QByteArray::fromBase64(content.toString().toUtf8());
        QString pathString = path.toString();
        qDebug() << pathString << contentString;
        if (keepGoing) {
            QTimer::singleShot(s_delayTime, Qt::CoarseTimer, this, SLOT(readFile()));
        }
    }
}
