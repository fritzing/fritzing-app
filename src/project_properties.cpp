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

#include "project_properties.h"

#include <utility>

ProjectProperties::ProjectProperties() {
    m_propertiesMap[ProjectPropertyKeySimulatorTimeStepMode] = "false";
    m_propertiesMap[ProjectPropertyKeySimulatorNumberOfSteps] = "400";
    m_propertiesMap[ProjectPropertyKeySimulatorTimeStepS] = "1us";
    m_propertiesMap[ProjectPropertyKeySimulatorAnimationTimeS] = "5s";
	m_keys = QStringList(m_propertiesMap.keys());
}

ProjectProperties::~ProjectProperties() {
}

void ProjectProperties::saveProperties(QXmlStreamWriter & streamWriter) {
	streamWriter.writeStartElement("project_properties");

	for (const QString & key: std::as_const(m_keys)) {
		streamWriter.writeStartElement(key);
		streamWriter.writeAttribute("value", m_propertiesMap[key]);
		streamWriter.writeEndElement();
	}

	streamWriter.writeEndElement();
}

void ProjectProperties::load(const QDomElement & projectProperties) {
	QMap<QString, bool> loadedValueMap;
	for (const QString & key: std::as_const(m_keys)) {
		loadedValueMap[key] = false;
	}
	if (!projectProperties.isNull()) {
		for (const QString & key: std::as_const(m_keys)) {
			QDomElement element = projectProperties.firstChildElement(key);
			if (!element.isNull()) {
				m_propertiesMap[key] = element.attribute("value");
				loadedValueMap[key] = true;
			}
		}
	}
	for (const QString & key: std::as_const(m_keys)) {
		if (!loadedValueMap[key]) {
			m_propertiesMap[key] = m_OldProjectValuePropertiesMap[key];
		}
	}
}

QString ProjectProperties::getProjectProperty(const QString & key) {
	return m_propertiesMap[key];
}

void ProjectProperties::setProjectProperty(const QString & key, QString value) {
	m_propertiesMap[key] = value;
}
