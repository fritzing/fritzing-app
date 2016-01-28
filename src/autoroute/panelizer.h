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

$Revision: 6968 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-15 17:44:14 +0200 (Mo, 15. Apr 2013) $

********************************************************************/

#ifndef PANELIZER_H
#define PANELIZER_H

#include <QString>
#include <QSizeF>

#include "../mainwindow/mainwindow.h"
#include "../items/itembase.h"
#include "cmrouter/tile.h"


struct PlanePair
{
	Plane * thePlane;
	Plane * thePlane90;
	TileRect tilePanelRect;
	TileRect tilePanelRect90;
    double panelWidth;
    double panelHeight;
	QStringList svgs;
	QString layoutSVG;
	int index;
};

struct PanelItem {
	// per window
	QString boardName;
	QString path;
	int required;
	int maxOptional;
    int optionalPriority;
    int produced;
	QSizeF boardSizeInches;
    long boardID;
    PanelItem * refPanelItem;

	// per instance
	double x, y;
	bool rotate90;
	PlanePair * planePair;

	PanelItem();

	PanelItem(PanelItem * from); 
};

struct BestPlace
{
	Tile * bestTile;
	TileRect bestTileRect;
	TileRect maxRect;
	int width;
	int height;
	double bestArea;
	bool rotate90;
	Plane* plane;

    BestPlace();
};

struct PanelType {
    double width;
    double height;
    double c1;
    double c2;
    QString name;
};

struct PanelParams
{
    QList<PanelType *> panelTypes;
	double panelSpacing;
	double panelBorder;
	QString prefix;
};

struct LayerThing {
    LayerList layerList;
    QString name;
    SVG2gerber::ForWhy forWhy;
    QString suffix;

    LayerThing(const QString & n, LayerList ll, SVG2gerber::ForWhy fw, const QString & s) {
            layerList = ll;
            name = n;
            forWhy = fw;
            suffix = s;
    }
};

class Panelizer
{
public:
	static void panelize(class FApplication *, const QString & panelFilename, bool customPartsOnly);
	static void inscribe(class FApplication *, const QString & panelFilename, bool drc, bool noMessages);
	static int placeBestFit(Tile * tile, UserData userData);
    static int checkDonuts(MainWindow *, bool displayMessage);
    static int checkText(MainWindow *, bool displayMessage);

protected:
	static bool initPanelParams(QDomElement & root, PanelParams &);
	static PlanePair * makePlanePair(PanelParams &, bool big);
	static void collectFiles(const QDir & outputDir, QDomElement & path, QHash<QString, QString> & fzzFilePaths);
	static bool checkBoards(QDomElement & board, QHash<QString, QString> & fzzFilePaths);
	static bool openWindows(QDomElement & board, QHash<QString, QString> & fzzFilePaths, class FApplication *, PanelParams &, QDir & fzDir, QDir & svgDir, QList<PanelItem *> & refPanelItems, QList<LayerThing> & layerThingList, bool customPartsOnly, QDir & copyDir);
	static void bestFit(QList<PanelItem *> & insertPanelItems, PanelParams &, QList<PlanePair *> &, bool customPartsOnly);
	static bool bestFitOne(PanelItem * panelItem, PanelParams & panelParams, QList<PlanePair *> & planePairs, bool createNew, bool customPartsOnly);
	static void addOptional(int optionalCount, QList<PanelItem *> & refPanelItems, QList<PanelItem *> & insertPanelItems, PanelParams &, QList<PlanePair *> &);
	static class MainWindow * inscribeBoard(QDomElement & board, QHash<QString, QString> & fzzFilePaths, FApplication * app, QDir & fzDir, bool drc, bool noMessages, QDir & copyDir);
    static void doOnePanelItem(PlanePair * planePair, QList<LayerThing> & layerThingList, PanelItem * panelItem, QDir & svgDir);
    static void makeSVGs(MainWindow *, ItemBase *, const QString & boardName, QList<LayerThing> & layerThingList, QDir & saveDir, QFileInfo & copyInfo);
    static void shrinkLastPanel( QList<PlanePair *> & planePairs, QList<PanelItem *> & insertPanelItems, PanelParams &, bool customPartsOnly);
    static int bestFitLoop(QList<PanelItem *> & refPanelItems, PanelParams &, bool customPartsOnly, QList<PlanePair *> & returnPlanePairs, QList<PanelItem *> & returnInsertPanelItems, const QDir & svgDir);
    static double calcCost(PanelParams &, QList<PlanePair *> &, int divisor);
    static void writePanelizerOutput(const QString & message);
    static void initPanelizerOutput(const QString & filename, const QString & initialMsg);
    static void collectFilenames(const QString & filenames);
    static void writePanelizerFilenames(const QString & panelFilename);
};

#endif
