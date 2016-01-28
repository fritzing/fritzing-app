/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2016 Fritzing

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

#include "syntaxer.h"

#include <QFile>
#include <QMap>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QTextEdit>
#include <QUrl>

#ifndef PLATFORM_H
#define PLATFORM_H

class Platform : public QObject
{
Q_OBJECT


public:
    Platform(const QString &name);
    ~Platform();

    virtual void upload(QWidget *source, const QString &port, const QString &board, const QString &fileLocation);
    Syntaxer *getSyntaxer();

    QString getName() const;
    QString getCommandLocation() const;
    void setCommandLocation(const QString &commandLocation);
    QStringList getExtensions() const;
    void setExtensions(const QStringList &suffixes);
    QMap<QString, QString> getBoards() const;
    void setBoards(const QMap<QString, QString> &boards);
    QUrl getReferenceUrl() const;
    void setReferenceUrl(const QUrl &referenceUrl);
    bool canProgram() const;
    void setCanProgram(bool canProgram);
    QUrl getDownloadUrl() const;
    void setDownloadUrl(const QUrl &downloadUrl);
    QString getMinVersion() const;
    void setMinVersion(const QString &minVersion);
    QString getDefaultBoardName() const;
    void setDefaultBoardName(const QString &defaultBoardName);
    QString getIdeName() const;
    void setIdeName(const QString &ideName);

signals:
    void commandLocationChanged();

protected:
    QString m_name;
    QString m_commandLocation;
    bool m_canProgram;
    QStringList m_extensions;
    QMap<QString, QString> m_boards;
    QString m_defaultBoardName;
    QUrl m_referenceUrl;
    QString m_ideName;
    QUrl m_downloadUrl;
    QString m_minVersion;

private:
    void initSyntaxer();
    void initCommandLocation();

private:
    QPointer<Syntaxer> m_syntaxer;

};

#endif // PLATFORM_H
