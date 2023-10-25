#define BOOST_TEST_MODULE NGSPICE Tests
#include <boost/test/included/unit_test.hpp>

#include "simulation/ngspice_simulator.h"

/*
Testing ngspice_simulator.cpp, an interface for the ngspice library.
*/

#include <algorithm>
#include <memory>

#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>

#include <QThread>

BOOST_AUTO_TEST_CASE( ngspice_simulator )
{
	std::string netlist = "NgSpice Simulation Netlist\n DLED1 1 0 LED_GENERIC\n R1 1 2 68\n VCC1 2 0 DC 3V\n \n *Typ RED,GREEN,YELLOW,AMBER GaAs LED: Vf=2.1V Vr=4V If=40mA trr=3uS\n .MODEL LED_GENERIC D (IS=93.1P RS=42M N=4.61 BV=4 IBV=10U CJO=2.97P VJ=.75 M=.333 TT=4.32U)\n \n .options savecurrents\n .OP\n *.TRAN 1ms 100ms\n * .AC DEC 100 100 1MEG\n .END\n";

	std::shared_ptr<NgSpiceSimulator> simulator = NgSpiceSimulator::getInstance();
	simulator->init();
	simulator->loadCircuit(netlist);
	simulator->command("bg_run");

	int count = 0;
	while(simulator->isBGThreadRunning() && count < 50) {
		QThread::msleep(100);
		++count;
	}

	int current = 1000000 * simulator->getVecInfo("@dled1[id]")[0];
	BOOST_CHECK_EQUAL(current, 11447);
}
