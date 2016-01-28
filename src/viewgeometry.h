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

$Revision: 6112 $:
$Author: cohen@irascible.com $:
$Date: 2012-06-28 00:18:10 +0200 (Do, 28. Jun 2012) $

********************************************************************/

#ifndef VIEWGEOMETRY_H
#define VIEWGEOMETRY_H


#include <QPointF>
#include <QDomElement>
#include <QLineF>
#include <QRectF>
#include <QTransform>

class ViewGeometry
{

public:
	ViewGeometry();
	ViewGeometry(QDomElement &);
	ViewGeometry(const ViewGeometry &);
	~ViewGeometry() {}

public:
	enum WireFlag {
		NoFlag = 0,
		RoutedFlag = 2,
		PCBTraceFlag = 4,
		ObsoleteJumperFlag = 8,
		RatsnestFlag = 16,
		AutoroutableFlag = 32,
		NormalFlag = 64,
		SchematicTraceFlag = 128		
	};
	Q_DECLARE_FLAGS(WireFlags, WireFlag)

protected:
	void setWireFlag(bool setting, WireFlag flag);

public:
	void setZ(double);
	double z() const;
	void setLoc(QPointF);
	QPointF loc() const;
	void setLine(QLineF);
	QLineF line() const;
	void offset(double x, double y);
	bool selected();
	void setSelected(bool);
	QRectF rect() const;
	void setRect(double x, double y, double width, double height);
	void setRect(const QRectF &);
	void setTransform(QTransform);
	QTransform transform() const;
	void set(const ViewGeometry &);
	void setRouted(bool);
	bool getRouted() const;
	void setPCBTrace(bool);
	bool getPCBTrace() const;
	bool getAnyTrace() const;
	void setSchematicTrace(bool);
	bool getSchematicTrace() const;
	void setRatsnest(bool);
	bool getRatsnest() const;
	void setAutoroutable(bool);
	bool getAutoroutable() const;
	void setNormal(bool);
	bool getNormal() const;
	void setWireFlags(WireFlags);
	bool hasFlag(WireFlag);
	bool hasAnyFlag(WireFlags);
	int flagsAsInt() const;
	ViewGeometry::WireFlags wireFlags() const;

protected:
	double m_z;
	QPointF m_loc;
	QLineF m_line;
	QRectF m_rect;
	bool m_selected;
	WireFlags m_wireFlags;
	QTransform m_transform;
};


Q_DECLARE_OPERATORS_FOR_FLAGS(ViewGeometry::WireFlags)

 //Q_DECLARE_METATYPE(ViewGeometry *);

#endif
