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

********************************************************************

$Revision: 6904 $:
$Author: irascibl@gmail.com $:
$Date: 2013-02-26 16:26:03 +0100 (Di, 26. Feb 2013) $

********************************************************************/

#ifndef VERSION_H
#define VERSION_H

#include <QString>

struct VersionThing {
	int majorVersion;
	int minorVersion;
	int minorSubVersion;
	QString releaseModifier;
	bool ok;
};

class Version {

public:
    static const QString & majorVersion();
    static const QString & minorVersion();
    static const QString & minorSubVersion();
    static const QString & modifier();
    static const QString & revision();
	static const QString & versionString();
    static const QString & date();
    static const QString & fullDate();
    static const QString & shortDate();
    static const QString & year();
    static bool candidateGreaterThanCurrent(const VersionThing &);
    static bool greaterThan(const VersionThing & myVersionThing, const VersionThing & yourVersionThing);
    static bool greaterThan(const QString & myVersionStr, const QString & yourVersionStr);
    static bool modifierGreaterThan(const QString & myReleaseModifier, const QString & yourReleaseModifier);
    static void toVersionThing(const QString & candidate, VersionThing & versionThing);
	static void cleanup();
    static QString makeRequestParamsString(bool withID);

public:
	static QString FirstVersionWithDetachedUserData;

protected:
	Version();

protected:

	static QString m_majorVersion;
	static QString m_minorVersion;
	static QString m_minorSubVersion;
    static QString m_gitCommit;
	static QString m_revision;
	static QString m_modifier;
	static QString m_versionString;
    static QString m_gitDate;
	static QString m_date;
	static QString m_shortDate;
	static QString m_year;
	static Version * m_singleton;
	static QStringList m_modifiers;

};

#endif
