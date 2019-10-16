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

#include "viewgeometry.h"
#include "utils/graphicsutils.h"

ViewGeometry::ViewGeometry(  ) 
	: m_loc(-1, -1),
	m_line(-1, -1, -1, -1),
	m_wireFlags(WireFlag::NoFlag)
{
}

ViewGeometry::ViewGeometry(const ViewGeometry& that) 
	: m_z(that.m_z),
	m_loc(that.m_loc),
	m_line(that.m_line),
	m_rect(that.m_rect),
	m_wireFlags(that.m_wireFlags),
	m_transform(that.m_transform) { }

ViewGeometry::ViewGeometry(QDomElement & geometry) 
	: m_z(geometry.attribute("z").toDouble()),
	m_loc(geometry.attribute("x").toDouble(),
	      geometry.attribute("y").toDouble()),
	m_wireFlags(static_cast<WireFlags>(geometry.attribute("wireFlags").toInt()))
{

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

void ViewGeometry::setZ(double z) noexcept {
	m_z = z;
}

void ViewGeometry::setLoc(QPointF loc) {
	m_loc = loc;
}

void ViewGeometry::setLine(QLineF loc) {
	m_line = loc;
}


void ViewGeometry::offset(double x, double y) {
	m_loc.setX(x + m_loc.x());
	m_loc.setY(y + m_loc.y());
}


void ViewGeometry::setSelected(bool selected) {
	m_selected = selected;
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
