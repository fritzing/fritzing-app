/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2022 Fritzing GmbH

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

#include "FProbeStartSimulator.h"

#include <QThread>

FProbeStartSimulator::FProbeStartSimulator(Simulator * simulator) :
	FProbe("StartSimulator"),
	m_simulator(simulator)
{
	connect(this, SIGNAL(startSimulator()), m_simulator, SLOT(startSimulation()));
}

void FProbeStartSimulator::write(QVariant data) {
	(void) data;
	m_simulator->enable(true);
	Q_EMIT startSimulator();
	QThread::usleep(300000);
}
