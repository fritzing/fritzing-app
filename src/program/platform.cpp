/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2019 Fritzing

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

#include "../utils/folderutils.h"

#include <QDir>
#include <QSettings>

Platform::Platform(const QString &name) 
	: QObject(),
	m_name(name),
	m_commandLocation(QSettings().value(QString("programwindow/programmer.%1").  arg(getName())).toString()),
	m_canProgram(false),
	m_extensions(),
	m_boards(),
	m_defaultBoardName(),
	m_referenceUrl(),
	m_ideName(),
	m_downloadUrl(),
	m_minVersion(),
	m_syntaxer(new Syntaxer())
{
	initSyntaxer();
}

void Platform::upload(QWidget *, const QString &, const QString &, const QString &)
{
	// stub
}

void Platform::initSyntaxer()
{
	QDir dir(FolderUtils::getApplicationSubFolderPath("translations"));
	dir.cd("syntax");
	QStringList nameFilters;
	nameFilters << "*.xml";
	QFileInfoList list = dir.entryInfoList(nameFilters, QDir::Files | QDir::NoSymLinks);
	Q_FOREACH (QFileInfo fileInfo, list) {
		if (fileInfo.completeBaseName().compare(getName(), Qt::CaseInsensitive) == 0) {
			m_syntaxer->loadSyntax(fileInfo.absoluteFilePath());
			break;
		}
	}
}


void Platform::setCommandLocation(const QString &commandLocation)
{
	m_commandLocation = commandLocation;
	QSettings settings;
	settings.setValue(QString("programwindow/programmer.%1").arg(getName()), commandLocation);

	Q_EMIT commandLocationChanged();
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
QString Platform::getDefaultBoardName() const
{
	return m_defaultBoardName;
}

void Platform::setDefaultBoardName(const QString &defaultBoardName)
{
	m_defaultBoardName = defaultBoardName;
}

QString Platform::getIdeName() const
{
	return m_ideName;
}

void Platform::setIdeName(const QString &ideName)
{
	m_ideName = ideName;
}
