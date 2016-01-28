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

$Revision: 6955 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-06 23:14:37 +0200 (Sa, 06. Apr 2013) $

********************************************************************/

#ifndef MAZEROUTER_H
#define MAZEROUTER_H

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

#include <limits>
#include <queue>

#include "../../viewgeometry.h"
#include "../../viewlayer.h"
#include "../../commands.h"
#include "../autorouter.h"

typedef quint64 GridValue;

struct GridPoint {
    int x, y, z;
    GridValue baseCost;
    double qCost;
    uchar flags;

    bool operator<(const GridPoint&) const;
    GridPoint(QPoint, int);
    GridPoint();
};

struct PointZ {
    QPointF p;
    int z;

    PointZ(QPointF _p, int _z) {
        p = _p;
        z = _z;
    }
};

struct Net {
    QList<class ConnectorItem *>* net;
    QList< QList<ConnectorItem *> > subnets;
    int pinsWithin;
    int id;
};

struct NetList {
    QList<Net *> nets;
};

struct Trace {
    int netIndex;
    int order;
    uchar flags;
    QList<GridPoint> gridPoints;
    
    Trace() {
        flags = 0;
    }
};

struct NetOrdering {
    QList<int> order;
};

struct Score {
    NetOrdering ordering;
    QMultiHash<int, Trace> traces;
    QHash<int, int> routedCount;
    QHash<int, int> viaCount;
	int totalRoutedCount;
	int totalViaCount;
    int reorderNet;
    bool anyUnrouted;

	Score();
    void setOrdering(const NetOrdering &);
};

struct Nearest {
    int i, j;
    double distance;
    ConnectorItem * ic;
    ConnectorItem * jc;
};

struct Grid {
    GridValue * data;
    int x;
    int y;
    int z;

    Grid(int x, int y, int layers);

    GridValue at(int x, int y, int z) const;
    void setAt(int x, int y, int z, GridValue value);
    QList<QPoint> init(int x, int y, int z, int width, int height, const QImage &, GridValue value, bool collectPoints);
    QList<QPoint> init4(int x, int y, int z, int width, int height, const QImage *, GridValue value, bool collectPoints);
    void clear();
    void free();
    void copy(int fromIndex, int toIndex);
};


struct NetElements {
    QList<QDomElement> net;
    QList<QDomElement> alsoNet;
    QList<QDomElement> notNet;
};

struct RouteThing {
    QRectF r;
    QRectF r4;
    QList<ViewLayer::ViewLayerPlacement> layerSpecs;
    Nearest nearest;
    std::priority_queue<GridPoint> sourceQ;
    std::priority_queue<GridPoint> targetQ;
    QPoint gridSourcePoint;
    QPoint gridTargetPoint;
    GridValue sourceValue;
    GridValue targetValue;
    double bestDistanceToTarget;
    double bestDistanceToSource;
    GridPoint bestLocationToTarget;
    GridPoint bestLocationToSource;
    bool unrouted;
    NetElements netElements[2];
    QSet<int> avoids;
};

struct TraceThing {
    QList<TraceWire *> newTraces;
    QList<Via *> newVias;
    QList<JumperItem *> newJumperItems;
    QList<SymbolPaletteItem *> newNetLabels;
    JumperItem * jumperItem;
    SymbolPaletteItem * netLabel;
    QPointF topLeft;
    GridPoint nextTraceStart;
};

struct ConnectionThing {
    QMultiHash<ConnectorItem *, QPointer<ConnectorItem> > sd;

    void add(ConnectorItem * s, ConnectorItem * d);
    void remove(ConnectorItem * s);
    void remove(ConnectorItem * s, ConnectorItem * d);
    bool multi(ConnectorItem * s);  
    QList<ConnectorItem *> values(ConnectorItem * s);
};

typedef bool (*JumperWillFitFunction)(GridPoint &, const Grid *, int halfSize);
typedef double (*CostFunction)(const QPoint & p1, const QPoint & p2);

////////////////////////////////////

class MazeRouter : public Autorouter
{
	Q_OBJECT

public:
	MazeRouter(class PCBSketchWidget *, QGraphicsItem * board, bool adjustIf);
	~MazeRouter(void);

	void start();

protected:
    void setUpWidths(double width);
    int findPinsWithin(QList<ConnectorItem *> * net);
    bool makeBoard(QImage *, double keepout, const QRectF & r);
    bool makeMasters(QString &);
	bool routeNets(NetList &, bool makeJumper, Score & currentScore, const QSizeF gridSize, QList<NetOrdering> & allOrderings);
    bool routeOne(bool makeJumper, Score & currentScore, int netIndex, RouteThing &, QList<NetOrdering> & allOrderings);
    void findNearestPair(QList< QList<ConnectorItem *> > & subnets, Nearest &);
    void findNearestPair(QList< QList<ConnectorItem *> > & subnets, int i, QList<ConnectorItem *> & inet, Nearest &);
    QList<QPoint> renderSource(QDomDocument * masterDoc, int z, ViewLayer::ViewLayerPlacement, Grid * grid, QList<QDomElement> & netElements, QList<ConnectorItem *> & subnet, GridValue value, bool clearElements, const QRectF & r);
    QList<GridPoint> route(RouteThing &, int & viaCount);
    void expand(GridPoint &, RouteThing &);
    void expandOne(GridPoint &, RouteThing &, int dx, int dy, int dz, bool crossLayer);
    bool viaWillFit(GridPoint &, Grid * grid);
    QList<GridPoint> traceBack(GridPoint, Grid *, int & viaCount, GridValue sourceValue, GridValue targetValue);
    GridPoint traceBackOne(GridPoint &, Grid *, int dx, int dy, int dz, GridValue sourceValue, GridValue targetValue);
    void updateDisplay(int iz);
    void updateDisplay(Grid *, int iz);
    void updateDisplay(GridPoint &);
    void clearExpansion(Grid * grid);
    void prepSourceAndTarget(QDomDocument * masterdoc, RouteThing &, QList< QList<ConnectorItem *> > & subnets, int z, ViewLayer::ViewLayerPlacement); 
    bool moveBack(Score & currentScore, int index, QList<NetOrdering> & allOrderings);
    void displayTrace(Trace &);
    void initTraceDisplay();
    void traceObstacles(QList<Trace> & traces, int netIndex, Grid * grid, int ikeepout);
    void traceAvoids(QList<Trace> & traces, int netIndex, RouteThing & routeThing);
    bool routeNext(bool makeJumper, RouteThing &, QList< QList<ConnectorItem *> > & subnets, Score & currentScore, int netIndex, QList<NetOrdering> & allOrderings);
    void cleanUpNets(NetList &);
    void createTraces(NetList & netList, Score & bestScore, QUndoCommand * parentCommand);
    void createTrace(Trace &, QList<GridPoint> &, TraceThing &, ConnectionThing &, Net *);
    void removeColinear(QList<GridPoint> & gridPoints);
    void removeSteps(QList<GridPoint> & gridPoints);
    void removeStep(int ix, QList<GridPoint> & gridPoints);
    ConnectorItem * findAnchor(GridPoint gp, TraceThing &, Net * net, QPointF & p, bool & onTrace, ConnectorItem * already);
    ConnectorItem * findAnchor(GridPoint gp, const QRectF &, TraceThing &, Net * net, QPointF & p, bool & onTrace, ConnectorItem * already);
    void addConnectionToUndo(ConnectorItem * from, ConnectorItem * to, QUndoCommand * parentCommand);
    void addViaToUndo(Via *, QUndoCommand * parentCommand);
    void addJumperToUndo(JumperItem *, QUndoCommand * parentCommand);
    void routeJumper(int netIndex, RouteThing &, Score & currentScore);
    void insertTrace(Trace & newTrace, int netIndex, Score & currentScore, int viaCount, bool incRouted);
    SymbolPaletteItem * makeNetLabel(GridPoint & center, SymbolPaletteItem * pairedNetLabel, uchar traceFlags);
    void addNetLabelToUndo(SymbolPaletteItem * netLabel, QUndoCommand * parentCommand);
    GridPoint lookForJumper(GridPoint initial, GridValue targetValue, QPoint targetLocation);
    void expandOneJ(GridPoint & gridPoint, std::priority_queue<GridPoint> & pq, int dx, int dy, int dz, GridValue targetValue, QPoint targetLocation, QSet<int> & already);
    void removeOffBoardAnd(bool isPCBType, bool removeSingletons, bool bothSides);
    void optimizeTraces(QList<int> & order, QMultiHash<int, QList< QPointer<TraceWire> > > &, QMultiHash<int, Via *> &, QMultiHash<int, JumperItem *> &, QMultiHash<int, SymbolPaletteItem *> &, NetList &, ConnectionThing &);
    void reducePoints(QList<QPointF> & points, QPointF topLeft, QList<TraceWire *> & bundle, int startIndex, int endIndex, ConnectionThing &, int netIndex, ViewLayer::ViewLayerPlacement);

public slots:
     void incCommandProgress();
	 void setMaxCycles(int);

protected:
	LayerList m_viewLayerIDs;
    QHash<ViewLayer::ViewLayerPlacement, QDomDocument *> m_masterDocs;
    double m_keepoutMils;
    double m_keepoutGrid;
    int m_keepoutGridInt;
    int m_halfGridViaSize;
    int m_halfGridJumperSize;
    double m_gridPixels;
    double m_standardWireWidth;
    QImage * m_displayImage[2];
    QImage * m_boardImage;
    QImage * m_spareImage;
    QImage * m_spareImage2;
    QGraphicsPixmapItem * m_displayItem[2];
    bool m_temporaryBoard;
    CostFunction m_costFunction;
    JumperWillFitFunction m_jumperWillFitFunction;
    uint m_traceColors[2];
    Grid * m_grid;
    int m_cleanupCount;
    int m_netLabelIndex;
    int m_commandCount;
};

#endif
