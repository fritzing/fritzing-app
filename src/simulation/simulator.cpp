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


	removeSimItems();

	QList< QList<ConnectorItem *>* > netList;
	QSet<ItemBase *> itemBases;
	QString spiceNetlist = m_mainWindow->getSpiceNetlist("Simulator Netlist", netList, itemBases);

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
		foreach (QGraphicsItem * bbItem, m_breadboardGraphicsView->scene()->items()) {
			ItemBase * bbPart = dynamic_cast<ItemBase *>(bbItem);
			if (!bbPart) continue;
			if (schPart->instanceTitle().compare(bbPart->instanceTitle()) == 0) {
				m_sch2bbItemHash.insert(schPart, bbPart);
			}
		}
	}


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
#if defined(__MINGW32__) || defined(_MSC_VER)
	Sleep(100);
#else
	usleep(100000);
#endif


	//QString command = QString("print v(2)");
	//m_simulator->Command( command.toStdString() );
	//std::cout << "-----------------------------------" <<std::endl;
	//double volt = m_simulator->GetDataPoint("v(2)");
	//std::cout << "-----------------------------------" <<std::endl;
	//std::cout << "voltage: " <<volt<<std::endl;
	//std::cout << "-----------------------------------" <<std::endl;
	//m_simulator->Stop();
	//std::cout << "-----------------------------------" <<std::endl;

	foreach (ItemBase * part, itemBases){
		std::cout << "-----------------------------------" <<std::endl;
		std::cout << "Instance Title: " << part->instanceTitle().toStdString() << std::endl;
		std::cout << "viewLayerID: " << part->viewLayerID() << std::endl;
		LED* led = dynamic_cast<LED *>(part);
		if (led) {
			QString instanceStr = led->instanceTitle().toLower();
			instanceStr.prepend("@d");
			instanceStr.append("[id]");
			double curr = m_simulator->GetDataPoint(instanceStr.toStdString());
			double maxCurr = led->getProperty("current").toDouble();
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
			QString instanceStr = resistor->instanceTitle().toLower();
			instanceStr.prepend("@");
			instanceStr.append("[i]");
			std::cout << "instanceStr: " << instanceStr.toStdString() <<std::endl;
			double curr = m_simulator->GetDataPoint(instanceStr.toStdString());
			std::cout << "Current: " <<curr<<std::endl;
			instanceStr.replace('i', 'p');
			double power = m_simulator->GetDataPoint(instanceStr.toStdString());
			std::cout << "Power: " << power <<std::endl;

			double maxPower;
			QString powerStr = resistor->getProperty("power");
			if(powerStr.isEmpty()) {
				maxPower = DBL_MAX;
			} else {
				maxPower = powerStr.toDouble();
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
			QString variant = part->getProperty("variant").toLower(); //TODO: change to type
			if (variant.compare("v_dc") == 0) {
				std::cout << "Multimeter (v_dc) found. " << std::endl;
				ConnectorItem * c0 = part->cachedConnectorItems().at(0);
				ConnectorItem * c1 = part->cachedConnectorItems().at(1);
				ConnectorItem * c2 = part->cachedConnectorItems().at(2);

				if(c0->connectedToWires() && c1->connectedToWires() && c2->connectedToWires()) {
					std::cout << "Multimeter (v_dc) connected with three terminals. " << std::endl;
					updateMultimeterScreen(part, "ERR");
					continue;
				}
				c0->debugInfo("connector c0: ");
				c1->debugInfo("connector c1: ");
				c2->debugInfo("connector c2: ");
				if(c0->connectedToWires() && c1->connectedToWires()) {
					std::cout << "Multimeter (v_dc) connected with three terminals. " << std::endl;
					double v = calculateVoltage(c1, c0);
					updateMultimeterScreen(part, QString::number(v, 'f', 2));
				}
				continue;
			}

		}


	}

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
	QFont font("Segment7", 14, QFont::Normal);
	bbScreen->setFont(font);
	bbScreen->setScale(4);
	bbScreen->setPos(QPointF(30,30));
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
