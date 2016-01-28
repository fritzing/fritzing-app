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

$Revision: 6705 $:
$Author: irascibl@gmail.com $:
$Date: 2012-12-12 15:36:22 +0100 (Mi, 12. Dez 2012) $

********************************************************************/

#ifndef JROUTER_H
#define JROUTER_H

#include <QAction>
#include <QHash>
#include <QVector>
#include <QList>
#include <QSet>
#include <QPointF>
#include <QGraphicsItem>
#include <QLine>
#include <QProgressDialog>
#include <QUndoCommand>
#include <QPointer>
#include <QGraphicsLineItem>

#include <limits>

#include "../../viewgeometry.h"
#include "../../viewlayer.h"
#include "../autorouter.h"
#include "tile.h"

struct TilePointRect
{
	TilePoint tilePoint;
	TileRect tileRect;
};


class GridEntry : public QGraphicsRectItem {
public:

public:
	GridEntry(QRectF &, QGraphicsItem * parent); 
	bool drawn();
	void setDrawn(bool);

protected:
	bool m_drawn;
};

////////////////////////////////////

class CMRouter : public QObject
{
	Q_OBJECT

public:
	CMRouter(class PCBSketchWidget *, ItemBase * board, bool adjustIf);
	~CMRouter(void);

public:
	enum OverlapType {
		IgnoreAllOverlaps = 0,
		ClipAllOverlaps,
		ReportAllOverlaps,
		AllowEquipotentialOverlaps
	};

public:
	Plane * initPlane(bool rotate90);
	Tile * insertTile(Plane* thePlane, QRectF &tileRect, QList<Tile *> &alreadyTiled, QGraphicsItem *, Tile::TileType type, CMRouter::OverlapType);
	TileRect boardRect();
	void setKeepout(double);
    void drcClean();

protected:

	GridEntry * drawGridItem(Tile * tile);
	void hideTiles();
	void displayBadTiles(QList<Tile *> & alreadyTiled);
	void displayBadTileRect(TileRect & tileRect);
	Tile * addTile(class NonConnectorItem * nci, Tile::TileType type, Plane *, QList<Tile *> & alreadyTiled, CMRouter::OverlapType);
	Tile * insertTile(Plane* thePlane, TileRect &tileRect, QList<Tile *> &alreadyTiled, QGraphicsItem *, Tile::TileType type, CMRouter::OverlapType);
	void clearGridEntries();
	void cleanPoints(QList<QPointF> & allPoints, Plane *); 
	bool insideV(const QPointF & check, const QPointF & vertex);
	void makeAlignTiles(QMultiHash<Tile *, TileRect *> &, Plane * thePlane);
	bool overlapsOnly(QGraphicsItem * item, QList<Tile *> & alreadyTiled);
	void clearPlane(Plane * thePlane);
	bool allowEquipotentialOverlaps(QGraphicsItem * item, QList<Tile *> & alreadyTiled);
	void insertUnion(TileRect & tileRect, QGraphicsItem *, Tile::TileType tileType);
	void drawTileRect(TileRect & tileRect, QColor & color);
	void setUpWidths(double width);

protected:
	TileRect m_tileMaxRect;
	QRectF m_maxRect90;
	TileRect m_overlappingTileRect;
	QList<Plane *> m_planes;
	Plane * m_unionPlane;
	Plane * m_union90Plane;
    ItemBase * m_board;
    QRectF m_maxRect;
	QString m_error;
    PCBSketchWidget * m_sketchWidget;
    double m_keepoutPixels;
};

#endif
