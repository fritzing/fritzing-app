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

#include "viewgeometry.h"
#include "utils/graphicsutils.h"

ViewGeometry::ViewGeometry(  )
{
	m_z = -1;
	m_loc.setX(-1);
	m_loc.setY(-1);
	m_line.setLine(-1,-1,-1,-1);
	m_wireFlags = NoFlag;
}

ViewGeometry::ViewGeometry(const ViewGeometry & that  )
{
	set(that);
}

ViewGeometry::ViewGeometry(QDomElement & geometry) {
	m_wireFlags = (WireFlags) geometry.attribute("wireFlags").toInt();
	m_z = geometry.attribute("z").toDouble();
	m_loc.setX(geometry.attribute("x").toDouble());
	m_loc.setY(geometry.attribute("y").toDouble());
	QString x1 = geometry.attribute("x1");
	if (!x1.isEmpty()) {
		m_line.setLine( geometry.attribute("x1").toDouble(),
						geometry.attribute("y1").toDouble(),
						geometry.attribute("x2").toDouble(),
						geometry.attribute("y2").toDouble() );
	}
	QString w = geometry.attribute("width");
	if (!w.isEmpty()) {
		m_rect.setX(geometry.attribute("x").toDouble());
		m_rect.setY(geometry.attribute("y").toDouble());
		m_rect.setWidth(geometry.attribute("width").toDouble());
		m_rect.setHeight(geometry.attribute("height").toDouble());
	}

	GraphicsUtils::loadTransform(geometry.firstChildElement("transform"), m_transform);
}

void ViewGeometry::setZ(double z) {
	m_z = z;
}

double ViewGeometry::z() const {
	return m_z ;
}

void ViewGeometry::setLoc(QPointF loc) {
	m_loc = loc;
}

QPointF ViewGeometry::loc() const {
	return m_loc ;
}

void ViewGeometry::setLine(QLineF loc) {
	m_line = loc;
}

QLineF ViewGeometry::line() const {
	return m_line;
}

void ViewGeometry::offset(double x, double y) {
	m_loc.setX(x + m_loc.x());
	m_loc.setY(y + m_loc.y());
}

bool ViewGeometry::selected() {
	return m_selected;
}

void ViewGeometry::setSelected(bool selected) {
	m_selected = selected;
}

QRectF ViewGeometry::rect() const {
	return m_rect;
}

void ViewGeometry::setRect(double x, double y, double width, double height) 
{
	m_rect.setRect(x, y, width, height);
}

void ViewGeometry::setRect(const QRectF & r) 
{
	setRect(r.x(), r.y(), r.width(), r.height());
}

void ViewGeometry::setTransform(QTransform transform) {
	m_transform = transform;
}

QTransform ViewGeometry::transform() const {
	return m_transform;
}

void ViewGeometry::set(const ViewGeometry & that) {
	m_z = that.m_z;
	m_line = that.m_line;
	m_loc = that.m_loc;
	m_transform = that.m_transform;
	m_wireFlags = that.m_wireFlags;
	m_rect = that.m_rect;
}

void ViewGeometry::setRouted(bool routed) {
	setWireFlag(routed, RoutedFlag);
}

void ViewGeometry::setPCBTrace(bool trace) {
	setWireFlag(trace, PCBTraceFlag);
}

void ViewGeometry::setSchematicTrace(bool trace) {
	setWireFlag(trace, SchematicTraceFlag);
}

void ViewGeometry::setRatsnest(bool ratsnest) {
	setWireFlag(ratsnest, RatsnestFlag);
}

void ViewGeometry::setNormal(bool normal) {
	setWireFlag(normal, NormalFlag);
}

void ViewGeometry::setAutoroutable(bool autoroutable) {
	setWireFlag(autoroutable, AutoroutableFlag);
}

bool ViewGeometry::getRouted() const {
	
	return m_wireFlags.testFlag(RoutedFlag);
}

bool ViewGeometry::getNormal() const {
	
	return m_wireFlags.testFlag(NormalFlag);
}

bool ViewGeometry::getPCBTrace() const {
	return m_wireFlags.testFlag(PCBTraceFlag);
}

bool ViewGeometry::getAnyTrace() const {
	return getPCBTrace() || getSchematicTrace();
}

bool ViewGeometry::getSchematicTrace() const {
	return m_wireFlags.testFlag(SchematicTraceFlag);
}

bool ViewGeometry::getRatsnest() const {
	return m_wireFlags.testFlag(RatsnestFlag);
}

bool ViewGeometry::getAutoroutable() const {
	return m_wireFlags.testFlag(AutoroutableFlag);
}

void ViewGeometry::setWireFlag(bool setting, WireFlag flag) {
	if (setting) {
		m_wireFlags |= flag;
	}
	else {
		m_wireFlags &= ~flag;
	}
}

int ViewGeometry::flagsAsInt() const {
	return (int) m_wireFlags;
}

void ViewGeometry::setWireFlags(WireFlags wireFlags) {
	m_wireFlags = wireFlags;
}

bool ViewGeometry::hasFlag(ViewGeometry::WireFlag flag) {
	return (m_wireFlags & flag) ? true : false;
}

bool ViewGeometry::hasAnyFlag(ViewGeometry::WireFlags flags) {
	return (m_wireFlags & flags) ? true : false;
}


ViewGeometry::WireFlags ViewGeometry::wireFlags() const {
	return m_wireFlags;
}
