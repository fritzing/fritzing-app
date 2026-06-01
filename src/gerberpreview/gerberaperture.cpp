/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2026 Fritzing

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

// Clean-room implementation of Ucamco's published Gerber spec
// (https://www.ucamco.com/en/gerber/downloads). No code from gerbv
// or any other GPL-2-only project is used here. See homerun-gui.md
// §12.1 for the license rationale.

#include "gerberaperture.h"

#include <QtMath>

QPainterPath GerberAperture::path() const
{
	QPainterPath p;
	switch (kind) {
		case Circle: {
			const double r = w * 0.5;
			p.addEllipse(QPointF(0, 0), r, r);
			break;
		}
		case Rectangle: {
			p.addRect(QRectF(-w * 0.5, -h * 0.5, w, h));
			break;
		}
		case Obround: {
			// Long axis gets the semicircular ends. If w == h this
			// degenerates to a circle, which is also spec-correct.
			const double rx = qMin(w, h) * 0.5;
			p.addRoundedRect(QRectF(-w * 0.5, -h * 0.5, w, h), rx, rx);
			break;
		}
		case Polygon: {
			const int    n = qMax(3, vertices);
			const double r = w * 0.5;
			const double phi0 = qDegreesToRadians(rotationDeg);
			QPolygonF poly;
			poly.reserve(n + 1);
			for (int i = 0; i < n; ++i) {
				const double a = phi0 + (2.0 * M_PI * i) / n;
				poly << QPointF(r * qCos(a), r * qSin(a));
			}
			poly << poly.first();
			p.addPolygon(poly);
			break;
		}
		case Unknown:
		default: {
			// Substitute a tiny circle so files with macros still
			// render; the parser will already have logged a warning.
			p.addEllipse(QPointF(0, 0), 0.05, 0.05);
			break;
		}
	}
	if (holeDiameter > 0.0) {
		const double hr = holeDiameter * 0.5;
		// OddEvenFill: hole punches through the body when the path
		// is filled.
		p.setFillRule(Qt::OddEvenFill);
		p.addEllipse(QPointF(0, 0), hr, hr);
	}
	return p;
}

double GerberAperture::strokeWidth() const
{
	switch (kind) {
		case Circle:    return w;
		case Rectangle: return qMin(w, h);
		case Obround:   return qMin(w, h);
		case Polygon:   return w;
		case Unknown:
		default:        return 0.1;
	}
}

QString GerberAperture::description() const
{
	// Unicode Ø (U+00D8) is the conventional "diameter" prefix; ×
	// (U+00D7) is multiplication. Both render in the Qt default font.
	QString core;
	switch (kind) {
		case Circle:
			core = QStringLiteral("Circle \u00d8%1 mm").arg(w, 0, 'f', 3);
			break;
		case Rectangle:
			core = QStringLiteral("Rect %1\u00d7%2 mm")
				.arg(w, 0, 'f', 3).arg(h, 0, 'f', 3);
			break;
		case Obround:
			core = QStringLiteral("Obround %1\u00d7%2 mm")
				.arg(w, 0, 'f', 3).arg(h, 0, 'f', 3);
			break;
		case Polygon:
			core = QStringLiteral("Polygon \u00d8%1 mm, %2v")
				.arg(w, 0, 'f', 3).arg(vertices);
			break;
		case Unknown:
		default:
			core = QStringLiteral("Unknown aperture");
			break;
	}
	if (holeDiameter > 0.0) {
		core += QStringLiteral(" (hole \u00d8%1 mm)")
			.arg(holeDiameter, 0, 'f', 3);
	}
	return core;
}
