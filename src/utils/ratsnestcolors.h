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

$Revision: 6912 $:
$Author: irascibl@gmail.com $:
$Date: 2013-03-09 08:18:59 +0100 (Sa, 09. Mrz 2013) $

********************************************************************/

#ifndef RATSNESTCOLORS_H
#define RATSNESTCOLORS_H

#include <QColor>
#include <QDomElement>
#include <QHash>
#include <QStringList>
#include "../viewlayer.h"

class RatsnestColors {

public:
	RatsnestColors(const QDomElement &);
	~RatsnestColors();

	static void initNames();
	static void cleanup();
	static const QColor & netColor(ViewLayer::ViewID m_viewID);
	static bool findConnectorColor(const QStringList & names, QColor & color);
	static bool isConnectorColor(ViewLayer::ViewID m_viewID, const QColor &);
	static void reset(ViewLayer::ViewID m_viewID);
	static QColor backgroundColor(ViewLayer::ViewID);
	static const QString & shadowColor(ViewLayer::ViewID, const QString& name);
	static QString wireColor(ViewLayer::ViewID, QString& name);

protected:
	const QColor & getNextColor();

protected:
	ViewLayer::ViewID m_viewID;
	QColor m_backgroundColor;
	int m_index;
	QHash<QString, class RatsnestColor *> m_ratsnestColorHash;
	QList<class RatsnestColor *> m_ratsnestColorList;

	static QHash<ViewLayer::ViewID, RatsnestColors *> m_viewList;
	static QHash<QString, class RatsnestColor *> m_allNames;
};


#endif
