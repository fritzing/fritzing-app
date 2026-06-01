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

#include "panelizerseparators.h"

#include <QRectF>
#include <QSet>
#include <QStringList>
#include <QtMath>
#include <algorithm>
#include <cmath>

namespace PanelizerSeparators {

// ======================================================================
// All SVG output here follows the Fritzing SVG normalizer convention:
// elements are grouped under `<g id="...">` rather than carrying a
// custom `layer` attribute. The recognized IDs that map to Gerber
// layers in GerberGenerator are: silkscreen0/silkscreen1,
// copper0/copper1, soldermask0/soldermask1, edge_cuts (board outline,
// .gko), drill (NPTH/PTH, .txt). See parts/svg/core/pcb/* for live
// examples.
//
// Coordinate space: Fritzing uses a 1000 dpi SVG unit (every inch =
// 1000 px). The viewBox is in those px, while width/height carry the
// physical inch dimension so downstream renderers know the scale.
// All inputs here are inches (PanelizerEngine::PanelSpec convention).
// ======================================================================

namespace {

constexpr double DPI = 1000.0;          // Fritzing SVG resolution
constexpr double DEDUPE_EPSILON = 1e-3;  // 0.001in dedupe tolerance

/**
 * @brief Compute the placed board's footprint rect in panel inches.
 * @note Honors rotated90 by swapping width/height. If the source
 *       board size is zero (unknown geometry) we return an empty
 *       rect; callers should skip it.
 */
static QRectF placedRect(const PanelizerEngine::PlacedBoard * pb) {
	if (pb == nullptr) return QRectF();
	QSizeF s = pb->source.boardSizeInches;
	if (pb->rotated90) s = QSizeF(s.height(), s.width());
	return QRectF(pb->positionInches, s);
}

/**
 * @brief Insert v into the sorted list iff no entry is within epsilon.
 *
 * Used to dedupe near-equal gutter centerline coordinates produced
 * by the layout pass (e.g. two boards in the same column produce the
 * same right-edge X within float rounding).
 */
static void insertUnique(QList<double> & list, double v) {
	for (double existing : list) {
		if (std::abs(existing - v) < DEDUPE_EPSILON) return;
	}
	list.append(v);
}

} // anonymous namespace

QString vcutSvg(const QList<PanelizerEngine::PlacedBoard*>& items,
                const PanelizerEngine::PanelSpec& spec,
                const PanelizerEngine::SeparationSpec& sep)
{
	const double Win = spec.panelSizeInches.width();
	const double Hin = spec.panelSizeInches.height();
	if (Win <= 0.0 || Hin <= 0.0) return QString();

	const double gutter = spec.gutterInches;
	const double halfGutter = gutter / 2.0;
	const double border = spec.borderInches;

	// Collect candidate cut centerlines. A board whose right (or bottom)
	// edge sits more than half a gutter away from the panel rail edge has
	// at least one neighbor or rail to its right - either way we score
	// at rightEdge + halfGutter.
	QList<double> vCutX;
	QList<double> hCutY;
	for (const PanelizerEngine::PlacedBoard * pb : items) {
		const QRectF r = placedRect(pb);
		if (r.isEmpty()) continue;
		const double railRight  = Win - border;
		const double railBottom = Hin - border;
		if (r.right() + halfGutter < railRight - DEDUPE_EPSILON) {
			insertUnique(vCutX, r.right() + halfGutter);
		}
		if (r.bottom() + halfGutter < railBottom - DEDUPE_EPSILON) {
			insertUnique(hCutY, r.bottom() + halfGutter);
		}
	}
	std::sort(vCutX.begin(), vCutX.end());
	std::sort(hCutY.begin(), hCutY.end());

	// Build SVG. Stroke width is given in mils -> inches.
	const double strokePx = (sep.vcutLineWidthMils / 1000.0) * DPI;
	const double viewW = Win * DPI;
	const double viewH = Hin * DPI;

	QStringList out;
	out << QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	out << QStringLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\""
	                      " width=\"%1in\" height=\"%2in\" viewBox=\"0 0 %3 %4\">")
	       .arg(Win, 0, 'f', 4).arg(Hin, 0, 'f', 4)
	       .arg(viewW, 0, 'f', 2).arg(viewH, 0, 'f', 2);
	out << QStringLiteral("  <g id=\"edge_cuts\" stroke=\"#000000\" stroke-width=\"%1\" fill=\"none\">")
	       .arg(strokePx, 0, 'f', 2);

	// NOTE: panel outline rect is NOT emitted here — frameOutlineSvg()
	// owns the frame. Emitting it from both producers caused gerbv to
	// see a duplicate Edge_Cuts rectangle.

	// Vertical V-cut lines: clipped to panel height. (Earlier versions
	// added an overrun past the panel edges to make the cut path obvious
	// in a viewer, but JLC's preflight flags geometry outside the board
	// outline. Stay strictly inside the frame.)
	for (double xIn : vCutX) {
		const double xPx = xIn * DPI;
		out << QStringLiteral("    <line x1=\"%1\" y1=\"0\" x2=\"%1\" y2=\"%2\"/>")
		       .arg(xPx, 0, 'f', 2).arg(viewH, 0, 'f', 2);
	}
	// Horizontal V-cut lines: clipped to panel width.
	for (double yIn : hCutY) {
		const double yPx = yIn * DPI;
		out << QStringLiteral("    <line x1=\"0\" y1=\"%1\" x2=\"%2\" y2=\"%1\"/>")
		       .arg(yPx, 0, 'f', 2).arg(viewW, 0, 'f', 2);
	}

	out << QStringLiteral("  </g>");
	out << QStringLiteral("</svg>");
	return out.join(QChar('\n'));
}

QString mouseBiteSvg(const QList<PanelizerEngine::PlacedBoard*>& items,
                     const PanelizerEngine::PanelSpec& spec,
                     const PanelizerEngine::SeparationSpec& sep)
{
	// Mouse-bites: replace each interior gutter with a tab pattern.
	//
	// Geometry (per gutter segment between two adjacent boards):
	//   - Two parallel "slot" cuts run on either side of the gutter
	//     centerline (one on each board's edge), stopping short of
	//     the tab so the tab itself stays connected.
	//   - Inside the tab region, holesPerTab non-plated through-holes
	//     are equispaced along the centerline. These perforate the
	//     substrate so the user can snap the boards apart.
	//
	// Defaults match KiKit (see ../../KiKit/.../substrate.py mouseBites):
	//   tab width = 3mm, 5 holes per tab, 0.5mm dia @ 0.8mm pitch.
	// Centered tab placement: one tab per gutter segment, midpoint.
	//
	// The holes go on `<g id="drill">` (-> .drl Excellon),
	// the slot cuts go on `<g id="edge_cuts">` (-> Edge_Cuts.gbr).

	const double Win = spec.panelSizeInches.width();
	const double Hin = spec.panelSizeInches.height();
	if (Win <= 0.0 || Hin <= 0.0) return QString();

	const double gutter   = spec.gutterInches;
	if (gutter <= 0.0) return QString();
	const double halfGutter = gutter / 2.0;
	const double tabInches  = sep.tabWidthInches;
	// Convert mil-denominated hole geometry to inches once.
	const double holeDiaIn  = sep.holeDiameterMils / 1000.0;
	const double holePitchIn = sep.holePitchMils / 1000.0;
	const int    nHoles     = qMax(1, sep.holesPerTab);

	// Stroke for the slot cuts: same width as v-cut so the fab sees
	// a familiar geometry. The slot only carries information about
	// where the router should cut; thickness is cosmetic on Edge_Cuts.
	const double strokePx = (sep.vcutLineWidthMils / 1000.0) * DPI;
	const double viewW = Win * DPI;
	const double viewH = Hin * DPI;

	QStringList out;
	out << QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	out << QStringLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\""
	                      " width=\"%1in\" height=\"%2in\" viewBox=\"0 0 %3 %4\">")
	       .arg(Win, 0, 'f', 4).arg(Hin, 0, 'f', 4)
	       .arg(viewW, 0, 'f', 2).arg(viewH, 0, 'f', 2);
	out << QStringLiteral("  <g id=\"edge_cuts\" stroke=\"#000000\" stroke-width=\"%1\" fill=\"none\">")
	       .arg(strokePx, 0, 'f', 2);

	// Collect vertical / horizontal gutter centerlines from board edges.
	// Reuse the same dedupe approach as vcutSvg so V-cut and mouse-bite
	// emit identical gutter sets when toggled.
	QList<double> vGutX, hGutY;
	for (const PanelizerEngine::PlacedBoard * pb : items) {
		const QRectF r = placedRect(pb);
		if (r.isEmpty()) continue;
		const double railRight  = Win - spec.borderInches;
		const double railBottom = Hin - spec.borderInches;
		if (r.right() + halfGutter < railRight - DEDUPE_EPSILON)
			insertUnique(vGutX, r.right() + halfGutter);
		if (r.bottom() + halfGutter < railBottom - DEDUPE_EPSILON)
			insertUnique(hGutY, r.bottom() + halfGutter);
	}
	std::sort(vGutX.begin(), vGutX.end());
	std::sort(hGutY.begin(), hGutY.end());

	// Buffer holes in a second pass; they belong to a different group.
	QStringList holes;

	// --- Vertical gutters (cut runs top->bottom; tab spans a window
	// in the middle, holes lie on the centerline) ---
	for (double xIn : vGutX) {
		const double xPx = xIn * DPI;
		// Slot cuts: two vertical lines, each offset half a gutter.
		// They run the full panel height EXCEPT for the tab window.
		const double tabY0 = (Hin / 2.0) - (tabInches / 2.0);
		const double tabY1 = (Hin / 2.0) + (tabInches / 2.0);
		const double leftXPx  = (xIn - halfGutter) * DPI;
		const double rightXPx = (xIn + halfGutter) * DPI;
		// upper segment of left slot
		out << QStringLiteral("    <line x1=\"%1\" y1=\"0\" x2=\"%1\" y2=\"%2\"/>")
		       .arg(leftXPx, 0, 'f', 2).arg(tabY0 * DPI, 0, 'f', 2);
		// lower segment of left slot
		out << QStringLiteral("    <line x1=\"%1\" y1=\"%2\" x2=\"%1\" y2=\"%3\"/>")
		       .arg(leftXPx, 0, 'f', 2).arg(tabY1 * DPI, 0, 'f', 2).arg(viewH, 0, 'f', 2);
		// upper segment of right slot
		out << QStringLiteral("    <line x1=\"%1\" y1=\"0\" x2=\"%1\" y2=\"%2\"/>")
		       .arg(rightXPx, 0, 'f', 2).arg(tabY0 * DPI, 0, 'f', 2);
		// lower segment of right slot
		out << QStringLiteral("    <line x1=\"%1\" y1=\"%2\" x2=\"%1\" y2=\"%3\"/>")
		       .arg(rightXPx, 0, 'f', 2).arg(tabY1 * DPI, 0, 'f', 2).arg(viewH, 0, 'f', 2);

		// Holes equispaced along centerline within the tab window.
		// span = (nHoles-1)*pitch; centered on tab midpoint.
		const double span = (nHoles - 1) * holePitchIn;
		const double y0 = (Hin / 2.0) - span / 2.0;
		const double rPx = (holeDiaIn / 2.0) * DPI;
		for (int i = 0; i < nHoles; ++i) {
			const double yPx = (y0 + i * holePitchIn) * DPI;
			holes << QStringLiteral("    <circle cx=\"%1\" cy=\"%2\" r=\"%3\"/>")
			          .arg(xPx, 0, 'f', 2).arg(yPx, 0, 'f', 2).arg(rPx, 0, 'f', 2);
		}
	}

	// --- Horizontal gutters (mirrored geometry) ---
	for (double yIn : hGutY) {
		const double yPx = yIn * DPI;
		const double tabX0 = (Win / 2.0) - (tabInches / 2.0);
		const double tabX1 = (Win / 2.0) + (tabInches / 2.0);
		const double topYPx    = (yIn - halfGutter) * DPI;
		const double botYPx    = (yIn + halfGutter) * DPI;
		out << QStringLiteral("    <line x1=\"0\" y1=\"%1\" x2=\"%2\" y2=\"%1\"/>")
		       .arg(topYPx, 0, 'f', 2).arg(tabX0 * DPI, 0, 'f', 2);
		out << QStringLiteral("    <line x1=\"%1\" y1=\"%2\" x2=\"%3\" y2=\"%2\"/>")
		       .arg(tabX1 * DPI, 0, 'f', 2).arg(topYPx, 0, 'f', 2).arg(viewW, 0, 'f', 2);
		out << QStringLiteral("    <line x1=\"0\" y1=\"%1\" x2=\"%2\" y2=\"%1\"/>")
		       .arg(botYPx, 0, 'f', 2).arg(tabX0 * DPI, 0, 'f', 2);
		out << QStringLiteral("    <line x1=\"%1\" y1=\"%2\" x2=\"%3\" y2=\"%2\"/>")
		       .arg(tabX1 * DPI, 0, 'f', 2).arg(botYPx, 0, 'f', 2).arg(viewW, 0, 'f', 2);

		const double span = (nHoles - 1) * holePitchIn;
		const double x0 = (Win / 2.0) - span / 2.0;
		const double rPx = (holeDiaIn / 2.0) * DPI;
		for (int i = 0; i < nHoles; ++i) {
			const double xPx = (x0 + i * holePitchIn) * DPI;
			holes << QStringLiteral("    <circle cx=\"%1\" cy=\"%2\" r=\"%3\"/>")
			          .arg(xPx, 0, 'f', 2).arg(yPx, 0, 'f', 2).arg(rPx, 0, 'f', 2);
		}
	}

	out << QStringLiteral("  </g>");
	// Holes ride a dedicated drill group so SVG2gerber (ForDrill)
	// can pick them up cleanly into the Excellon .drl file.
	out << QStringLiteral("  <g id=\"drill\" fill=\"#000000\" stroke=\"none\">");
	for (const QString & h : holes) out << h;
	out << QStringLiteral("  </g>");
	out << QStringLiteral("</svg>");
	return out.join(QChar('\n'));
}

QString fiducialSvg(const PanelizerEngine::PanelSpec& spec,
                    double fiducialDiameter,
                    double fiducialClear)
{
	// Three corner fiducials (top-left, top-right, bottom-right) at
	// rail center, inset borderInches/2 from each edge. Each fiducial
	// emits a copper pad + a slightly larger mask opening so the
	// vision system can detect a high-contrast disc.
	//
	// Inputs are in **mils** to match SeparationSpec convention.

	const double Win = spec.panelSizeInches.width();
	const double Hin = spec.panelSizeInches.height();
	if (Win <= 0.0 || Hin <= 0.0) return QString();

	const double padDiaIn   = fiducialDiameter / 1000.0;
	const double maskDiaIn  = (fiducialDiameter + 2 * fiducialClear) / 1000.0;
	const double rPad       = (padDiaIn / 2.0) * DPI;
	const double rMask      = (maskDiaIn / 2.0) * DPI;
	const double inset      = spec.borderInches / 2.0;
	const double viewW      = Win * DPI;
	const double viewH      = Hin * DPI;

	// Corner centers (px).
	const QList<QPointF> centers = {
		QPointF(inset * DPI,              inset * DPI),               // top-left
		QPointF((Win - inset) * DPI,      inset * DPI),               // top-right
		QPointF((Win - inset) * DPI,      (Hin - inset) * DPI)        // bottom-right
	};

	QStringList out;
	out << QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	out << QStringLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\""
	                      " width=\"%1in\" height=\"%2in\" viewBox=\"0 0 %3 %4\">")
	       .arg(Win, 0, 'f', 4).arg(Hin, 0, 'f', 4)
	       .arg(viewW, 0, 'f', 2).arg(viewH, 0, 'f', 2);
	// Top copper pads
	out << QStringLiteral("  <g id=\"copper1\" fill=\"#000000\" stroke=\"none\">");
	for (const QPointF & c : centers) {
		out << QStringLiteral("    <circle cx=\"%1\" cy=\"%2\" r=\"%3\"/>")
		       .arg(c.x(), 0, 'f', 2).arg(c.y(), 0, 'f', 2).arg(rPad, 0, 'f', 2);
	}
	out << QStringLiteral("  </g>");
	// Top soldermask openings (clearance discs)
	out << QStringLiteral("  <g id=\"soldermask1\" fill=\"#000000\" stroke=\"none\">");
	for (const QPointF & c : centers) {
		out << QStringLiteral("    <circle cx=\"%1\" cy=\"%2\" r=\"%3\"/>")
		       .arg(c.x(), 0, 'f', 2).arg(c.y(), 0, 'f', 2).arg(rMask, 0, 'f', 2);
	}
	out << QStringLiteral("  </g>");
	out << QStringLiteral("</svg>");
	return out.join(QChar('\n'));
}

QString toolingHoleSvg(const PanelizerEngine::PanelSpec& spec,
                       double toolingHoleDiameter)
{
	// Four NPTH tooling holes, one at each rail corner, inset by 5mm
	// (0.197") from each edge - the de-facto standard for JLC/PCBWay.
	// All four go on `<g id="drill">` for the Excellon writer.

	const double Win = spec.panelSizeInches.width();
	const double Hin = spec.panelSizeInches.height();
	if (Win <= 0.0 || Hin <= 0.0) return QString();
	if (toolingHoleDiameter <= 0.0) return QString();

	const double offset = 0.197;                   // 5mm in inches
	const double rPx    = (toolingHoleDiameter / 2.0) * DPI;
	const double viewW  = Win * DPI;
	const double viewH  = Hin * DPI;

	const QList<QPointF> centers = {
		QPointF(offset * DPI,         offset * DPI),
		QPointF((Win - offset)*DPI,   offset * DPI),
		QPointF(offset * DPI,         (Hin - offset)*DPI),
		QPointF((Win - offset)*DPI,   (Hin - offset)*DPI)
	};

	QStringList out;
	out << QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	out << QStringLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\""
	                      " width=\"%1in\" height=\"%2in\" viewBox=\"0 0 %3 %4\">")
	       .arg(Win, 0, 'f', 4).arg(Hin, 0, 'f', 4)
	       .arg(viewW, 0, 'f', 2).arg(viewH, 0, 'f', 2);
	out << QStringLiteral("  <g id=\"drill\" fill=\"#000000\" stroke=\"none\">");
	for (const QPointF & c : centers) {
		out << QStringLiteral("    <circle cx=\"%1\" cy=\"%2\" r=\"%3\"/>")
		       .arg(c.x(), 0, 'f', 2).arg(c.y(), 0, 'f', 2).arg(rPx, 0, 'f', 2);
	}
	out << QStringLiteral("  </g>");
	out << QStringLiteral("</svg>");
	return out.join(QChar('\n'));
}

QString frameOutlineSvg(const PanelizerEngine::PanelSpec& spec)
{
	// Panel outer outline on Edge_Cuts. Style + fillets per spec.
	const double Win = spec.panelSizeInches.width();
	const double Hin = spec.panelSizeInches.height();
	if (Win <= 0.0 || Hin <= 0.0) return QString();
	if (spec.frameStyle == PanelizerEngine::FrameStyle::Hidden) return QString();

	const double strokePx = 0.010 * DPI; // 10-mil hairline; cosmetic
	const double viewW = Win * DPI;
	const double viewH = Hin * DPI;
	const double rxPx  = qMax(0.0, spec.frameFilletInches) * DPI;

	QStringList out;
	out << QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	out << QStringLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\""
	                      " width=\"%1in\" height=\"%2in\" viewBox=\"0 0 %3 %4\">")
	       .arg(Win, 0, 'f', 4).arg(Hin, 0, 'f', 4)
	       .arg(viewW, 0, 'f', 2).arg(viewH, 0, 'f', 2);
	out << QStringLiteral("  <g id=\"edge_cuts\" stroke=\"#000000\" stroke-width=\"%1\" fill=\"none\">")
	       .arg(strokePx, 0, 'f', 2);

	switch (spec.frameStyle) {
		case PanelizerEngine::FrameStyle::Hidden:
			break; // unreachable; early-returned above
		case PanelizerEngine::FrameStyle::Rails: {
			// Only top + bottom rail rectangles (each borderInches tall).
			const double railH = spec.borderInches * DPI;
			out << QStringLiteral("    <rect x=\"0\" y=\"0\" width=\"%1\" height=\"%2\"/>")
			       .arg(viewW, 0, 'f', 2).arg(railH, 0, 'f', 2);
			out << QStringLiteral("    <rect x=\"0\" y=\"%1\" width=\"%2\" height=\"%3\"/>")
			       .arg(viewH - railH, 0, 'f', 2).arg(viewW, 0, 'f', 2).arg(railH, 0, 'f', 2);
			break;
		}
		case PanelizerEngine::FrameStyle::Rectangle:
		case PanelizerEngine::FrameStyle::TightFrame: {
			if (rxPx > 0.0) {
				out << QStringLiteral("    <rect x=\"0\" y=\"0\" width=\"%1\" height=\"%2\" rx=\"%3\" ry=\"%3\"/>")
				       .arg(viewW, 0, 'f', 2).arg(viewH, 0, 'f', 2).arg(rxPx, 0, 'f', 2);
			} else {
				out << QStringLiteral("    <rect x=\"0\" y=\"0\" width=\"%1\" height=\"%2\"/>")
				       .arg(viewW, 0, 'f', 2).arg(viewH, 0, 'f', 2);
			}
			break;
		}
	}

	out << QStringLiteral("  </g>");
	out << QStringLiteral("</svg>");
	return out.join(QChar('\n'));
}

} // namespace PanelizerSeparators
