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

********************************************************************/

#include "platform.h"
#include "syntaxer.h"

#include <QDir>

#include <../../utils/folderutils.h>

Platform::Platform(const QString &name) : QObject()
{
    m_name = name;
    m_syntaxer = loadSyntaxer();
}

Platform::~Platform()
{
    // nothing to do
}

QString Platform::getName() const
{
    return m_name;
}

void Platform::upload(QString port, QString board, QString fileLocation)
{
    // stub
}

Syntaxer * Platform::getSyntaxer() {
    return m_syntaxer;
}

Syntaxer * Platform::loadSyntaxer()
{
    Syntaxer * syntaxer = new Syntaxer();
    QDir dir(FolderUtils::getApplicationSubFolderPath("translations"));
    dir.cd("syntax");
    QStringList nameFilters;
    nameFilters << "*.xml";
    QFileInfoList list = dir.entryInfoList(nameFilters, QDir::Files | QDir::NoSymLinks);
    foreach (QFileInfo fileInfo, list) {
        if (fileInfo.completeBaseName().compare(getName(), Qt::CaseInsensitive) == 0) {
            syntaxer->loadSyntax(fileInfo.absoluteFilePath());
            break;
        }
    }
    return syntaxer;
}

QString Platform::getCommandLocation() const
{
    return m_commandLocation;
}

void Platform::setCommandLocation(const QString &commandLocation)
{
    m_commandLocation = commandLocation;
}

QStringList Platform::getExtensions() const
{
    return m_extensions;
}

void Platform::setExtensions(const QStringList &suffixes)
{
    m_extensions = suffixes;
}

QMap<QString, QString> Platform::getBoards() const
{
    return m_boards;
}

void Platform::setBoards(const QMap<QString, QString> &boards)
{
    m_boards = boards;
}

QUrl Platform::getReferenceUrl() const
{
    return m_referenceUrl;
}

void Platform::setReferenceUrl(const QUrl &referenceUrl)
{
    m_referenceUrl = referenceUrl;
}

bool Platform::canProgram() const
{
    return m_canProgram;
}

void Platform::setCanProgram(bool canProgram)
{
    m_canProgram = canProgram;
}

QUrl Platform::getDownloadUrl() const
{
    return m_downloadUrl;
}

void Platform::setDownloadUrl(const QUrl &downloadUrl)
{
    m_downloadUrl = downloadUrl;
}

QString Platform::getMinVersion() const
{
    return m_minVersion;
}

void Platform::setMinVersion(const QString &minVersion)
{
    m_minVersion = minVersion;
}



