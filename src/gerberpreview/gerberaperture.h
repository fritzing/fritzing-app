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

#ifndef GERBERAPERTURE_H
#define GERBERAPERTURE_H

#include <QPainterPath>
#include <QPointF>
#include <QString>

/**
 * @brief A single Gerber RS-274X aperture (the "brush" the plotter
 *        uses to stroke or flash). One aperture per dcode (>= 10).
 *
 * Fritzing only emits the four standard primitives: Circle ("C"),
 * Rectangle ("R"), Obround ("O"), and Polygon ("P"). Aperture macros
 * ("AM") are intentionally unsupported; the parser will warn and
 * substitute a small circle so the file still renders.
 *
 * All sizes are stored in millimetres after parser conversion.
 */
class GerberAperture {
public:
	enum Kind {
		Circle,     ///< C,<diameter>[,<hole>]
		Rectangle,  ///< R,<x>x<y>[,<hole>]
		Obround,    ///< O,<x>x<y>[,<hole>]  — rect with semicircular ends on long axis
		Polygon,    ///< P,<diameter>x<verts>[x<rot>[x<hole>]]
		Unknown     ///< Macro or unparsed; rendered as 0.1 mm circle
	};

	GerberAperture()
		: kind(Unknown), w(0.1), h(0.1), holeDiameter(0.0),
		  vertices(0), rotationDeg(0.0) {}

	Kind   kind;
	double w;             ///< Width (diameter for Circle/Polygon)
	double h;             ///< Height (Rectangle/Obround only)
	double holeDiameter;  ///< Optional centred hole
	int    vertices;      ///< Polygon only
	double rotationDeg;   ///< Polygon only

	/**
	 * @brief Builds the aperture outline centred on (0,0) as a
	 *        QPainterPath so the renderer can flash it with a single
	 *        translate + fillPath call. The optional centred hole is
	 *        appended with Qt::OddEvenFill so it punches through.
	 */
	QPainterPath path() const;

	/**
	 * @brief Stroke width to use when this aperture draws a line
	 *        (D01). Convention: Gerber stroke uses the aperture's
	 *        smallest dimension. Returns 0.0 for region mode (the
	 *        caller is expected to use fillPath instead).
	 */
	double strokeWidth() const;

	/**
	 * @brief Short human-readable description of this aperture,
	 *        used by the preview dialog's status bar when the user
	 *        clicks a flash/stroke. Format examples:
	 *        "Circle \u00d80.80 mm", "Rect 1.60\u00d72.00 mm",
	 *        "Obround 0.60\u00d71.20 mm", "Polygon \u00d81.00 mm, 6v".
	 *        Hole, when present, is appended as " (hole \u00d80.30 mm)".
	 */
	QString description() const;
};

#endif // GERBERAPERTURE_H
