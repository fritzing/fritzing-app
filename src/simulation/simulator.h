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

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "../mainwindow/mainwindow.h"
#include "../items/itembase.h"
#include "../simulation/ngspice.h"
#include "../simulation/spice_simulator.h"

enum TransistorLeg { BASE, COLLECTOR, EMITER };

class Simulator : public QObject
{
	Q_OBJECT

public:
	Simulator(class MainWindow *mainWindow);
	~Simulator();
	bool isEnabled();

public slots:
	void enable(bool);
	void reset();
	void simulate();

protected:
	void drawSmoke(ItemBase* part);
	void updateMultimeterScreen(ItemBase *, QString);
	void removeSimItems();
	void removeSimItems(QList<QGraphicsItem *>);
	void greyOutNonSimParts(const QSet<class ItemBase *>&);
	void greyOutParts(const QList<QGraphicsItem *> &);
	void removeItemsToBeSimulated(QList<QGraphicsItem*> &);

	QChar getDeviceType (ItemBase*);
	double getMaxPropValue(ItemBase*, QString);
	QString getSymbol(ItemBase*, QString);
	double calculateVoltage(ConnectorItem *, ConnectorItem *);
	double getCurrent(ItemBase*, QString subpartName="");
	double getTransistorCurrent(QString spicePartName, TransistorLeg leg);
	double getPower(ItemBase*, QString subpartName="");

	//Functions to update the parts
	void updateCapacitor(ItemBase *);
	void updateDiode(ItemBase *);
	void updateLED(ItemBase *);
	void updateMultimeter(ItemBase *);
	void updateResistor(ItemBase *);
	void updatePotentiometer(ItemBase *);
	void updateDcMotor(ItemBase *);
	void updateIRSensor(ItemBase *);
	void updateBattery(ItemBase *);


	MainWindow *m_mainWindow;
	std::shared_ptr<SPICE_SIMULATOR> m_simulator;
	QPointer<class BreadboardSketchWidget> m_breadboardGraphicsView;
	QPointer<class SchematicSketchWidget> m_schematicGraphicsView;

	bool m_enabled;

	QHash<ItemBase *, ItemBase *> m_sch2bbItemHash;
	QHash<ConnectorItem *, int> m_connector2netHash;

	QList<QString>* m_instanceTitleSim;

};

#endif // SIMULATOR_H
