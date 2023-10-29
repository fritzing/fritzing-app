/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2023 Fritzing GmbH

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


#include "debugconnectorsprobe.h"
#include "sketch/sketchwidget.h"

DebugConnectorsProbe::DebugConnectorsProbe(
	SketchWidget *breadboardGraphicsView,
	SketchWidget *schematicGraphicsView,
	SketchWidget *pcbGraphicsView)
	: 	FProbe("DebugConnectors"),
	m_breadboardGraphicsView(breadboardGraphicsView),
	m_schematicGraphicsView(schematicGraphicsView),
	m_pcbGraphicsView(pcbGraphicsView),
	m_debugConnectors(nullptr)
{
}

/**
 * @brief Retrieves the number of BaseItems that have unexpected connections.
 *
 * Like all other FProbes, this method should not be called directly, but only through
 * FTesting.
 *
 * @return Returns the number of errors detected as an integer wrapped in a QVariant.
 *         This count represents the number of BaseItems with unexpected connections.
 *
 */
QVariant DebugConnectorsProbe::read()
{
	if (!m_debugConnectors) {
		m_debugConnectors = new DebugConnectors(m_breadboardGraphicsView,
												m_schematicGraphicsView,
												m_pcbGraphicsView);
	}

	auto errorList = m_debugConnectors->doRoutingCheck();
	return errorList.size();
}
