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

$Revision: 6980 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-22 01:45:43 +0200 (Mo, 22. Apr 2013) $

********************************************************************/

#include "panelizer.h"
#include "../debugdialog.h"
#include "../sketch/pcbsketchwidget.h"
#include "../utils/textutils.h"
#include "../utils/graphicsutils.h"
#include "../utils/folderutils.h"
#include "../utils/folderutils.h"
#include "../items/resizableboard.h"
#include "../items/logoitem.h"
#include "../items/groundplane.h"
#include "../fsvgrenderer.h"
#include "../fapplication.h"
#include "../svg/gerbergenerator.h"
#include "../referencemodel/referencemodel.h"
#include "../version/version.h"
#include "../processeventblocker.h"
#include "../connectors/connectoritem.h"
#include "../connectors/svgidlayer.h"

#include "cmrouter/tileutils.h"

#include <QFile>
#include <QDomDocument>
#include <QDomElement>
#include <QDir>
#include <qmath.h>
#include <limits>
#include <QPrinter>

static int OutlineLayer = 0;
static int SilkTopLayer = 0;
static QString PanelizerOutputPath;
static QSet<QString> PanelizerFileNames;

///////////////////////////////////////////////////////////

void collectTexts(const QString & svg, QStringList & strings) {
    QDomDocument doc;
    doc.setContent(svg);
    QDomElement root = doc.documentElement();
    QDomNodeList domNodeList = root.elementsByTagName("text");
	for (int i = 0; i < domNodeList.count(); i++) {
        QDomElement textElement = domNodeList.at(i).toElement();
        QString string;
        QDomNodeList childList = textElement.childNodes();
	    for (int j = 0; j < childList.count(); j++) {
		    QDomNode child = childList.item(j);
		    if (child.isText()) {
			    string.append(child.nodeValue());
		    }
	    }
        strings.append(string);
    }
}


bool byOptionalPriority(PanelItem * p1, PanelItem * p2) {
    return p1->optionalPriority > p2->optionalPriority;
}

bool areaGreaterThan(PanelItem * p1, PanelItem * p2)
{
	return p1->boardSizeInches.width() * p1->boardSizeInches.height() > p2->boardSizeInches.width() * p2->boardSizeInches.height();
}

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

static int PlanePairIndex = 0;

static double Worst = std::numeric_limits<double>::max() / 4;

int roomOn(Tile * tile, TileRect & tileRect, BestPlace * bestPlace)
{
	int w = tileRect.xmaxi - tileRect.xmini;
	int h = tileRect.ymaxi - tileRect.ymini;
	if (bestPlace->width <= w && bestPlace->height <= h) {
		bestPlace->bestTile = tile;
		return 1;
	}

	TileRect temp;
	temp.xmini = tileRect.xmini;
	temp.xmaxi = temp.xmini + bestPlace->width;
	temp.ymini = tileRect.ymini;
	temp.ymaxi = temp.ymini + bestPlace->height;
	QList<Tile*> spaces;
	TiSrArea(tile, bestPlace->plane, &temp, allSpaces, &spaces);
	if (spaces.count()) {
		bestPlace->bestTile = tile;
		return 1;
	}

	return 0;
}

int roomAnywhere(Tile * tile, UserData userData) 
{
	if (TiGetType(tile) != Tile::SPACE) return 0;

	BestPlace * bestPlace = (BestPlace *) userData;
	TileRect tileRect;
	TiToRect(tile, &tileRect);

	return roomOn(tile, tileRect, bestPlace);
}

int roomOnTop(Tile * tile, UserData userData) 
{
	if (TiGetType(tile) != Tile::SPACE) return 0;

	BestPlace * bestPlace = (BestPlace *) userData;
	TileRect tileRect;
	TiToRect(tile, &tileRect);

	if (tileRect.ymini != bestPlace->maxRect.ymini) return 0;

	return roomOn(tile, tileRect, bestPlace);
}

int roomOnBottom(Tile * tile, UserData userData) 
{
	if (TiGetType(tile) != Tile::SPACE) return 0;

	BestPlace * bestPlace = (BestPlace *) userData;
	TileRect tileRect;
	TiToRect(tile, &tileRect);

	if (tileRect.ymaxi != bestPlace->maxRect.ymaxi) return 0;

	return roomOn(tile, tileRect, bestPlace);
}

int roomOnLeft(Tile * tile, UserData userData) 
{
	if (TiGetType(tile) != Tile::SPACE) return 0;

	BestPlace * bestPlace = (BestPlace *) userData;
	TileRect tileRect;
	TiToRect(tile, &tileRect);

	if (tileRect.xmini != bestPlace->maxRect.xmini) return 0;

	return roomOn(tile, tileRect, bestPlace);
}

int roomOnRight(Tile * tile, UserData userData) 
{
	if (TiGetType(tile) != Tile::SPACE) return 0;

	BestPlace * bestPlace = (BestPlace *) userData;
	TileRect tileRect;
	TiToRect(tile, &tileRect);

	if (tileRect.xmaxi != bestPlace->maxRect.xmaxi) return 0;

	return roomOn(tile, tileRect, bestPlace);
}


BestPlace::BestPlace() {
    bestTile = NULL;
    bestArea = Worst;
}

PanelItem::PanelItem() {
    produced = 0;
    boardID = 0;
    refPanelItem = NULL;
}

PanelItem::PanelItem(PanelItem * from) {
	this->produced = from->produced;
	this->boardName = from->boardName;
	this->path = from->path;
	this->required = from->required;
	this->maxOptional = from->maxOptional;
	this->optionalPriority = from->optionalPriority;
	this->boardSizeInches = from->boardSizeInches;
	this->boardID = from->boardID;
    this->refPanelItem = from;
}

/////////////////////////////////////////////////////////////////////////////////

void Panelizer::panelize(FApplication * app, const QString & panelFilename, bool customPartsOnly) 
{
    QString msg = "panelize";
    if (customPartsOnly) msg += " custom parts";
    initPanelizerOutput(panelFilename, msg);

	QFile panelizerFile(panelFilename);

    QFileInfo info(panelFilename);
    QDir copyDir = info.absoluteDir();
    copyDir.cd("copies");
    if (!copyDir.exists()) {
		writePanelizerOutput(QString("unable to create 'copies' folder in '%1'").arg(info.absoluteDir().absolutePath()));
		return;
	}


	QString errorStr;
	int errorLine;
	int errorColumn;

	DebugDialog::setEnabled(true);

	QDomDocument panelizerDocument;
	if (!panelizerDocument.setContent(&panelizerFile, true, &errorStr, &errorLine, &errorColumn)) {
		writePanelizerOutput(QString("Unable to parse '%1': '%2' line:%3 column:%4").arg(panelFilename).arg(errorStr).arg(errorLine).arg(errorColumn));
		return;
	}

	QDomElement panelizerRoot = panelizerDocument.documentElement();
	if (panelizerRoot.isNull() || panelizerRoot.tagName() != "panelizer") {
		writePanelizerOutput(QString("root element is not 'panelizer'"));
		return;
	}

	PanelParams panelParams;
	if (!initPanelParams(panelizerRoot, panelParams)) return;

    QFileInfo pinfo(panelFilename);
	QDir outputDir = pinfo.absoluteDir();

    if (customPartsOnly) {
        outputDir.mkdir("custom");
        outputDir.cd("custom");
    }

	outputDir.mkdir("svg");
	outputDir.mkdir("gerber");
	outputDir.mkdir("fz");

	QDir svgDir(outputDir);
	svgDir.cd("svg");
	if (!svgDir.exists()) {
		writePanelizerOutput(QString("unable to create svg folder in '%1'").arg(pinfo.absolutePath()));
		return;
	}

	DebugDialog::debug(QString("svg folder '%1'\n").arg(svgDir.absolutePath()));

	QDir gerberDir(outputDir);
	gerberDir.cd("gerber");
	if (!gerberDir.exists()) {
		writePanelizerOutput(QString("unable to create gerber folder in '%1'").arg(pinfo.absolutePath()));
		return;
	}

	DebugDialog::debug(QString("gerber folder '%1'\n").arg(gerberDir.absolutePath()));

	QDir fzDir(outputDir);
	fzDir.cd("fz");
	if (!fzDir.exists()) {
		writePanelizerOutput(QString("unable to create fz folder in '%1'").arg(pinfo.absolutePath()));
		return;
	}

	DebugDialog::debug(QString("fz folder '%1'\n").arg(fzDir.absolutePath()));


	QDomElement boards = panelizerRoot.firstChildElement("boards");
	QDomElement board = boards.firstChildElement("board");
	if (board.isNull()) {
		writePanelizerOutput(QString("no <board> elements found"));
		return;
	}

	QHash<QString, QString> fzzFilePaths;
	QDomElement paths = panelizerRoot.firstChildElement("paths");
	QDomElement path = paths.firstChildElement("path");
	if (path.isNull()) {
		writePanelizerOutput(QString("no <path> elements found"));
		return;
	}

	collectFiles(pinfo.absoluteDir(), path, fzzFilePaths);
    if (fzzFilePaths.count() == 0) {
		writePanelizerOutput(QString("no fzz files found in paths"));
		return;
	}
	
	board = boards.firstChildElement("board");
	if (!checkBoards(board, fzzFilePaths)) return;

    app->createUserDataStoreFolderStructures();
	app->registerFonts();
	app->loadReferenceModel("", false);

    QList<LayerThing> layerThingList;
	layerThingList.append(LayerThing("outline", ViewLayer::outlineLayers(), SVG2gerber::ForOutline, GerberGenerator::OutlineSuffix));  
	layerThingList.append(LayerThing("copper_top", ViewLayer::copperLayers(ViewLayer::NewTop), SVG2gerber::ForCopper, GerberGenerator::CopperTopSuffix));
	layerThingList.append(LayerThing("copper_bottom", ViewLayer::copperLayers(ViewLayer::NewBottom), SVG2gerber::ForCopper, GerberGenerator::CopperBottomSuffix));
	layerThingList.append(LayerThing("mask_top", ViewLayer::maskLayers(ViewLayer::NewTop), SVG2gerber::ForMask, GerberGenerator:: MaskTopSuffix));
	layerThingList.append(LayerThing("mask_bottom", ViewLayer::maskLayers(ViewLayer::NewBottom), SVG2gerber::ForMask, GerberGenerator::MaskBottomSuffix));
	layerThingList.append(LayerThing("paste_mask_top", ViewLayer::maskLayers(ViewLayer::NewTop), SVG2gerber::ForPasteMask, GerberGenerator:: PasteMaskTopSuffix));
	layerThingList.append(LayerThing("paste_mask_bottom", ViewLayer::maskLayers(ViewLayer::NewBottom), SVG2gerber::ForPasteMask, GerberGenerator::PasteMaskBottomSuffix));
	layerThingList.append(LayerThing("silk_top", ViewLayer::silkLayers(ViewLayer::NewTop), SVG2gerber::ForSilk, GerberGenerator::SilkTopSuffix));
	layerThingList.append(LayerThing("silk_bottom", ViewLayer::silkLayers(ViewLayer::NewBottom), SVG2gerber::ForSilk, GerberGenerator::SilkBottomSuffix));
	layerThingList.append(LayerThing("drill", ViewLayer::drillLayers(), SVG2gerber::ForDrill, GerberGenerator::DrillSuffix));

    for (int i = 0; i < layerThingList.count(); i++) {
        LayerThing layerThing = layerThingList.at(i);
        if (layerThing.name.compare("outline") == 0) OutlineLayer = i;
        else if (layerThing.name.compare("silk_top") == 0) SilkTopLayer = i;
    }

	QList<PanelItem *> refPanelItems;
	board = boards.firstChildElement("board");
	if (!openWindows(board, fzzFilePaths, app, panelParams, fzDir, svgDir, refPanelItems, layerThingList, customPartsOnly, copyDir)) return;

	QList<PlanePair *> planePairs;
    QList<PanelItem *> insertPanelItems;
    int duplicates = bestFitLoop(refPanelItems, panelParams, customPartsOnly, planePairs, insertPanelItems, svgDir);

    foreach (PanelItem * panelItem, insertPanelItems) {
        if (panelItem->refPanelItem) {
            panelItem->refPanelItem->produced += duplicates;
        }
    }

    QString custom = customPartsOnly ? "custom." : "";

	foreach (PlanePair * planePair, planePairs) {
		planePair->layoutSVG += "</svg>"; 
		QString fname = svgDir.absoluteFilePath(QString("%1%2.panel_%3.layout.svg").arg(custom).arg(panelParams.prefix).arg(planePair->index));
        TextUtils::writeUtf8(fname, planePair->layoutSVG);
	}

	foreach (PlanePair * planePair, planePairs) {
		for (int i = 0; i < layerThingList.count(); i++) {
			planePair->svgs << TextUtils::makeSVGHeader(1, GraphicsUtils::StandardFritzingDPI, planePair->panelWidth, planePair->panelHeight);
		}

        QList<PanelItem *> needToRotate;
		foreach (PanelItem * panelItem, insertPanelItems) {
			if (panelItem->planePair != planePair) continue;

			DebugDialog::debug(QString("placing %1 on panel %2").arg(panelItem->boardName).arg(planePair->index));

            doOnePanelItem(planePair, layerThingList, panelItem, svgDir);
		}


		DebugDialog::debug("after placement");
		QString prefix = QString("%1%2.panel_%3").arg(custom).arg(panelParams.prefix).arg(planePair->index);
		for (int i = 0; i < planePair->svgs.count(); i++) {
			if (planePair->svgs.at(i).isEmpty()) continue;

			planePair->svgs.replace(i, planePair->svgs.at(i) + "</svg>");

			QString fname = svgDir.absoluteFilePath(QString("%1.%2.svg").arg(prefix).arg(layerThingList.at(i).name));
			TextUtils::writeUtf8(fname, planePair->svgs.at(i));

			QString suffix = layerThingList.at(i).suffix;
			DebugDialog::debug("converting " + prefix + " " + suffix);
			QSizeF svgSize(planePair->panelWidth, planePair->panelHeight);
			SVG2gerber::ForWhy forWhy = layerThingList.at(i).forWhy;
			if (forWhy == SVG2gerber::ForMask || forWhy == SVG2gerber::ForPasteMask) forWhy = SVG2gerber::ForCopper;
			GerberGenerator::doEnd(planePair->svgs.at(i), 2, layerThingList.at(i).name, forWhy, svgSize * GraphicsUtils::StandardFritzingDPI, gerberDir.absolutePath(), prefix, suffix, false);
			DebugDialog::debug("after converting " + prefix + " " + suffix);
		}

		QDomDocument doc;
		QString merger = planePair->svgs.at(OutlineLayer);  
		merger.replace("black", "#90f0a0");
		merger.replace("#000000", "#90f0a0");
		merger.replace("fill-opacity=\"0.5\"", "fill-opacity=\"1\"");

        //DebugDialog::debug("outline identification");
        //DebugDialog::debug(merger);
        //DebugDialog::debug("");

		TextUtils::mergeSvg(doc, merger, "");
		merger = planePair->svgs.at(SilkTopLayer);			
		merger.replace("black", "#909090");
		merger.replace("#000000", "#909090");

        //DebugDialog::debug("silk identification");
        //DebugDialog::debug(merger);
        //DebugDialog::debug("");

		TextUtils::mergeSvg(doc, merger, "");		
		merger = planePair->layoutSVG;				// layout
		merger.replace("'red'", "'none'");		// hide background rect

        //DebugDialog::debug("layout identification");
        //DebugDialog::debug(merger);
        //DebugDialog::debug("");

		TextUtils::mergeSvg(doc, merger, "");
		merger = TextUtils::mergeSvgFinish(doc);

        int endix = merger.lastIndexOf("</svg>");
        QString text = "<text x='%1' y='%2' text-anchor='end' fill='#000000' stroke='none' stroke-width='0' font-size='250' font-family='OCRA'>%3 panel %4  %5 %6</text>\n";
        text = text
                .arg(planePair->panelWidth * GraphicsUtils::StandardFritzingDPI)
                .arg(planePair->panelHeight * GraphicsUtils::StandardFritzingDPI + (250 / 2))
                .arg(outputDir.dirName())
                .arg(planePair->index)
                .arg(panelParams.prefix)
                .arg(QTime::currentTime().toString())
                ;

        merger.insert(endix, text);

		QString fname = svgDir.absoluteFilePath(QString("%1.%2.svg").arg(prefix).arg("identification"));
		TextUtils::writeUtf8(fname, merger);

        // save to pdf
		QPrinter printer(QPrinter::HighResolution);
		printer.setOutputFormat(QPrinter::PdfFormat);
        QString pdfname = fname;
        pdfname.replace(".svg", ".pdf");
		printer.setOutputFileName(pdfname);
		int res = printer.resolution();
			
		// now convert to pdf
		QSvgRenderer svgRenderer;
		svgRenderer.load(merger.toLatin1());
		double trueWidth = planePair->panelWidth;
		double trueHeight = planePair->panelHeight;
		QRectF target(0, 0, trueWidth * res, trueHeight * res);

		QSizeF psize((target.width() + printer.paperRect().width() - printer.width()) / res, 
						(target.height() + printer.paperRect().height() - printer.height()) / res);
		printer.setPaperSize(psize, QPrinter::Inch);

		QPainter painter;
		if (painter.begin(&printer))
		{
			svgRenderer.render(&painter, target);
		}

		painter.end();
	}

    boards = panelizerRoot.firstChildElement("boards");
    board = boards.firstChildElement("board");
    while (!board.isNull()) {
        foreach (PanelItem * panelItem, refPanelItems) {
            if (panelItem->path.endsWith("/" + board.attribute("name"))) {
                board.setAttribute("produced", panelItem->produced);
                break;
            }
        }
        board = board.nextSiblingElement("board");
    }

    TextUtils::writeUtf8(panelFilename, panelizerDocument.toString(4));

    QString message = QString("Panelizer finished: %1 panel(s), with %2 additional copy(ies) for each panel").arg(planePairs.count()).arg(duplicates - 1);
    writePanelizerOutput("--------------------------------");
    writePanelizerOutput(message);
    QMessageBox::information(NULL, QObject::tr("Fritzing"), message);
}


void Panelizer::bestFit(QList<PanelItem *> & insertPanelItems, PanelParams & panelParams, QList<PlanePair *> & planePairs, bool customPartsOnly)
{
	foreach (PanelItem * panelItem, insertPanelItems) {
		bestFitOne(panelItem, panelParams, planePairs, true, customPartsOnly);
	}
}

bool Panelizer::bestFitOne(PanelItem * panelItem, PanelParams & panelParams, QList<PlanePair *> & planePairs, bool createNew, bool customPartsOnly)
{
	//DebugDialog::debug(QString("panel %1").arg(panelItem->boardName));
	BestPlace bestPlace1, bestPlace2;
	bestPlace1.rotate90 = bestPlace2.rotate90 = false;
	bestPlace1.width = bestPlace2.width = realToTile(panelItem->boardSizeInches.width() + panelParams.panelSpacing);
	bestPlace1.height = bestPlace2.height = realToTile(panelItem->boardSizeInches.height() + panelParams.panelSpacing);
	int ppix = 0;
	while (ppix < planePairs.count()) {
		PlanePair *  planePair = planePairs.at(ppix);
		bestPlace1.plane = planePair->thePlane;
		bestPlace2.plane = planePair->thePlane90;
		bestPlace1.maxRect = planePair->tilePanelRect;
		TiSrArea(NULL, planePair->thePlane, &planePair->tilePanelRect, placeBestFit, &bestPlace1);
        if (customPartsOnly) {
            // make sure board is rotated
            bestPlace1.bestTile = NULL;
            DebugDialog::debug("forcing rotation");
        }
		if (bestPlace1.bestTile == NULL) {
			bestPlace2.maxRect = planePair->tilePanelRect90;
			TiSrArea(NULL, planePair->thePlane90, &planePair->tilePanelRect90, placeBestFit, &bestPlace2);
		}
		if (bestPlace1.bestTile == NULL && bestPlace2.bestTile == NULL ) {
			if (++ppix < planePairs.count()) {
				// try next panel
				continue;
			}

			if (!createNew) {
				return false;
			}

			// create next panel
			planePair = makePlanePair(panelParams, true);
			planePairs << planePair;
			DebugDialog::debug(QString("ran out of room placing %1").arg(panelItem->boardName));
			continue;
		}

		bool use2 = false;
		if (bestPlace1.bestTile == NULL) {
			use2 = true;
		}
		else if (bestPlace2.bestTile == NULL) {
		}
		else {
			// never actually get here
			use2 = bestPlace2.bestArea < bestPlace1.bestArea;
		}

		if (use2) {
			tileUnrotate90(bestPlace2.bestTileRect, bestPlace1.bestTileRect);
			bestPlace1.rotate90 = !bestPlace2.rotate90;
		}

		panelItem->x = tileToReal(bestPlace1.bestTileRect.xmini) ;
		panelItem->y = tileToReal(bestPlace1.bestTileRect.ymini);
		panelItem->rotate90 = bestPlace1.rotate90;

		DebugDialog::debug(QString("setting rotate90:%1 %2").arg(panelItem->rotate90).arg(panelItem->path));
		panelItem->planePair = planePair;

		TileRect tileRect;
		tileRect.xmini = bestPlace1.bestTileRect.xmini;
		tileRect.ymini = bestPlace1.bestTileRect.ymini;
		if (bestPlace1.rotate90) {
			tileRect.xmaxi = tileRect.xmini + bestPlace1.height;
			tileRect.ymaxi = tileRect.ymini + bestPlace1.width;
		}
		else {
			tileRect.ymaxi = tileRect.ymini + bestPlace1.height;
			tileRect.xmaxi = tileRect.xmini + bestPlace1.width;
		}

		double w = panelItem->boardSizeInches.width();
		double h = panelItem->boardSizeInches.height();
		if (panelItem->rotate90) {
			w = h;
			h = panelItem->boardSizeInches.width();
		}

		planePair->layoutSVG += QString("<rect x='%1' y='%2' width='%3' height='%4' stroke='none' fill='red'/>\n")
			.arg(panelItem->x * GraphicsUtils::StandardFritzingDPI)
			.arg(panelItem->y * GraphicsUtils::StandardFritzingDPI)
			.arg(GraphicsUtils::StandardFritzingDPI * w)
			.arg(GraphicsUtils::StandardFritzingDPI * h);

		QStringList strings = QFileInfo(panelItem->path).completeBaseName().split("_");
        if (strings.count() >= 5) {
            QString start = strings.takeFirst();
            QStringList middle;
            middle << strings.at(0) << strings.at(1) << strings.at(2);
            strings.removeAt(0);
            strings.removeAt(0);
            strings.removeAt(0);
            QString end = strings.join("_");
            strings.clear();
            strings << start << middle.join(" ") << end;
        }
		double cx = GraphicsUtils::StandardFritzingDPI * (panelItem->x + (w / 2));
		int fontSize1 = 250;
		int fontSize2 = 150;
		int fontSize = fontSize1;
		double cy = GraphicsUtils::StandardFritzingDPI * (panelItem->y + (h  / 2));
		cy -= ((strings.count() - 1) * fontSize2 / 2);
		foreach (QString string, strings) {
			planePair->layoutSVG += QString("<text x='%1' y='%2' anchor='middle' font-family='%5' stroke='none' fill='#000000' text-anchor='middle' font-size='%3'>%4</text>\n")
				.arg(cx)
				.arg(cy)
				.arg(fontSize)
				.arg(string)
                .arg(OCRAFontName);
			cy += fontSize;
			if (fontSize == fontSize1) fontSize = fontSize2;
		}

		TiInsertTile(planePair->thePlane, &tileRect, NULL, Tile::OBSTACLE);
		TileRect tileRect90;
		tileRotate90(tileRect, tileRect90);
		TiInsertTile(planePair->thePlane90, &tileRect90, NULL, Tile::OBSTACLE);

		return true;
	}

	DebugDialog::debug("bestFitOne should never reach here");
	return false;
}

PlanePair * Panelizer::makePlanePair(PanelParams & panelParams, bool big)
{
	PlanePair * planePair = new PlanePair;

    if (big) {
        foreach (PanelType * panelType, panelParams.panelTypes) {
            if (panelType->name == "big") {
                planePair->panelWidth = panelType->width;
                planePair->panelHeight = panelType->height;
                break;
            }
        }
    }
    else {
        foreach (PanelType * panelType, panelParams.panelTypes) {
            if (panelType->name == "small") {
                planePair->panelWidth = panelType->width;
                planePair->panelHeight = panelType->height;
                break;
            }
        }
    }

	// for debugging
	planePair->layoutSVG = TextUtils::makeSVGHeader(1, GraphicsUtils::StandardFritzingDPI, planePair->panelWidth, planePair->panelHeight);
	planePair->index = PlanePairIndex++;

	Tile * bufferTile = TiAlloc();
	TiSetType(bufferTile, Tile::BUFFER);
	TiSetBody(bufferTile, NULL);

	QRectF panelRect(0, 0, planePair->panelWidth + panelParams.panelSpacing - panelParams.panelBorder, 
							planePair->panelHeight + panelParams.panelSpacing - panelParams.panelBorder);

    int l = fasterRealToTile(panelRect.left() - 10);
    int t = fasterRealToTile(panelRect.top() - 10);
    int r = fasterRealToTile(panelRect.right() + 10);
    int b = fasterRealToTile(panelRect.bottom() + 10);
    SETLEFT(bufferTile, l);
    SETYMIN(bufferTile, t);		

	planePair->thePlane = TiNewPlane(bufferTile, l, t, r, b);

    SETRIGHT(bufferTile, r);
	SETYMAX(bufferTile, b);		

	qrectToTile(panelRect, planePair->tilePanelRect);
	TiInsertTile(planePair->thePlane, &planePair->tilePanelRect, NULL, Tile::SPACE); 

	QMatrix matrix90;
	matrix90.rotate(90);
	QRectF panelRect90 = matrix90.mapRect(panelRect);

	Tile * bufferTile90 = TiAlloc();
	TiSetType(bufferTile90, Tile::BUFFER);
	TiSetBody(bufferTile90, NULL);

    l = fasterRealToTile(panelRect90.left() - 10);
    t = fasterRealToTile(panelRect90.top() - 10);
    r = fasterRealToTile(panelRect90.right() + 10);
    b = fasterRealToTile(panelRect90.bottom() + 10);
    SETLEFT(bufferTile90, l);
    SETYMIN(bufferTile90, t);		

	planePair->thePlane90 = TiNewPlane(bufferTile90, l, t, r, b);

    SETRIGHT(bufferTile90, r);
	SETYMAX(bufferTile90, b);		

	qrectToTile(panelRect90, planePair->tilePanelRect90);
	TiInsertTile(planePair->thePlane90, &planePair->tilePanelRect90, NULL, Tile::SPACE); 

	return planePair;
}

void Panelizer::collectFiles(const QDir & outputFolder, QDomElement & path, QHash<QString, QString> & fzzFilePaths)
{
    QList<QDir> dirList;
    dirList << outputFolder;

	while (!path.isNull()) {
		QDomNode node = path.firstChild();
		if (!node.isText()) {
			DebugDialog::debug(QString("missing text in <path> element"));
			return;
		}

        QString p = node.nodeValue().trimmed();
        DebugDialog::debug("p folder " + p);
        QString savep = p;
        if (savep.startsWith(".")) {
            p = outputFolder.absolutePath() + "/" + savep;
        }

		QDir dir(p);
		if (!dir.exists()) {
			DebugDialog::debug(QString("Directory '%1' doesn't exist").arg(p));
			return;
		}

        dirList << dir;
		path = path.nextSiblingElement("path");
	}

    foreach (QDir dir, dirList) {
        DebugDialog::debug("directory " + dir.absolutePath());
	    QStringList filepaths;
        QStringList filters("*" + FritzingBundleExtension);
        FolderUtils::collectFiles(dir, filters, filepaths, false);
	    foreach (QString filepath, filepaths) {
		    QFileInfo fileInfo(filepath);

		    fzzFilePaths.insert(fileInfo.fileName(), filepath);
	    }
    }
}

bool Panelizer::checkBoards(QDomElement & board, QHash<QString, QString> & fzzFilePaths)
{
	while (!board.isNull()) {
		QString boardname = board.attribute("name");
		//DebugDialog::debug(QString("board %1").arg(boardname));
		bool ok;
		int optional = board.attribute("maxOptionalCount", "").toInt(&ok);
		if (!ok) {
			writePanelizerOutput(QString("maxOptionalCount for board '%1' not an integer: '%2'").arg(boardname).arg(board.attribute("maxOptionalCount")));
			return false;
		}

		int optionalPriority = board.attribute("optionalPriority", "").toInt(&ok);
        Q_UNUSED(optionalPriority);
		if (!ok) {
			writePanelizerOutput(QString("optionalPriority for board '%1' not an integer: '%2'").arg(boardname).arg(board.attribute("optionalPriority")));
			return false;
		}

		int required = board.attribute("requiredCount", "").toInt(&ok);
		if (!ok) {
			writePanelizerOutput(QString("required for board '%1' not an integer: '%2'").arg(boardname).arg(board.attribute("maxOptionalCount")));
			return false;
		}

        if (optional > 0 || required> 0) {
		    QString path = fzzFilePaths.value(boardname, "");
		    if (path.isEmpty()) {
			    writePanelizerOutput(QString("File for board '%1' not found in search paths").arg(boardname));
			    return false;
		    }
        }
        else {
            writePanelizerOutput(QString("skipping board '%1'").arg(boardname));
        }

		board = board.nextSiblingElement("board");
	}

	return true;
}

bool Panelizer::openWindows(QDomElement & boardElement, QHash<QString, QString> & fzzFilePaths, 
                    FApplication * app, PanelParams & panelParams, 
                    QDir & fzDir, QDir & svgDir, QList<PanelItem *> & refPanelItems, 
                    QList<LayerThing> & layerThingList, bool customPartsOnly, QDir & copyDir)
{
    QDir rotateDir(svgDir);
    QDir norotateDir(svgDir);
    rotateDir.mkdir("rotate");
    rotateDir.cd("rotate");
    norotateDir.mkdir("norotate");
    norotateDir.cd("norotate");

    PanelType * bigPanelType = NULL;
    foreach (PanelType * panelType, panelParams.panelTypes) {
        if (panelType->name == "big") {
            bigPanelType = panelType;
            break;
        }
    }

    if (bigPanelType == NULL) {
        writePanelizerOutput("No panel types defined");
        return false;
    }

	while (!boardElement.isNull()) {
		int required = boardElement.attribute("requiredCount", "").toInt();
		int optional = boardElement.attribute("maxOptionalCount", "").toInt();
		int priority = boardElement.attribute("optionalPriority", "").toInt();
        if (customPartsOnly) {
            optional = 0;
            if (required > 1) required = 1;
        }
		if (required == 0 && optional == 0) {
			boardElement = boardElement.nextSiblingElement("board"); 
			continue;
		}

		QString boardName = boardElement.attribute("name");
		QString originalPath = fzzFilePaths.value(boardName, "");
        QFileInfo originalInfo(originalPath);
        QString copyPath = copyDir.absoluteFilePath(boardName);
        QFileInfo copyInfo(copyPath);

        if (!copyInfo.exists()) {
            writePanelizerOutput(QString("failed to load copy'%1'").arg(copyPath));
            return false;
        }

        if (!originalInfo.exists()) {
            writePanelizerOutput(QString("failed to find original'%1'").arg(originalPath));
            return false;
        }

        if (originalInfo.lastModified() > copyInfo.lastModified()) {
            writePanelizerOutput(QString("copy %1 is not up to date--rerun the inscriber").arg(copyPath));
            return false;
        }

		MainWindow * mainWindow = app->openWindowForService(false, 3);
        mainWindow->setCloseSilently(true);

		FolderUtils::setOpenSaveFolderAux(fzDir.absolutePath());

		if (!mainWindow->loadWhich(copyPath, false, false, false, "")) {
			writePanelizerOutput(QString("failed to load '%1'").arg(copyPath));
			return false;
		}

        if (customPartsOnly && 
            !mainWindow->hasAnyAlien() && 
            !mainWindow->hasCustomBoardShape() && 
            (mainWindow->pcbView()->checkLoadedTraces() == 0) &&
            (checkDonuts(mainWindow, false) == 0) &&
            (checkText(mainWindow, false) == 0)
           ) 
        {
            mainWindow->close();
            delete mainWindow,
			boardElement = boardElement.nextSiblingElement("board"); 
			continue;
        }
        
		foreach (QGraphicsItem * item, mainWindow->pcbView()->scene()->items()) {
			ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
			if (itemBase == NULL) continue;

			itemBase->setMoveLock(false);
		}
		
		QList<ItemBase *> boards = mainWindow->pcbView()->findBoard();
        foreach (ItemBase * boardItem, boards) {
		    PanelItem * panelItem = new PanelItem;
		    panelItem->boardName = boardName;
		    panelItem->path = originalPath;
		    panelItem->required = required;
		    panelItem->maxOptional = optional;
            panelItem->optionalPriority = priority;
            panelItem->boardID = boardItem->id();

		    QRectF sbr = boardItem->layerKinChief()->sceneBoundingRect();
		    panelItem->boardSizeInches = sbr.size() / GraphicsUtils::SVGDPI;
		    DebugDialog::debug(QString("board size inches c %1, %2, %3")
				    .arg(panelItem->boardSizeInches.width())
				    .arg(panelItem->boardSizeInches.height())
				    .arg(copyPath));

		    /*
		    QSizeF boardSize = boardItem->size();
		    ResizableBoard * resizableBoard = qobject_cast<ResizableBoard *>(boardItem->layerKinChief());
		    if (resizableBoard != NULL) {
			    panelItem->boardSizeInches = resizableBoard->getSizeMM() / 25.4;
			    DebugDialog::debug(QString("board size inches a %1, %2, %3")
				    .arg(panelItem->boardSizeInches.width())
				    .arg(panelItem->boardSizeInches.height())
				    .arg(path), boardItem->sceneBoundingRect());
		    }
		    else {
			    panelItem->boardSizeInches = boardSize / GraphicsUtils::SVGDPI;
			    DebugDialog::debug(QString("board size inches b %1, %2, %3")
				    .arg(panelItem->boardSizeInches.width())
				    .arg(panelItem->boardSizeInches.height())
				    .arg(path), boardItem->sceneBoundingRect());
		    }
		    */

		    bool tooBig = false;
		    if (panelItem->boardSizeInches.width() >= bigPanelType->width) {
			    tooBig = panelItem->boardSizeInches.width() >= bigPanelType->height;
			    if (!tooBig) {
				    tooBig = panelItem->boardSizeInches.height() >= bigPanelType->width;
			    }
		    }

		    if (!tooBig) {
			    if (panelItem->boardSizeInches.height() >= bigPanelType->height) {
				    tooBig = panelItem->boardSizeInches.height() >= bigPanelType->width;
				    if (!tooBig) {
					    tooBig = panelItem->boardSizeInches.width() >= bigPanelType->height;
				    }
			    }
		    }

		    if (tooBig) {
			    writePanelizerOutput(QString("board is too big for panel '%1'").arg(originalPath));
			    return false;
		    }

            makeSVGs(mainWindow, boardItem, boardName, layerThingList, norotateDir, copyInfo);

		    refPanelItems << panelItem;
        }

        // now save the rotated version
		mainWindow->pcbView()->selectAllItems(true, false);
	    QMatrix matrix;
		mainWindow->pcbView()->rotateX(90, false, NULL);

        foreach (ItemBase * boardItem, boards) {
            makeSVGs(mainWindow, boardItem, boardName, layerThingList, rotateDir, copyInfo);
        }

        mainWindow->close();
        delete mainWindow,

		boardElement = boardElement.nextSiblingElement("board");
	}

	return true;
}

bool Panelizer::initPanelParams(QDomElement & root, PanelParams & panelParams)
{
	panelParams.prefix = root.attribute("prefix");
	if (panelParams.prefix.isEmpty()) {
		writePanelizerOutput(QString("Output file prefix not specified"));
		return false;
	}

    QDomElement panels = root.firstChildElement("panels");
    QDomElement panel = panels.firstChildElement("panel");
    while (!panel.isNull()) {
        PanelType * panelType = new PanelType;

        panelType->name = panel.attribute("name");

        bool ok;
	    panelType->width = TextUtils::convertToInches(panel.attribute("width"), &ok, false);
	    if (!ok) {
		    writePanelizerOutput(QString("Can't parse panel width '%1'").arg(panel.attribute("width")));
		    return false;
	    }

	    panelType->height = TextUtils::convertToInches(panel.attribute("height"), &ok, false);
	    if (!ok) {
		    writePanelizerOutput(QString("Can't parse panel height '%1'").arg(panel.attribute("height")));
		    return false;
	    }

	    panelType->c1 = panel.attribute("c1").toDouble(&ok);
	    if (!ok) {
		    writePanelizerOutput(QString("Can't parse panel c1 '%1'").arg(panel.attribute("c1")));
		    return false;
	    }

	    panelType->c2 = panel.attribute("c2").toDouble(&ok);
	    if (!ok) {
		    writePanelizerOutput(QString("Can't parse panel c2 '%1'").arg(panel.attribute("c2")));
		    return false;
	    }

        panelParams.panelTypes << panelType;

        panel = panel.nextSiblingElement("panel");
    }

    if (panelParams.panelTypes.count() == 0) {
		writePanelizerOutput(QString("No panel types defined."));
		return false;
    }

    bool ok;
	panelParams.panelSpacing = TextUtils::convertToInches(root.attribute("spacing"), &ok, false);
	if (!ok) {
		writePanelizerOutput(QString("Can't parse panel spacing '%1'").arg(root.attribute("spacing")));
		return false;
	}

	panelParams.panelBorder = TextUtils::convertToInches(root.attribute("border"), &ok, false);
	if (!ok) {
		writePanelizerOutput(QString("Can't parse panel border '%1'").arg(root.attribute("border")));
		return false;
	}

	return true;

}

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

void Panelizer::addOptional(int optionalCount, QList<PanelItem *> & refPanelItems, QList<PanelItem *> & insertPanelItems, PanelParams & panelParams, QList<PlanePair *> & planePairs)
{
    if (optionalCount == 0) return;

    QList<PanelItem *> copies(refPanelItems);
    qSort(copies.begin(), copies.end(), byOptionalPriority);
	while (optionalCount > 0) {
        int pool = 0;
        int priority = -1;
        foreach(PanelItem * panelItem, copies) {
            if (panelItem->maxOptional > 0) {
                if (priority == -1) priority = panelItem->optionalPriority;
                if (panelItem->optionalPriority == priority) {
                    pool += panelItem->maxOptional;
                }
                else break;
            }
        }
        if (pool == 0) break;

		int ix = qFloor(qrand() * pool / (double) RAND_MAX);
		int soFar = 0;
		foreach (PanelItem * panelItem, copies) {
			if (panelItem->maxOptional == 0) continue;
			if (panelItem->optionalPriority != priority) continue;

			if (ix >= soFar && ix < soFar + panelItem->maxOptional) {
				PanelItem * copy = new PanelItem(panelItem);
				if (bestFitOne(copy, panelParams, planePairs, false, false)) {
					// got one
					panelItem->maxOptional--;
					optionalCount--;
					insertPanelItems.append(copy);
				}
				else {
					// don't bother trying this one again
					optionalCount -= panelItem->maxOptional;
					panelItem->maxOptional = 0;
				}
				break;
			}

			soFar += panelItem->maxOptional;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////

void Panelizer::inscribe(FApplication * app, const QString & panelFilename, bool drc, bool noMessages) 
{
    initPanelizerOutput(panelFilename, "inscribe");

	QFile file(panelFilename);

    QFileInfo info(panelFilename);
    QDir copyDir = info.absoluteDir();
    copyDir.mkdir("copies");
    copyDir.cd("copies");
    if (!copyDir.exists()) {
		DebugDialog::debug(QString("unable to create 'copies' folder in '%1'").arg(info.absoluteDir().absolutePath()));
		return;
	}

	QString errorStr;
	int errorLine;
	int errorColumn;

	DebugDialog::setEnabled(true);

	QDomDocument domDocument;
	if (!domDocument.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
        writePanelizerOutput(QString("Unable to parse '%1': '%2' line:%3 column:%4").arg(panelFilename).arg(errorStr).arg(errorLine).arg(errorColumn));
		return;
	}

	QDomElement root = domDocument.documentElement();
	if (root.isNull() || root.tagName() != "panelizer") {
		writePanelizerOutput(QString("root element is not 'panelizer'"));
		return;
	}

	PanelParams panelParams;
	if (!initPanelParams(root, panelParams)) return;

	QDir outputDir = QDir::temp();
	QDir fzDir(outputDir);
	fzDir.cd("fz");
	if (!fzDir.exists()) {
		writePanelizerOutput(QString("unable to create fz folder in '%1'").arg(outputDir.absolutePath()));
		return;
	}

	DebugDialog::debug(QString("fz folder '%1'\n").arg(fzDir.absolutePath()));

	QDomElement boards = root.firstChildElement("boards");
	QDomElement board = boards.firstChildElement("board");
	if (board.isNull()) {
		writePanelizerOutput(QString("no <board> elements found"));
		return;
	}

	QHash<QString, QString> fzzFilePaths;
	QDomElement paths = root.firstChildElement("paths");
	QDomElement path = paths.firstChildElement("path");
	if (path.isNull()) {
		writePanelizerOutput(QString("no <path> elements found"));
		return;
	}

    QFileInfo pinfo(panelFilename);
	collectFiles(pinfo.absoluteDir(), path, fzzFilePaths);
	if (fzzFilePaths.count() == 0) {
		writePanelizerOutput(QString("no fzz files found in paths"));
		return;
	}
	
	board = boards.firstChildElement("board");
	if (!checkBoards(board, fzzFilePaths)) return;

    app->createUserDataStoreFolderStructures();
	app->registerFonts();
	app->loadReferenceModel("", false);

	board = boards.firstChildElement("board");
	while (!board.isNull()) {
		MainWindow * mainWindow = inscribeBoard(board, fzzFilePaths, app, fzDir, drc, noMessages, copyDir);
		if (mainWindow) {
			mainWindow->setCloseSilently(true);
			mainWindow->close();
            delete mainWindow;
		}
		board = board.nextSiblingElement("board");
	}

    writePanelizerFilenames(panelFilename);

	// TODO: delete temp fz folder

}

MainWindow * Panelizer::inscribeBoard(QDomElement & board, QHash<QString, QString> & fzzFilePaths, FApplication * app, QDir & fzDir, bool drc, bool noMessages, QDir & copyDir)
{
	QString boardName = board.attribute("name");
	int optional = board.attribute("maxOptionalCount", "").toInt();
	int required = board.attribute("requiredCount", "").toInt();
    if (optional <= 0 && required <= 0) return NULL;

	QString originalPath = fzzFilePaths.value(boardName, "");
    QFileInfo originalInfo(originalPath);
    QString copyPath = copyDir.absoluteFilePath(originalInfo.fileName());
    QFileInfo copyInfo(copyPath);


    if (!copyInfo.exists()) {
    }
    else {
        DebugDialog::debug("");
        //DebugDialog::debug(oldPath);
        //DebugDialog::debug(copyPath);
        DebugDialog::debug(QString("%1 original=%2, copy=%3").arg(originalInfo.fileName()).arg(originalInfo.lastModified().toString()).arg(copyInfo.lastModified().toString()));
        if (originalInfo.lastModified() <= copyInfo.lastModified()) {
            DebugDialog::debug(QString("copy %1 is up to date").arg(copyPath));
            return NULL;
        }
    }

    QFile file(originalPath);
    bool ok = FolderUtils::slamCopy(file, copyPath);
    if (!ok) {
        QMessageBox::warning(NULL, QObject::tr("Fritzing"), QObject::tr("unable to copy file '%1' to '%2'.").arg(originalPath).arg(copyPath));
        return NULL;
    }

	MainWindow * mainWindow = app->openWindowForService(false, 3);

	FolderUtils::setOpenSaveFolderAux(fzDir.absolutePath());

	if (!mainWindow->loadWhich(copyPath, false, false, false, "")) {
		writePanelizerOutput(QString("failed to load '%1'").arg(copyPath));
		return mainWindow;
	}

    // performance optimization
    mainWindow->pcbView()->setGridSize("0.1in");
    mainWindow->pcbView()->showGrid(false);

    QList<ItemBase *> items = mainWindow->pcbView()->selectAllObsolete();
	if (items.count() > 0) {
        QFileInfo info(copyPath);
        writePanelizerOutput(QString("%2 ... %1 obsolete items").arg(items.count()).arg(info.fileName()));
    }

    int moved = mainWindow->pcbView()->checkLoadedTraces();
    if (moved > 0) {
        QFileInfo info(originalPath);
        QString message = QObject::tr("%2 ... %1 wires moved from their saved position").arg(moved).arg(info.fileName());
        if (!noMessages) {
            QMessageBox::warning(NULL, QObject::tr("Fritzing"), message);
        }
        writePanelizerOutput(message); 
        collectFilenames(info.fileName());
    }

	foreach (QGraphicsItem * item, mainWindow->pcbView()->scene()->items()) {
		ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase == NULL) continue;

		itemBase->setMoveLock(false);
	}

	QString fritzingVersion = mainWindow->fritzingVersion();
	VersionThing versionThing;
	versionThing.majorVersion = 0;
	versionThing.minorVersion = 7;
	versionThing.minorSubVersion = 0;
	versionThing.releaseModifier = "";

	VersionThing versionThingFz;
	Version::toVersionThing(fritzingVersion, versionThingFz);
	bool oldGround = !Version::greaterThan(versionThing, versionThingFz);
		
    bool filled = false;
    QList<ItemBase *> boards = mainWindow->pcbView()->findBoard();	
    bool wasOne = false;
	foreach (ItemBase * boardItem, boards) {
        mainWindow->pcbView()->selectAllItems(false, false);
        boardItem->setSelected(true);
	    if (boardItem->prop("layers").compare("1") == 0) {
            mainWindow->swapLayers(boardItem, 2, "", 0);
            ProcessEventBlocker::processEvents();
            wasOne = true;
        }

        LayerList groundLayers;
        groundLayers << ViewLayer::GroundPlane0 << ViewLayer::GroundPlane1;
        foreach (ViewLayer::ViewLayerID viewLayerID, groundLayers) {
            QString fillType = mainWindow->pcbView()->characterizeGroundFill(viewLayerID);
	        if (fillType == GroundPlane::fillTypeNone) {
		        mainWindow->copperFill(viewLayerID);
                ProcessEventBlocker::processEvents();
		        filled = true;
	        }
	        else if ((fillType == GroundPlane::fillTypeGround) && oldGround) {
		        mainWindow->groundFill(viewLayerID);
                ProcessEventBlocker::processEvents();
		        filled = true;
	        }
        }
    }

    if (filled) {
        mainWindow->saveAsAux(copyPath);
		DebugDialog::debug(QString("%1 filled:%2").arg(copyPath).arg(filled));
	}

    checkDonuts(mainWindow, !noMessages);
    checkText(mainWindow, !noMessages);

    if (drc) {
	    foreach (ItemBase * boardItem, boards) {
            mainWindow->pcbView()->resetKeepout();
            mainWindow->pcbView()->selectAllItems(false, false);
            boardItem->setSelected(true);
            QStringList messages = mainWindow->newDesignRulesCheck(false);
            if (messages.count() > 0) {
                QFileInfo info(mainWindow->fileName());
                writePanelizerOutput(QString("%2 ... %1 drc complaints").arg(messages.count()).arg(info.fileName()));
                collectFilenames(info.fileName());
                //foreach (QString message, messages) {
                //    writePanelizerOutput("\t" + message);
                //}
            }
        }
    }

	return mainWindow;
}

void Panelizer::makeSVGs(MainWindow * mainWindow, ItemBase * board, const QString & boardName, QList<LayerThing> & layerThingList, QDir & saveDir, QFileInfo & copyInfo) {
	try {
		
		QString maskTop;
		QString maskBottom;
        QStringList texts;
        QMultiHash<long, ConnectorItem *> treatAsCircle;

        bool needsRedo = false;
        int missing = 0;
		foreach (LayerThing layerThing, layerThingList) {	
			QString name = layerThing.name;
            QString filename = saveDir.absoluteFilePath(QString("%1_%2_%3.svg").arg(boardName).arg(board->id()).arg(name));
            QFileInfo info(filename);
            if (info.exists()) {
                if (info.lastModified() <= copyInfo.lastModified()) {
                    // need to save these files again
                    needsRedo = true;
                    break;
                }
            }
            else {
                missing++;
            }
        }

        if (!needsRedo) {
            if (missing < layerThingList.count()) {
                // assume some number of missing files wouldn't have been written out anyway
                // but if all are missing, then they were never written in the first place
                return;
            }
        }

		foreach (LayerThing layerThing, layerThingList) {	
			QString name = layerThing.name;
            QString filename = saveDir.absoluteFilePath(QString("%1_%2_%3.svg").arg(boardName).arg(board->id()).arg(name));

			SVG2gerber::ForWhy forWhy = layerThing.forWhy;
			QList<ItemBase *> copperLogoItems, holes;
            switch (forWhy) {
                case SVG2gerber::ForPasteMask:
                    mainWindow->pcbView()->hideHoles(holes);
                case SVG2gerber::ForMask:
				    mainWindow->pcbView()->hideCopperLogoItems(copperLogoItems);
                default:
                    break;
			}

	        RenderThing renderThing;
            renderThing.printerScale = GraphicsUtils::SVGDPI;
            renderThing.blackOnly = true;
            renderThing.dpi = GraphicsUtils::StandardFritzingDPI;
            renderThing.hideTerminalPoints = true;
            renderThing.selectedItems = renderThing.renderBlocker = false;
			QString one = mainWindow->pcbView()->renderToSVG(renderThing, board, layerThing.layerList);
					
			QString clipString;
		    bool wantText = false;	
			switch (forWhy) {
				case SVG2gerber::ForOutline:
					one = GerberGenerator::cleanOutline(one);
					break;
				case SVG2gerber::ForPasteMask:
					mainWindow->pcbView()->restoreCopperLogoItems(copperLogoItems);
					mainWindow->pcbView()->restoreCopperLogoItems(holes);
                    one = mainWindow->pcbView()->makePasteMask(one, board, GraphicsUtils::StandardFritzingDPI, layerThing.layerList);
                    if (one.isEmpty()) continue;

					forWhy = SVG2gerber::ForCopper;
                    break;
				case SVG2gerber::ForMask:
					mainWindow->pcbView()->restoreCopperLogoItems(copperLogoItems);
					one = TextUtils::expandAndFill(one, "black", GerberGenerator::MaskClearanceMils * 2);
					forWhy = SVG2gerber::ForCopper;
					if (name.contains("bottom")) {
						maskBottom = one;
					}
					else {
						maskTop = one;
					}
					break;
				case SVG2gerber::ForSilk:
                    wantText = true;
					if (name.contains("bottom")) {
						clipString = maskBottom;
					}
					else {
						clipString = maskTop;
					}
					break;
                case SVG2gerber::ForCopper:
                case SVG2gerber::ForDrill:
                    treatAsCircle.clear();
                    foreach (QGraphicsItem * item, mainWindow->pcbView()->scene()->collidingItems(board)) {
                        ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(item);
                        if (connectorItem == NULL) continue;
                        if (!connectorItem->isPath()) continue;
                        if (connectorItem->radius() == 0) continue;

                        treatAsCircle.insert(connectorItem->attachedToID(), connectorItem);
                    }
                    wantText = true;
                    break;
				default:
                    wantText = true;
					break;
			}

            if (wantText) {
                collectTexts(one, texts);
                //DebugDialog::debug("one " + one);
            }
					
            QString two = GerberGenerator::clipToBoard(one, board, name, forWhy, clipString, true, treatAsCircle);
            treatAsCircle.clear();
            if (two.isEmpty()) continue;

            TextUtils::writeUtf8(filename, two);
		}

        if (texts.count() > 0) {
            QString filename = saveDir.absoluteFilePath(QString("%1_%2_%3.txt").arg(boardName).arg(board->id()).arg("texts"));
            TextUtils::writeUtf8(filename, texts.join("\n"));
        }
	}
	catch (const char * msg) {
		DebugDialog::debug(QString("panelizer error 1 %1 %2").arg(boardName).arg(msg));
	}
	catch (const QString & msg) {
		DebugDialog::debug(QString("panelizer error 2 %1 %2").arg(boardName).arg(msg));
	}
	catch (...) {
		DebugDialog::debug(QString("panelizer error 3 %1").arg(boardName));
	}
}

void Panelizer::doOnePanelItem(PlanePair * planePair, QList<LayerThing> & layerThingList, PanelItem * panelItem, QDir & svgDir) {
	try {
		
		for (int i = 0; i < planePair->svgs.count(); i++) {					
			QString name = layerThingList.at(i).name;

            QString rot = panelItem->rotate90 ? "rotate" : "norotate";
            QString filename = svgDir.absoluteFilePath(QString("%1/%2_%3_%4.svg").arg(rot).arg(panelItem->boardName).arg(panelItem->boardID).arg(name));
            QFile file(filename);
            if (file.open(QFile::ReadOnly)) {		
			    QString one = file.readAll();
			    if (one.isEmpty()) continue;

			    int left = one.indexOf("<svg");
			    left = one.indexOf(">", left + 1);
			    int right = one.lastIndexOf("<");
			    one = QString("<g transform='translate(%1,%2)'>\n").arg(panelItem->x * GraphicsUtils::StandardFritzingDPI).arg(panelItem->y * GraphicsUtils::StandardFritzingDPI) + 
							    one.mid(left + 1, right - left - 1) + 
							    "</g>\n";

			    planePair->svgs.replace(i, planePair->svgs.at(i) + one);
            }
            else {
                DebugDialog::debug(QString("panelizer error? 1 %1 %2").arg(filename).arg("one text not found"));
            }
		}
	}
	catch (const char * msg) {
		DebugDialog::debug(QString("panelizer error 1 %1 %2").arg(panelItem->boardName).arg(msg));
	}
	catch (const QString & msg) {
		DebugDialog::debug(QString("panelizer error 2 %1 %2").arg(panelItem->boardName).arg(msg));
	}
	catch (...) {
		DebugDialog::debug(QString("panelizer error 3 %1").arg(panelItem->boardName));
	}
}

void Panelizer::shrinkLastPanel( QList<PlanePair *> & planePairs, QList<PanelItem *> & insertPanelItems, PanelParams & panelParams, bool customPartsOnly) 
{
    PlanePair * lastPlanePair = planePairs.last();
    PlanePair * smallPlanePair = makePlanePair(panelParams, false);
    smallPlanePair->index = lastPlanePair->index;
    QList<PlanePair *> smallPlanePairs;
    smallPlanePairs << smallPlanePair;
    bool canFitSmaller = true;
    QList<PanelItem *> smallPanelItems;
    foreach (PanelItem * panelItem, insertPanelItems) {
        if (panelItem->planePair != lastPlanePair) continue;

        PanelItem * copy = new PanelItem(panelItem);
        smallPanelItems << copy;

		if (!bestFitOne(copy, panelParams, smallPlanePairs, false, customPartsOnly)) {
            canFitSmaller = false;
        }
	}

    if (canFitSmaller) {
        foreach (PanelItem * panelItem, insertPanelItems) {
            if (panelItem->planePair != lastPlanePair) continue;

            insertPanelItems.removeOne(panelItem);
            delete panelItem;
        }
        planePairs.removeOne(lastPlanePair);
        delete lastPlanePair;
        insertPanelItems.append(smallPanelItems);
        planePairs.append(smallPlanePair);      
    }
    else {
        foreach (PanelItem * copy, smallPanelItems) {
            delete copy;
        }
        delete smallPlanePair;
    }

}


int Panelizer::checkText(MainWindow * mainWindow, bool displayMessage) {
	QHash<QString, QString> svgHash;
    QList<ItemBase *> missing;

    foreach (QGraphicsItem * item, mainWindow->pcbView()->scene()->items()) {
        ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
        if (itemBase == NULL) continue;
        if (!itemBase->isEverVisible()) continue;

        double factor;
        QString itemSvg = itemBase->retrieveSvg(itemBase->viewLayerID(), svgHash, false, GraphicsUtils::StandardFritzingDPI, factor);
		if (itemSvg.isEmpty()) continue;

        QDomDocument doc;
        QString errorStr;
	    int errorLine;
	    int errorColumn;
	    if (!doc.setContent(itemSvg, &errorStr, &errorLine, &errorColumn)) {
            DebugDialog::debug(QString("itembase svg failure %1").arg(itemBase->id()));
            continue;
        }

        QDomElement root = doc.documentElement();
        QDomNodeList domNodeList = root.elementsByTagName("path");
	    for (int i = 0; i < domNodeList.count(); i++) {
            QDomElement textElement = domNodeList.at(i).toElement();
            if (textElement.attribute("fill").isEmpty() && textElement.attribute("stroke").isEmpty() && textElement.attribute("stroke-width").isEmpty()) {
                missing.append(itemBase);
                break;
            }
        }
    }

    if (displayMessage && missing.count() > 0) {
        mainWindow->pcbView()->selectAllItems(false, false);
        mainWindow->pcbView()->selectItems(missing);
        QMessageBox::warning(NULL, "Text", QString("There are %1 possible instances of parts with <path> elements missing stroke/fill/stroke-width attributes").arg(missing.count()));
    }

    if (missing.count() > 0) {
        QFileInfo info(mainWindow->fileName());
        writePanelizerOutput(QString("%2 ... There are %1 possible instances of parts with <path> elements missing stroke/fill/stroke-width attributes")
                .arg(missing.count()).arg(info.fileName())
            );
        collectFilenames(info.fileName());
    }

    return  missing.count();
}

int Panelizer::checkDonuts(MainWindow * mainWindow, bool displayMessage) {
    QList<ConnectorItem *> donuts;
    foreach (QGraphicsItem * item, mainWindow->pcbView()->scene()->items()) {
        ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(item);
        if (connectorItem == NULL) continue;
        if (!connectorItem->attachedTo()->isEverVisible()) continue;

        if (connectorItem->isPath() && connectorItem->getCrossLayerConnectorItem() != NULL) {  // && connectorItem->radius() == 0
            connectorItem->debugInfo("possible donut");
            connectorItem->attachedTo()->debugInfo("\t");
            donuts << connectorItem;
        }
    }

    if (displayMessage && donuts.count() > 0) {
        mainWindow->pcbView()->selectAllItems(false, false);
        QSet<ItemBase *> itemBases;
        foreach (ConnectorItem * connectorItem, donuts) {
            itemBases.insert(connectorItem->attachedTo());
        }
        mainWindow->pcbView()->selectItems(itemBases.toList());
        QMessageBox::warning(NULL, "Donuts", QString("There are %1 possible donut connectors").arg(donuts.count() / 2));
    }

    if (donuts.count() > 0) {
        QFileInfo info(mainWindow->fileName());
        writePanelizerOutput(QString("%2 ... %1 possible donuts").arg(donuts.count() / 2).arg(info.fileName()));
        collectFilenames(mainWindow->fileName());
    }

    return donuts.count() / 2;
}


int Panelizer::bestFitLoop(QList<PanelItem *> & refPanelItems, PanelParams & panelParams, bool customPartsOnly, QList<PlanePair *> & returnPlanePairs, QList<PanelItem *> & returnInsertPanelItems, const QDir & svgDir) 
{
	int optionalCount = 0;
	foreach (PanelItem * panelItem, refPanelItems) {
		optionalCount += panelItem->maxOptional;
	}

    QDir intermediates(svgDir);
    intermediates.mkdir("intermediates");
    intermediates.cd("intermediates");

    double bestCost = 0;
    double bestDivisor = 1;
    bool firstTime = true;
    for (int divisor = 1; true; divisor++) {
        QList<PlanePair *> planePairs;
        QList<PanelItem *> insertPanelItems;

        PlanePairIndex = 0;         // reset to zero for each new list of PlanePairs

        bool stillMoreThanOne = false;
	    foreach (PanelItem * panelItem, refPanelItems) {
            int count = (panelItem->required + divisor - 1) / divisor;
            if (count > 1) stillMoreThanOne = true;
		    for (int i = 0; i < count; i++) {
			    PanelItem * copy = new PanelItem(panelItem);
			    insertPanelItems.append(copy);
		    }
	    }

	    planePairs << makePlanePair(panelParams, true);  
        
	    qSort(insertPanelItems.begin(), insertPanelItems.end(), areaGreaterThan);
	    bestFit(insertPanelItems, panelParams, planePairs, customPartsOnly);

        shrinkLastPanel(planePairs, insertPanelItems, panelParams, customPartsOnly);

        double cost = calcCost(panelParams, planePairs, divisor);
        foreach (PlanePair * planePair, planePairs) {
		    planePair->layoutSVG += "</svg>";
		    QString fname = intermediates.absoluteFilePath(QString("%3.divisor_%4.cost_%1.panel_%2.layout_.svg")
                .arg(panelParams.prefix).arg(planePair->index).arg(divisor).arg(cost)
            );
            TextUtils::writeUtf8(fname, planePair->layoutSVG);

            // make sure to leave layoutSVG without a trailing </svg> since optionals are added later
		    planePair->layoutSVG.chop(6);
	    }


        DebugDialog::debug("");
        DebugDialog::debug(QString("%1 panels, %2 additional copies of each: cost %3").arg(planePairs.count()).arg(divisor - 1).arg(cost));
        DebugDialog::debug("");
        if (firstTime) {
            bestCost = cost;
            bestDivisor = 1;
            returnPlanePairs = planePairs;
            returnInsertPanelItems = insertPanelItems;
            firstTime = false;
        }
        else {
            if (cost < bestCost) {
                bestCost = cost;
                bestDivisor = divisor;
                foreach (PlanePair * planePair, returnPlanePairs) {
                    delete planePair;
                }
                foreach (PanelItem * panelItem, returnInsertPanelItems) {
                    delete panelItem;
                }
                returnPlanePairs.clear();
                returnInsertPanelItems.clear();

                returnPlanePairs = planePairs;
                returnInsertPanelItems = insertPanelItems;
            }
        }

        if (customPartsOnly) break;
        if (planePairs.count() == 1) break;
        if (!stillMoreThanOne) break;
    }

	addOptional(optionalCount, refPanelItems, returnInsertPanelItems, panelParams, returnPlanePairs);
    return bestDivisor;
}

double Panelizer::calcCost(PanelParams & panelParams, QList<PlanePair *> & planePairs, int divisor) {
    double total = 0;
    foreach (PlanePair * planePair, planePairs) {
        foreach (PanelType * panelType, panelParams.panelTypes) {
            if (planePair->panelWidth == panelType->width && planePair->panelHeight == panelType->height) {
                total += panelType->c1;
                total += panelType->c2 * (divisor - 1);
                break;
            }
        }
    }

    return total;
}

void Panelizer::writePanelizerOutput(const QString & message) {
    DebugDialog::debug(message);
    if (PanelizerOutputPath.length() > 0) {
        QFile file(PanelizerOutputPath);
        if (file.open(QFile::Append)) {
		    QTextStream out(&file);
		    out.setCodec("UTF-8");
		    out << message << "\n";
		    file.close();
        }
    }
}

void Panelizer::initPanelizerOutput(const QString & panelFilename, const QString & msg) {
    PanelizerFileNames.clear();
    QFileInfo info(panelFilename);
    PanelizerOutputPath = info.absoluteDir().absoluteFilePath("panelizer_output.txt");

    QDateTime dt = QDateTime::currentDateTime();
    writePanelizerOutput(QString("\n--------- %1 --- %2 ---")
                            .arg(msg).arg(dt.toString())
                        );
}

void Panelizer::collectFilenames(const QString & filename) {
    if (filename.endsWith(".fz")) {
        PanelizerFileNames.insert(filename + "z");
    }
    else PanelizerFileNames.insert(filename);
}

void Panelizer::writePanelizerFilenames(const QString & panelFilename) {
    if (PanelizerFileNames.count() == 0) return;

    QFileInfo info(panelFilename);
    QDateTime dt = QDateTime::currentDateTime();
    QString path = info.absoluteDir().absoluteFilePath("panelizer_files_%1.txt").arg(dt.toString().replace(":", "."));
    QFile file(path);
    if (file.open(QFile::WriteOnly)) {
        QTextStream out(&file);
		out.setCodec("UTF-8");
        foreach (QString name, PanelizerFileNames) {
            out << name << "\n";
        }
        file.close();
    }
}
