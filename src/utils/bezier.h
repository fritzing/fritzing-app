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

#ifndef BEZIER_H
#define BEZIER_H

#include <QPointF>
#include <QDomElement>
#include <QXmlStreamWriter>
#include <tuple>

// cubic bezier auxiliary implementation

class Bezier
{
public:
	using SplitBezier = std::tuple<Bezier, Bezier>;
public:
	Bezier(QPointF cp1, QPointF cp2);
	Bezier();
	Bezier(const Bezier&);
	Bezier(QPointF endpoint0, QPointF endpoint1, QPointF cp0, QPointF cp1) noexcept;
	constexpr const QPointF& cp0() const { return m_cp0; }
	constexpr const QPointF& cp1() const { return m_cp1; }
	constexpr const QPointF& endpoint0() const { return m_endpoint0; }
	constexpr const QPointF& endpoint1() const { return m_endpoint1; }
	void set_cp0(QPointF);
	void set_cp1(QPointF);
	void set_endpoints(QPointF, QPointF);
	constexpr bool isEmpty() const noexcept { return m_isEmpty; }
	void clear();
	void write(QXmlStreamWriter &);
	bool operator==(const Bezier &) const;
	bool operator!=(const Bezier &) const;
	void recalc(QPointF p);
	void initToEnds(QPointF cp0, QPointF cp1);
	double xFromT(double t) const noexcept;
	double xFromTPrime(double t) const noexcept;
	double yFromT(double t) const noexcept;
	void split(double t, Bezier & left, Bezier & right) const noexcept;
	SplitBezier split(double t) const noexcept;
	void initControlIndex(QPointF fromPoint, double width);
	double computeCubicCurveLength(double z, int n) const noexcept;
	void copy(const Bezier *);
	double findSplit(QPointF p, double minDistance) const noexcept;
	void translateToZero();
	void translate(QPointF);
	Bezier join(const Bezier * other) const;
	Bezier join(const Bezier& other) const noexcept;
	constexpr bool drag0() const noexcept { return m_drag_cp0; }

protected:
	double cubicF(double t) const noexcept;


public:
	static Bezier fromElement(QDomElement & element);

protected:
	QPointF m_endpoint0;
	QPointF m_endpoint1;
	QPointF m_cp0;
	QPointF m_cp1;
	bool m_isEmpty;
	bool m_drag_cp0 = false;
};

#endif
