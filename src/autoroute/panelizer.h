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

#include <QString>
#include <QSizeF>

#include "../mainwindow/mainwindow.h"
#include "../items/itembase.h"
#include "cmrouter/tile.h"

constexpr double Worst = std::numeric_limits<double>::max() / 4;
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
	int required = 0;
	int maxOptional = 0;
	int optionalPriority = 0;
	int produced = 0;
	QSizeF boardSizeInches;
	long boardID = 0;
	PanelItem * refPanelItem = nullptr;

	// per instance
	double x = 0.0, y = 0.0;
	bool rotate90 = false;
	PlanePair * planePair = nullptr;

	PanelItem() = default;
    PanelItem(const PanelItem& item);

	PanelItem(PanelItem * from) : PanelItem(*from) { }
};

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

struct PanelType {
	double width = 0.0;
	double height = 0.0;
	double c1 = 0.0;
	double c2 = 0.0;
	QString name;
};

struct PanelParams
{
	QList<PanelType *> panelTypes;
	double panelSpacing = 0.0;
	double panelBorder = 0.0;
	QString prefix;
};

struct LayerThing {
	LayerList layerList;
	QString name;
	SVG2gerber::ForWhy forWhy;
	QString suffix;

	LayerThing(const QString & n, LayerList ll, SVG2gerber::ForWhy fw, const QString & s) 
        : layerList(ll), name(n), forWhy(fw), suffix(s)
    {
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
