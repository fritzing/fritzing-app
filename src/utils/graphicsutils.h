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

$Revision: 6912 $:
$Author: irascibl@gmail.com $:
$Date: 2013-03-09 08:18:59 +0100 (Sa, 09. Mrz 2013) $

********************************************************************/

#ifndef GRAPHICSUTILS_H
#define GRAPHICSUTILS_H

#include <QPointF>
#include <QRectF>
#include <QTransform>
#include <QXmlStreamWriter>
#include <QDomElement>
#include <QPixmap>
#include <QPainterPath>
#include <QPen>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QPixmap>

class GraphicsUtils
{

public:
	static void distanceFromLine(double cx, double cy, double ax, double ay, double bx, double by, 
								 double & dx, double & dy, double &distanceSegment, bool & atEndpoint);
	static QPointF calcConstraint(QPointF initial, QPointF current);

	static double pixels2mils(double p, double dpi);
	static double pixels2ins(double p, double dpi);
	static double distanceSqd(QPointF p1, QPointF p2);
	static double distanceSqd(QPoint p1, QPoint p2);
	static double getNearestOrdinate(double ordinate, double units);

	static double mm2mils(double mm);
	static double pixels2mm(double p, double dpi);
	static double mm2pixels(double mm);
	static double mils2pixels(double m, double dpi);
	static void saveTransform(QXmlStreamWriter & streamWriter, const QTransform & transform);
	static bool loadTransform(const QDomElement & transformElement, QTransform & transform);
	static bool isRect(const QPolygonF & poly);
	static QRectF getRect(const QPolygonF & poly);
	static void shortenLine(QPointF & p1, QPointF & p2, double d1, double d2);
	static bool liangBarskyLineClip(double x1, double y1, double x2, double y2, 
									double wxmin, double wxmax, double wymin, double wymax, 
									double & x11, double & y11, double & x22, double & y22);
	static QString toHtmlImage(QPixmap *pixmap, const char* format = "PNG");
	static QPainterPath shapeFromPath(const QPainterPath &path, const QPen &pen, double shapeStrokeWidth, bool includeOriginalPath);
	static void qt_graphicsItem_highlightSelected(QPainter *painter, const QStyleOptionGraphicsItem *option, const QRectF & boundingRect, const QPainterPath & path);
	static QPointF calcRotation(QTransform & rotation, QPointF rCenter, QPointF p, QPointF pCenter); 
    static void drawBorder(QImage * image, int border);
    static bool isFlipped(const QMatrix & matrix, double & rotation);

public:
	static const double IllustratorDPI;
	static const double StandardFritzingDPI;
	static const double SVGDPI;
	static const double InchesPerMeter;
	static const double StandardSchematicSeparationMils;
	static const double StandardSchematicSeparation10thinMils;


};

#endif
