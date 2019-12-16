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

#ifndef SVGIDLAYER_H
#define SVGIDLAYER_H

#include <QString>

#include "../viewlayer.h"


class PointRect {
	public:
		PointRect() = default;
		PointRect(bool _svgVisible, bool _processed, QRectF _rect, QPointF _point) noexcept : 
			rect(_rect),
			point(_point),
			processed(_processed),
			svgVisible(_svgVisible) { }
		PointRect(const PointRect& other);
		void unprocess() noexcept;
		void setInvisible() noexcept;
		constexpr QPointF getPoint() const noexcept { return point; }
		constexpr QRectF getRect() const noexcept { return rect; }
		constexpr bool isProcessed() const noexcept { return processed; }
		constexpr bool isSvgVisible() const noexcept { return svgVisible; }
		void setPoint(QPointF other) noexcept {
			this->point = other;
		}
		void setRect(QRectF other) noexcept {
			this->rect = other;
		}
	private:
		QRectF rect;
		QPointF point;
		bool processed = false;
		bool svgVisible = false;
};

class SvgIdLayer
{
public:
	SvgIdLayer(ViewLayer::ViewID);
	SvgIdLayer(const SvgIdLayer&);
	SvgIdLayer * copyLayer();

	QPointF point(ViewLayer::ViewLayerPlacement);
	QRectF rect(ViewLayer::ViewLayerPlacement);
	bool processed(ViewLayer::ViewLayerPlacement);
	bool svgVisible(ViewLayer::ViewLayerPlacement);
	void unprocess() noexcept;
	inline void setInvisible(ViewLayer::ViewLayerPlacement placement) noexcept {
		if (placement == ViewLayer::ViewLayerPlacement::NewBottom) {
			m_pointRectBottom.setInvisible();
		} else {
			m_pointRectTop.setInvisible();
		}
	}
	void setPointRect(ViewLayer::ViewLayerPlacement, QPointF, QRectF, bool svgVisible);
	void setPointRect(ViewLayer::ViewLayerPlacement, const PointRect& newPointRect);

public:
	ViewLayer::ViewID m_viewID;
	ViewLayer::ViewLayerID m_svgViewLayerID;
	QString m_svgId;
	QString m_terminalId;
	QString m_legId;
	QString m_legColor;
	QLineF m_legLine;
	double m_radius;
	double m_strokeWidth;
	double m_legStrokeWidth;
	bool m_hybrid;
	bool m_path;

protected:
	PointRect m_pointRectBottom;
	PointRect m_pointRectTop;
};


#endif
