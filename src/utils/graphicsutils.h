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
#include <tuple>

class GraphicsUtils
{

public:
	static void distanceFromLine(double cx, double cy, double ax, double ay, double bx, double by,
	  double & dx, double & dy, double &distanceSegment, bool & atEndpoint);
	static constexpr std::tuple<double, double, double, bool> distanceFromLine(double cx, double cy, double ax, double ay, double bx, double by) noexcept {
		// http://www.codeguru.com/forum/showthread.php?t=194400

		//
		// find the distance from the point (cx,cy) to the line
		// determined by the points (ax,ay) and (bx,by)
		//

		double r_numerator = (cx-ax)*(bx-ax) + (cy-ay)*(by-ay);
		double r_denomenator = (bx-ax)*(bx-ax) + (by-ay)*(by-ay);
		double r = r_numerator / r_denomenator;
		double dx = 0;
		double dy = 0;
		double distanceSegment = 0;
		bool atEndpoint = false;

		if ( (r >= 0) && (r <= 1) )
		{
			dx = ax + r*(bx-ax);
			dy = ay + r*(by-ay);
			distanceSegment = (cx-dx)*(cx-dx) + (cy-dy)*(cy-dy);
			atEndpoint = false;
		}
		else
		{
			atEndpoint = true;
			double dist1 = (cx-ax)*(cx-ax) + (cy-ay)*(cy-ay);
			double dist2 = (cx-bx)*(cx-bx) + (cy-by)*(cy-by);
			if (dist1 < dist2)
			{
				dx = ax;
				dy = ay;
				distanceSegment = dist1;
			}
			else
			{
				dx = bx;
				dy = by;
				distanceSegment = dist2;
			}
		}

		return {dx, dy, distanceSegment, atEndpoint};
	}
	static QPointF calcConstraint(QPointF initial, QPointF current);

	static constexpr double pixels2mils(double p, double dpi) noexcept { 
		return p * 1000.0 / dpi;
	}
	static constexpr double pixels2ins(double p, double dpi) noexcept {
		return p / dpi;
	}
	static constexpr double distanceSqd(QPointF p1, QPointF p2) noexcept {
		return ((p1.x() - p2.x()) * (p1.x() - p2.x())) + ((p1.y() - p2.y()) * (p1.y() - p2.y()));
	}
	static constexpr double distanceSqd(QPoint p1, QPoint p2) noexcept {
		double dpx = p1.x() - p2.x();
		double dpy = p1.y() - p2.y();
		return (dpx * dpx) + (dpy * dpy);
	}
	static double getNearestOrdinate(double ordinate, double units);

	static constexpr double mm2mils(double mm) noexcept {
		return (mm / 25.4 * 1000);
	}
	static constexpr double pixels2mm(double p, double dpi) noexcept {
		return (p / dpi * 25.4);
	}
	static constexpr double mm2pixels(double mm) noexcept {
		return (90 * mm / 25.4);
	}
	static constexpr double mils2pixels(double m, double dpi) noexcept {
		return (dpi * m / 1000);
	}
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
	static constexpr double IllustratorDPI = 72;
	static constexpr double StandardFritzingDPI = 1000;
	static constexpr double SVGDPI = 90;
	static constexpr double InchesPerMeter = 39.370078;
	static constexpr double StandardSchematicSeparationMils = 295.275591; // 7.5mm
	static constexpr double StandardSchematicSeparation10thinMils = 100;  // 0.1 inches


};

#endif
