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

#include "oscilloscope.h"

#include <random>

#include "debugdialog.h"
#include "symbolpaletteitem.h"
#include "connectors/connectoritem.h"
#include "../utils/textutils.h"
#include "../simulation/simulator.h"


Oscilloscope::Oscilloscope( ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: Capacitor(modelPart, viewID, viewGeometry, id, itemMenu, doLabel)
{
}

Oscilloscope::~Oscilloscope() {
}

/**
 * Updates and checks a oscilloscope. If the ground connection is not connected, plots a noisy signal.
 * Calculates the parameter to measure and updates the display of the multimeter.
 * @param[in] part An oscilloscope that is going to be checked and updated.
 */
void Oscilloscope::updateOscilloscope(unsigned long timeStep, double simStartTime, double simStepTime, Simulator * sim, ItemBase * bbOscilloscope) {
	ConnectorItem * comProbe = nullptr, * v1Probe = nullptr, * v2Probe = nullptr, * v3Probe = nullptr, * v4Probe = nullptr;
	QList<ConnectorItem *> probes = cachedConnectorItems();

	foreach(ConnectorItem * ci, probes) {
		if(ci->connectorSharedName().toLower().compare("com probe") == 0) comProbe = ci;
		if(ci->connectorSharedName().toLower().compare("v1 probe") == 0) v1Probe = ci;
		if(ci->connectorSharedName().toLower().compare("v2 probe") == 0) v2Probe = ci;
		if(ci->connectorSharedName().toLower().compare("v3 probe") == 0) v3Probe = ci;
		if(ci->connectorSharedName().toLower().compare("v4 probe") == 0) v4Probe = ci;
	}

	if(!comProbe || !v1Probe || !v2Probe || !v3Probe || !v4Probe)
		return;

	if(!v1Probe->connectedToWires() && !v2Probe->connectedToWires() && !v3Probe->connectedToWires() && !v4Probe->connectedToWires()) {
		DebugDialog::stream() << "Oscilloscope does not have any wire connected to the probe terminals. ";
		return;
	}
	ConnectorItem * probesArray[4] = {v1Probe, v2Probe, v3Probe, v4Probe};

	//TODO: use convertFromPowerPrefixU
	int nChannels = TextUtils::convertFromPowerPrefix(getProperty("channels"), "");
	double timeDiv = TextUtils::convertFromPowerPrefix(getProperty("time/div"), "s");
	double hPos = TextUtils::convertFromPowerPrefix(getProperty("horizontal position"), "s");
	double ch1_volsDiv = TextUtils::convertFromPowerPrefix(getProperty("ch1 volts/div"), "V");
	double ch1_offset = TextUtils::convertFromPowerPrefix(getProperty("ch1 offset"), "V");
	double ch2_volsDiv = TextUtils::convertFromPowerPrefix(getProperty("ch2 volts/div"), "V");
	double ch2_offset = TextUtils::convertFromPowerPrefix(getProperty("ch2 offset"), "V");
	double ch3_volsDiv = TextUtils::convertFromPowerPrefix(getProperty("ch3 volts/div"), "V");
	double ch3_offset = TextUtils::convertFromPowerPrefix(getProperty("ch3 offset"), "V");
	double ch4_volsDiv = TextUtils::convertFromPowerPrefix(getProperty("ch4 volts/div"), "V");
	double ch4_offset = TextUtils::convertFromPowerPrefix(getProperty("ch4 offset"), "V");
	QString lineColor[4] = {"#ffff50", "lightgreen", "lightblue", "pink"};
	double voltsDiv[4] ={ch1_volsDiv, ch2_volsDiv, ch3_volsDiv, ch4_volsDiv};
	double chOffsets[4] ={ch1_offset, ch2_offset, ch3_offset, ch4_offset};

	double screenWidth = 3376.022, screenHeight = 2700.072, bbScreenStrokeWidth= 20;
	double verDivisions = 8, horDivisions = 10, divisionSize = screenHeight/verDivisions;
	double bbScreenOffsetX = 290.544, bbScreenOffsetY = 259.061, schScreenOffsetX = 906.07449, schScreenOffsetY = 354.60801;
	QString svgHeader = "<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n%5"
						"<svg xmlns:svg='http://www.w3.org/2000/svg' xmlns='http://www.w3.org/2000/svg' "
						"version='1.2' baseProfile='tiny' "
						"x='0in' y='0in' width='%1in' height='%2in' "
						"viewBox='0 0 %3 %4' >\n";
	QString bbSvg = QString(svgHeader)
						.arg((screenWidth+bbScreenOffsetX)/1000)
						.arg((screenHeight+bbScreenOffsetY*2)/1000)
						.arg(screenWidth+bbScreenOffsetX)
						.arg(screenHeight+bbScreenOffsetY*2)
						.arg(TextUtils::CreatedWithFritzingXmlComment);
	QString schSvg = QString(svgHeader)
						 .arg((screenWidth+schScreenOffsetX*2)/1000)
						 .arg((screenHeight+schScreenOffsetY*2)/1000)
						 .arg(screenWidth+schScreenOffsetX*2)
						 .arg(screenHeight+schScreenOffsetY*2)
						 .arg(TextUtils::CreatedWithFritzingXmlComment);

	// Generate the signal for each channel and the auxiliary marks (offsets, volts/div, etc.)
	for (int channel = 0; channel < nChannels; channel++) {
		if (!probesArray[channel]->connectedToWires()) continue;

		//Get the signal and com voltages
		auto v = sim->voltageVector(probesArray[channel]);
		std::vector<double> vCom(v.size(), 0.0);
		if (!comProbe->connectedToWires()) {
			//There is no com probe connected, we need to generate noise
			std::random_device rd;
			std::mt19937 gen(rd());
			std::normal_distribution<> dist(0.0, voltsDiv[channel]);
			// Generate random doubles and fill the vector
			for(auto& val : vCom) {
				val = dist(gen);
			}
		} else {
			vCom = sim->voltageVector(comProbe);
		}

		//Draw the signal
		QString pathId = QString("ch%1-path").arg(channel+1);
		QString signalPath = generateSvgPath(v, vCom, timeStep, pathId, simStartTime, simStepTime, hPos, timeDiv, divisionSize/voltsDiv[channel], chOffsets[channel],
											 screenHeight, screenWidth, lineColor[channel], "20");
		bbSvg += signalPath.arg(bbScreenOffsetX).arg(bbScreenOffsetY);
		schSvg += signalPath.arg(schScreenOffsetX).arg(schScreenOffsetY);

		//Add text label about volts/div for each channel
		bbSvg += QString("<text x='%1' y='%2' font-family='Droid Sans' font-size='60' fill='%3'>CH%4: %5V</text>\n")
					 .arg(bbScreenOffsetX + divisionSize*channel)
					 .arg(screenHeight + bbScreenOffsetY * 1.35)
					 .arg(lineColor[channel]).arg(channel+1)
					 .arg(TextUtils::convertToPowerPrefix(voltsDiv[channel]));

		//Add triangle as a mark for the offset for each channel
		double arrowSize = 50;
		double arrowPos = -1*chOffsets[channel]/ch1_volsDiv*divisionSize+screenHeight/2+bbScreenOffsetY-arrowSize;
		bbSvg += QString("<polygon points='0,0 %1,%1, 0,%2' stroke='none' fill='%3' transform='translate(%4,%5)'/>\n")
					 .arg(arrowSize)
					 .arg(arrowSize*2)
					 .arg(lineColor[channel])
					 .arg(bbScreenOffsetX - arrowSize - 10)
					 .arg(arrowPos);

		//Add voltage scale axis in sch
		double xOffset[4] = {schScreenOffsetX*0.95, schScreenOffsetX*0.62,
							 screenWidth + schScreenOffsetX*1.05, screenWidth + schScreenOffsetX*1.4};
		if(!probesArray[0]->connectedToWires())
			xOffset[1]=xOffset[0];
		if(!probesArray[2]->connectedToWires())
			xOffset[3]=xOffset[2];

		//Add line of the scale axis
		schSvg += QString("<line x1='%1' y1='%2' x2='%1' y2='%3' stroke='%4' stroke-width='4' />\n")
					  .arg(xOffset[channel])
					  .arg(schScreenOffsetY)
					  .arg(schScreenOffsetY+screenHeight)
					  .arg(lineColor[channel]);

		double tickSize = 10;
		double paddingAlignment = channel>=(nChannels/2)? 1 : -1;
		QString textAlignment = channel>=(nChannels/2)? "start": "end";

		//Add name of the scale axis
		QString netName = QString("Channel %1 (V)").arg(channel + 1);
		QList<ConnectorItem *> connectorItems;
		connectorItems.append(probesArray[channel]);
		ConnectorItem::collectEqualPotential(connectorItems, false, ViewGeometry::RatsnestFlag);

		Q_FOREACH ( ConnectorItem * cItem, connectorItems) {
			SymbolPaletteItem* symbolItem = dynamic_cast<SymbolPaletteItem *>(cItem->attachedTo());
			if(symbolItem && symbolItem->isOnlyNetLabel() ) {
				netName = symbolItem->getLabel();
				netName += " (V)";
				break;
			}
		}

		schSvg += QString("<text font-family='Droid Sans' font-size='60' fill='%3' "
						  "text-anchor='middle' transform='translate(%1, %2) rotate(-90)'>%4</text>\n")
					  .arg(xOffset[channel] + paddingAlignment * 180 + (1+paddingAlignment)*30)
					  .arg(schScreenOffsetY + screenHeight/2)
					  .arg(lineColor[channel], netName);



		for (int tick = 0; tick < (verDivisions+1); ++tick) {
			double vTick = voltsDiv[channel]*(verDivisions/2-tick)-chOffsets[channel];
			QString voltageText = TextUtils::convertToPowerPrefix(vTick);
			schSvg += QString("<text x='%1' y='%2' font-family='Droid Sans' font-size='60' fill='%3' text-anchor='%4'>%5</text>\n")
						  .arg(xOffset[channel] +  paddingAlignment * 10)
						  .arg(schScreenOffsetY + divisionSize * tick + 20)
						  .arg(lineColor[channel], textAlignment, voltageText);

			schSvg += QString("<line x1='%1' y1='%2' x2='%3' y2='%2' stroke='%4' stroke-width='4' />\n")
						  .arg(xOffset[channel] - tickSize + paddingAlignment * tickSize * -1)
						  .arg(schScreenOffsetY + divisionSize * tick)
						  .arg(xOffset[channel] + tickSize + paddingAlignment * tickSize * -1)
						  .arg(lineColor[channel]);
		}


	} //End of for each channel

	//Add time scale axis in bb
	bbSvg += QString("<text x='%1' y='%2' font-family='Droid Sans' text-anchor='end' font-size='60' fill='white' xml:space='preserve'>time/div: %3s </text>")
				 .arg(bbScreenOffsetX + screenWidth / 2)
				 .arg(bbScreenOffsetY * 0.85)
				 .arg(TextUtils::convertToPowerPrefix(timeDiv));
	bbSvg += QString("<text x='%1' y='%2' font-family='Droid Sans' text-anchor='start' font-size='60' fill='white' xml:space='preserve'> pos: %4s</text>")
				 .arg(bbScreenOffsetX + screenWidth/2)
				 .arg(bbScreenOffsetY * 0.85)
				 .arg(TextUtils::convertToPowerPrefix(hPos));

	//Add time scale axis in sch
	for (int tick = 0; tick < (horDivisions+1); ++tick) {
		schSvg += QString("<text x='%1' y='%2' text-anchor='middle' font-family='Droid Sans' font-size='60' fill='%3'>%4</text>")
		.arg(schScreenOffsetX+divisionSize*tick).arg(screenHeight+schScreenOffsetY*1.25)
			.arg("white", TextUtils::convertToPowerPrefix(hPos + timeDiv*tick));
	}
	schSvg += QString("<text x='%1' y='%2' text-anchor='middle' font-family='Droid Sans' font-size='60' fill='%3'>Time (s)</text>")
				  .arg(schScreenOffsetX + screenWidth / 2)
				  .arg(screenHeight + schScreenOffsetY * 1.5)
				  .arg("white");

	bbSvg += "</svg>";
	schSvg += "</svg>";

	QGraphicsSvgItem * schGraph = new QGraphicsSvgItem(this);
	QGraphicsSvgItem * bbGraph = new QGraphicsSvgItem(bbOscilloscope);
	QSvgRenderer *schGraphRender = new QSvgRenderer(schSvg.toUtf8());
	QSvgRenderer *bbGraphRender = new QSvgRenderer(bbSvg.toUtf8());
	if(!schGraphRender->isValid())
		DebugDialog::stream() << "SCH SVG Graph is NOT VALID \n";

	if(!bbGraphRender->isValid())
		DebugDialog::stream() << "BB SVG Graph is NOT VALID\n";

	schGraph->setSharedRenderer(schGraphRender);
	schGraph->setZValue(std::numeric_limits<double>::max());
	bbGraph->setSharedRenderer(bbGraphRender);
	bbGraph->setZValue(std::numeric_limits<double>::max());

	addSimulationGraphicsItem(schGraph);
	bbOscilloscope->addSimulationGraphicsItem(bbGraph);
}

QString Oscilloscope::generateSvgPath(std::vector<double> proveVector, std::vector<double> comVector, int currTimeStep, QString nameId, double simStartTime, double simTimeStep, double timePos, double timeScale, double verticalScale, double verOffset, double screenHeight, double screenWidth, QString color, QString strokeWidth ) {
	// DebugDialog::stream() << "OSCILLOSCOPE: pos " << timePos << ", timeScale: " << timeScale;
	// DebugDialog::stream() << "OSCILLOSCOPE: VOLTAGE VALUES " << nameId.toStdString() << ": ";

	QString svg;
	double screenOffset = 0;//132.87378;
	if (!nameId.isEmpty())
		svg += QString("<path id='%1' d='").arg(nameId);
	else
		svg += QString("<path d='");


	double vScale = -1*verticalScale;
	double y_0 = screenOffset + screenHeight/2; // the center of the screen

	int points = std::min( proveVector.size(), comVector.size() );
	double oscEndTime = timePos + timeScale * 10;
	double nSampleInScreen = (oscEndTime - timePos)/simTimeStep + 1;
	double horScale = screenWidth/(nSampleInScreen-1);

	//DebugDialog::stream() << "OSCILLOSCOPE: nSampleInScreen " << nSampleInScreen;
	int screenPoint = 0;
	for (int vPoint = 0; vPoint <  points; vPoint++) {
		if (currTimeStep < vPoint)
			break;
		double time = simStartTime + simTimeStep * vPoint;
		if (time < timePos)
			continue;
		if (time > oscEndTime)
			break;

		double voltage = proveVector[vPoint] - comVector[vPoint];
		double vPos = (voltage + verOffset) * vScale + y_0;
		//Do not go out of the screen
		vPos = (vPos < screenOffset) ? screenOffset : vPos;
		vPos = (vPos > (screenOffset+screenHeight)) ? screenOffset+screenHeight : vPos;

		if (screenPoint == 0) {
			svg.append("M "+ QString::number(screenOffset, 'f', 3) +" " + QString::number( vPos, 'f', 3) + " ");
		} else {
			svg.append("L " + QString::number(screenPoint*horScale + screenOffset, 'f', 3) + " " + QString::number(vPos, 'f', 3) + " ");
		}
		//DebugDialog::stream() <<" ("<< time << "): " << voltage << ' ';
		screenPoint++;
	}
	svg += "' transform='translate(%1,%2)' stroke='"+ color + "' stroke-width='"+ strokeWidth + "' fill='none' /> \n"; //
	return svg;
}
