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

#include "ercdata.h"

void ValidReal::setValue(double v) noexcept {
	m_value = v;
	m_ok = true;
}

bool ValidReal::setValue(const QString & v) {
	m_value = v.toDouble(&m_ok);
	return m_ok;
}

//////////////////////////////////////////////////////////

ErcData::ErcData(const QDomElement & ercElement) : 
	m_eType(UnknownEType),
	m_ignore(Never),
	m_currentFlow(UnknownFlow)
{
	QString eType = ercElement.attribute("etype");
	if (eType.compare("VCC", Qt::CaseInsensitive) == 0) {
		m_eType = VCC;
	}
	else if (eType.compare("ground", Qt::CaseInsensitive) == 0) {
		m_eType = Ground;
	}

	QString ig = ercElement.attribute("ignore");
	if (ig.compare("ifUnconnected", Qt::CaseInsensitive) == 0) {
		m_ignore = IfUnconnected;
	}
	else if (ig.compare("always", Qt::CaseInsensitive) == 0) {
		m_ignore = Always;
	}

	QDomElement ercChild = ercElement.firstChildElement();
	while (!ercChild.isNull()) {
		QString nodeName = ercChild.nodeName();
		if (nodeName.compare("voltage", Qt::CaseInsensitive) == 0) {
			readVoltage(ercChild);
		}
		else if (nodeName.compare("current", Qt::CaseInsensitive) == 0) {
			readCurrent(ercChild);
		}
		ercChild = ercChild.nextSiblingElement();
	}
}

void ErcData::readCurrent(QDomElement & currentElement) {
	m_currentMin.setValue(currentElement.attribute("valueMin"));
	m_currentMax.setValue(currentElement.attribute("valueMax"));
	m_current.setValue(currentElement.attribute("value"));
	QString flow = currentElement.attribute("flow");
	if (flow.compare("source", Qt::CaseInsensitive) == 0) {
		m_currentFlow = Source;
	}
	else if (flow.compare("sink", Qt::CaseInsensitive) == 0) {
		m_currentFlow = Sink;
	}
	else {
		m_currentFlow = UnknownFlow;
	}
}

void ErcData::readVoltage(QDomElement & voltageElement) {
	m_voltageMin.setValue(voltageElement.attribute("valueMin"));
	m_voltageMax.setValue(voltageElement.attribute("valueMax"));
	m_voltage.setValue(voltageElement.attribute("value"));
}

bool ErcData::writeToElement(QDomElement & ercElement, QDomDocument & doc) {
    bool success = true;
	switch (m_eType) {
	case Ground:
		ercElement.setAttribute("etype", "ground");
		writeCurrent(ercElement, doc);
		break;
	case VCC:
		ercElement.setAttribute("etype", "VCC");
		writeCurrent(ercElement, doc);
		writeVoltage(ercElement, doc);
		break;
	default:
        success = false;
        break;
	}

    return success;
}

void ErcData::writeCurrent(QDomElement & parent, QDomDocument & doc) {
	if (m_current || m_currentMin || m_currentMax || m_currentFlow != UnknownFlow) {
		QDomElement currentElement = doc.createElement("current");
		parent.appendChild(currentElement);
		if (m_current) {
			currentElement.setAttribute("value", QString::number(m_current.value()));
		}
		if (m_currentMin) {
			currentElement.setAttribute("valueMin", QString::number(m_currentMin.value()));
		}
		if (m_currentMax) {
			currentElement.setAttribute("valueMax", QString::number(m_currentMax.value()));
		}
		switch (m_currentFlow) {
		case Source:
			currentElement.setAttribute("flow", "source");
			break;
		case Sink:
			currentElement.setAttribute("flow", "sink");
			break;
		default:
			break;
		}
	}
}

void ErcData::writeVoltage(QDomElement & parent, QDomDocument & doc) {
	if (m_voltage || m_voltageMin || m_voltageMax) {
		QDomElement voltageElement = doc.createElement("voltage");
		parent.appendChild(voltageElement);
		if (m_voltage) {
			voltageElement.setAttribute("value", QString::number(m_voltage.value()));
		}
		if (m_voltageMin) {
			voltageElement.setAttribute("valueMin", QString::number(m_voltageMin.value()));
		}
		if (m_voltageMax) {
			voltageElement.setAttribute("valueMax", QString::number(m_voltageMax.value()));
		}
	}
}

