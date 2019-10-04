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

SvgIdLayer::SvgIdLayer(ViewLayer::ViewID viewID) : m_viewID(viewID) { }

SvgIdLayer * SvgIdLayer::copyLayer() {
	SvgIdLayer * toSvgIdLayer = new SvgIdLayer(m_viewID);
	*toSvgIdLayer = *this;
	return toSvgIdLayer;
}

QPointF SvgIdLayer::point(ViewLayer::ViewLayerPlacement viewLayerPlacement) {
    if (viewLayerPlacement == ViewLayer::NewBottom) {
        return m_pointRectBottom.point;
    } else {
        return m_pointRectTop.point;
    }
}

QRectF SvgIdLayer::rect(ViewLayer::ViewLayerPlacement viewLayerPlacement) {
	if (viewLayerPlacement == ViewLayer::NewBottom) {
        return m_pointRectBottom.rect;
    } else {
        return m_pointRectTop.rect;
    }
}

bool SvgIdLayer::processed(ViewLayer::ViewLayerPlacement viewLayerPlacement) {
	if (viewLayerPlacement == ViewLayer::NewBottom) {
        return m_pointRectBottom.processed;
    } else {
	    return m_pointRectTop.processed;
    }
}

bool SvgIdLayer::svgVisible(ViewLayer::ViewLayerPlacement viewLayerPlacement) {
	if (viewLayerPlacement == ViewLayer::NewBottom) {
        return m_pointRectBottom.svgVisible;
    } else {
	    return m_pointRectTop.svgVisible;
    }
}

void PointRect::unprocess() {
    processed = false;
}

void PointRect::setInvisible() {
    svgVisible = false;
    processed = true;
}
PointRect::PointRect() { }
PointRect::PointRect(bool _svgVisible, bool _processed, QRectF _rect, QPointF _point) : 
    rect(_rect), 
    point(_point),
    processed(_processed),
    svgVisible(_svgVisible) { }

PointRect::PointRect(const PointRect& other) : rect(other.rect), point(other.point), processed(other.processed), svgVisible(other.svgVisible) { }

void SvgIdLayer::unprocess() {
    m_pointRectTop.unprocess();
    m_pointRectBottom.unprocess();
}

void SvgIdLayer::setInvisible(ViewLayer::ViewLayerPlacement viewLayerPlacement) {
	if (viewLayerPlacement == ViewLayer::NewBottom) {
        m_pointRectBottom.setInvisible();
	} else {
        m_pointRectTop.setInvisible();
    }
}

void SvgIdLayer::setPointRect(ViewLayer::ViewLayerPlacement viewLayerPlacement, const PointRect& other) {
    if (viewLayerPlacement == ViewLayer::NewBottom) {
        m_pointRectBottom = other;
    } else {
        m_pointRectTop = other;
    }
}
void SvgIdLayer::setPointRect(ViewLayer::ViewLayerPlacement viewLayerPlacement, QPointF point, QRectF rect, bool svgVisible) {
    setPointRect(viewLayerPlacement, PointRect(svgVisible, true, point, rect));
}
