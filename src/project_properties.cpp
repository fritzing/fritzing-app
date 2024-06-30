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

ProjectProperties::ProjectProperties() {
	m_propertiesMap[ProjectPropertyKeySimulatorTimeStepMode] = "false";
	m_propertiesMap[ProjectPropertyKeySimulatorNumberOfSteps] = "400";
	m_propertiesMap[ProjectPropertyKeySimulatorTimeStepS] = "1us";
	m_propertiesMap[ProjectPropertyKeySimulatorAnimationTimeS] = "5s";
}

ProjectProperties::~ProjectProperties() {
}

void ProjectProperties::saveProperties(QXmlStreamWriter & streamWriter) {
	streamWriter.writeStartElement("project_properties");

	for (auto it = m_propertiesMap.constBegin(); it != m_propertiesMap.constEnd(); ++it) {
		streamWriter.writeStartElement(it.key());
		streamWriter.writeAttribute("value", it.value());
		streamWriter.writeEndElement();
	}

	streamWriter.writeEndElement();
}

void ProjectProperties::load(const QDomElement & projectProperties) {
	// If required set differing legacy project defaults after this line.
	if (projectProperties.isNull()) return;
	for (auto it = m_propertiesMap.begin(); it != m_propertiesMap.end(); ++it) {
		QDomElement element = projectProperties.firstChildElement(it.key());
		if (!element.isNull()) {
			it.value() = element.attribute("value");
		}
	}
}

QString ProjectProperties::getProjectProperty(const QString & key) {
	return m_propertiesMap[key];
}

void ProjectProperties::setProjectProperty(const QString & key, QString value) {
	m_propertiesMap[key] = value;
}
