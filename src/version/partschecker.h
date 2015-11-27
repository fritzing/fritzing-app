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

#ifndef PARTSCHECKER_H
#define PARTSCHECKER_H

#include <QObject>
#include <QNetworkAccessManager>

struct Commit {
    QString sha;
    QString date;
    QStringList fileNamesToDo;
    QStringList fileNamesDone;

    bool lessThan(const Commit & a, const Commit & b) {
        return a.date < b.date;
    }
};

class PartsChecker : public QObject
{
    Q_OBJECT
public:
    explicit PartsChecker(QObject *parent = 0);

    void start();
    void stop();

protected:
    enum State {
        READING_COMMITS,
        READING_ONE_COMMIT,
        READING_FILE,
    };

    QMutex m_managersLock;
    QMutex m_listsLock;
    PartsChecker::State m_state;
    QList<Commit> m_commitsToDo;
    QList<Commit> m_commitsDone;
    Commit m_currentCommit;
    bool m_keepGoing;
    QList<QNetworkAccessManager *> m_networkAccessManagers;


signals:
    void httpError(QString);
    void jsonError(QString);

protected slots:
    void requestFinished(QNetworkReply * networkReply);
    void readSha();
    void readFile();

protected:
    void startState(PartsChecker::State state, const QString & urlString);
    QString lastDate();
    void readCommits(bool keepGoing, const QJsonDocument & jsonDocument);
    void readCommit(bool keepGoing, const QJsonDocument & jsonDocument);
    void readFile(bool keepGoing, const QJsonDocument & jsonDocument);

};

#endif // PARTSCHECKER_H
