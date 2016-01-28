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

$Revision: 6904 $:
$Author: irascibl@gmail.com $:
$Date: 2013-02-26 16:26:03 +0100 (Di, 26. Feb 2013) $

********************************************************************/

#ifndef BEZIER_H
#define BEZIER_H

#include <QPointF>
#include <QDomElement>
#include <QXmlStreamWriter>

// cubic bezier auxiliary implementation

class Bezier
{
public:
	Bezier(QPointF cp1, QPointF cp2);
	Bezier();

	QPointF cp0() const;
	QPointF cp1() const;
	QPointF endpoint0() const;
	QPointF endpoint1() const;
	void set_cp0(QPointF);
	void set_cp1(QPointF);
	void set_endpoints(QPointF, QPointF);
	bool isEmpty() const;
	void clear();
	void write(QXmlStreamWriter &);
	bool operator==(const Bezier &) const;
	bool operator!=(const Bezier &) const;
	void recalc(QPointF p);
	void initToEnds(QPointF cp0, QPointF cp1); 
	double xFromT(double t) const;
	double xFromTPrime(double t) const;
	double yFromT(double t) const;
	void split(double t, Bezier & left, Bezier & right) const;
	void initControlIndex(QPointF fromPoint, double width);
	double computeCubicCurveLength(double z, int n) const;
	void copy(const Bezier *);
	double findSplit(QPointF p, double minDistance) const;
	void translateToZero();
	void translate(QPointF);
	Bezier join(const Bezier * other) const;
	bool drag0();

protected:
	double cubicF(double t) const;


public:
	static Bezier fromElement(QDomElement & element);

protected:
	QPointF m_endpoint0;
	QPointF m_endpoint1;
	QPointF m_cp0;
	QPointF m_cp1;
	bool m_isEmpty;
	bool m_drag_cp0;
};

#endif
