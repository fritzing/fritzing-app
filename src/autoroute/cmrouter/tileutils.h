/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2016 Fritzing

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

********************************************************************

$Revision: 2979 $:
$Author: irascibl@gmail.com $:
$Date: 2013-02-26 16:26:03 +0100 (Di, 26. Feb 2013) $

********************************************************************/

#ifndef TILEUTILS
#define TILEUTILS

#include "tile.h"

static const int TILEFACTOR = 1000;

inline int fasterRealToTile(double x) {
        return qRound(x * TILEFACTOR);
}

inline void realsToTile(TileRect & tileRect, double l, double t, double r, double b) {
        tileRect.xmini = fasterRealToTile(l);
        tileRect.ymini = fasterRealToTile(t);
        tileRect.xmaxi = fasterRealToTile(r);
        tileRect.ymaxi = fasterRealToTile(b);
}

inline void qrectToTile(QRectF & rect, TileRect & tileRect) {
	realsToTile(tileRect, rect.left(), rect.top(), rect.right(), rect.bottom());
}

inline int realToTile(double x) {
	return qRound(x * TILEFACTOR);
}

inline double tileToReal(int x) {
	return x / ((double) TILEFACTOR);
}

inline void tileRectToQRect(TileRect & tileRect, QRectF & rect) {
	rect.setCoords(tileToReal(tileRect.xmini), tileToReal(tileRect.ymini), tileToReal(tileRect.xmaxi), tileToReal(tileRect.ymaxi));
}

inline void tileToQRect(Tile * tile, QRectF & rect) {
	TileRect tileRect;
	TiToRect(tile, &tileRect);
	tileRectToQRect(tileRect, rect);
}

inline void tileRotate90(TileRect & tileRect, TileRect & tileRect90)
{
	// x' = x*cos - y*sin
	// y' = x*sin + y*cos
	// where cos90 = 0 and sin90 = 1 (effectively clockwise)

	// rotate top right corner of rect
	tileRect90.xmini = -tileRect.ymaxi;
	tileRect90.ymini = tileRect.xmini;

	// swap width and height
	tileRect90.xmaxi = tileRect90.xmini + (tileRect.ymaxi - tileRect.ymini);
	tileRect90.ymaxi = tileRect90.ymini + (tileRect.xmaxi - tileRect.xmini);
}


inline void tileUnrotate90(TileRect & tileRect90, TileRect & tileRect)
{
	tileRect.xmini = tileRect90.ymini;
	tileRect.ymaxi = -tileRect90.xmini;

	// swap width and height
	tileRect.xmaxi = tileRect.xmini + (tileRect90.ymaxi - tileRect90.ymini);
	tileRect.ymini = tileRect.ymaxi - (tileRect90.xmaxi - tileRect90.xmini);
}

#endif
