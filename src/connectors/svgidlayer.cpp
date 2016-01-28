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

#include "svgidlayer.h"

SvgIdLayer::SvgIdLayer(ViewLayer::ViewID viewID) {
    m_viewID = viewID;
	m_pointRectBottom.processed = m_pointRectTop.processed = m_path = m_hybrid = false;
	m_radius = m_strokeWidth = 0;
}

SvgIdLayer * SvgIdLayer::copyLayer() {
	SvgIdLayer * toSvgIdLayer = new SvgIdLayer(m_viewID);
	*toSvgIdLayer = *this;
	return toSvgIdLayer;
}

QPointF SvgIdLayer::point(ViewLayer::ViewLayerPlacement viewLayerPlacement) {
    if (viewLayerPlacement == ViewLayer::NewBottom) return m_pointRectBottom.point;

    return m_pointRectTop.point;
}

QRectF SvgIdLayer::rect(ViewLayer::ViewLayerPlacement viewLayerPlacement) {
    if (viewLayerPlacement == ViewLayer::NewBottom) return m_pointRectBottom.rect;

    return m_pointRectTop.rect;
}

bool SvgIdLayer::processed(ViewLayer::ViewLayerPlacement viewLayerPlacement) {
    if (viewLayerPlacement == ViewLayer::NewBottom) return m_pointRectBottom.processed;

    return m_pointRectTop.processed;
}

bool SvgIdLayer::svgVisible(ViewLayer::ViewLayerPlacement viewLayerPlacement) {
    if (viewLayerPlacement == ViewLayer::NewBottom) return m_pointRectBottom.svgVisible;

    return m_pointRectTop.svgVisible;
}

void SvgIdLayer::unprocess() {
    m_pointRectTop.processed = m_pointRectBottom.processed = false;
}

void SvgIdLayer::setInvisible(ViewLayer::ViewLayerPlacement viewLayerPlacement) {
    if (viewLayerPlacement == ViewLayer::NewBottom) {
        m_pointRectBottom.svgVisible = false;
        m_pointRectBottom.processed = true;
        return;
    }

    m_pointRectTop.svgVisible = false;
    m_pointRectTop.processed = true;
}

void SvgIdLayer::setPointRect(ViewLayer::ViewLayerPlacement viewLayerPlacement, QPointF point, QRectF rect, bool svgVisible) {
    if (viewLayerPlacement == ViewLayer::NewBottom) {
        m_pointRectBottom.svgVisible = svgVisible;
        m_pointRectBottom.processed = true;
        m_pointRectBottom.point = point;
        m_pointRectBottom.rect = rect;
        return;
    }

    m_pointRectTop.svgVisible = svgVisible;
    m_pointRectTop.processed = true;
    m_pointRectTop.point = point;
    m_pointRectTop.rect = rect;
}

