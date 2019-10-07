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

#include "svgidlayer.h"

SvgIdLayer::SvgIdLayer(ViewLayer::ViewID viewID) : 
	m_viewID(viewID),
	m_svgViewLayerID(),
	m_svgId(),
	m_terminalId(),
	m_legId(),
	m_legColor(),
	m_legLine(),
	m_radius(0.0),
	m_strokeWidth(0.0),
	m_legStrokeWidth(0.0),
	m_hybrid(false),
	m_path(false)
{ }

SvgIdLayer::SvgIdLayer(const SvgIdLayer& other) : 
	m_viewID(other.m_viewID),
	m_svgViewLayerID(other.m_svgViewLayerID),
	m_svgId(other.m_svgId),
	m_terminalId(other.m_terminalId),
	m_legId(other.m_legId),
	m_legColor(other.m_legColor),
	m_legLine(other.m_legLine),
	m_radius(other.m_radius),
	m_strokeWidth(other.m_strokeWidth),
	m_legStrokeWidth(other.m_legStrokeWidth),
	m_hybrid(other.m_hybrid),
	m_path(other.m_path) { }

SvgIdLayer * SvgIdLayer::copyLayer() {
	return new SvgIdLayer(*this);
}

QPointF SvgIdLayer::point(ViewLayer::ViewLayerPlacement viewLayerPlacement) {
	if (viewLayerPlacement == ViewLayer::NewBottom) return m_pointRectBottom.getPoint();
	return m_pointRectTop.getPoint();
}

QRectF SvgIdLayer::rect(ViewLayer::ViewLayerPlacement viewLayerPlacement) {
	if (viewLayerPlacement == ViewLayer::NewBottom) return m_pointRectBottom.getRect();
	return m_pointRectTop.getRect();
}

bool SvgIdLayer::processed(ViewLayer::ViewLayerPlacement viewLayerPlacement) {
	if (viewLayerPlacement == ViewLayer::NewBottom) return m_pointRectBottom.isProcessed();
	return m_pointRectTop.isProcessed();
}

bool SvgIdLayer::svgVisible(ViewLayer::ViewLayerPlacement viewLayerPlacement) {
	if (viewLayerPlacement == ViewLayer::NewBottom) return m_pointRectBottom.isSvgVisible();
	    return m_pointRectTop.isSvgVisible();
}


void SvgIdLayer::setPointRect(ViewLayer::ViewLayerPlacement viewLayerPlacement, const PointRect& other) {
	if (viewLayerPlacement == ViewLayer::NewBottom) {
		m_pointRectBottom = other;
	} else {
		m_pointRectTop = other;
	}
}
void SvgIdLayer::setPointRect(ViewLayer::ViewLayerPlacement viewLayerPlacement, QPointF point, QRectF rect, bool svgVisible) {
	setPointRect(viewLayerPlacement, PointRect(svgVisible, true, rect, point));
}

void PointRect::unprocess() noexcept {
	processed = false;
}

void PointRect::setInvisible() noexcept {
	svgVisible = false;
	processed = true;
}

void SvgIdLayer::unprocess() noexcept {
	m_pointRectTop.unprocess();
	m_pointRectBottom.unprocess();
}
