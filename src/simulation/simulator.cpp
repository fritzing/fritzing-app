/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2020 Fritzing

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

#include "simulator.h"
#include <QtCore>

#include <QSvgGenerator>
#include <QColor>
#include <QImageWriter>
#include <QPrinter>
#include <QSettings>
#include <QDesktopServices>
#include <QPrintDialog>
#include <QClipboard>
#include <QApplication>
#include <QGraphicsColorizeEffect>

#include "../mainwindow/mainwindow.h"
#include "../debugdialog.h"
#include "../waitpushundostack.h"
#include "../help/aboutbox.h"
#include "../autoroute/autorouteprogressdialog.h"
#include "../items/virtualwire.h"
#include "../items/jumperitem.h"
#include "../items/via.h"
#include "../fsvgrenderer.h"
#include "../items/note.h"
#include "../items/partfactory.h"
#include "../eagle/fritzing2eagle.h"
#include "../sketch/breadboardsketchwidget.h"
#include "../sketch/schematicsketchwidget.h"
#include "../sketch/pcbsketchwidget.h"
#include "../partsbinpalette/binmanager/binmanager.h"
#include "../utils/expandinglabel.h"
#include "../infoview/htmlinfoview.h"
#include "../utils/bendpointaction.h"
#include "../sketch/fgraphicsscene.h"
#include "../utils/fileprogressdialog.h"
#include "../svg/svgfilesplitter.h"
#include "../version/version.h"
#include "../help/tipsandtricks.h"
#include "../dialogs/setcolordialog.h"
#include "../utils/folderutils.h"
#include "../utils/graphicsutils.h"
#include "../utils/textutils.h"
#include "../connectors/ercdata.h"
#include "../items/moduleidnames.h"
#include "../utils/zoomslider.h"
#include "../dock/layerpalette.h"
#include "../program/programwindow.h"
#include "../utils/autoclosemessagebox.h"
#include "../svg/gerbergenerator.h"
#include "../processeventblocker.h"

#include "../simulation/ngspice.h"
#include "../simulation/spice_simulator.h"
#include "../simulation/sim_types.h"

#include "../items/led.h"
#include "../items/resistor.h"
#include "../items/wire.h"
#include "../items/breadboard.h"


#if defined(__MINGW32__) || defined(_MSC_VER)
#include <windows.h>  // for Sleep
#else
#include <unistd.h>
#include <ctype.h>
#endif
#include <iostream>

////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////
Simulator::Simulator(MainWindow *mainWindow) : QObject(mainWindow) {
	m_mainWindow = mainWindow;
	m_breadboardGraphicsView = dynamic_cast<BreadboardSketchWidget *>(mainWindow->sketchWidgets().at(0));
	m_schematicGraphicsView = dynamic_cast<SchematicSketchWidget *>(mainWindow->sketchWidgets().at(1));
	m_instanceTitleSim = new QList<QString>;
}

Simulator::~Simulator() {
}


void Simulator::simulate() {
	m_simulator = SPICE_SIMULATOR::CreateInstance( "ngspice" );
	if( !m_simulator )
	{
		throw std::runtime_error( "Could not create simulator instance" );
		return;
	}

	QList< QList<ConnectorItem *>* > netList;
	QSet<ItemBase *> itemBases;
	QString spiceNetlist = m_mainWindow->getSpiceNetlist("Simulator Netlist", netList, itemBases);

	std::cout << "Netlist: " << spiceNetlist.toStdString() << std::endl;
	std::cout << "-----------------------------------" <<std::endl;
	m_simulator->Command("remcirc");
	std::cout << "-----------------------------------" <<std::endl;
	m_simulator->Init();
	std::cout << "-----------------------------------" <<std::endl;
	std::string netlist = "Test simulator\nV1 1 0 5\nR1 1 2 100\nR2 2 0 100\n.OP\n.END";
	m_simulator->LoadNetlist( spiceNetlist.toStdString());
	std::cout << "-----------------------------------" <<std::endl;
	m_simulator->Command("listing");
	std::cout << "-----------------------------------" <<std::endl;
	m_simulator->Run();
	std::cout << "-----------------------------------" <<std::endl;

	//While the spice simulator runs, we will perform some tasks:

	//Removes the items added by the simulator last time it run (smoke, displayed text in multimeters, etc.)
	removeSimItems();

	//Generate a hash table to find the net of specific connectors
	m_connector2netHash.clear();
	for (int i=0; i<netList.size(); i++) {
		QList<ConnectorItem *> * net = netList.at(i);
		foreach (ConnectorItem * ci, *net) {
			m_connector2netHash.insert(ci, i);
		}
	}


	//Generate a hash table to find the breadboard parts from parts in the schematic view
	m_sch2bbItemHash.clear();
	foreach(ItemBase* schPart, itemBases) {
		m_instanceTitleSim->append(schPart->instanceTitle());
		foreach (QGraphicsItem * bbItem, m_breadboardGraphicsView->scene()->items()) {
			ItemBase * bbPart = dynamic_cast<ItemBase *>(bbItem);
			if (!bbPart) continue;
			if (schPart->instanceTitle().compare(bbPart->instanceTitle()) == 0) {
				m_sch2bbItemHash.insert(schPart, bbPart);
			}
		}
	}

	//If there are parts that are not being simulated, grey them out
	greyOutNonSimParts(itemBases);


	int elapsedTime = 0, simTimeOut = 3000; // in ms
	while (m_simulator->IsRunning() && elapsedTime < simTimeOut) {
	#if defined(__MINGW32__) || defined(_MSC_VER)
		Sleep(1);
	#else
		usleep(1000);
	#endif
		elapsedTime++;
	}
	if (elapsedTime >= simTimeOut) {
		m_simulator->Stop();
		throw std::runtime_error( QString("The spice simulator did not finish after %1 ms. Aborting simulation.").arg(simTimeOut).toStdString() );
		return;
	}

	//The spice simulation has finished, iterate over each part being simulated and update it (if it is necessary).
	//This loops is in charge of:
	// * update the multimeters sreen
	// * add smoke to a part if something is out of its specifications
	// * update the brighness of the LEDs
	foreach (ItemBase * part, itemBases){
		//Remove the effects, if any
		part->setGraphicsEffect(NULL);
		m_sch2bbItemHash.value(part)->setGraphicsEffect(NULL);

		std::cout << "-----------------------------------" <<std::endl;
		std::cout << "Instance Title: " << part->instanceTitle().toStdString() << std::endl;
		std::cout << "viewLayerID: " << part->viewLayerID() << std::endl;
		LED* led = dynamic_cast<LED *>(part);
		if (led) {
			double curr = getCurrent(part);
			//TODO: Check that this is ok. Or should we use QLocale.toDouble()?
			//TODO: Handle metric prefixes
			bool ok;
			double maxCurr = led->getProperty("current").remove(QRegularExpression("[^.+\\-\\d]")).toDouble(&ok);
			if(!ok){
				QString maxCurrStr = led->getProperty("current").remove(QRegularExpression("[^.+-\\d]"));
				maxCurrStr.prepend("Error conveting the maxCurrent of the LED to double. String: ");
				throw (maxCurrStr);
				continue;
			}

			std::cout << "Current: " <<curr<<std::endl;
			std::cout << "MaxCurrent: " <<maxCurr<<std::endl;

			LED* bbLed = dynamic_cast<LED *>(m_sch2bbItemHash.value(part));
			bbLed->setBrightness(curr/maxCurr);

			if (curr > maxCurr) {
				drawSmoke(part);
			}
			continue;
		}
		Resistor* resistor = dynamic_cast<Resistor *>(part);
		if (resistor) {
			double curr = getCurrent(part);
			std::cout << "Current: " <<curr<<std::endl;
			QString instanceStr = resistor->instanceTitle().toLower();
			instanceStr.prepend("@");
			instanceStr.append("[p]");
			double power = m_simulator->GetDataPoint(instanceStr.toStdString());
			std::cout << "Power: " << power <<std::endl;

			double maxPower;
			QString powerStr = resistor->getProperty("power");
			if(powerStr.isEmpty()) {
				maxPower = DBL_MAX;
			} else {
				//TODO: Check that this is ok. Or should we use QLocale.toDouble()?
				//TODO: Handle metric prefixes
				bool ok;
				maxPower = powerStr.remove(QRegularExpression("[^.+\\-\\d]")).toDouble(&ok);
				if(!ok){
					powerStr.prepend("Error conveting the maxPower of the resistor to double. String: ");
					throw (powerStr);
					continue;
				}
				std::cout << "MaxPower through the resistor: " << powerStr.toStdString() << QString(" %1").arg(maxPower).toStdString() << std::endl;
			}

			if(resistor->cachedConnectorItems().size()>1){
				ConnectorItem * com = resistor->cachedConnectorItems().at(0);
				ConnectorItem * v = resistor->cachedConnectorItems().at(1);

				double voltage = abs(calculateVoltage(v, com));
				std::cout << "Voltage through the resistor: " << voltage <<std::endl;

				if (power > maxPower) {
					drawSmoke(resistor);
				}
			}
			continue;
		}

		QString family = part->family().toLower();
		if (family.compare("multimeter") == 0) {
			std::cout << "Multimeter found. " << std::endl;
			QString type = part->getProperty("type").toLower(); //TODO: change to type

			ConnectorItem * comProbe, * vProbe, * aProbe;
			QList<ConnectorItem *> probes = part->cachedConnectorItems();
			foreach(ConnectorItem * ci, probes) {
				if(ci->connectorSharedName().toLower().compare("com probe") == 0) comProbe = ci;
				if(ci->connectorSharedName().toLower().compare("v probe") == 0) vProbe = ci;
				if(ci->connectorSharedName().toLower().compare("a probe") == 0) aProbe = ci;
			}
			if(!comProbe || !vProbe || !aProbe)
				continue;

			if(comProbe->connectedToWires() && vProbe->connectedToWires() && aProbe->connectedToWires()) {
				std::cout << "Multimeter (v_dc) connected with three terminals. " << std::endl;
				updateMultimeterScreen(part, "ERR");
				continue;
			}

			if (type.compare("v_dc") == 0) {
				std::cout << "Multimeter (v_dc) found. " << std::endl;
				if(comProbe->connectedToWires() && vProbe->connectedToWires()) {
					std::cout << "Multimeter (v_dc) connected with two terminals. " << std::endl;
					double v = calculateVoltage(vProbe, comProbe);
					updateMultimeterScreen(part, QString::number(v, 'f', 2));
				}
				continue;
			} else if (type.compare("c_dc") == 0) {
				std::cout << "Multimeter (c_dc) found. " << std::endl;
				updateMultimeterScreen(part, QString::number(getCurrent(part), 'f', 3));
			}

		}


	}

	//Delete the pointers
	foreach (QList<ConnectorItem *> * net, netList) {
		delete net;
	}
	netList.clear();

}

void Simulator::drawSmoke(ItemBase* part) {
	QGraphicsSvgItem * bbSmoke = new QGraphicsSvgItem(":resources/images/smoke.svg");
	QGraphicsSvgItem * schSmoke = new QGraphicsSvgItem(":resources/images/smoke.svg");
	if (!bbSmoke || !schSmoke) return;
	m_breadboardGraphicsView->scene()->addItem(bbSmoke);
	m_schematicGraphicsView->scene()->addItem(schSmoke);
	m_bbSimItems.append(bbSmoke);
	m_schSimItems.append(schSmoke);

	schSmoke->setPos(part->pos());
	schSmoke->setZValue(DBL_MAX);
	bbSmoke->setPos(m_sch2bbItemHash.value(part)->pos());
	bbSmoke->setZValue(DBL_MAX);
}

void Simulator::updateMultimeterScreen(ItemBase * multimeter, QString msg){
	//The '.' does not occuppy a position in the screen (is printed with the previous number)
	//So, do not take them into account to fill with spaces
	QString aux = QString(msg);
	aux.remove(QChar('.'));
	std::cout << "msg size: " << msg.size() <<std::endl;
	std::cout << "aux size: " << aux.size() <<std::endl;
	if(aux.size() < 5) {
		msg.prepend(QString(5-aux.size(),' '));
	}
	std::cout << "msg is now: " << msg.toStdString() <<std::endl;

	QGraphicsTextItem * bbScreen = new QGraphicsTextItem(msg, m_sch2bbItemHash.value(multimeter));
	//QGraphicsTextItem * schScreen = new QGraphicsTextItem(msg, multimeter);
	m_bbSimItems.append(bbScreen);
	//m_schSimItems.append(schScreen);
	//schScreen->setPos(QPointF(10,10));
	//schScreen->setZValue(DBL_MAX);
	QFont font("Segment7", 30, QFont::Normal);
	bbScreen->setFont(font);
	//bbScreen->setScale(4);
	bbScreen->setPos(QPointF(15,20));
	bbScreen->setZValue(DBL_MAX);
}

void Simulator::removeSimItems() {
	foreach(QGraphicsObject * item, m_bbSimItems) {
		m_breadboardGraphicsView->scene()->removeItem(item);
		delete item;
	}
	m_bbSimItems.clear();
	foreach(QGraphicsObject * item, m_schSimItems) {
		m_schematicGraphicsView->scene()->removeItem(item);
		delete item;
	}
	m_schSimItems.clear();
	m_instanceTitleSim->clear();
}

double Simulator::calculateVoltage(ConnectorItem * c0, ConnectorItem * c1) {
	int net0 = m_connector2netHash.value(c0);
	int net1 = m_connector2netHash.value(c1);

	QString net0str = QString("v(%1)").arg(net0);
	QString net1str = QString("v(%1)").arg(net1);
	std::cout << "net0str: " << net0str.toStdString() <<std::endl;
	std::cout << "net1str: " << net1str.toStdString() <<std::endl;

	double volt0 = 0.0, volt1 = 0.0;
	if (net0!=0) volt0 = m_simulator->GetDataPoint(net0str.toStdString());
	if (net1!=0) volt1 = m_simulator->GetDataPoint(net1str.toStdString());
	return volt0-volt1;
}

double Simulator::getCurrent(ItemBase* part) {
	QString instanceStr = part->instanceTitle().toLower();
	int index = part->spice().indexOf("{instanceTitle}");
	if (index > 0){
		QChar deviceType = part->spice().at(index-1).toLower();
		if(deviceType == instanceStr.at(0)) {
			instanceStr.prepend(QString("@"));
		} else {
			//f. ex. Leds are DLED1 in ngpice and LED1 in Fritzing
			instanceStr.prepend(QString("@%1").arg(deviceType));
		}
		switch(deviceType.toLatin1()){
			case 'd':
				instanceStr.append("[id]");
			break;
			case 'r': //resistors
			case 'c': //capacitors
			case 'l': //inductors
			case 'v': //voltage sources
			case 'e': //Voltage-controlled voltage source (VCVS)
			case 'f': //Current-controlled current source (CCCs)
			case 'g': //Voltage-controlled current source (VCCS)
			case 'h': //Current-controlled voltage source (CCVS)
			case 'i': //Current source
				instanceStr.append("[i]");
				break;
			default:
				throw tr("Error getting the current of the device.The device type is not recognized. First letter is ").arg(deviceType);
			break;

		}
		return m_simulator->GetDataPoint(instanceStr.toStdString());
	}
	throw tr("Error getting the current of the device.The device type is not recognized.");
	return 0;
}

void Simulator::greyOutNonSimParts(const QSet<ItemBase *>& simParts) {
	//Find the parts that are not being simulated.
	//First, get all the parts from the scenes...
	QList<QGraphicsItem *> noSimSchParts = m_schematicGraphicsView->scene()->items();
	QList<QGraphicsItem *> noSimBbParts = m_breadboardGraphicsView->scene()->items();


	//Remove the parts that are going to be simulated and the wires connected to them
	QList<ConnectorItem *> bbConnectors;
	foreach (ItemBase * part, simParts){
		noSimSchParts.removeAll(part);
		noSimBbParts.removeAll(m_sch2bbItemHash.value(part));
		bbConnectors.append(m_sch2bbItemHash.value(part)->cachedConnectorItems());

//		foreach (ConnectorItem * connectorItem, part->cachedConnectorItems()) {
//			QList<Wire *> wires;
//			QList<ConnectorItem *> ends;
//			Wire::collectChained(connectorItem, wires, ends);
//			foreach (Wire * wire, wires) {
//				noSimSchParts.removeAll(wire);
//			}
//		}
//		foreach (ConnectorItem * connectorItem, m_sch2bbItemHash.value(part)->cachedConnectorItems()) {
//			QList<Wire *> wires;
//			QList<ConnectorItem *> ends;
//			Wire::collectChained(connectorItem, wires, ends);
//			foreach (Wire * wire, wires) {
//				noSimBbParts.removeAll(wire);
//			}
//		}
	}

	//TODO: grey out the wires that are not connected to parts to be simulated
	removeItemsToBeSimulated(noSimSchParts);
	removeItemsToBeSimulated(noSimBbParts);

	//... and grey them out to indicate it
	greyOutParts(noSimSchParts);
	greyOutParts(noSimBbParts);
}

void Simulator::greyOutParts(const QList<QGraphicsItem*> & parts) {
	foreach (QGraphicsItem * part, parts){
		QGraphicsColorizeEffect * schEffect = new QGraphicsColorizeEffect();
		schEffect->setColor(QColor(100,100,100));
		part->setGraphicsEffect(schEffect);
	}
}

void Simulator::removeItemsToBeSimulated(QList<QGraphicsItem*> & parts) {
	foreach (QGraphicsItem * part, parts){
		Wire* wire = dynamic_cast<Wire *>(part);
		if (wire) {
			parts.removeAll(part);
			continue;
		}

		ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(part);
		if (connectorItem) {
			//The connectors and rubber legs are not wires, but we should nor get them out if they are
			//connected to a part that we are simulating
			if (m_instanceTitleSim->contains(connectorItem->attachedToInstanceTitle())) {
				parts.removeAll(part);
				continue;
			}
		}

		Breadboard* breadboard = dynamic_cast<Breadboard *>(part);
		if (breadboard) {
			parts.removeAll(part);
			continue;
//			if (bbConnectors.contains(wire->connector0()) ||
//									bbConnectors.contains(wire->connector1())) {
//				QList<Wire *> wires;
//				QList<ConnectorItem *> ends;
//				wire->collectChained(wires, ends);
//				foreach (Wire * wireToRemove, wires) {
//					noSimBbParts.removeAll(wireToRemove);
//				}
//			}
		}
	}
}
