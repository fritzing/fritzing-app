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

$Revision: 6926 $:
$Author: irascibl@gmail.com $:
$Date: 2013-03-10 21:27:34 +0100 (So, 10. Mrz 2013) $

********************************************************************/

#include "graphicsutils.h"

#include <QList>
#include <QLineF>
#include <QBuffer>
#include <qmath.h>
#include <QtDebug>

const double GraphicsUtils::IllustratorDPI = 72;
const double GraphicsUtils::StandardFritzingDPI = 1000;
const double GraphicsUtils::SVGDPI = 90;
const double GraphicsUtils::InchesPerMeter = 39.370078;
const double GraphicsUtils::StandardSchematicSeparationMils = 295.275591;   // 7.5mm
const double GraphicsUtils::StandardSchematicSeparation10thinMils = 100;   // 0.1 inches


void GraphicsUtils::distanceFromLine(double cx, double cy, double ax, double ay, double bx, double by, 
									 double & dx, double & dy, double &distanceSegment, bool & atEndpoint)
{

	// http://www.codeguru.com/forum/showthread.php?t=194400

	//
	// find the distance from the point (cx,cy) to the line
	// determined by the points (ax,ay) and (bx,by)
	//

	double r_numerator = (cx-ax)*(bx-ax) + (cy-ay)*(by-ay);
	double r_denomenator = (bx-ax)*(bx-ax) + (by-ay)*(by-ay);
	double r = r_numerator / r_denomenator;
     
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

	return;
}

struct PD {
	QPointF p;
	double d;
};

bool pdLessThan(PD* pd1, PD* pd2) {
	return pd1->d < pd2->d;
}

QPointF GraphicsUtils::calcConstraint(QPointF initial, QPointF current) {
	QList<PD *> pds;

	PD * pd = new PD;
	pd->p.setX(current.x());
	pd->p.setY(initial.y());
	pd->d = (current.y() - initial.y()) * (current.y() - initial.y());
	pds.append(pd);

	pd = new PD;
	pd->p.setX(initial.x());
	pd->p.setY(current.y());
	pd->d = (current.x() - initial.x()) * (current.x() - initial.x());
	pds.append(pd);

	double dx, dy, d;
	bool atEndpoint;

	QLineF plus45(initial.x() - 10000, initial.y() - 10000, initial.x() + 10000, initial.y() + 10000);
	distanceFromLine(current.x(), current.y(), plus45.p1().x(), plus45.p1().y(), plus45.p2().x(), plus45.p2().y(), dx, dy, d, atEndpoint);
	pd = new PD;
	pd->p.setX(dx);
	pd->p.setY(dy);
	pd->d = d;
	pds.append(pd);
		
	QLineF minus45(initial.x() + 10000, initial.y() - 10000, initial.x() - 10000, initial.y() + 10000);
	distanceFromLine(current.x(), current.y(), minus45.p1().x(), minus45.p1().y(), minus45.p2().x(), minus45.p2().y(), dx, dy, d, atEndpoint);
	pd = new PD;
	pd->p.setX(dx);
	pd->p.setY(dy);
	pd->d = d;
	pds.append(pd);

	qSort(pds.begin(), pds.end(), pdLessThan);
	QPointF result = pds[0]->p;
	foreach (PD* pd, pds) {
		delete pd;
	}
	return result;
}

double GraphicsUtils::pixels2mils(double p, double dpi) {
	return p * 1000.0 / dpi;
}

double GraphicsUtils::pixels2ins(double p, double dpi) {
	return p / dpi;
}

double GraphicsUtils::distanceSqd(QPointF p1, QPointF p2) {
	return ((p1.x() - p2.x()) * (p1.x() - p2.x())) + ((p1.y() - p2.y()) * (p1.y() - p2.y()));
}

double GraphicsUtils::distanceSqd(QPoint p1, QPoint p2) {
    double dpx = p1.x() - p2.x();
    double dpy = p1.y() - p2.y();
	return (dpx * dpx) + (dpy * dpy);
}

double GraphicsUtils::mm2mils(double mm) {
	return (mm / 25.4 * 1000);
}

double GraphicsUtils::mm2pixels(double mm) {
	return (90 * mm / 25.4);
}

double GraphicsUtils::pixels2mm(double p, double dpi) {
	return (p / dpi * 25.4);
}

double GraphicsUtils::mils2pixels(double m, double dpi) {
	return (dpi * m / 1000);
}

void GraphicsUtils::saveTransform(QXmlStreamWriter & streamWriter, const QTransform & transform) {
	if (transform.isIdentity()) return;

	streamWriter.writeStartElement("transform");
	streamWriter.writeAttribute("m11", QString::number(transform.m11()));
	streamWriter.writeAttribute("m12", QString::number(transform.m12()));
	streamWriter.writeAttribute("m13", QString::number(transform.m13()));
	streamWriter.writeAttribute("m21", QString::number(transform.m21()));
	streamWriter.writeAttribute("m22", QString::number(transform.m22()));
	streamWriter.writeAttribute("m23", QString::number(transform.m23()));
	streamWriter.writeAttribute("m31", QString::number(transform.m31()));
	streamWriter.writeAttribute("m32", QString::number(transform.m32()));
	streamWriter.writeAttribute("m33", QString::number(transform.m33()));
	streamWriter.writeEndElement();
}

bool GraphicsUtils::loadTransform(const QDomElement & transformElement, QTransform & transform)
{
	if (transformElement.isNull()) return false;

	double m11 = transform.m11();
	double m12 = transform.m12();
	double m13 = transform.m13();
	double m21 = transform.m21();
	double m22 = transform.m22();
	double m23 = transform.m23();
	double m31 = transform.m31();
	double m32 = transform.m32();
	double m33 = transform.m33();
	bool ok;
	double temp;

	temp = transformElement.attribute("m11").toDouble(&ok);
	if (ok) m11 = temp;
	temp = transformElement.attribute("m12").toDouble(&ok);
	if (ok) m12 = temp;
	temp = transformElement.attribute("m13").toDouble(&ok);
	if (ok) m13 = temp;
	temp = transformElement.attribute("m21").toDouble(&ok);
	if (ok) m21 = temp;
	temp = transformElement.attribute("m22").toDouble(&ok);
	if (ok) m22 = temp;
	temp = transformElement.attribute("m23").toDouble(&ok);
	if (ok) m23 = temp;
	temp = transformElement.attribute("m31").toDouble(&ok);
	if (ok) m31 = temp;
	temp = transformElement.attribute("m32").toDouble(&ok);
	if (ok) m32 = temp;
	temp = transformElement.attribute("m33").toDouble(&ok);
	if (ok) m33 = temp;

	transform.setMatrix(m11, m12, m13, m21, m22, m23, m31, m32, m33);
	return true;
}

double GraphicsUtils::getNearestOrdinate(double ordinate, double units) {
	double lo = qFloor(ordinate / units) * units;
	double hi = qCeil(ordinate / units) * units;
	return (qAbs(lo - ordinate) <= qAbs(hi - ordinate)) ? lo : hi;
}

void GraphicsUtils::shortenLine(QPointF & p1, QPointF & p2, double d1, double d2) {
	double angle1 = atan2(p2.y() - p1.y(), p2.x() - p1.x());
	double angle2 = angle1 - M_PI;  
	double dx1 = d1 * cos(angle1);
	double dy1 = d1 * sin(angle1);
	double dx2 = d2 * cos(angle2);
	double dy2 = d2 * sin(angle2);
	p1.setX(p1.x() + dx1);
	p1.setY(p1.y() + dy1);
	p2.setX(p2.x() + dx2);
	p2.setY(p2.y() + dy2);
}

bool GraphicsUtils::isRect(const QPolygonF & poly) {
	if (poly.count() != 5) return false;
	if (poly.at(0) != poly.at(4)) return false;

	// either we start running across top or running along side

	if (poly.at(0).x() == poly.at(1).x() && 
		poly.at(1).y() == poly.at(2).y() &&
		poly.at(2).x() == poly.at(3).x() &&
		poly.at(3).y() == poly.at(4).y()) return true;

	if (poly.at(0).y() == poly.at(1).y() && 
		poly.at(1).x() == poly.at(2).x() &&
		poly.at(2).y() == poly.at(3).y() &&
		poly.at(3).x() == poly.at(4).x()) return true;

	return false;
}

QRectF GraphicsUtils::getRect(const QPolygonF & poly) 
{
	// assumes poly is known to be a rect
	double minX, maxX, minY, maxY;

	minX = maxX = poly.at(0).x();
	minY = maxY = poly.at(0).y();

	for (int i = 1; i < 4; i++) {
		QPointF p = poly.at(i);
		if (p.x() > maxX) maxX = p.x();
		if (p.x() < minX) minX = p.x();
		if (p.y() > maxY) maxY = p.y();
		if (p.y() < minY) minY = p.y();
	}

	return QRectF(minX, minY, maxX - minX, maxY - minY);

}

// based on code from http://code-heaven.blogspot.com/2009/05/c-program-for-liang-barsky-line.html
bool GraphicsUtils::liangBarskyLineClip(double x1, double y1, double x2, double y2, double wxmin, double wxmax, double wymin, double wymax, 
							double & x11, double & y11, double & x22, double & y22)
{
	double p1 = -(x2 - x1 ); 
	double q1 = x1 - wxmin;
	double p2 = ( x2 - x1 ); 
	double q2 = wxmax - x1;
	double p3 = - ( y2 - y1 ); 
	double q3 = y1 - wymin;
	double p4 = ( y2 - y1 ); 
	double q4 = wymax - y1;

	x11 = x1;
	y11 = y1;
	x22 = x2;
	y22 = y2;

	if( ( ( p1 == 0.0 ) && ( q1 < 0.0 ) ) ||
		( ( p2 == 0.0 ) && ( q2 < 0.0 ) ) ||
		( ( p3 == 0.0 ) && ( q3 < 0.0 ) ) ||
		( ( p4 == 0.0 ) && ( q4 < 0.0 ) ) )
	{
		return false;
	}

	double u1 = 0.0, u2 = 1.0;

	if( p1 != 0.0 )
	{
		double r1 = q1 /p1 ;
		if( p1 < 0 )
			u1 = qMax(r1, u1 );
		else
			u2 = qMin(r1, u2 );
	}
	if( p2 != 0.0 )
	{
		double r2 = q2 /p2 ;
		if( p2 < 0 )
			u1 = qMax(r2, u1 );
		else
			u2 = qMin(r2, u2 );

	}
	if( p3 != 0.0 )
	{
		double r3 = q3 /p3 ;
		if( p3 < 0 )
			u1 = qMax(r3, u1 );
		else
			u2 = qMin(r3, u2 );
	}
	if( p4 != 0.0 )
	{
		double r4 = q4 /p4 ;
		if( p4 < 0 )
			u1 = qMax(r4, u1 );
		else
			u2 = qMin(r4, u2 );
	}

	if( u1 > u2 ) {
		return false;
	}

	x11 = x1 + u1 * ( x2 - x1 ) ;
	y11 = y1 + u1 * ( y2 - y1 ) ;

	x22 = x1 + u2 * ( x2 - x1 );
	y22 = y1 + u2 * ( y2 - y1 );

	return true;
}

QString GraphicsUtils::toHtmlImage(QPixmap *pixmap, const char* format) {
	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	pixmap->save(&buffer, format); // writes pixmap into bytes in PNG format
	return QString("data:image/%1;base64,%2").arg(QString(format).toLower()).arg(QString(bytes.toBase64()));
}

QPainterPath GraphicsUtils::shapeFromPath(const QPainterPath &path, const QPen &pen, double shapeStrokeWidth, bool includeOriginalPath)
{
	// this function mostly copied from QGraphicsItem::qt_graphicsItem_shapeFromPath


    // We unfortunately need this hack as QPainterPathStroker will set a width of 1.0
    // if we pass a value of 0.0 to QPainterPathStroker::setWidth()
    static const double penWidthZero = double(0.00000001);

    if (path == QPainterPath())
        return path;
    QPainterPathStroker ps;
    ps.setCapStyle(pen.capStyle());
    //ps.setCapStyle(Qt::FlatCap);
    if (shapeStrokeWidth <= 0.0)
        ps.setWidth(penWidthZero);
    else
        ps.setWidth(shapeStrokeWidth);

    ps.setJoinStyle(pen.joinStyle());
    ps.setMiterLimit(pen.miterLimit());
    QPainterPath p = ps.createStroke(path);
	if (includeOriginalPath) {
		p.addPath(path);
	}
    return p;
}

void GraphicsUtils::qt_graphicsItem_highlightSelected(QPainter *painter, const QStyleOptionGraphicsItem *option, const QRectF & boundingRect, const QPainterPath & path)
{	
    const QRectF murect = painter->transform().mapRect(QRectF(0, 0, 1, 1));
    if (qFuzzyCompare(qMax(murect.width(), murect.height()) + 1, 1))
        return;

    const QRectF mbrect = painter->transform().mapRect(boundingRect);
    if (qMin(mbrect.width(), mbrect.height()) < double(1.0))
        return;

    double itemPenWidth = 1.0;
	const double pad = itemPenWidth / 2;
    const double penWidth = 0; // cosmetic pen

    const QColor fgcolor = option->palette.windowText().color();
    const QColor bgcolor( // ensure good contrast against fgcolor
        fgcolor.red()   > 127 ? 0 : 255,
        fgcolor.green() > 127 ? 0 : 255,
        fgcolor.blue()  > 127 ? 0 : 255);

    painter->setPen(QPen(bgcolor, penWidth, Qt::SolidLine));
    painter->setBrush(Qt::NoBrush);
	if (path.isEmpty()) {
		painter->drawRect(boundingRect.adjusted(pad, pad, -pad, -pad));
	}
	else {
		painter->drawPath(path);
	}

	painter->setPen(QPen(option->palette.windowText(), 0, Qt::DashLine));
    painter->setBrush(Qt::NoBrush);
	if (path.isEmpty()) {
		painter->drawRect(boundingRect.adjusted(pad, pad, -pad, -pad));
	}
	else {
		painter->drawPath(path);
	} 
}

QPointF GraphicsUtils::calcRotation(QTransform & rotation, QPointF rCenter, QPointF p, QPointF pCenter) 
{
	QPointF dp = rCenter - p;
	QTransform tp = QTransform().translate(-dp.x(), -dp.y()) * rotation * QTransform().translate(dp.x(), dp.y());
	QTransform tc = QTransform().translate(-pCenter.x(), -pCenter.y()) * rotation * QTransform().translate(pCenter.x(), pCenter.y());
	QPointF mp = tp.map(QPointF(0,0));
	QPointF mc = tc.map(QPointF(0,0));
	return (p + mp - mc);
}

void GraphicsUtils::drawBorder(QImage * image, int border) {
    int halfBorder = border / 2;
    QPainter painter;
	painter.begin(image);
	painter.setRenderHint(QPainter::Antialiasing, false);
    QPen pen = painter.pen();
    pen.setWidth(border);
    pen.setColor(0xff000000);
    painter.setPen(pen);
    painter.drawLine(0, halfBorder, image->width() - 1, halfBorder);
    painter.drawLine(0, image->height() - halfBorder, image->width() - 1, image->height() - halfBorder);
    painter.drawLine(halfBorder, 0, halfBorder, image->height() - 1);
    painter.drawLine(image->width() - halfBorder, 0, image->width() - halfBorder, image->height() - 1);
	painter.end();
}

bool almostEqual(qreal a, qreal b) {
    static qreal nearly = 0.001;
    return (qAbs(a - b) < nearly);
}

bool GraphicsUtils::isFlipped(const QMatrix & matrix, double & rotation) {
    static qreal halfSqrt2 = 0.7071;

    // flipped means flipped horizontally (around the vertical axis)
    // rotation values are ccw and upright is zero
    if (almostEqual(matrix.m11(), 1.0))  {
        if (almostEqual(matrix.m22(), 1.0)) {
            rotation = 0;
            return false;
        }
        rotation = 180;
        return true;
    }
    if (almostEqual(matrix.m11(), -1.0)) {
        if (almostEqual(matrix.m22(), -1.0)) {
            rotation = 180;
            return false;
        }
        rotation = 0;
        return true;
    }
    if (almostEqual(matrix.m12(), 1.0)) {
        if (almostEqual(matrix.m21(), -1.0)) {
            rotation = 90;
            return false;
        }
        rotation = 270;
        return true;
    }
    if (almostEqual(matrix.m12(), -1.0)) {
        if (almostEqual(matrix.m21(), 1)) {
            rotation = 270;
            return false;
        }
        rotation = 90;
        return true;
    }
    if (almostEqual(matrix.m11(), halfSqrt2)) {
        if (almostEqual(matrix.m22(), halfSqrt2)) {
            if (almostEqual(matrix.m12(), -halfSqrt2)) {
                rotation = 315;
                return false;
            }

            rotation = 45;
            return false;
        }

        if (almostEqual(matrix.m12(), halfSqrt2)) {
            rotation = 225;
            return true;
        }

        rotation = 135;            
        return true;
    }
    if (almostEqual(matrix.m11(), -halfSqrt2)) {
        if (almostEqual(matrix.m22(), -halfSqrt2)) {
            if (almostEqual(matrix.m12(), -halfSqrt2)) {
                rotation = 225;
                return false;
            }
                
            rotation = 135;
            return false;
        }
      
        if (almostEqual(matrix.m12(), -halfSqrt2)) {
            rotation = 45;
            return true;
        }

        rotation = 315;
        return true;
    }

    qWarning() << QString("unknown matrix %1 %2 %3 %4").arg(matrix.m11()).arg(matrix.m12()).arg(matrix.m21()).arg(matrix.m22());
    rotation = 0;
    return false;
}

