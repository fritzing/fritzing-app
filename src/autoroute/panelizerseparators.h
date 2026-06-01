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

#ifndef PANELIZERSEPARATORS_H
#define PANELIZERSEPARATORS_H

#include "panelizerengine.h"

namespace PanelizerSeparators {

/**
 * @brief Generates SVG content for V-cut (straight scoring) separation lines.
 *
 * V-cut lines are straight lines that score the panel edge-to-edge
 * along each interior gutter. Each line extends border + epsilon
 * past the panel outline on both ends — this is required by most
 * fabs so the V-scoring saw can run off the edge without stopping
 * mid-board.
 *
 * The SVG output contains, wrapped in a `<g id="edge_cuts">` group
 * (the Fritzing SVG normalizer keys on group IDs, not custom attributes):
 * - A `<rect>` for the panel outer outline.
 * - One `<line>` per interior gutter centerline (vertical + horizontal).
 *
 * The edge_cuts layer feeds GerberGenerator's outline pass (.gko).
 * Most fabs accept multiple polylines on .gko and infer "scoring" from
 * straight lines that cross the whole panel.
 *
 * @param items The placed panel items from layout().
 * @param spec Panel dimensions and border configuration.
 * @param sep V-cut configuration (line width, edge layer name).
 * @return SVG string with V-cut geometry.
 */
QString vcutSvg(const QList<PanelizerEngine::PlacedBoard*>& items,
                const PanelizerEngine::PanelSpec& spec,
                const PanelizerEngine::SeparationSpec& sep);

/**
 * @brief Generates SVG content for mouse-bite (perforated tab) separation.
 *
 * Mouse-bites are perforated tabs between adjacent boards that allow
 * the panel to be snapped apart by hand. Each tab cluster consists of:
 *
 * 1. holesPerTab non-plated through-holes centered on the gutter midline.
 * 2. Two parallel arcs/lines on the outline layer that do NOT cross
 *    through the holes — this leaves the tab intact while the holes
 *    weaken it for snap-off.
 *
 * The holes go on the .txt (drill) Gerber file, and the slots go on
 * the .gko (board outline) Gerber file.
 *
 * @param items The placed panel items from layout().
 * @param spec Panel dimensions and border configuration.
 * @param sep Mouse-bite configuration (tab width, hole count, diameter, pitch).
 * @return SVG string with mouse-bite geometry.
 */
QString mouseBiteSvg(const QList<PanelizerEngine::PlacedBoard*>& items,
                     const PanelizerEngine::PanelSpec& spec,
                     const PanelizerEngine::SeparationSpec& sep);

/**
 * @brief Generates SVG content for fiducial markers.
 *
 * Fiducials are placed on the rails (not the gutter) at three corners:
 * top-left, top-right, and bottom-right (the de-facto standard).
 * Each fiducial consists of:
 * - A copper pad circle on the top copper layer.
 * - A soldermask opening (slightly larger circle) on the top soldermask.
 *
 * @param spec Panel dimensions and border configuration.
 * @param fiducialDiameter Diameter of the copper pad in mm.
 * @param fiducialClear Mask clearance around the pad in mm.
 * @return SVG string with fiducial geometry.
 */
QString fiducialSvg(const PanelizerEngine::PanelSpec& spec,
                    double fiducialDiameter,
                    double fiducialClear);

/**
 * @brief Generates SVG content for tooling holes.
 *
 * Tooling holes are placed at all four rail corners with a 5 mm
 * offset from the panel edge. They are non-plated through-holes
 * (NPTH) used by the fab for board placement during SMT.
 *
 * @param spec Panel dimensions and border configuration.
 * @param toolingHoleDiameter Diameter of the tooling hole in mm.
 * @return SVG string with tooling hole geometry.
 */
QString toolingHoleSvg(const PanelizerEngine::PanelSpec& spec,
                       double toolingHoleDiameter);

/**
 * @brief Generates the panel outer outline SVG (Edge_Cuts).
 *
 * Honors PanelSpec::frameStyle:
 *   Hidden     -> empty SVG (no outline; rare)
 *   Rails      -> two rectangles (top + bottom rails only)
 *   Rectangle  -> single full-perimeter rectangle (most common)
 *   TightFrame -> single rectangle inset by gutter (future use)
 * Honors PanelSpec::frameFilletInches for corner rounding.
 *
 * @param spec Panel dimensions, border, and frame style.
 * @return SVG document fragment with the outline geometry on the
 *         edge_cuts group, or empty string for FrameStyle::Hidden.
 */
QString frameOutlineSvg(const PanelizerEngine::PanelSpec& spec);

} // namespace PanelizerSeparators

#endif // PANELIZERSEPARATORS_H
