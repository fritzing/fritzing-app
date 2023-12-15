/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2022 Fritzing GmbH

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

#ifndef PROJECT_PROPERTIES_H
#define PROJECT_PROPERTIES_H

#include <QMap>
#include <QString>
#include <QXmlStreamWriter>
#include <QDomElement>

const QString ProjectPropertyKeySimulatorFrequencyHz = "simulator_frequency_hz";

class ProjectProperties {
public:
	ProjectProperties();

	~ProjectProperties();

	void saveProperties(QXmlStreamWriter & streamWriter);
	void load(const QDomElement & projectProperties);
	QString getProjectProperty(const QString & key);
	void setProjectProperty(const QString & key, QString value);

private:
	QMap<QString, QString> m_propertiesMap;
	QMap<QString, QString> m_OldProjectValuePropertiesMap;
	QStringList m_keys;
};

#endif // PROJECT_PROPERTIES_H
