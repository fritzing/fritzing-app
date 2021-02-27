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
#include <QRegularExpression>
#include <QMessageBox>

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
#include "../items/ruler.h"
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

	QSettings settings;
	int enabled = settings.value("simulatorEnabled", 0).toInt();
	enable(enabled);
}

Simulator::~Simulator() {
}

/**
 * Returns the status of the simulator (enabled/disabled).
 * @returns true if the simulator is enabled and false otherwise.
 */
bool Simulator::isEnabled() {
	return m_enabled;
}

/**
 * Enables or disables the simulator. If it is disabled, removes the simulation effects: the grey out of
 * the parts that are not simulated and the messages previously added.
 * @param[in] enable boolean to indicate if the simulator needs to be enabled or disabled.
 */
void Simulator::enable(bool enable) {
	m_enabled = enable;
	if (!m_enabled) {
		removeSimItems();
	}
}

/**
 * Resets the simulator and removes the simulation effects: the grey out of
 * the parts that are not simulated and the messages previously added.
 */
void Simulator::reset() {
	removeSimItems();
}

/**
 * Main function that is in charge of simulating the circuit and show components working out of its specifications.
 * Components working outside its specifications are shown by adding a smoke image over them.
 * The steps performed are:
 * - Creates an instance of the Ngspice simulator (if it was not created before)
 * - Gets the current spice netlist
 * - Loads the netlist in Ngspice
 * - Runs a operating point analysis in a background thread
 * - Remove all previous items placed by the simulator (smokes, messages in the multimeters, etc.)
 * - Grey out the parts that are not being simulated
 * - Wait until the simulation has finished (timeout of 3s)
 * - Iterate for all parts being simulated to
 *     - Check if they work within specifications, add smoke if needed
 *     - Update display messages in the multimeters
 *     - Update LEDs colours
 *
 * Excludes all the parts that don not have spice models or that they are not connected to other parts
 * @brief Simulate the current circuit and check for components working out of specifications
 */
void Simulator::simulate() {
	if (!m_enabled) {
		std::cout << "The simulator is not enabled" << std::endl;
		return;
	}

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

	//Generate a hash table to find the net of specific connectors
	std::cout << "Generate a hash table to find the net of specific connectors" <<std::endl;
	m_connector2netHash.clear();
	for (int i=0; i<netList.size(); i++) {
		QList<ConnectorItem *> * net = netList.at(i);
		foreach (ConnectorItem * ci, *net) {
			m_connector2netHash.insert(ci, i);
		}
	}
	std::cout << "-----------------------------------" <<std::endl;


	//Generate a hash table to find the breadboard parts from parts in the schematic view
	std::cout << "Generate a hash table to find the breadboard parts from parts in the schematic view" <<std::endl;
	m_sch2bbItemHash.clear();
	foreach (ItemBase* schPart, itemBases) {
		m_instanceTitleSim->append(schPart->instanceTitle());
		foreach (QGraphicsItem * bbItem, m_breadboardGraphicsView->scene()->items()) {
			ItemBase * bbPart = dynamic_cast<ItemBase *>(bbItem);
			if (!bbPart) continue;
			if (schPart->instanceTitle().compare(bbPart->instanceTitle()) == 0) {
				m_sch2bbItemHash.insert(schPart, bbPart);
			}
		}
	}
	std::cout << "-----------------------------------" <<std::endl;

	//Removes the items added by the simulator last time it run (smoke, displayed text in multimeters, etc.)
	std::cout << "removeSimItems(itemBases);" <<std::endl;
	removeSimItems();
	std::cout << "-----------------------------------" <<std::endl;

	//If there are parts that are not being simulated, grey them out
	std::cout << "greyOutNonSimParts(itemBases);" <<std::endl;
	greyOutNonSimParts(itemBases);
	std::cout << "-----------------------------------" <<std::endl;

	std::cout << "Waiting for simulator thread to stop" <<std::endl;
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
	} else {
		std::cout << "The spice simulator has finished." <<std::endl;
	}
	std::cout << "-----------------------------------" <<std::endl;

	if (m_simulator->ErrorFound()) {
		//Ngspice found an error, do not continue
		std::cout << "Fatal error found, stopping the simulation." <<std::endl;
		removeSimItems();
		QWidget * tempWidget = new QWidget();
		QMessageBox::information(tempWidget, tr("Simulator Error"),
								 tr("The simulator gave an error when trying to simulate this circuit. "
									"Please, check the wiring and try again."));
		delete tempWidget;
		return;
	}
	//The spice simulation has finished, iterate over each part being simulated and update it (if it is necessary).
	//This loops is in charge of:
	// * update the multimeters screen
	// * add smoke to a part if something is out of its specifications
	// * update the brightness of the LEDs
	foreach (ItemBase * part, itemBases){
		//Remove the effects, if any
		part->setGraphicsEffect(NULL);
		m_sch2bbItemHash.value(part)->setGraphicsEffect(NULL);

		std::cout << "-----------------------------------" <<std::endl;
		std::cout << "Instance Title: " << part->instanceTitle().toStdString() << std::endl;

		QString family = part->family().toLower();

		if (family.contains("capacitor")) {
			updateCapacitor(part);
			continue;
		}
		if (family.contains("diode")) {
			updateDiode(part);
			continue;
		}
		if (family.contains("led")) {
			updateLED(part);
			continue;
		}
		if (family.contains("resistor")) {
			updateResistor(part);
			continue;
		}
		if (family.contains("multimeter")) {
			updateMultimeter(part);
			continue;
		}
		if (family.contains("dc motor")) {
			updateDcMotor(part);
			continue;
		}
		if (family.contains("line sensor") || family.contains("distance sensor")) {
			updateIRSensor(part);
			continue;
		}
		if (family.contains("battery")) {
			updateBattery(part);
			continue;
		}

	}

	//Delete the pointers
	foreach (QList<ConnectorItem *> * net, netList) {
		delete net;
	}
	netList.clear();
}

/**
 * Adds an smoke image on top of a part in the breadboard and schematic views.
 * @param[in] part Part where the smoke is going to be placed
 */
void Simulator::drawSmoke(ItemBase* part) {
	QGraphicsSvgItem * bbSmoke = new QGraphicsSvgItem(":resources/images/smoke.svg", m_sch2bbItemHash.value(part));
	QGraphicsSvgItem * schSmoke = new QGraphicsSvgItem(":resources/images/smoke.svg", part);
	if (!bbSmoke || !schSmoke) return;

	schSmoke->setZValue(DBL_MAX);
	bbSmoke->setZValue(DBL_MAX);
	part->addSimulationGraphicsItem(schSmoke);
	m_sch2bbItemHash.value(part)->addSimulationGraphicsItem(bbSmoke);
}

/**
 * Adds a message to the display of a multimeter. It does not update the message, it just adds
 * some text (the previous message is removed at the beginning of the simulation). The message
 * is displayed in a 7-segments font.
 * @param[in] multimeter The part where the message is going to be displayed
 * @param[in] msg The message to be displayed
 */
void Simulator::updateMultimeterScreen(ItemBase * multimeter, QString msg){
	//The '.' does not occupy a position in the screen (is printed with the previous number)
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
//	m_bbSimItems.append(bbScreen);
	//m_schSimItems.append(schScreen);
	//schScreen->setPos(QPointF(10,10));
	//schScreen->setZValue(DBL_MAX);
	QFont font("Segment7", 30, QFont::Normal);
	bbScreen->setFont(font);
	//bbScreen->setScale(4);
	bbScreen->setPos(QPointF(15,20));
	bbScreen->setZValue(DBL_MAX);
	m_sch2bbItemHash.value(multimeter)->addSimulationGraphicsItem(bbScreen);
}

/**
 * Removes all the items (images and texts) and effects (grey out) that have been placed
 * in previous simulations in the breadboard and schematic views.
 */
void Simulator::removeSimItems() {
	removeSimItems(m_schematicGraphicsView->scene()->items());
	removeSimItems(m_breadboardGraphicsView->scene()->items());
}

/**
 * Removes all the items (images and texts) and effects (grey out)
 * from the specified list of QGraphicsItem.
 */
void Simulator::removeSimItems(QList<QGraphicsItem *> items) {
	foreach (QGraphicsItem * item, items) {
		item->setGraphicsEffect(NULL);
		ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase) {
			itemBase->removeSimulationGraphicsItem();
			if (itemBase->viewID() == ViewLayer::ViewID::BreadboardView) {
				LED * led = dynamic_cast<LED *>(item);
				if (led) {
					led->resetBrightness();
				}
			}
		}
	}
}

/**
 * Returns the voltage between two connectors.
 * @param[in] c0 the first connector
 * @param[in] c1 the second connector
 * @returns the voltage between the connector c0 and c1
 */
double Simulator::calculateVoltage(ConnectorItem * c0, ConnectorItem * c1) {
	int net0 = m_connector2netHash.value(c0);
	int net1 = m_connector2netHash.value(c1);

	QString net0str = QString("v(%1)").arg(net0);
	QString net1str = QString("v(%1)").arg(net1);
	//std::cout << "net0str: " << net0str.toStdString() <<std::endl;
	//std::cout << "net1str: " << net1str.toStdString() <<std::endl;

	double volt0 = 0.0, volt1 = 0.0;
	if (net0!=0) volt0 = m_simulator->GetDataPoint(net0str.toStdString());
	if (net1!=0) volt1 = m_simulator->GetDataPoint(net1str.toStdString());
	return volt0-volt1;
}

/**
 * Returns the symbol of a part´s property. It is needed to be able to remove the symbol from the value of the property.
 * @param[in] part The part that has a property
 * @param[in] property The property
 * @returns the symbol for that property
 */
QString Simulator::getSymbol(ItemBase* part, QString property) {
	//Find the symbol of this property, TODO: is there an easy way of doing this?
	QHash<PropertyDef *, QString> propertyDefs;
	PropertyDefMaster::initPropertyDefs(part->modelPart(), propertyDefs);
	foreach (PropertyDef * propertyDef, propertyDefs.keys()) {
		if (property.compare(propertyDef->name, Qt::CaseInsensitive) == 0) {
			return propertyDef->symbol;
		}
	}
	return "";
}

/**
 * Returns the type of component of the first´s spice line. It is better to use the family field of a part
 * to determine what kind of device is. This is because a part can have several spice lines.
 * @param[in] part The part to get the type of spice component
 * @returns a character that represents the type of device (R->Resistor, D-Diode, C-capacitor, etc.)
 */
QChar Simulator::getDeviceType (ItemBase* part) {
	int index = part->spice().indexOf("{instanceTitle}");
	if (index > 0) {
		return part->spice().at(index-1).toLower();
	}
	QString msg = QString("Error getting the device type. The type is not recognized. Part=%1, Spice line=%2").arg(part->instanceTitle()).arg(part->spice());
	//TODO: Add tr()
	std::cout << msg.toStdString() << std::endl;
	throw msg.toStdString();
	return QChar('0');
}

/**
 * Returns the maximum value of a part´s property.
 * @param[in] part The part that has a property
 * @param[in] property The name of property.
 * @returns the value of the property for the part. If it is empty, returns the maximum value allowed.
 */
double Simulator::getMaxPropValue(ItemBase *part, QString property) {
	double value;
	QString propertyStr = part->getProperty(property);
	QString symbol = getSymbol(part, property);

	if(propertyStr.isEmpty()) {
		value = DBL_MAX;
	} else {
		if (!symbol.isEmpty()) {
			value = TextUtils::convertFromPowerPrefix(propertyStr, symbol);
		} else {
			//Attempt to remove the symbol: Remove all the letters, except the multipliers
			propertyStr.remove(QRegularExpression("[^pnu\x00B5mkMGT^\\d.]"));
			value = TextUtils::convertFromPowerPrefix(propertyStr, symbol);
		}
	}
	return value;
}

/**
 * Returns the power that a part is consuming/producing.
 * The subpartName is used for multiple spice lines for a part. For example, a potentiometer (R1)
 * is a part that has two spice components, each of them in a spice line (R1A and R1B). Therefore,
 * to get the power through R1A, the subpart parameter should be "A".
 * Note that not all the spice devices are able to return the power.
 * @param[in] part The part to get the power
 * @param[in] subpartName The name of the subpart. Leave it empty if there is only one spice line for the device. Otherwise, give the suffix of the subpart.
 * @returns the power that a part is consuming/producing.
 */
double Simulator::getPower(ItemBase* part, QString subpartName) {
	//TODO: Handle devices that do not return the power
	QString instanceStr = part->instanceTitle().toLower();
	instanceStr.append(subpartName);
	instanceStr.prepend("@");
	instanceStr.append("[p]");
	return m_simulator->GetDataPoint(instanceStr.toStdString());
}

/**
 * Returns the current that flows through a part.
 * The subpartName is used for multiple spice lines for a part. For example, a potentiometer (R1)
 * is a part that has two spice components, each of them in a spice line (R1A and R1B). Therefore,
 * to get the current that flows through R1A, the subpart parameter should be "A".
 * Note that this function only works for a few spice components: resistors, capacitors, inductors,
 * diodes (LEDs included) and voltage and current sources.
 * @param[in] part The part to get the current
 * @param[in] subpartName The name of the subpart. Leave it empty if there is only one spice line for the device. Otherwise, give the suffix of the subpart.
 * @returns the current that a part is consuming/producing.
 */
double Simulator::getCurrent(ItemBase* part, QString subpartName) {
	QString instanceStr = part->instanceTitle().toLower();
	instanceStr.append(subpartName);

	QChar deviceType = getDeviceType(part);
	//std::cout << "deviceType: " << deviceType.toLatin1() <<std::endl;
	if (deviceType == instanceStr.at(0)) {
		instanceStr.prepend(QString("@"));
	} else {
		//f. ex. Leds are DLED1 in ngpice and LED1 in Fritzing
		instanceStr.prepend(QString("@%1").arg(deviceType));
	}
	switch (deviceType.toLatin1()) {
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
		//TODO: Add tr()
		throw QString("Error getting the current of the device.The device type is not recognized. First letter is ").arg(deviceType);
		break;

	}
	return m_simulator->GetDataPoint(instanceStr.toStdString());
}

/**
 * Returns the current that flows through a transistor.
 * @param[in] spicePartName The name of the spice transistor.
 * @returns the current that the transistor is sinking/sourcing.
 */
double Simulator::getTransistorCurrent(QString spicePartName, TransistorLeg leg) {
	if(spicePartName.at(0).toLower()!="q") {
		//TODO: Add tr()
		throw QString("Error getting the current of a transistor. The device is not a transistor, its first letter is not a Q. Name: %1").arg(spicePartName);
	}
	spicePartName.prepend(QString("@"));
	switch (leg) {
		case BASE:
			spicePartName.append("[ib]");
			break;
		case COLLECTOR:
			spicePartName.append("[ic]");
			break;
		case EMITER:
			spicePartName.append("[ie]");
			break;
		default:
		throw QString("Error getting the current of a transistor. The transistor leg or property is not recognized. Leg: %1").arg(leg);
	}

	return m_simulator->GetDataPoint(spicePartName.toStdString());
}

/**
 * Greys out the parts that are not being simulated to inform the user.
 * @param[in] simParts A set of parts that are being simulated.
 */
void Simulator::greyOutNonSimParts(const QSet<ItemBase *>& simParts) {
	//Find the parts that are not being simulated.
	//First, get all the parts from the scenes...
	QList<QGraphicsItem *> noSimSchParts = m_schematicGraphicsView->scene()->items();
	QList<QGraphicsItem *> noSimBbParts = m_breadboardGraphicsView->scene()->items();


	//Remove the parts that are going to be simulated and the wires connected to them
	QList<ConnectorItem *> bbConnectors;
	foreach (ItemBase * part, simParts) {
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

/**
 * Greys out the parts that are passed.
 * @param[in] parts A list of parts to grey out.
 */
void Simulator::greyOutParts(const QList<QGraphicsItem*> & parts) {
	foreach (QGraphicsItem * part, parts){
		QGraphicsColorizeEffect * schEffect = new QGraphicsColorizeEffect();
		schEffect->setColor(QColor(100,100,100));
		part->setGraphicsEffect(schEffect);
	}
}

/**
 * Removes items that are being simulated but without spice lines. Basically, remove
 * the wires and the breadboards, which are part of the simulation and leave the rest.
 * @param[in/out] parts A list of parts which will be filtered to remove parts that
 * are being simulated
 */
void Simulator::removeItemsToBeSimulated(QList<QGraphicsItem*> & parts) {
	foreach (QGraphicsItem * part, parts){
		Wire* wire = dynamic_cast<Wire *>(part);
		if (wire) {
			parts.removeAll(part);
			continue;
		}
		Note* note = dynamic_cast<Note *>(part);
		if (note) {
			parts.removeAll(part);
			continue;
		}

		Ruler* ruler = dynamic_cast<Ruler *>(part);
		if (ruler) {
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

/*********************************************************************************************************************/
/*                          Update functions for the different parts												 */
/* *******************************************************************************************************************/

/**
 * Updates and checks a diode. Checks that the power is less than the maximum power.
 * @param[in] diode A part that is going to be checked and updated.
 */
void Simulator::updateDiode(ItemBase * diode) {
	double maxPower = getMaxPropValue(diode, "power");
	double power = getPower(diode);
	if (power > maxPower) {
		drawSmoke(diode);
	}
}

/**
 * Updates and checks an LED. Checks that the current is less than the maximum current
 * and updates the brightness of the LED in the breadboard view.
 * @param[in] part An LED that is going to be checked and updated.
 */
void Simulator::updateLED(ItemBase * part) {
	LED* led = dynamic_cast<LED *>(part);
	if (led) {
		double curr = getCurrent(part);
		double maxCurr = getMaxPropValue(part, "current");

		std::cout << "LED Current: " <<curr<<std::endl;
		std::cout << "LED MaxCurrent: " <<maxCurr<<std::endl;
		if (curr > maxCurr) {
			drawSmoke(part);
		}
		LED* bbLed = dynamic_cast<LED *>(m_sch2bbItemHash.value(part));
		bbLed->setBrightness(curr/maxCurr);
	} else {
		//It is probably an LED display (LED matrix)
		//TODO: Add spice lines to the part and handle this here
	}
}

/**
 * Updates and checks a capacitor. Checks that the voltage is less than the maximum voltage
 * and reverse voltage in electrolytic and tantalum capacitors (unidirectional capacitors).
 * @param[in] part A capacitor that is going to be checked and updated.
 */
void Simulator::updateCapacitor(ItemBase * part) {
	QString family = part->getProperty("family").toLower();

	ConnectorItem * negLeg, * posLeg;
	QList<ConnectorItem *> legs = part->cachedConnectorItems();
	foreach(ConnectorItem * ci, legs) {
		if(ci->connectorSharedName().toLower().compare("+") == 0) posLeg = ci;
		if(ci->connectorSharedName().toLower().compare("-") == 0) negLeg = ci;
	}
	if(!negLeg || !posLeg )
		return;

	double maxV = getMaxPropValue(part, "voltage");
	double v = calculateVoltage(posLeg, negLeg);
	std::cout << "MaxVoltage of the capacitor: " << maxV << std::endl;
	std::cout << "Capacitor voltage is : " << QString("%1").arg(v).toStdString() << std::endl;

	if (family.contains("bidirectional")) {
		//This is a ceramic capacitor (or not polarized)
		if (abs(v) > maxV) {
			drawSmoke(part);
		}
	} else {
		//This is an electrolytic o tantalum capacitor (polarized)
		if (v > maxV/2 || v < 0) {
			drawSmoke(part);
		}
	}
}

/**
 * Updates and checks a resistor. Checks that the power is less than the maximum power.
 * @param[in] part A resistor that is going to be checked and updated.
 */
void Simulator::updateResistor(ItemBase * part) {
	double maxPower = getMaxPropValue(part, "power");
	double power = getPower(part);
	std::cout << "Power: " << power <<std::endl;
	if (power > maxPower) {
		drawSmoke(part);
	}
}

/**
 * Updates and checks a potentiometer. Checks that the power is less than the maximum power
 * for the two resistors "A" and "B".
 * @param[in] part A potentiometer that is going to be checked and updated.
 */
void Simulator::updatePotentiometer(ItemBase * part) {
	double maxPower = getMaxPropValue(part, "power");
	double powerA = getPower(part, "A"); //power through resistor A
	double powerB = getPower(part, "B"); //power through resistor B
	double power = max(powerA, powerB);
	if (power > maxPower) {
		drawSmoke(part);
	}
}

/**
 * Updates and checks a battery. Checks that there are no short circuits.
 * @param[in] part A battery that is going to be checked and updated.
 */
void Simulator::updateBattery(ItemBase * part) {
	double maxCurrent = 500; //TODO: Use the capacity of the battery and multiply by 30.
	double current = getCurrent(part); //current that the battery delivers
	//std::cout << "Battery: MaxCurr=" << maxCurrent << ", Curr=" << current  <<std::endl;
	if (abs(current) > maxCurrent) {
		drawSmoke(part);
	}
}

/**
 * Updates and checks a IR sensor. Checks that the voltage is between the allowed range
 * and that the current of the output is less than  the maximum.
 * @param[in] part A IR sensor that is going to be checked and updated.
 */
void Simulator::updateIRSensor(ItemBase * part) {
	double maxV = getMaxPropValue(part, "voltage (max)");
	double minV = getMaxPropValue(part, "voltage (min)");
	double maxIout = getMaxPropValue(part, "max output current");
	std::cout << "IR sensor VCC range: " << maxV << " " << minV << std::endl;
	ConnectorItem *gnd, *vcc, *out;
	QList<ConnectorItem *> terminals = part->cachedConnectorItems();
	foreach(ConnectorItem * ci, terminals) {
		if(ci->connectorSharedDescription().toLower().compare("vcc") == 0 ||
				ci->connectorSharedDescription().toLower().compare("supply voltage") ==0)
			vcc = ci;
		if(ci->connectorSharedDescription().toLower().compare("gnd") == 0 ||
				ci->connectorSharedDescription().toLower().compare("ground") ==0)
			gnd = ci;
		if(ci->connectorSharedDescription().toLower().compare("out") == 0 ||
				ci->connectorSharedDescription().toLower().compare("output voltage") ==0) out = ci;
	}
	if(!gnd || !vcc || !out )
		return;

	double v = calculateVoltage(vcc, gnd); //voltage applied to the motor
	double i;
	if (part->family().contains("line sensor")) {
		//digital sensor (push-pull output)
		QString spicename = part->instanceTitle().toLower();
		spicename.prepend("q");
		i = getTransistorCurrent(spicename, COLLECTOR); //voltage applied to the motor
	} else {
		//analogue sensor (modelled by a voltage source and a resistor)
		i = getCurrent(part, "a"); //voltage applied to the motor
	}
	std::cout << "IR sensor Max Iout: " << maxIout << ", current Iout " << i << std::endl;
	if (v > maxV || v < 0 || abs(i) > maxIout) {
		drawSmoke(part);
		return;
	}
}

/**
 * Updates and checks a DC motor. Checks that the voltage is less than the maximum voltage.
 * If the voltage is bigger than the minimum, it plots an arrow to indicate that is turning.
 * TODO: The number of arrows are proportional to the voltage applied.
 * @param[in] part A DC motor that is going to be checked and updated.
 */
void Simulator::updateDcMotor(ItemBase * part) {
	double maxV = getMaxPropValue(part, "voltage (max)");
	double minV = getMaxPropValue(part, "voltage (min)");
	std::cout << "Motor1: " << std::endl;
	ConnectorItem * terminal1, * terminal2;
	QList<ConnectorItem *> probes = part->cachedConnectorItems();
	foreach(ConnectorItem * ci, probes) {
		if(ci->connectorSharedName().toLower().compare("pin 1") == 0) terminal1 = ci;
		if(ci->connectorSharedName().toLower().compare("pin 2") == 0) terminal2 = ci;
	}
	if(!terminal1 || !terminal2 )
		return;

	double v = calculateVoltage(terminal1, terminal2); //voltage applied to the motor
	if (abs(v) > maxV) {
		drawSmoke(part);
		return;
	}
	if (abs(v) >= minV) {
		std::cout << "motor rotates " << std::endl;
		QGraphicsSvgItem * bbRotate;
		QGraphicsSvgItem * schRotate;
		QString image;
		if(v > 0) {
			image = QString(":resources/images/rotateCW.svg");
		} else {
			image = QString(":resources/images/rotateCCW.svg");
		}
		bbRotate = new QGraphicsSvgItem(image, m_sch2bbItemHash.value(part));
		schRotate = new QGraphicsSvgItem(image, part);
		if (!bbRotate || !schRotate) return;

		schRotate->setZValue(DBL_MAX);
		bbRotate->setZValue(DBL_MAX);
		part->addSimulationGraphicsItem(schRotate);
		m_sch2bbItemHash.value(part)->addSimulationGraphicsItem(bbRotate);
	}
}

/**
 * Updates and checks a multimeter. Checks that there are not 3 probes connected.
 * Calculates the parameter to measure and updates the display of the multimeter.
 * @param[in] part A multimeter that is going to be checked and updated.
 */
void Simulator::updateMultimeter(ItemBase * part) {
	QString variant = part->getProperty("variant").toLower();
	ConnectorItem * comProbe, * vProbe, * aProbe;
	QList<ConnectorItem *> probes = part->cachedConnectorItems();
	foreach(ConnectorItem * ci, probes) {
		if(ci->connectorSharedName().toLower().compare("com probe") == 0) comProbe = ci;
		if(ci->connectorSharedName().toLower().compare("v probe") == 0) vProbe = ci;
		if(ci->connectorSharedName().toLower().compare("a probe") == 0) aProbe = ci;
	}
	if(!comProbe || !vProbe || !aProbe)
		return;

	if(comProbe->connectedToWires() && vProbe->connectedToWires() && aProbe->connectedToWires()) {
		std::cout << "Multimeter (v_dc) connected with three terminals. " << std::endl;
		updateMultimeterScreen(part, "ERR");
		return;
	}

	if (variant.compare("voltmeter (dc)") == 0) {
		std::cout << "Multimeter (v_dc) found. " << std::endl;
		if(aProbe->connectedToWires()) {
			std::cout << "Multimeter (v_dc) has the current terminal connected. " << std::endl;
			updateMultimeterScreen(part, "ERR");
			return;
		}
		if(comProbe->connectedToWires() && vProbe->connectedToWires()) {
			std::cout << "Multimeter (v_dc) connected with two terminals. " << std::endl;
			double v = calculateVoltage(vProbe, comProbe);
			updateMultimeterScreen(part, QString::number(v, 'f', 2));
		}
		return;
	} else if (variant.compare("ammeter (dc)") == 0) {
		std::cout << "Multimeter (c_dc) found. " << std::endl;
		if(vProbe->connectedToWires()) {
			std::cout << "Multimeter (c_dc) has the voltage terminal connected. " << std::endl;
			updateMultimeterScreen(part, "ERR");
			return;
		}
		updateMultimeterScreen(part, QString::number(getCurrent(part), 'f', 3));
		return;
	} else if (variant.compare("ohmmeter") == 0) {
		std::cout << "Ohmmeter found. " << std::endl;
		if(aProbe->connectedToWires()) {
			std::cout << "Ohmmeter has the current terminal connected. " << std::endl;
			updateMultimeterScreen(part, "ERR");
			return;
		}
		double v = calculateVoltage(vProbe, comProbe);
		double a = getCurrent(part);
		double r = abs(v/a);
		std::cout << "Ohmmeter: Volt: " << v <<", Curr: " << a <<", Ohm: " << r << std::endl;
		int precision = 1;
		if (r < 1)
			precision = 2;
		if (r >= 200)
			precision = 0;
		//TODO: Handle resistances bigger than 10k
		updateMultimeterScreen(part, QString::number(r, 'f', precision));
		return;
	}
}
