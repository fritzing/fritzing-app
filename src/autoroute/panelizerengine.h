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

#ifndef PANELIZERENGINE_H
#define PANELIZERENGINE_H

#include <QList>
#include <QPointF>
#include <QSizeF>
#include <QString>

// Final destination: src/autoroute/panelizerengine.h
// Staged in: src/panelizer/panelizerengine.h

// NOTE: PanelSpec uses inches throughout to match the legacy
// Panelizer class and GerberGenerator. TextUtils::convertToInches()
// is used at the mm↔inch boundary. The legacy code mixes units
// silently — this has burned contributors (see AGENTS.md §10).

class FApplication;
namespace PanelizerEngine {

// ======================================================================
// Data model structs — these replace the XML-driven structs in the
// legacy Panelizer class. The existing panelizer.cpp will be refactored
// (Step 1 PR) to build these from XML and call into this namespace.
// ======================================================================

/**
 * @brief A single source board to be placed on the panel.
 *
 * Represents one .fzz file (or one instance of the current sketch)
 * that will be tiled onto the panel. Coordinates are in inches
 * throughout this struct to match the legacy Panelizer + GerberGenerator.
 */
struct SourceBoard {
    QString fzzPath;            // absolute path to .fzz file (empty for current sketch)
    class MainWindow *openWindow; // non-null for the current sketch (step-and-repeat mode)
    int copies = 1;             // number of copies (1+)
    bool allowRotate90 = true;  // allow 90° rotation during layout
    QSizeF boardSizeInches;     // board size in inches (0,0 if not known)
};

/**
 * @brief Panel dimensions and border configuration.
 *
 * All dimensions are in **inches** to match the legacy Panelizer class
 * and GerberGenerator. TextUtils::convertToInches() is used at the
 * mm↔inch boundary. The legacy code mixes units silently — this has
 * burned contributors (see AGENTS.md §10).
 *
 * The border (rail) is the solid frame around the panel edge that the
 * fab uses for mounting. Rails extend beyond the border on all sides.
 */
/**
 * @brief Panel frame / outline rendering style.
 *
 * Controls how the panel's outer Edge_Cuts outline is drawn:
 *  - Hidden:      no outer outline (boards-only); rare, mostly used
 *                 when the fab supplies their own router file.
 *  - Rails:       only top + bottom rails are drawn as rectangles.
 *                 Useful for V-scored panels where vertical edges are
 *                 implied by the saw kerf.
 *  - Rectangle:   single full-perimeter rectangle around the panel
 *                 (the default, JLC-compatible).
 *  - TightFrame:  perimeter only, with no gutter beyond the rails;
 *                 reserved for future fab-specific tweaks.
 */
enum class FrameStyle { Hidden, Rails, Rectangle, TightFrame };

struct PanelSpec {
    QSizeF panelSizeInches;     // outer panel dimensions in inches
    double gutterInches = 0.08; // board-to-board spacing in inches
    double borderInches = 0.2;  // panel rail width in inches
    bool addRails = true;       // add top+bottom rails (recommended for V-cut)
    FrameStyle frameStyle = FrameStyle::Rectangle;
    double frameFilletInches = 0.0; // 0 = sharp corners; >0 = rounded corner radius
};

/**
 * @brief Configuration for board separation geometry.
 *
 * Separation defines how individual boards are cut apart after
 * panelization. Only one method is active at a time.
 */
enum class Separation { None, VCut, MouseBites };

struct SeparationSpec {
    Separation kind = Separation::VCut;
    // V-cut:
    double vcutLineWidthMils = 10.0;
    QString vcutLayer = "Edge_Cuts"; // emit on outline gerber
    // Mouse-bites:
    double tabWidthInches = 0.118;  // 3 mm
    int holesPerTab = 5;
    double holeDiameterMils = 20.0;
    double holePitchMils = 31.5;
};

/**
 * @brief Configuration for optional fiducials and tooling holes.
 *
 * Fiducials are placed on the rails (not the gutter) at three corners:
 * top-left, top-right, and bottom-right (the de-facto standard).
 * Tooling holes are placed at all four rail corners with a 5 mm offset.
 */
struct ExtrasSpec {
    bool addFiducials = true;
    double fiducialDiameterMils = 40.0;
    double fiducialClearMils = 80.0;
    bool addToolingHoles = false;
    double toolingHoleDiameterInches = 0.125;
};

/**
 * @brief A placed board instance on the panel layout.
 *
 * Produced by layout() and consumed by emitPanel(). Coordinates are in
 * inches in the panel's local coordinate system (origin at panel top-left).
 * The PlacedBoard* objects are owned by the caller — the caller must
 * qDeleteAll() them when done.
 */
struct PlacedBoard {
    QPointF positionInches;   // top-left corner in panel coords (inches)
    bool rotated90 = false;   // whether this instance is rotated 90° (kept for
                              // back-compat; mirrors rotationDegrees==90/270)
    // Full per-board orientation, set by the interactive arrange editor.
    // rotated90 above stays in sync for the legacy 0/90-only render path.
    int rotationDegrees = 0;  // 0/90/180/270, clockwise
    bool flippedHorizontal = false; // mirror left-to-right about the board centre
    QString boardId;          // unique ID for the placed board instance
    SourceBoard source;       // which board this is (copied)
};

/**
 * @brief Result of PanelizerEngine::emitPanel().
 *
 * Contains the paths to the synthetic .fzz panel sketch and the
 * Gerber output directory, plus any warnings generated during emission.
 */
struct Result {
    QString panelFzzPath;     // synthetic panel sketch path
    QString gerberDir;        // where the gerbers landed
    QStringList gerberFiles;  // absolute paths of every emitted gerber/drill file
    QStringList warnings;
    bool success = false;
};

// ======================================================================
// Public API — the GUI wizard and CLI both call these functions.
// The existing Panelizer::panelize() will be refactored to build
// SourceBoard/PanelSpec/SeparationSpec from XML and call these.
// ======================================================================

/**
 * @brief Lays out source boards into a panel using corner-stitching bin packing.
 *
 * Wraps the legacy Panelizer::bestFit pipeline with a parameter struct
 * driven by the GUI wizard rather than panelizer.xml. Pure layout: no
 * file IO, no Gerber emission. Caller owns the PlacedBoard* objects in
 * @p outItems and must qDeleteAll() them.
 *
 * This is the core layout algorithm. It takes a list of source boards
 * and packs them onto the panel using the same corner-stitching tile
 * data structure as the autorouter (see Ousterhout 1984). The layout
 * is deterministic: the same inputs always produce the same output.
 *
 * @param sources    Boards to place. Each may request multiple copies
 *                   via SourceBoard::copies. Order matters only for
 *                   tie-breaking — the packer is greedy by area.
 * @param spec       Panel dimensions, gutter, and border. Must satisfy
 *                   spec.panelSizeInches > (largestBoard + 2*border).
 * @param outItems   [out] Filled with placed PlacedBoard instances.
 *                   Cleared on entry. Empty on failure.
 * @param errorOut   [out, optional] Human-readable failure reason.
 * @return true on success, false if any required board does not fit.
 *
 * @note Must be called on the main (GUI) thread because it instantiates
 *       MainWindow objects to read board geometry from the .fzz files.
 *       Use a QFutureWatcher with QtConcurrent::run + signal hop if you
 *       need it off the UI thread for live preview.
 *
 * @see PanelizerEngine::emitPanel  for the IO half of the pipeline.
 * @see Panelizer::bestFitOne  src/autoroute/panelizer.cpp:495 — the
 *      underlying packer this delegates to.
 */
bool layout(const QList<SourceBoard>& sources,
            const PanelSpec&        spec,
            QList<PlacedBoard*>&    outItems,
            QString*                errorOut);

/**
 * @brief Generates the SVG content for V-cut or mouse-bite separation lines.
 *
 * The SVG is layered: V-cut lines go into a panel_vcut layer, mouse-bite
 * holes go into a drill layer, and slots go into the edge_cuts layer.
 * V-cut lines extend border + epsilon past the panel outline so the fab's
 * V-scoring saw runs off the edge (required by most fabs).
 *
 * @param items The placed panel items from layout().
 * @param spec Panel dimensions.
 * @param sep Separation configuration.
 * @return SVG string with the separation geometry.
 */
QString separationSvg(const QList<PlacedBoard*>& items,
                      const PanelSpec& spec,
                      const SeparationSpec& sep);

/**
 * @brief Emits Gerber files for the panel from placed items.
 *
 * Calls the existing GerberGenerator for each placed board and appends
 * synthetic elements (V-cut lines, mouse-bite holes, fiducials, tooling
 * holes) to the appropriate Gerber layers. The output is a synthetic
 * .fzz panel sketch plus a Gerber set.
 *
 * @param laidOut The placed panel items from layout(). Caller retains
 *                ownership — emitPanel() does not qDeleteAll() them.
 * @param spec Panel dimensions.
 * @param sep Separation configuration.
 * @param extras Fiducial and tooling hole configuration.
 * @param outputDir Directory to write Gerber files to.
 * @param app FApplication pointer for ProcessEventBlocker yields and
 *            MainWindow access during synthetic .fzz creation.
 * @return Result struct with panelFzzPath, gerberDir, warnings, and
 *         success flag.
 *
 * @note This must run on the main thread (QGraphicsScene is not
 *       thread-safe). Use ProcessEventBlocker for long phases.
 */
Result emitPanel(const QList<PlacedBoard*>& laidOut,
            const PanelSpec&         spec,
            const SeparationSpec&    sep,
            const ExtrasSpec&        extras,
            const QString&           outputDir,
            FApplication*      app);

} // namespace PanelizerEngine

#endif // PANELIZERENGINE_H
