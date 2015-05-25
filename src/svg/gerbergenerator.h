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

#ifndef GERBERGENERATOR_H
#define GERBERGENERATOR_H

#include <QString>

#include "svg2gerber.h"
#include "../items/itembase.h"
#include "../sketch/pcbsketchwidget.h"
#include "../lib/clipper/clipper.hpp"

class GerberGenerator
{

public:
	static void exportToGerber(const QString & prefix, const QString & exportDir, class ItemBase * board, class PCBSketchWidget *, bool displayMessageBoxes);
	static QString cleanOutline(const QString & svgOutline);
    static void exportFile(const QString &svg, const QString &layerName, SVG2gerber::ForWhy forWhy, const QString &exportDir, const QString &prefix, const QString &suffix);
public:
	static const QString SilkTopSuffix;
	static const QString SilkBottomSuffix;
	static const QString CopperTopSuffix;
	static const QString CopperBottomSuffix;
	static const QString MaskTopSuffix;
	static const QString MaskBottomSuffix;
	static const QString PasteMaskTopSuffix;
	static const QString PasteMaskBottomSuffix;
	static const QString DrillSuffix;
	static const QString OutlineSuffix;
	static const QString MagicBoardOutlineID;

	static const double MaskClearanceMils;	
    static const QRegExp MultipleZs;


protected:
    static int doDrill(ItemBase *board, PCBSketchWidget *sketchWidget, const QString &filename, const QString &exportDir, const ClipperLib::Paths &outline);
	static void displayMessage(const QString & message, bool displayMessageBoxes);
    static void exportPickAndPlace(const QString & prefix, const QString & exportDir, ItemBase * board, PCBSketchWidget * sketchWidget, bool displayMessageBoxes);
    static QString renderTo(const LayerList &, ItemBase *board, PCBSketchWidget *sketchWidget);

    static void svgToExcellon(QString const &filename, QString const &exportDir,const QString &svgDrill, const ClipperLib::Paths &outline);
};

#endif // GERBERGENERATOR_H
