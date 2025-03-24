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

#ifndef OSCILLOSCOPE_H
#define OSCILLOSCOPE_H

#include "capacitor.h"
#include "simulation/simulator.h"


class Oscilloscope : public Capacitor
{
	Q_OBJECT

public:
	// after calling this constructor if you want to render the loaded svg (either from model or from file), MUST call <renderImage>
	explicit Oscilloscope(ModelPart *, ViewLayer::ViewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel);
	~Oscilloscope();
	void updateOscilloscope(unsigned long timeStep, double simStartTime, double simStepTime, Simulator * sim, ItemBase * bbOscilloscope);

protected:
	QString generateSvgPath(std::vector<double> proveVector, std::vector<double> comVector, int currTimeStep, QString nameId, double simStartTime, double simTimeStep, double timePos, double timeScale, double verticalScale, double verOffset, double screenHeight, double screenWidth, QString color, QString strokeWidth );

};

#endif // OSCILLOSCOPE_H
