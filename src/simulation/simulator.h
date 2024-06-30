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
#include "../simulation/ngspice_simulator.h"

enum TransistorLeg { BASE, COLLECTOR, EMITER };

class Simulator : public QObject
{
	Q_OBJECT

public:
	Simulator(class MainWindow *mainWindow);
	~Simulator();
	bool isEnabled();
	bool isTransientSimulation();
	bool isSimulating();
	void triggerSimulation();
	void simulate();

private:
	void resetTimer();

public slots:
	void enable(bool);
	void enableTransientSimulation(bool);
	void stopSimulation();
	void startSimulation();
	void showSimulationResults();


signals:
	void simulationStartedOrStopped(bool running);
	void simulationEnabled(bool enabled);

protected:
	void updateParts(QSet<ItemBase *>, int);
	void drawSmoke(ItemBase* part);
	void updateMultimeterScreen(ItemBase *, QString);
	void updateMultimeterScreen(ItemBase *, double);
	void removeSimItems();
	void removeSimItems(QList<QGraphicsItem *>);
	void greyOutNonSimParts(const QSet<class ItemBase *>&);
	void greyOutParts(const QList<QGraphicsItem *> &);
	void removeItemsToBeSimulated(QList<QGraphicsItem*> &);

	QChar getDeviceType (ItemBase*);
	double getMaxPropValue(ItemBase*, QString);
	QString getSymbol(ItemBase*, QString);
	double getVectorValueOrDefault(unsigned long timeStep, const std::string & vecName,  double defaultValue);
	double calculateVoltage(unsigned long, ConnectorItem *, ConnectorItem *);
	std::vector<double> voltageVector(ConnectorItem *);
	QString generateSvgPath(std::vector<double>, std::vector<double>, int, QString, double, double, double, double, double, double, double, double, QString, QString);
	double getCurrent(unsigned long, ItemBase*, QString subpartName="");
	double getTransistorCurrent(unsigned long timeStep, QString spicePartName, TransistorLeg leg);
	double getPower(unsigned long, ItemBase*, QString subpartName="");

	//Functions to update the parts
	void updateCapacitor(unsigned long, ItemBase *);
	void updateDiode(unsigned long, ItemBase *);
	void updateLED(unsigned long, ItemBase *);
	void updateMultimeter(unsigned long, ItemBase *);
	void updateOscilloscope(unsigned long, ItemBase *);
	void updateResistor(unsigned long, ItemBase *);
	void updatePotentiometer(unsigned long, ItemBase *);
	void updateDcMotor(unsigned long, ItemBase *);
	void updateIRSensor(unsigned long, ItemBase *);
	void updateBattery(unsigned long, ItemBase *);

	bool m_simulating = false;
	MainWindow *m_mainWindow;
	std::shared_ptr<NgSpiceSimulator> m_simulator;
	QPointer<class BreadboardSketchWidget> m_breadboardGraphicsView;
	QPointer<class SchematicSketchWidget> m_schematicGraphicsView;
	double m_simStartTime, m_simStepTime, m_simEndTime, m_simNumberOfSteps;

	bool m_enabled = false;
	bool m_transientSimulationEnabled = false;

	QSet<ItemBase *> itemBases;
	QHash<ItemBase *, ItemBase *> m_sch2bbItemHash;
	QHash<ConnectorItem *, int> m_connector2netHash;

	QTimer *m_simTimer, *m_showResultsTimer;
	unsigned long m_currSimStep;

	static constexpr int SimDelay = 200;
	static constexpr double HarmfulNegativeVoltage = -0.5;

};

#endif // SIMULATOR_H
