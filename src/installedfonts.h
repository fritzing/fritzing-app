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

#ifndef INSTALLEDFONTS_H
#define INSTALLEDFONTS_H

#include <QSet>
#include <QString>
#include <QMultiHash>

class InstalledFonts
{

public:
	static QSet<QString> InstalledFontsList;
	static QMultiHash<QString, QString> InstalledFontsNameMapper;   // family name to filename; SVG files seem to have to use filename
	// note: these static variables are initialized in fapplication.cpp
};

#endif
