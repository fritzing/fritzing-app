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

#include "via.h"
#include "../utils/graphicsutils.h"
#include "../fsvgrenderer.h"
#include "../utils/textutils.h"
#include "../viewlayer.h"
#include "../connectors/connectoritem.h"
#include "../connectors/svgidlayer.h"

#include <QSettings>

static HoleClassThing TheHoleThing;

const QString Via::AutorouteViaHoleSize = "autorouteViaHoleSize";
const QString Via::AutorouteViaRingThickness = "autorouteViaRingThickness";
QString Via::DefaultAutorouteViaHoleSize;
QString Via::DefaultAutorouteViaRingThickness;

Via::Via( ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: Hole(modelPart, viewID, viewGeometry, id, itemMenu, doLabel)
{
	QSettings settings;
	QString ringThickness = settings.value(AutorouteViaRingThickness, "").toString();
	QString holeSize = settings.value(AutorouteViaHoleSize, "").toString();

    bool holeSizeWasEmpty = holeSize.isEmpty();
    bool ringThicknessWasEmpty = holeSize.isEmpty();

    PaletteItem::setUpHoleSizes("via", TheHoleThing);

	if (ringThicknessWasEmpty) {
		settings.setValue(AutorouteViaRingThickness, TheHoleThing.ringThickness);
		DefaultAutorouteViaRingThickness = TheHoleThing.ringThickness;
	}

	if (holeSizeWasEmpty) {
		settings.setValue(AutorouteViaHoleSize, TheHoleThing.holeSize);
		DefaultAutorouteViaHoleSize = TheHoleThing.holeSize;
	}

	//DebugDialog::debug(QString("creating via %1 %2 %3").arg((long) this, 0, 16).arg(id).arg(m_viewID));
}

Via::~Via() {
	//DebugDialog::debug(QString("deleting via %1 %2 %3").arg((long) this, 0, 16).arg(m_id).arg(m_viewID));
}

void Via::initHoleSettings(HoleSettings & holeSettings) 
{
    // called only by AutorouterSettingsDialog
    PaletteItem::initHoleSettings(holeSettings, &TheHoleThing);
}

void Via::setBoth(const QString & holeDiameter, const QString & ringThickness) {
	if (this->m_viewID != ViewLayer::PCBView) return;

	ItemBase * otherLayer = setBothSvg(holeDiameter, ringThickness);
	resetConnectors(otherLayer, otherLayer->fsvgRenderer());

    double hd = TextUtils::convertToInches(holeDiameter) * GraphicsUtils::SVGDPI;
    double rt = TextUtils::convertToInches(ringThickness) * GraphicsUtils::SVGDPI;
    ConnectorItem * ci = connectorItem();
    ci->setRadius((hd / 2) + (rt / 2), rt);
    ci->getCrossLayerConnectorItem()->setRadius((hd / 2) + (rt / 2), rt);
}


QString Via::makeID() {
	return "connector0pin";
}

void Via::setAutoroutable(bool ar) {
	m_viewGeometry.setAutoroutable(ar);
}

bool Via::getAutoroutable() {
	return m_viewGeometry.getAutoroutable();
}

ConnectorItem * Via::connectorItem() {
	foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
		return connectorItem;
	}

	return NULL;
}

void Via::saveInstanceLocation(QXmlStreamWriter & streamWriter)
{
	streamWriter.writeAttribute("x", QString::number(m_viewGeometry.loc().x()));
	streamWriter.writeAttribute("y", QString::number(m_viewGeometry.loc().y()));
	streamWriter.writeAttribute("wireFlags", QString::number(m_viewGeometry.flagsAsInt()));
	GraphicsUtils::saveTransform(streamWriter, m_viewGeometry.transform());
}
