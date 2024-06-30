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
#include <QTimer>
#include <iostream>

#include "../mainwindow/mainwindow.h"
#include "../items/note.h"
#include "../items/ruler.h"
#include "../sketch/breadboardsketchwidget.h"
#include "../sketch/schematicsketchwidget.h"
#include "../utils/fmessagebox.h"
#include "../utils/textutils.h"
#include "../simulation/ngspice_simulator.h"
#include "../items/led.h"
#include "../items/wire.h"
#include "../items/breadboard.h"
#include "../items/resizableboard.h"
#include "../items/symbolpaletteitem.h"
#include "../items/perfboard.h"
#include "../items/partlabel.h"

#include <ngspice/sharedspice.h>

#include <iostream>


/////////////////////////////////////////////////////////
Simulator::Simulator(MainWindow *mainWindow) : QObject(mainWindow) {
	m_mainWindow = mainWindow;
	m_breadboardGraphicsView = dynamic_cast<BreadboardSketchWidget *>(mainWindow->sketchWidgets().at(0));
	m_schematicGraphicsView = dynamic_cast<SchematicSketchWidget *>(mainWindow->sketchWidgets().at(1));

	m_simTimer = new QTimer(this);
	m_simTimer->setSingleShot(true);
	connect(m_simTimer, &QTimer::timeout, this, &Simulator::simulate);

	// Configure the timer to show the simulation results
	m_showResultsTimer = new QTimer(this);
	connect(m_showResultsTimer, &QTimer::timeout, this, &Simulator::showSimulationResults);

	enable(true);
	m_simulating = false;
}

Simulator::~Simulator() {
}

/**
 * This function triggers a simulation if the simulator has been created, and the
 * the simulator is sumulating. is Simulating is controlled by "Start Simulation" and
 * "Stop Simulator" buttons. Of corse, to be able to simulate, the simulator needs to
 * be enabled. This function can be called from everywhere in the code as it is a static.
 */
void Simulator::triggerSimulation()
{
	if(m_simulating) {
		resetTimer();
	}
}

/**
 * This function resets the timer of the simulation, which triggers a simulation after
 * the timeout. Several commands can trigger the simulation, and each of them will reset
 * the timer. Thus, the simulation will only be triggered once, even if a user action
 * calls several times to triggerSimulation.
 */
void Simulator::resetTimer(){
	m_simTimer->start(SimDelay);
}

/**
 * Returns the status of the simulator (enabled/disabled).
 * @returns true if the simulator is enabled and false otherwise.
 */
bool Simulator::isEnabled() {
	return m_enabled;
}

bool Simulator::isTransientSimulation() {
	return m_transientSimulationEnabled;
}

/**
 * Enables or disables the simulator. If it is disabled, removes the simulation effects: the grey out of
 * the parts that are not simulated and the messages previously added.
 * @param[in] enable boolean to indicate if the simulator needs to be enabled or disabled.
 */
void Simulator::enable(bool enable) {
	if (m_enabled != enable) {
		emit simulationEnabled(enable);
	}
	m_enabled = enable;
	if (!m_enabled) {
		removeSimItems();
	}
}

void Simulator::enableTransientSimulation(bool enable) {
	m_transientSimulationEnabled = enable;
}

/**
 * This function starts the simulator and triggers a simulation. Once the simulator has
 * been started, the simulaton will run after any user actions that modifies the circuit
 * (adding wires, parts, modifying property values, etc.)
 */
void Simulator::startSimulation()
{
	m_simulating = true;
	emit simulationStartedOrStopped(m_simulating);
	simulate();
}

/**
 * Stops the simulator (the simulator will not run if the user modifies the circuit) and
 * removes the simulation effects: the grey out effects on the parts that are not being,
 * simulated, the smoke images, and the messages on the multimeter.
 */
void Simulator::stopSimulation() {
	m_showResultsTimer->stop();
	m_simulating = false;
	removeSimItems();
	emit simulationStartedOrStopped(m_simulating);
	m_breadboardGraphicsView->setSimulatorMessage("");
	m_schematicGraphicsView->setSimulatorMessage("");
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
	if (!m_enabled || !m_simulating) {
		std::cout << "The simulator is not enabled or simulating" << std::endl;
		return;
	}

	m_simulator = NgSpiceSimulator::getInstance();
	try {
		m_simulator->init();
	}
	catch (std::exception& e) {
		FMessageBox::warning(nullptr, tr("Simulator Error"), tr("An error occurred when starting the simulation."));
		stopSimulation();
		return;
	}

	if( !m_simulator )
	{
		throw std::runtime_error( "Could not create simulator instance" );
		return;
	}

	//Empty the stderr and stdout buffers
	m_simulator->clearLog();

	QList< QList<ConnectorItem *>* > netList;
	itemBases.clear();
	QString spiceNetlist = m_mainWindow->getSpiceNetlist("Simulator Netlist", netList, itemBases);

	//Select the type of analysis based on if there is an oscilloscope in the simulation
	m_simEndTime = -1, m_simStartTime = std::numeric_limits<double>::max();;
	foreach (ItemBase * item, itemBases) {
		if(item->family().toLower().contains("oscilloscope")) {
			//TODO: Use TextUtils::convertFromPowerPrefixU function
			double time_div = TextUtils::convertFromPowerPrefix(item->getProperty("time/div"), "s");
			double pos = TextUtils::convertFromPowerPrefix(item->getProperty("horizontal position"), "s");
			std::cout << "Found oscilloscope: time/div: " << item->getProperty("time/div").toStdString() << " " << time_div << item->getProperty("horizontal position").toStdString() << " " << pos << std::endl;
			if (pos < m_simStartTime) {
				m_simStartTime = pos;
			}
			double maxSimTimeOsc = pos + time_div * 10;
			if (maxSimTimeOsc > m_simEndTime) {
				m_simEndTime = maxSimTimeOsc;
			}
		}
	}

	if (m_simEndTime <= 0 && m_transientSimulationEnabled) {
		m_simStartTime = 0;
		m_simEndTime = 10;
	}

	//Read the project properties
	QString timeStepModeStr = m_mainWindow->getProjectProperties()->getProjectProperty(ProjectPropertyKeySimulatorTimeStepMode);
	QString numStepsStr = m_mainWindow->getProjectProperties()->getProjectProperty(ProjectPropertyKeySimulatorNumberOfSteps);
	QString timeStepStr = m_mainWindow->getProjectProperties()->getProjectProperty(ProjectPropertyKeySimulatorTimeStepS);
	QString animationTimeStr = m_mainWindow->getProjectProperties()->getProjectProperty(ProjectPropertyKeySimulatorAnimationTimeS);

	std::cout << "" << timeStepModeStr.toStdString() << " " << numStepsStr.toStdString() << " " << timeStepStr.toStdString()
		  << " " << animationTimeStr.toStdString() << std::endl;
	if (m_simEndTime > 0 && m_mainWindow->isTransientSimulationEnabled()) {
		if (timeStepModeStr.contains("true", Qt::CaseInsensitive)) {
			m_simStepTime = TextUtils::convertFromPowerPrefixU(timeStepStr, "s");
			m_simNumberOfSteps = (m_simEndTime-m_simStartTime)/m_simStepTime;
		} else {
			m_simNumberOfSteps = TextUtils::convertFromPowerPrefixU(numStepsStr, "");
			m_simStepTime = (m_simEndTime-m_simStartTime)/m_simNumberOfSteps;
		}

		int timerInterval = TextUtils::convertFromPowerPrefixU(animationTimeStr, "s")/m_simNumberOfSteps*1000;
		m_showResultsTimer->setInterval(timerInterval);

		QString tranAnalysis = QString(".TRAN %1 %2 %3").arg(m_simStepTime).arg(m_simEndTime).arg(m_simStartTime);
		spiceNetlist.replace(".OP", tranAnalysis);
	}


	std::cout << "Netlist: " << spiceNetlist.toStdString() << std::endl;

	//std::cout << "-----------------------------------" <<std::endl;
	std::cout << "Running command(remcirc):" <<std::endl;
	m_simulator->command("remcirc");
	//std::cout << "-----------------------------------" <<std::endl;
	std::cout << "Running m_simulator->command('reset'):" <<std::endl;
	m_simulator->command("reset");
	m_simulator->clearLog();

	std::cout << "-----------------------------------" <<std::endl;
	std::cout << "Running LoadNetlist:" <<std::endl;

	m_simulator->loadCircuit(spiceNetlist.toStdString());

	if (QString::fromStdString(m_simulator->getLog(false)).toLower().contains("error") || // "error on line"
			QString::fromStdString(m_simulator->getLog(true)).toLower().contains("warning")) { // "warning, can't find model"
		//Ngspice found an error, do not continue
		std::cout << "Error loading the netlist. Probably some SPICE field is wrong, check them." <<std::endl;

		QMessageBox messageBox(QMessageBox::Warning, tr("Simulator Error"), tr("The simulator gave an error when loading the netlist. "
										       "Probably some SPICE field is wrong, please, check them.\n"
										       "If the parts are from the simulation bin, please, report the bug in GitHub "
										       "(https://github.com/fritzing/fritzing-app/issues) and copy the details available below."));
		messageBox.setDetailedText("Errors:\n" +
					   QString::fromStdString(m_simulator->getLog(false)) +
					   QString::fromStdString(m_simulator->getLog(true)) +
					   "\n\nNetlist:\n" + spiceNetlist);
		messageBox.setMinimumSize(400, 0);
		messageBox.exec();

		stopSimulation();
		return;
	}
	std::cout << "-----------------------------------" <<std::endl;
	std::cout << "Running command(listing):" <<std::endl;
	m_simulator->command("listing");
	std::cout << "-----------------------------------" <<std::endl;
	std::cout << "Running m_simulator->command(bg_run):" <<std::endl;
	m_simulator->resetIsBGThreadRunning();
	m_simulator->command("bg_run");
	std::cout << "-----------------------------------" <<std::endl;
	std::cout << "Generating a hash table to find the net of specific connectors:" <<std::endl;
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
	std::cout << "Generating a hash table to find the breadboard parts from parts in the schematic view:" <<std::endl;

	//Generate a hash table to find the breadboard parts from parts in the schematic view
	std::cout << "Generate a hash table to find the breadboard parts from parts in the schematic view" <<std::endl;
	m_sch2bbItemHash.clear();
	foreach (ItemBase* schPart, itemBases) {
		foreach (QGraphicsItem * bbItem, m_breadboardGraphicsView->scene()->items()) {
			ItemBase * bbPart = dynamic_cast<ItemBase *>(bbItem);
			if (!bbPart) continue;
			if (schPart->id() == bbPart->id()) {
				m_sch2bbItemHash.insert(schPart, bbPart);
			}
		}
	}
	std::cout << "-----------------------------------" <<std::endl;
	std::cout << "Removing the items added by the simulator last time it run (smoke, displayed text in multimeters, etc.):" <<std::endl;

	//Removes the items added by the simulator last time it run (smoke, displayed text in multimeters, etc.)
	std::cout << "removeSimItems(itemBases);" <<std::endl;
	removeSimItems();
	std::cout << "-----------------------------------" <<std::endl;
	std::cout << "If there are parts that are not being simulated, grey them out:" <<std::endl;

	//If there are parts that are not being simulated, grey them out
	std::cout << "greyOutNonSimParts(itemBases);" <<std::endl;
	greyOutNonSimParts(itemBases);
	std::cout << "-----------------------------------" <<std::endl;

	std::cout << "Waiting for simulator thread to stop" <<std::endl;
	int elapsedTime = 0, simTimeOut = 3000; // in ms
	while (m_simulator->isBGThreadRunning() && elapsedTime < simTimeOut) {
		auto timeInfo = m_simulator->getVecInfo(QString("time").toStdString());
		QThread::usleep(100);
		elapsedTime++;
		//If this a transitory simulation and we have partial results, start the animation
		if (m_simEndTime > 0 && timeInfo.size() > 0)
			break;
	}
	std::cout << "-------- SIM END or TRANS SIM WITH PARTIAL RESULTS ------------" << std::endl;

	if (elapsedTime >= simTimeOut) {
		m_simulator->command("bg_halt");
		stopSimulation();
		FMessageBox::warning(m_mainWindow, tr("Simulator Timeout"), tr("The spice simulator did not finish after %1 ms. Aborting simulation.").arg(simTimeOut));
		return;
	} else {
		std::cout << "The spice simulator has finished." <<std::endl;
	}
	std::cout << "-----------------------------------" <<std::endl;

	if (m_simulator->errorOccured() ||
			QString::fromStdString(m_simulator->getLog(true)).toLower().contains("there aren't any circuits loaded")) {
		//Ngspice found an error, do not continue
		std::cout << "Fatal error found, stopping the simulation." <<std::endl;
		removeSimItems();
		QWidget * tempWidget = new QWidget();
		QMessageBox::warning(tempWidget, tr("Simulator Error"),
				     tr("The simulator gave an error when trying to simulate this circuit. "
					"Please, check the wiring and try again. \n\nErrors:\n") +
				     QString::fromStdString(m_simulator->getLog(false)) +
				     QString::fromStdString(m_simulator->getLog(true)) +
				     "\n\nNetlist:\n" + spiceNetlist);
		delete tempWidget;
		return;
	}
	std::cout << "No fatal error found, continuing..." <<std::endl;

	//m_simulator->command("bg_halt");

	//Delete the pointers
	foreach (QList<ConnectorItem *> * net, netList) {
		delete net;
	}
	netList.clear();



	//The spice simulation has finished, iterate over each part being simulated and update it (if it is necessary).
	updateParts(itemBases, 0);

	//If this a transitory simulation, set the timer for the animation
	if (m_simEndTime > 0) {
		m_currSimStep = 1;
		m_showResultsTimer->start();
	}

}

void Simulator::showSimulationResults() {
	if (m_currSimStep <= m_simNumberOfSteps) {
		//Check that we have the sim results for this time step
		auto timeInfo = m_simulator->getVecInfo(QString("time").toStdString());
		std::cout << "Time showSimulationResults (" << timeInfo.size() << " points): ";
		if (m_currSimStep > timeInfo.size())
			return;

		QElapsedTimer elapsedTimer;
		elapsedTimer.start();
		removeSimItems();
		updateParts(itemBases, m_currSimStep);

		double simTime = m_simStartTime + m_currSimStep * m_simStepTime;

		QString simMessage = QString::number(simTime, 'f', 3) + " s";

		m_breadboardGraphicsView->setSimulatorMessage(simMessage);
		m_schematicGraphicsView->setSimulatorMessage(simMessage);

		if (elapsedTimer.elapsed() < m_showResultsTimer->interval()) {
			m_currSimStep++;
		} else {
			//Animation is going very slowly, skip some time steps
			m_currSimStep += (unsigned int) (elapsedTimer.elapsed()/m_showResultsTimer->interval());
			if (m_currSimStep > m_simNumberOfSteps)
				m_currSimStep = m_simNumberOfSteps;
		}
		std::cout << "Time to perform the animation (" << elapsedTimer.elapsed() << " ms): ";
	} else {
		m_showResultsTimer->stop();
	}

}

/**
 * Update the parts with the vidual effects. E.g:
 * * update the multimeters screen
 * * add smoke to a part if something is out of its specifications
 * * update the brightness of the LEDs
 * @param[in] itemBases A set of parts to be updated
 * @param[in] time The simulation time to be used for getting the voltages and currents
 */
void Simulator::updateParts(QSet<ItemBase *> itemBases, int timeStep) {
	foreach (ItemBase * part, itemBases){
		//Remove the effects, if any
		part->setGraphicsEffect(nullptr);
		m_sch2bbItemHash.value(part)->setGraphicsEffect(nullptr);

		std::cout << "-----------------------------------" <<std::endl;
		std::cout << "Instance Title: " << part->instanceTitle().toStdString() << std::endl;

		QString family = part->family().toLower();

		if (family.contains("capacitor")) {
			updateCapacitor(timeStep, part);
			continue;
		}
		if (family.contains("diode")) {
			updateDiode(timeStep, part);
			continue;
		}
		if (family.contains("led")) {
			updateLED(timeStep, part);
			continue;
		}
		if (family.contains("resistor")) {
			updateResistor(timeStep, part);
			continue;
		}
		if (family.contains("multimeter")) {
			updateMultimeter(timeStep, part);
			continue;
		}
		if (family.contains("dc motor")) {
			updateDcMotor(timeStep, part);
			continue;
		}
		if (family.contains("line sensor") || family.contains("distance sensor")) {
			updateIRSensor(timeStep, part);
			continue;
		}
		if (family.contains("battery") || family.contains("voltage source")) {
			updateBattery(timeStep, part);
			continue;
		}
		if (family.contains("potentiometer") || family.contains("sparkfun trimpot")) {
			updatePotentiometer(timeStep, part);
			continue;
		}
		if (family.contains("oscilloscope")) {
			updateOscilloscope(timeStep, part);
			continue;
		}
	}
}

/**
 * Adds an smoke image on top of a part in the breadboard and schematic views.
 * @param[in] part Part where the smoke is going to be placed
 */
void Simulator::drawSmoke(ItemBase* part) {
	QGraphicsSvgItem * bbSmoke = new QGraphicsSvgItem(":resources/images/smoke.svg", m_sch2bbItemHash.value(part));
	QGraphicsSvgItem * schSmoke = new QGraphicsSvgItem(":resources/images/smoke.svg", part);
	if (!bbSmoke || !schSmoke) return;

	schSmoke->setZValue(std::numeric_limits<double>::max());
	bbSmoke->setZValue(std::numeric_limits<double>::max());
	bbSmoke->setOpacity(0.7);
	schSmoke->setOpacity(0.7);

	//Scale the smoke images
	QRectF bbPartBoundingBox = m_sch2bbItemHash.value(part)->boundingRectWithoutLegs();
	QRectF schSmokeBoundingBox = schSmoke->boundingRect();
	QRectF schPartBoundingBox = part->boundingRect();
	QRectF bbSmokeBoundingBox = bbSmoke->boundingRect();
	std::cout << "bbSmokeBoundingBox w and h: " << bbSmokeBoundingBox.width() << " " << bbSmokeBoundingBox.height() << std::endl;

	double scaleWidth = bbPartBoundingBox.width()/schSmokeBoundingBox.width();
	double scaleHeight = bbPartBoundingBox.height()/schSmokeBoundingBox.height();
	double scale;
	(scaleWidth < scaleHeight) ? scale = scaleWidth : scale = scaleHeight;
	if (scale > 1) {
		//we can scale the smoke
		bbSmoke->setScale(scale);
	}else{
		scale = 1; //Do not scale down the smoke
	}

	//Center the smoke in bb (bottom right corner of the smoke at the center of the part)
	bbSmoke->setPos(QPointF(bbPartBoundingBox.width()/2-bbSmokeBoundingBox.width()*scale,
				bbPartBoundingBox.height()/2-bbSmokeBoundingBox.height()*scale));

	//Scale sch image
	scaleWidth = schPartBoundingBox.width()/schSmokeBoundingBox.width();
	scaleHeight = schPartBoundingBox.height()/schSmokeBoundingBox.height();
	(scaleWidth < scaleHeight) ? scale = scaleWidth : scale = scaleHeight;
	if (scale > 1) {
		//we can scale the smoke
		schSmoke->setScale(scale);
	}else{
		scale = 1; //Do not scale down the smoke
	}

	//Center the smoke in sch view (bottom right corner of the smoke at the center of the part)
	schSmoke->setPos(QPointF(schPartBoundingBox.width()/2-schSmokeBoundingBox.width()*scale,
				 schPartBoundingBox.height()/2-schSmokeBoundingBox.height()*scale));

	part->addSimulationGraphicsItem(schSmoke);
	m_sch2bbItemHash.value(part)->addSimulationGraphicsItem(bbSmoke);
}

/**
 * Display a number in the screen of a multimeter. The message
 * is displayed in a 7-segments font.
 * @param[in] multimeter The part where the message is going to be displayed
 * @param[in] number The number to be displayed
 */
void Simulator::updateMultimeterScreen(ItemBase * multimeter, double number){
	std::cout << "updateMultimeterScreen with number: " << number <<std::endl;
	if (abs(number) < 1.0e-12)
		number = 0.0; //Show 0.000 instead of 0.000p
	QString textToDisplay = TextUtils::convertToPowerPrefix(number, 'f', 6);
	int indexPoint = textToDisplay.indexOf('.');
	textToDisplay = TextUtils::convertToPowerPrefix(number, 'f', 4 - indexPoint);
	textToDisplay.replace('k', 'K');
	updateMultimeterScreen(multimeter, textToDisplay);
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
	QGraphicsTextItem * schScreen = new QGraphicsTextItem(msg, multimeter);
	schScreen->setPos(QPointF(10,10));
	schScreen->setZValue(std::numeric_limits<double>::max());
	QFont font("Segment16C", 10, QFont::Normal);
	bbScreen->setFont(font);
	//There are issues as the size of the text changes depending on the display settings in windows
	//This hack scales the text to match the appropiate value
	QRectF bbMultBoundingBox = m_sch2bbItemHash.value(multimeter)->boundingRect();
	QRectF bbBoundingBox = bbScreen->boundingRect();
	QRectF schMultBoundingBox = multimeter->boundingRect();
	QRectF schBoundingBox = schScreen->boundingRect();

	//Set the text to be a 80% percent of the multimeter´s width and 50% in sch view
	bbScreen->setScale((0.8*bbMultBoundingBox.width())/bbBoundingBox.width());
	schScreen->setScale((0.5*schMultBoundingBox.width())/schBoundingBox.width());

	//Update the bounding box after scaling them
	bbBoundingBox = bbScreen->mapRectToParent(bbScreen->boundingRect());
	schBoundingBox = schScreen->mapRectToParent(schScreen->boundingRect());

	//Center the text
	bbScreen->setPos(QPointF((bbMultBoundingBox.width()-bbBoundingBox.width())/2
				 ,0.07*bbMultBoundingBox.height()));
	schScreen->setPos(QPointF((schMultBoundingBox.width()-schBoundingBox.width())/2
				  ,0.13*schMultBoundingBox.height()));

	bbScreen->setDefaultTextColor(QColor(48, 48, 48));
	schScreen->setDefaultTextColor(QColor(48, 48, 48));

	bbScreen->setZValue(std::numeric_limits<double>::max());
	schScreen->setZValue(std::numeric_limits<double>::max());
	m_sch2bbItemHash.value(multimeter)->addSimulationGraphicsItem(bbScreen);
	multimeter->addSimulationGraphicsItem(schScreen);
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
 * Returns the first element of ngspice vector or a default value.
 * @param[in] vecName name of ngspice vector to get value from
 * @param[in] defaultValue value to return on empty vector
 * @returns the first vector element or the given default value
 */
double Simulator::getVectorValueOrDefault(unsigned long timeStep, const std::string & vecName, double defaultValue) {
	auto vecInfo = m_simulator->getVecInfo(vecName);
	if (vecInfo.empty()) {
		return defaultValue;
	} else {
		if (timeStep < 0 || timeStep >= vecInfo.size())
			return defaultValue;
		return vecInfo[timeStep];
	}
}

/**
 * Returns the voltage between two connectors.
 * @param[in] c0 the first connector
 * @param[in] c1 the second connector
 * @returns the voltage between the connector c0 and c1
 */
double Simulator::calculateVoltage(unsigned long timeStep, ConnectorItem * c0, ConnectorItem * c1) {
	int net0 = m_connector2netHash.value(c0);
	int net1 = m_connector2netHash.value(c1);

	QString net0str = QString("v(%1)").arg(net0);
	QString net1str = QString("v(%1)").arg(net1);
	//std::cout << "net0str: " << net0str.toStdString() <<std::endl;
	//std::cout << "net1str: " << net1str.toStdString() <<std::endl;

	double volt0 = 0.0, volt1 = 0.0;
	if (net0 != 0) {
		auto vecInfo = m_simulator->getVecInfo(net0str.toStdString());
		if (vecInfo.empty()) return 0.0;
		volt0 = vecInfo[timeStep];
	}
	if (net1 != 0) {
		auto vecInfo = m_simulator->getVecInfo(net1str.toStdString());
		if (vecInfo.empty()) return 0.0;
		volt1 = vecInfo[timeStep];
	}
	return volt0-volt1;
}

std::vector<double> Simulator::voltageVector(ConnectorItem * c0) {
	int net0 = m_connector2netHash.value(c0);
	QString net0str = QString("v(%1)").arg(net0);

	if (net0 != 0) {
		return m_simulator->getVecInfo(net0str.toStdString());
	}

	//This is the ground (node 0), return a vector with 0s, same size as the time vector
	auto timeInfo = m_simulator->getVecInfo(QString("time").toStdString());
	std::vector<double> voltageVector(timeInfo.size(), 0.0);
	return voltageVector;
}

QString Simulator::generateSvgPath(std::vector<double> proveVector, std::vector<double> comVector, int currTimeStep, QString nameId, double simStartTime, double simTimeStep, double timePos, double timeScale, double verticalScale, double verOffset, double screenHeight, double screenWidth, QString color, QString strokeWidth ) {
	std::cout << "OSCILLOSCOPE: pos " << timePos << ", timeScale: " << timeScale << std::endl;
	std::cout << "OSCILLOSCOPE: VOLTAGE VALUES " << nameId.toStdString() << ": ";
	QString svg;
	double screenOffset = 0;//132.87378;
	//svg += QString("<rect x='%1' y='%1' width='%2' height='%3' stroke='red' stroke-width='%4'/>\n").arg(screenOffset).arg(screenWidth).arg(screenHeight).arg(strokeWidth);
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
	std::cout << "OSCILLOSCOPE: nSampleInScreen " << nSampleInScreen << std::endl;
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
		//std::cout <<" ("<< time << "): " << voltage << ' ';
		screenPoint++;
	}
	svg += "' transform='translate(%1,%2)' stroke='"+ color + "' stroke-width='"+ strokeWidth + "' fill='none' /> \n"; //

	std::cout << std::endl;
	return svg;

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
	QString msg = QString("Error getting the device type. The type is not recognized. Part=%1, Spice line=%2").arg(part->instanceTitle(), part->spice());
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
	QString propertyStr = part->getProperty(property);
	if (propertyStr.isEmpty()) {
		return std::numeric_limits<double>::max();
	}

	double value;
	QString symbol = getSymbol(part, property);
	static QRegularExpression regExp("[^pnu\x00B5mkMGT^\\d.]");

	if (!symbol.isEmpty()) {
		value = TextUtils::convertFromPowerPrefix(propertyStr, symbol);
	} else {
		//Attempt to remove the symbol: Remove all the letters, except the multipliers
		propertyStr.remove(regExp);
		value = TextUtils::convertFromPowerPrefix(propertyStr, symbol);
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
double Simulator::getPower(unsigned long timeStep, ItemBase* part, QString subpartName) {
	//TODO: Handle devices that do not return the power
	QString instanceStr = part->instanceTitle().toLower();
	instanceStr.append(subpartName.toLower());
	instanceStr.prepend("@");
	instanceStr.append("[p]");
	return getVectorValueOrDefault(timeStep, instanceStr.toStdString(), 0.0);
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
double Simulator::getCurrent(unsigned long timeStep, ItemBase* part, QString subpartName) {
	QString instanceStr = part->instanceTitle().toLower();
	instanceStr.append(subpartName.toLower());

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
	return getVectorValueOrDefault(timeStep, instanceStr.toStdString(), 0.0);
}

/**
 * Returns the current that flows through a transistor.
 * @param[in] spicePartName The name of the spice transistor.
 * @returns the current that the transistor is sinking/sourcing.
 */
double Simulator::getTransistorCurrent(unsigned long timeStep, QString spicePartName, TransistorLeg leg) {
	if(spicePartName.at(0).toLower()!=QChar('q')) {
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

	return getVectorValueOrDefault(timeStep, spicePartName.toStdString(), 0.0);
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
		foreach (QGraphicsItem * schItem, noSimSchParts) {
			ItemBase * schPart = dynamic_cast<ItemBase *>(schItem);
			if (!schPart) continue;
			if (part->instanceTitle().compare(schPart->instanceTitle()) == 0) {
				noSimSchParts.removeAll(schItem);
			}
		}

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
 * the wires, breadboards, power symbols, etc. (which are part of the simulation) and
 * leave the rest.
 * @param[in/out] parts A list of parts which will be filtered to remove parts that
 * are being simulated
 */
void Simulator::removeItemsToBeSimulated(QList<QGraphicsItem*> & parts) {
	foreach (QGraphicsItem * part, parts) {
		ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(part);
		if (connectorItem) {
			parts.removeAll(part);
			continue;
		}

		Wire* wire = dynamic_cast<Wire *>(part);
		if (wire) {
			parts.removeAll(part);
			continue;
		}

		PartLabel* label = dynamic_cast<PartLabel *>(part);
		if (label) {
			parts.removeAll(part);
			continue;
		}

		Note* note = dynamic_cast<Note *>(part);
		if (note) {
			parts.removeAll(part);
			continue;
		}

		LedLight* ledLight = dynamic_cast<LedLight *>(part);
		if (ledLight) {
			parts.removeAll(part);
			continue;
		}

		SymbolPaletteItem* symbol = dynamic_cast<SymbolPaletteItem *>(part);
		if (symbol) {
			parts.removeAll(part);
			continue;
		}

		ResizableBoard* board = dynamic_cast<ResizableBoard *>(part);
		if (board) {
			parts.removeAll(part);
			continue;
		}

		Perfboard* perfBoard = dynamic_cast<Perfboard *>(part);
		if (perfBoard) {
			parts.removeAll(part);
			continue;
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

		Ruler* ruler = dynamic_cast<Ruler *>(part);
		if (ruler) {
			parts.removeAll(part);
			continue;
		}

		ItemBase* item = dynamic_cast<ItemBase *>(part);
		if (!item) {
			//We only remove the parts, we do not touch other elements of the scene (text of the net labels, etc.)
			parts.removeAll(part);
		} else {
			if (item->family().compare("power label") == 0
					|| item->family().compare("net label") == 0
					|| item->family().compare("breadboard") == 0) //hack as half+ is not generated as breadboard object, see #3873
			{
				parts.removeAll(part);
			}
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
void Simulator::updateDiode(unsigned long timeStep, ItemBase * diode) {
	double maxPower = getMaxPropValue(diode, "power");
	double power = getPower(timeStep, diode);
	if (power > maxPower) {
		drawSmoke(diode);
	}
}

/**
 * Updates and checks an LED. Checks that the current is less than the maximum current
 * and updates the brightness of the LED in the breadboard view.
 * @param[in] part An LED that is going to be checked and updated.
 */
void Simulator::updateLED(unsigned long timeStep, ItemBase * part) {
	LED* led = dynamic_cast<LED *>(part);
	if (led) {
		//Check if this an RGB led
		QString rgbString = part->getProperty("rgb");

		if (rgbString.isEmpty()) {
			// Just one LED
			double curr = getCurrent(timeStep, part);
			double maxCurr = getMaxPropValue(part, "current");

			std::cout << "LED Current: " <<curr<<std::endl;
			std::cout << "LED MaxCurrent: " <<maxCurr<<std::endl;

			LED* bbLed = dynamic_cast<LED *>(m_sch2bbItemHash.value(part));
			bbLed->setBrightness(curr/maxCurr);
			if (curr > maxCurr) {
				drawSmoke(part);
				bbLed->setBrightness(0);
			}
		} else {
			// The part is an RGB LED
			double currR = getCurrent(timeStep, part, "R");
			double currG = getCurrent(timeStep, part, "G");
			double currB = getCurrent(timeStep, part, "B");
			double curr = std::max({currR, currG, currB});
			double maxCurr = getMaxPropValue(part, "current");

			std::cout << "LED Current (R, G, B): " << currR << " " << currG << " " << currB <<std::endl;
			std::cout << "LED MaxCurrent: " << maxCurr << std::endl;

			LED* bbLed = dynamic_cast<LED *>(m_sch2bbItemHash.value(part));
			bbLed->setBrightnessRGB(currR/maxCurr, currG/maxCurr, currB/maxCurr);
			if (curr > maxCurr) {
				drawSmoke(part);
				bbLed->setBrightness(0);
			}
		}
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
void Simulator::updateCapacitor(unsigned long timeStep, ItemBase * part) {
	QString family = part->getProperty("family").toLower();

	ConnectorItem * negLeg = nullptr, * posLeg = nullptr;
	QList<ConnectorItem *> legs = part->cachedConnectorItems();
	foreach(ConnectorItem * ci, legs) {
		if(ci->connectorSharedName().toLower().compare("+") == 0) posLeg = ci;
		if(ci->connectorSharedName().toLower().compare("-") == 0) negLeg = ci;
	}
	if(!negLeg || !posLeg )
		return;

	double maxV = getMaxPropValue(part, "voltage");
	double v = calculateVoltage(timeStep, posLeg, negLeg);
	std::cout << "MaxVoltage of the capacitor: " << maxV << std::endl;
	std::cout << "Capacitor voltage is : " << QString("%1").arg(v).toStdString() << std::endl;

	if (family.contains("bidirectional")) {
		//This is a ceramic capacitor (or not polarized)
		if (abs(v) > maxV) {
			drawSmoke(part);
		}
	} else {
		//This is an electrolytic o tantalum capacitor (polarized)
		if (v > maxV/2 || v < HarmfulNegativeVoltage) {
			drawSmoke(part);
		}
	}
}

/**
 * Updates and checks a resistor. Checks that the power is less than the maximum power.
 * @param[in] part A resistor that is going to be checked and updated.
 */
void Simulator::updateResistor(unsigned long timeStep, ItemBase * part) {
	double maxPower = getMaxPropValue(part, "power");
	double power = getPower(timeStep, part);
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
void Simulator::updatePotentiometer(unsigned long timeStep, ItemBase * part) {
	double maxPower = getMaxPropValue(part, "power");
	double powerA = getPower(timeStep, part, "A"); //power through resistor A
	double powerB = getPower(timeStep, part, "B"); //power through resistor B
	double power = powerA + powerB;
	if (power > maxPower) {
		drawSmoke(part);
	}
}

/**
 * Updates and checks a battery. Checks that there are no short circuits.
 * @param[in] part A battery that is going to be checked and updated.
 */
void Simulator::updateBattery(unsigned long timeStep, ItemBase * part) {
	double voltage = getMaxPropValue(part, "voltage");
	double resistance = getMaxPropValue(part, "internal resistance");
	double safetyMargin = 0.1; //TODO: This should be adjusted
	double maxCurrent = voltage/resistance * safetyMargin;
	double current = getCurrent(timeStep, part); //current that the battery delivers
	std::cout << "Battery: voltage=" << voltage << ", resistance=" << resistance  <<std::endl;
	std::cout << "Battery: MaxCurr=" << maxCurrent << ", Curr=" << current  <<std::endl;
	if (abs(current) > maxCurrent) {
		drawSmoke(part);
	}
}

bool Simulator::isSimulating()
{
	return m_simulating;
}

/**
 * Updates and checks a IR sensor. Checks that the voltage is between the allowed range
 * and that the current of the output is less than  the maximum.
 * @param[in] part A IR sensor that is going to be checked and updated.
 */
void Simulator::updateIRSensor(unsigned long timeStep, ItemBase * part) {
	double maxV = getMaxPropValue(part, "voltage (max)");
	double minV = getMaxPropValue(part, "voltage (min)");
	double maxIout = getMaxPropValue(part, "max output current");
	std::cout << "IR sensor VCC range: " << maxV << " " << minV << std::endl;
	ConnectorItem *gnd(nullptr);
	ConnectorItem *vcc(nullptr);
	ConnectorItem *out(nullptr);
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

	double v = calculateVoltage(timeStep, vcc, gnd); //voltage applied to the motor
	double i;
	if (part->family().contains("line sensor")) {
		//digital sensor (push-pull output)
		QString spicename = part->instanceTitle().toLower();
		spicename.prepend("q");
		i = getTransistorCurrent(timeStep, spicename, COLLECTOR); //voltage applied to the motor
	} else {
		//analogue sensor (modelled by a voltage source and a resistor)
		i = getCurrent(timeStep, part, "a"); //voltage applied to the motor
	}
	std::cout << "IR sensor Max Iout: " << maxIout << ", current Iout " << i << std::endl;
	std::cout << "IR sensor Max V: " << maxV << ", current V " << v << std::endl;
	if (v > maxV || v < HarmfulNegativeVoltage || abs(i) > maxIout) {
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
void Simulator::updateDcMotor(unsigned long timeStep, ItemBase * part) {
	double maxV = getMaxPropValue(part, "voltage (max)");
	double minV = getMaxPropValue(part, "voltage (min)");
	std::cout << "Motor1: " << std::endl;
	ConnectorItem *terminal1 = nullptr, *terminal2 = nullptr;
	QList<ConnectorItem *> probes = part->cachedConnectorItems();
	foreach(ConnectorItem * ci, probes) {
		if(ci->connectorSharedName().toLower().compare("pin 1") == 0) terminal1 = ci;
		if(ci->connectorSharedName().toLower().compare("pin 2") == 0) terminal2 = ci;
	}
	if(!terminal1 || !terminal2 )
		return;

	double v = calculateVoltage(timeStep, terminal1, terminal2); //voltage applied to the motor
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

		//Scale the smoke images
		QRectF bbPartBoundingBox = m_sch2bbItemHash.value(part)->boundingRectWithoutLegs();
		QRectF schRotateBoundingBox = schRotate->boundingRect();
		QRectF schPartBoundingBox = part->boundingRect();
		QRectF bbRotateBoundingBox = bbRotate->boundingRect();


		double scaleWidth = bbPartBoundingBox.width()/bbRotateBoundingBox.width();
		double scaleHeight = bbPartBoundingBox.height()/bbRotateBoundingBox.height();
		double scale;
		scale = std::max(scaleWidth, scaleHeight)*0.5;
		bbRotate->setScale(scale);

		//Center the arrow in bb
		bbRotate->setPos(QPointF(bbPartBoundingBox.width()/2-bbRotateBoundingBox.width()*scale/2,
					 bbPartBoundingBox.height()/2-bbRotateBoundingBox.height()*scale/2));

		scaleWidth = schPartBoundingBox.width()/schRotateBoundingBox.width();
		scaleHeight = schPartBoundingBox.height()/schRotateBoundingBox.height();
		scale = std::max(scaleWidth, scaleHeight)*0.5;
		schRotate->setScale(scale);

		//Center the arrow in bb
		schRotate->setPos(QPointF(schPartBoundingBox.width()/2-schRotateBoundingBox.width()*scale/2,
					  schPartBoundingBox.height()/2-schRotateBoundingBox.height()*scale/2));

		schRotate->setZValue(std::numeric_limits<double>::max());
		bbRotate->setZValue(std::numeric_limits<double>::max());
		part->addSimulationGraphicsItem(schRotate);
		m_sch2bbItemHash.value(part)->addSimulationGraphicsItem(bbRotate);
	}
}

/**
 * Updates and checks a multimeter. Checks that there are not 3 probes connected.
 * Calculates the parameter to measure and updates the display of the multimeter.
 * @param[in] part A multimeter that is going to be checked and updated.
 */
void Simulator::updateMultimeter(unsigned long timeStep, ItemBase * part) {
	QString variant = part->getProperty("variant").toLower();
	ConnectorItem * comProbe = nullptr, * vProbe = nullptr, * aProbe = nullptr;
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
			double v = calculateVoltage(timeStep, vProbe, comProbe);
			updateMultimeterScreen(part, v);
		}
		return;
	} else if (variant.compare("ammeter (dc)") == 0) {
		std::cout << "Multimeter (c_dc) found. " << std::endl;
		if(vProbe->connectedToWires()) {
			std::cout << "Multimeter (c_dc) has the voltage terminal connected. " << std::endl;
			updateMultimeterScreen(part, "ERR");
			return;
		}
		updateMultimeterScreen(part, getCurrent(timeStep, part));
		return;
	} else if (variant.compare("ohmmeter") == 0) {
		std::cout << "Ohmmeter found. " << std::endl;
		if(aProbe->connectedToWires()) {
			std::cout << "Ohmmeter has the current terminal connected. " << std::endl;
			updateMultimeterScreen(part, "ERR");
			return;
		}
		double v = calculateVoltage(timeStep, vProbe, comProbe);
		double a = getCurrent(timeStep, part);
		double r = abs(v/a);
		std::cout << "Ohmmeter: Volt: " << v <<", Curr: " << a <<", Ohm: " << r << std::endl;
		updateMultimeterScreen(part, r);
		return;
	}
}

/**
 * Updates and checks a oscilloscope. If the ground connection is not connected, plots a noisy signal.
 * Calculates the parameter to measure and updates the display of the multimeter.
 * @param[in] part An oscilloscope that is going to be checked and updated.
 */
void Simulator::updateOscilloscope(unsigned long timeStep, ItemBase * part) {
	std::cout << "updateOscilloscope: " << std::endl;
	ConnectorItem * comProbe = nullptr, * v1Probe = nullptr, * v2Probe = nullptr, * v3Probe = nullptr, * v4Probe = nullptr;
	QList<ConnectorItem *> probes = part->cachedConnectorItems();
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
		std::cout << "Oscilloscope does not have any wire connected to the probe terminals. " << std::endl;
		return;
	}
	ConnectorItem * probesArray[4] = {v1Probe, v2Probe, v3Probe, v4Probe};


	std::cout << "Oscilloscope probe v1 connected. " << std::endl;


	//TODO: use convertFromPowerPrefixU
	int nChannels = TextUtils::convertFromPowerPrefix(part->getProperty("channels"), "");
	double timeDiv = TextUtils::convertFromPowerPrefix(part->getProperty("time/div"), "s");
	double hPos = TextUtils::convertFromPowerPrefix(part->getProperty("horizontal position"), "s");
	double ch1_volsDiv = TextUtils::convertFromPowerPrefix(part->getProperty("ch1 volts/div"), "V");
	double ch1_offset = TextUtils::convertFromPowerPrefix(part->getProperty("ch1 offset"), "V");
	double ch2_volsDiv = TextUtils::convertFromPowerPrefix(part->getProperty("ch2 volts/div"), "V");
	double ch2_offset = TextUtils::convertFromPowerPrefix(part->getProperty("ch2 offset"), "V");
	double ch3_volsDiv = TextUtils::convertFromPowerPrefix(part->getProperty("ch3 volts/div"), "V");
	double ch3_offset = TextUtils::convertFromPowerPrefix(part->getProperty("ch3 offset"), "V");
	double ch4_volsDiv = TextUtils::convertFromPowerPrefix(part->getProperty("ch4 volts/div"), "V");
	double ch4_offset = TextUtils::convertFromPowerPrefix(part->getProperty("ch4 offset"), "V");
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
		auto v = voltageVector(probesArray[channel]);
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
			vCom = voltageVector(comProbe);
		}

		//Draw the signal
		QString pathId = QString("ch%1-path").arg(channel+1);
		QString signalPath = generateSvgPath(v, vCom, timeStep, pathId, m_simStartTime, m_simStepTime, hPos, timeDiv, divisionSize/voltsDiv[channel], chOffsets[channel],
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

	QGraphicsSvgItem * schGraph = new QGraphicsSvgItem(part);
	QGraphicsSvgItem * bbGraph = new QGraphicsSvgItem(m_sch2bbItemHash.value(part));
	QSvgRenderer *schGraphRender = new QSvgRenderer(schSvg.toUtf8());
	QSvgRenderer *bbGraphRender = new QSvgRenderer(bbSvg.toUtf8());
	if(schGraphRender->isValid())
		std::cout << "SCH SVG Graph is VALID \n" << std::endl;
	else
		std::cout << "SCH SVG Graph is NOT VALID \n" << std::endl;
	//std::cout << "SCH SVG: " << schSvg.toStdString() << std::endl;

	if(bbGraphRender->isValid())
		std::cout << "BB SVG Graph is VALID \n" << std::endl;
	else
		std::cout << "BB SVG Graph is NOT VALID\n" << std::endl;
	//std::cout << "BB SVG: " << bbSvg.toStdString() << std::endl;

	schGraph->setSharedRenderer(schGraphRender);
	schGraph->setZValue(std::numeric_limits<double>::max());
	bbGraph->setSharedRenderer(bbGraphRender);
	bbGraph->setZValue(std::numeric_limits<double>::max());

	part->addSimulationGraphicsItem(schGraph);
	m_sch2bbItemHash.value(part)->addSimulationGraphicsItem(bbGraph);



}
