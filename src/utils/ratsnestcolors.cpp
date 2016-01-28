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

#include "ratsnestcolors.h"
#include "../debugdialog.h"

#include <QFile>

QHash<ViewLayer::ViewID, RatsnestColors *> RatsnestColors::m_viewList;
QHash<QString, class RatsnestColor *> RatsnestColors::m_allNames;

QColor ErrorColor(0, 0, 0);

//////////////////////////////////////////////////////

class RatsnestColor {
	RatsnestColor(const QDomElement &);
	~RatsnestColor();

    bool matchColor(const QString &);

	friend class RatsnestColors;


protected:
	QString m_name;
	QString m_wire;
	QColor m_ratsnest;
	QString m_shadow;
	QStringList m_connectorNames;
    QList<RatsnestColor *> m_obsoleteList;
};

//////////////////////////////////////////////////////

RatsnestColor::RatsnestColor(const QDomElement & color) {
	m_name = color.attribute("name");
	//DebugDialog::debug("color name " + m_name);
	m_ratsnest.setNamedColor(color.attribute("ratsnest"));
	m_wire = color.attribute("wire");
	m_shadow = color.attribute("shadow");
	QDomElement connector = color.firstChildElement("connector");
	while (!connector.isNull()) {
		m_connectorNames.append(connector.attribute("name"));
		connector = connector.nextSiblingElement("connector");
	}
    QDomElement obsolete = color.firstChildElement("obsolete");
    while (!obsolete.isNull()) {
        m_obsoleteList << new RatsnestColor(obsolete);
        obsolete = obsolete.nextSiblingElement("obsolete");
    }
}

RatsnestColor::~RatsnestColor() {
    foreach (RatsnestColor * ratsnestColor, m_obsoleteList) {
        delete ratsnestColor;
    }
    m_obsoleteList.clear();
}

bool RatsnestColor::matchColor(const QString & string) {
    if (m_wire.compare(string, Qt::CaseInsensitive) == 0) return true;

    foreach (RatsnestColor * obsolete, m_obsoleteList) {
        if (obsolete->m_wire.compare(string, Qt::CaseInsensitive) == 0) return true;
    }

    return false;
}

//////////////////////////////////////////////////////

RatsnestColors::RatsnestColors(const QDomElement & view) 
{
	m_viewID = ViewLayer::idFromXmlName(view.attribute("name"));
	m_backgroundColor.setNamedColor(view.attribute("background"));
	m_index = 0;
	QDomElement color = view.firstChildElement("color");
	while (!color.isNull()) {
		RatsnestColor * ratsnestColor = new RatsnestColor(color);
		m_ratsnestColorHash.insert(ratsnestColor->m_name, ratsnestColor);
		m_ratsnestColorList.append(ratsnestColor);
		foreach (QString name, ratsnestColor->m_connectorNames) {
			m_allNames.insert(name.toLower(), ratsnestColor);
		}
		color = color.nextSiblingElement("color");
	}
}

RatsnestColors::~RatsnestColors()
{
	foreach (RatsnestColor * ratsnestColor, m_ratsnestColorHash.values()) {
		delete ratsnestColor;
	}
	m_ratsnestColorHash.clear();
	m_ratsnestColorList.clear();
}

void RatsnestColors::initNames() {
	QFile file(":/resources/ratsnestcolors.xml");

	QString errorStr;
	int errorLine;
	int errorColumn;
	QDomDocument domDocument;

	if (!domDocument.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
		return;
	}

	QDomElement root = domDocument.documentElement();
	if (root.isNull()) {
		return;
	}

	if (root.tagName() != "colors") {
		return;
	}

	QDomElement view = root.firstChildElement("view");
	while (!view.isNull()) {
		RatsnestColors * ratsnestColors = new RatsnestColors(view);
		m_viewList.insert(ratsnestColors->m_viewID, ratsnestColors);
		view = view.nextSiblingElement("view");
	}
}

void RatsnestColors::cleanup() {
	foreach (RatsnestColors * ratsnestColors, m_viewList.values()) {
		delete ratsnestColors;
	}
	m_viewList.clear();
}

const QColor & RatsnestColors::netColor(ViewLayer::ViewID viewID) {
	RatsnestColors * ratsnestColors = m_viewList.value(viewID, NULL);
	if (ratsnestColors == NULL) return ErrorColor;

	return ratsnestColors->getNextColor();
}

const QColor & RatsnestColors::getNextColor() {
	if (m_ratsnestColorList.count() <= 0) return ErrorColor;

	int resetCount = 0;
	while (true) {
		if (m_index < 0 || m_index >= m_ratsnestColorList.count()) {
			m_index = 0;
			if (++resetCount > 2) {
				// prevent infinite loops if all the colors in the list are for particular connectors
				return ErrorColor;
			}
		}
		RatsnestColor * ratsnestColor = m_ratsnestColorList[m_index++];
		if (ratsnestColor->m_connectorNames.length() > 0) {
			// don't use colors designated for particular connectors
			m_index++;
			continue;
		}

		if (!ratsnestColor->m_ratsnest.isValid()){
			m_index++;
			continue;
		}

		return ratsnestColor->m_ratsnest;
	}
}

	
bool RatsnestColors::findConnectorColor(const QStringList & names, QColor & color) {
	foreach (QString name, names) {
		RatsnestColor * ratsnestColor = m_allNames.value(name.toLower(), NULL);
		if (ratsnestColor == NULL) continue;

		color = ratsnestColor->m_ratsnest;
		return true;
	}

	return false;
}

bool RatsnestColors::isConnectorColor(ViewLayer::ViewID m_viewID, const QColor & color) {
	RatsnestColors * ratsnestColors = m_viewList.value(m_viewID, NULL);
	if (ratsnestColors == NULL) return false;

	foreach (RatsnestColor * ratsnestColor, ratsnestColors->m_ratsnestColorList) {
		if (ratsnestColor->m_ratsnest == color) {
			return (ratsnestColor->m_connectorNames.length() > 0);
		}
	}

	return false;
}

void RatsnestColors::reset(ViewLayer::ViewID m_viewID) {
	RatsnestColors * ratsnestColors = m_viewList.value(m_viewID, NULL);
	if (ratsnestColors == NULL) return;

	ratsnestColors->m_index = 0;
}

QColor RatsnestColors::backgroundColor(ViewLayer::ViewID viewID) 
{
	RatsnestColors * ratsnestColors = m_viewList.value(viewID, NULL);
	if (ratsnestColors == NULL) return QColor();

	return ratsnestColors->m_backgroundColor;
}

const QString & RatsnestColors::shadowColor(ViewLayer::ViewID viewID, const QString & name)
{
	RatsnestColors * ratsnestColors = m_viewList.value(viewID, NULL);
	if (ratsnestColors == NULL) return ___emptyString___;

	RatsnestColor * ratsnestColor = ratsnestColors->m_ratsnestColorHash.value(name.toLower(), NULL);
	if (ratsnestColor == NULL) return ___emptyString___;

	return ratsnestColor->m_shadow;
}

QString RatsnestColors::wireColor(ViewLayer::ViewID viewID, QString & string)
{
	RatsnestColors * ratsnestColors = m_viewList.value(viewID, NULL);
	if (ratsnestColors == NULL) return ___emptyString___;

	RatsnestColor * ratsnestColor = ratsnestColors->m_ratsnestColorHash.value(string.toLower(), NULL);
	if (ratsnestColor != NULL) {
		QString w = ratsnestColor->m_wire;
		if (!w.isEmpty()) return w;
	}

	// reverse lookup
	foreach (QString cname, ratsnestColors->m_ratsnestColorHash.keys()) {
        RatsnestColor * candidate = ratsnestColors->m_ratsnestColorHash.value(cname);
        if (candidate->matchColor(string)) {
	        string = cname;
			return candidate->m_wire;
        }
	}

	return ___emptyString___;
}

