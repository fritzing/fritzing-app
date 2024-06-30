/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2019 Fritzing
Copyright (c) 2020 Fritzing GmbH

Fritzing is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Fritzing is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty ofswap
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************/

#include "version.h"

#include <QString>
#include <QStringList>
#include <QSettings>
#include <QUrl>
#include <QSysInfo>
#include <QRegularExpression>

#include "../debugdialog.h"
#include "../utils/textutils.h"


QString Version::m_majorVersion("1");
QString Version::m_minorVersion("0");
QString Version::m_minorSubVersion("3");
QString Version::m_modifier("b");
QString Version::m_gitVersion(GIT_VERSION);
QString Version::m_gitDate(GIT_DATE);  // want standard ISO form
QString Version::m_date;
QString Version::m_shortDate;
QString Version::m_versionString;
QString Version::m_year;
QStringList Version::m_modifiers;

Version * Version::m_singleton = new Version();

QString Version::FirstVersionWithDetachedUserData = "0.3.1b.05.26.3016";

Version::Version() {
	if (m_modifiers.count() == 0) {
		m_modifiers << "a" << "b" << "rc" << "";
	}

	QStringList strings;
	strings = m_gitDate.split("T", Qt::SkipEmptyParts);
	if (strings.size() >= 2) {
		m_date = strings[0];
		strings = m_date.split("-", Qt::SkipEmptyParts);
		if (strings.size() >= 3) {
			m_shortDate = strings[1] + "." + strings[2];
			m_year = strings[0];
		}
	}

    m_versionString = QString("%1.%2.%3%4.%5.%6").arg(m_majorVersion).arg(m_minorVersion).arg(m_minorSubVersion).arg(m_modifier).arg(m_date).arg(m_gitVersion);
}

const QString & Version::majorVersion() {
	return m_majorVersion;
}

const QString & Version::minorVersion() {
	return m_minorVersion;
}

const QString & Version::minorSubVersion() {
	return m_minorSubVersion;
}

const QString & Version::modifier() {
	return m_modifier;
}

const QString & Version::versionString() {
	return m_versionString;
}

const QString & Version::gitVersion() {
	return m_gitVersion;
}

const QString & Version::fullDate() {
	return m_gitDate;
}

const QString & Version::date() {
	return m_date;
}

const QString & Version::shortDate() {
	return m_shortDate;
}

const QString & Version::year() {
	return m_year;
}

bool Version::candidateGreaterThanCurrent(const VersionThing & candidateVersionThing)
{
	VersionThing myVersionThing;
	myVersionThing.majorVersion = majorVersion().toInt();
	myVersionThing.minorVersion = minorVersion().toInt();
	myVersionThing.minorSubVersion = minorSubVersion().toInt();
	myVersionThing.releaseModifier = modifier();

	return greaterThan(myVersionThing, candidateVersionThing);
}

bool Version::greaterThan(const QString & myVersionStr, const QString & yourVersionStr) {
	VersionThing myVersionThing;
	Version::toVersionThing(myVersionStr,myVersionThing);
	VersionThing yourVersionThing;
	Version::toVersionThing(yourVersionStr,yourVersionThing);
	return greaterThan(myVersionThing,yourVersionThing);
}

bool Version::greaterThan(const VersionThing & myVersionThing, const VersionThing & yourVersionThing)
{
	// yourVersionThing > myVersionThing

	bool newOne = false;
	if (yourVersionThing.majorVersion > myVersionThing.majorVersion) {
		newOne = true;
	}
	else if (yourVersionThing.majorVersion == myVersionThing.majorVersion) {
		if (yourVersionThing.minorVersion > myVersionThing.minorVersion) {
			newOne = true;
		}
		else if (yourVersionThing.minorVersion == myVersionThing.minorVersion) {
			if (yourVersionThing.minorSubVersion > myVersionThing.minorSubVersion) {
				newOne = true;
			}
			else if (yourVersionThing.minorSubVersion == myVersionThing.minorSubVersion) {
				newOne = modifierGreaterThan(myVersionThing.releaseModifier, yourVersionThing.releaseModifier);
			}
		}
	}

	return newOne;
}

bool Version::modifierGreaterThan(const QString & myReleaseModifier, const QString & yourReleaseModifier) {

	int yourIndex = m_modifiers.indexOf(yourReleaseModifier);
	int myIndex = m_modifiers.indexOf(myReleaseModifier);
	return yourIndex > myIndex;
}

void Version::toVersionThing(const QString & candidate, VersionThing & versionThing)
{
	versionThing.ok = false;
	QString modString;
	Q_FOREACH (QString s, m_modifiers) {
		modString += s + "|";
	}
	modString.chop(1);
	QRegularExpression sw(QString("([\\d]+)\\.([\\d]+)\\.([\\d]+)[\\.]{0,1}(%1)").arg(modString));
	QRegularExpressionMatch match;
	if (candidate.indexOf(sw, 0, &match) != 0) {
		return;
	}

	versionThing.majorVersion = match.captured(1).toInt(&versionThing.ok);
	if (!versionThing.ok) return;

	versionThing.minorVersion = match.captured(2).toInt(&versionThing.ok);
	if (!versionThing.ok) return;

	versionThing.minorSubVersion = match.captured(3).toInt(&versionThing.ok);
	if (!versionThing.ok) return;

	versionThing.releaseModifier = match.captured(4);
}

void Version::cleanup() {
	if (m_singleton != nullptr) {
		delete m_singleton;
		m_singleton = nullptr;
	}
}

QString Version::makeRequestParamsString(bool withID) {
	QSettings settings;
	if (settings.value("pid").isNull()) {
		settings.setValue("pid", TextUtils::getRandText());
	}

	QString id;
	if (withID) id = QString("&pid=%1").arg(settings.value("pid").toString());
	QString siVersion(QUrl::toPercentEncoding(Version::versionString()));
	QString siSystemName(QUrl::toPercentEncoding(QSysInfo::productType()));
	QString siSystemVersion(QUrl::toPercentEncoding(QSysInfo::productVersion()));
	QString siKernelName(QUrl::toPercentEncoding(QSysInfo::kernelType()));
	QString siKernelVersion(QUrl::toPercentEncoding(QSysInfo::kernelVersion()));
	QString siArchitecture(QUrl::toPercentEncoding(QSysInfo::currentCpuArchitecture()));
	QString string = QString("?version=%2&sysname=%3&kernname=%4&kernversion=%5&arch=%6&sysversion=%7%8")
	                 .arg(siVersion)
	                 .arg(siSystemName)
	                 .arg(siKernelName)
	                 .arg(siKernelVersion)
	                 .arg(siArchitecture)
	                 .arg(siSystemVersion)
	                 .arg(id)
	                 ;
	return string;
}
