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

#ifndef PANELIZER_H
#define PANELIZER_H

#include "cmrouter/tile.h"

constexpr double Worst = std::numeric_limits<double>::max() / 4;

struct BestPlace
{
	Tile * bestTile = nullptr;
	TileRect bestTileRect;
	TileRect maxRect;
	int width = 0;
	int height = 0;
	double bestArea = Worst;
	bool rotate90 = false;
	Plane* plane = nullptr;
};

class Panelizer
{
public:
	static int placeBestFit(Tile * tile, UserData userData);
};

#endif
