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

#include "panelizer.h"

#include "cmrouter/tileutils.h"

///////////////////////////////////////////////////////////

int allSpaces(Tile * tile, UserData userData) {
	QList<Tile*> * tiles = (QList<Tile*> *) userData;
	if (TiGetType(tile) == Tile::SPACE) {
		tiles->append(tile);
		return 0;
	}

	tiles->clear();
	return 1;			// stop the search
}

int allObstacles(Tile * tile, UserData userData) {
	if (TiGetType(tile) == Tile::OBSTACLE) {
		QList<Tile*> * obstacles = (QList<Tile*> *) userData;
		obstacles->append(tile);

	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////

int Panelizer::placeBestFit(Tile * tile, UserData userData) {
	if (TiGetType(tile) != Tile::SPACE) return 0;

	BestPlace * bestPlace = (BestPlace *) userData;
	TileRect tileRect;
	TiToRect(tile, &tileRect);
	int w = tileRect.xmaxi - tileRect.xmini;
	int h = tileRect.ymaxi - tileRect.ymini;
	if (bestPlace->width > w && bestPlace->height > w) {
		return 0;
	}

	int fitCount = 0;
	bool fit[4];
	double area[4];
	for (int i = 0; i < 4; i++) {
		fit[i] = false;
		area[i] = Worst;
	}

	if (w >= bestPlace->width && h >= bestPlace->height) {
		fit[0] = true;
		area[0] = (w * h) - (bestPlace->width * bestPlace->height);
		fitCount++;
	}
	if (h >= bestPlace->width && w >= bestPlace->height) {
		fit[1] = true;
		area[1] = (w * h) - (bestPlace->width * bestPlace->height);
		fitCount++;
	}

	if (!fit[0] && w >= bestPlace->width) {
		// see if adjacent tiles below are open
		TileRect temp;
		temp.xmini = tileRect.xmini;
		temp.xmaxi = temp.xmini + bestPlace->width;
		temp.ymini = tileRect.ymini;
		temp.ymaxi = temp.ymini + bestPlace->height;
		if (temp.ymaxi < bestPlace->maxRect.ymaxi) {
			QList<Tile*> spaces;
			TiSrArea(tile, bestPlace->plane, &temp, allSpaces, &spaces);
			if (spaces.count()) {
				int y = temp.ymaxi;
				foreach (Tile * t, spaces) {
					if (YMAX(t) > y) y = YMAX(t);			// find the bottom of the lowest open tile
				}
				fit[2] = true;
				fitCount++;
				area[2] = (w * (y - temp.ymini)) - (bestPlace->width * bestPlace->height);
			}
		}
	}

	if (!fit[1] && w >= bestPlace->height) {
		// see if adjacent tiles below are open
		TileRect temp;
		temp.xmini = tileRect.xmini;
		temp.xmaxi = temp.xmini + bestPlace->height;
		temp.ymini = tileRect.ymini;
		temp.ymaxi = temp.ymini + bestPlace->width;
		if (temp.ymaxi < bestPlace->maxRect.ymaxi) {
			QList<Tile*> spaces;
			TiSrArea(tile, bestPlace->plane, &temp, allSpaces, &spaces);
			if (spaces.count()) {
				int y = temp.ymaxi;
				foreach (Tile * t, spaces) {
					if (YMAX(t) > y) y = YMAX(t);			// find the bottom of the lowest open tile
				}
				fit[3] = true;
				fitCount++;
				area[3] = (w * (y - temp.ymini)) - (bestPlace->width * bestPlace->height);
			}
		}
	}

	if (fitCount == 0) return 0;

	// area is white space remaining after board has been inserteds

	int result = -1;
	for (int i = 0; i < 4; i++) {
		if (area[i] < bestPlace->bestArea) {
			result = i;
			break;
		}
	}
	if (result < 0) return 0;			// current bestArea is better

	bestPlace->bestTile = tile;
	bestPlace->bestTileRect = tileRect;
	if (fitCount == 1 || (bestPlace->width == bestPlace->height)) {
		if (fit[0] || fit[2]) {
			bestPlace->rotate90 = false;
		}
		else {
			bestPlace->rotate90 = true;
		}
		bestPlace->bestArea = area[result];
		return 0;
	}

	if (TiGetType(BL(tile)) == Tile::BUFFER) {
		// this is a leftmost tile
		// select for creating the biggest area after putting in the tile;
		double a1 = (w - bestPlace->width) * (bestPlace->height);
		double a2 = (h - bestPlace->height) * w;
		double a = qMax(a1, a2);
		double b1 = (w - bestPlace->height) * (bestPlace->width);
		double b2 = (h - bestPlace->width) * w;
		double b = qMax(b1, b2);
		bestPlace->rotate90 = (a < b);
		if (bestPlace->rotate90) {
			bestPlace->bestArea = fit[1] ? area[1] : area[3];
		}
		else {
			bestPlace->bestArea = fit[0] ? area[0] : area[2];
		}

		return 0;
	}

	TileRect temp;
	temp.xmini = bestPlace->maxRect.xmini;
	temp.xmaxi = tileRect.xmini - 1;
	temp.ymini = tileRect.ymini;
	temp.ymaxi = tileRect.ymaxi;
	QList<Tile*> obstacles;
	TiSrArea(tile, bestPlace->plane, &temp, allObstacles, &obstacles);
	int maxBottom = 0;
	foreach (Tile * obstacle, obstacles) {
		if (YMAX(obstacle) > maxBottom) maxBottom = YMAX(obstacle);
	}

	if (tileRect.ymini + bestPlace->width <= maxBottom && tileRect.ymini + bestPlace->height <= maxBottom) {
		// use the max length
		if (bestPlace->width >= bestPlace->height) {
			bestPlace->rotate90 = true;
			bestPlace->bestArea = fit[1] ? area[1] : area[3];
		}
		else {
			bestPlace->rotate90 = false;
			bestPlace->bestArea = fit[0] ? area[0] : area[2];
		}

		return 0;
	}

	if (tileRect.ymini + bestPlace->width > maxBottom && tileRect.ymini + bestPlace->height > maxBottom) {
		// use the min length
		if (bestPlace->width <= bestPlace->height) {
			bestPlace->rotate90 = true;
			bestPlace->bestArea = fit[1] ? area[1] : area[3];
		}
		else {
			bestPlace->rotate90 = false;
			bestPlace->bestArea = fit[0] ? area[0] : area[2];
		}

		return 0;
	}

	if (tileRect.ymini + bestPlace->width <= maxBottom) {
		bestPlace->rotate90 = true;
		bestPlace->bestArea = fit[1] ? area[1] : area[3];
		return 0;
	}

	bestPlace->rotate90 = false;
	bestPlace->bestArea = fit[0] ? area[0] : area[2];
	return 0;
}
