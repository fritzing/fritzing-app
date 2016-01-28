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

#ifndef SVGIDLAYER_H
#define SVGIDLAYER_H

#include <QString>

#include "../viewlayer.h"


struct PointRect {
    QRectF rect;		
	QPointF point;	
	bool processed;
	bool svgVisible;
};

class SvgIdLayer 
{
public:
	SvgIdLayer(ViewLayer::ViewID);
	SvgIdLayer * copyLayer();

    QPointF point(ViewLayer::ViewLayerPlacement);
    QRectF rect(ViewLayer::ViewLayerPlacement);
    bool processed(ViewLayer::ViewLayerPlacement);
    bool svgVisible(ViewLayer::ViewLayerPlacement);
    void unprocess();
    void setInvisible(ViewLayer::ViewLayerPlacement);
    void setPointRect(ViewLayer::ViewLayerPlacement, QPointF, QRectF, bool svgVisible);

public:
	QString m_svgId;
	QString m_terminalId;
    ViewLayer::ViewID m_viewID;
	ViewLayer::ViewLayerID m_svgViewLayerID;
	bool m_hybrid;
	double m_radius;
	double m_strokeWidth;
	QString m_legId;
	QString m_legColor;
	double m_legStrokeWidth;
	QLineF m_legLine;
    bool m_path;

protected:
    PointRect m_pointRectBottom;
    PointRect m_pointRectTop;

protected:
};


#endif
