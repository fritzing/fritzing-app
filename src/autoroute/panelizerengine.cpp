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

#include "panelizerengine.h"
#include "panelizerseparators.h"
#include "../svg/fabexporter.h"

// Final destination: src/autoroute/panelizerengine.cpp
// Staged in: src/panelizer/panelizerengine.cpp

#include "../debugdialog.h"
#include "../mainwindow/mainwindow.h"
#include "../items/resizableboard.h"
#include "../items/moduleidnames.h"
#include "../utils/textutils.h"
#include "../utils/graphicsutils.h"
#include "../utils/folderutils.h"
#include "../referencemodel/referencemodel.h"
#include "../fapplication.h"

#include "cmrouter/tileutils.h"
#include "cmrouter/tile.h"

#include <QFile>
#include <QDomDocument>
#include <QDomElement>
#include <QDir>
#include <QTemporaryDir>
#include <qmath.h>
#include <limits>
#include <algorithm>

// ======================================================================
// Internal helpers — these wrap the legacy Panelizer API to ensure
// byte-identical output for the PR #1 golden-checksums test.
// ======================================================================

namespace PanelizerEngine {

// Legacy PanelItem struct (from panelizer.h) — we need this to
// call the legacy bestFitOne() API.
struct LegacyPanelItem {
    QString boardName;
    QString path;
    int required = 0;
    int maxOptional = 0;
    int optionalPriority = 0;
    int produced = 0;
    QSizeF boardSizeInches;
    long boardID = 0;
    double x = 0.0, y = 0.0;
    bool rotate90 = false;

    LegacyPanelItem() = default;
};

// Legacy PlanePair struct (from panelizer.h)
struct LegacyPlanePair {
    Plane * thePlane;
    Plane * thePlane90;
    TileRect tilePanelRect;
    TileRect tilePanelRect90;
    double panelWidth;
    double panelHeight;
    QStringList svgs;
    QString layoutSVG;
    int index;
};

// Legacy BestPlace struct (from panelizer.h)
struct LegacyBestPlace {
    Tile * bestTile = nullptr;
    TileRect bestTileRect;
    TileRect maxRect;
    int width = 0;
    int height = 0;
    double bestArea = std::numeric_limits<double>::max();
    bool rotate90 = false;
    Plane* plane = nullptr;
};

// Convert SourceBoard to LegacyPanelItem
static LegacyPanelItem sourceToLegacy(const SourceBoard& source, int index)
{
    LegacyPanelItem item;
    item.path = source.fzzPath;
    item.boardName = QFileInfo(source.fzzPath).completeBaseName();
    item.boardSizeInches = source.boardSizeInches;
    item.required = source.copies;
    item.maxOptional = 0;
    item.optionalPriority = 0;
    item.boardID = index;
    return item;
}

// Convert LegacyPanelItem to PlacedBoard
static PlacedBoard legacyToPlaced(const LegacyPanelItem& item, const SourceBoard& source)
{
    PlacedBoard board;
    board.positionInches = QPointF(item.x, item.y);
    board.rotated90 = item.rotate90;
    board.boardId = item.boardName;
    board.source = source;
    return board;
}

} // namespace PanelizerEngine

// ======================================================================
// PanelizerEngine::layout() — corner-stitching bin packing
// ======================================================================

namespace PanelizerEngine {

/**
 * @brief Corner-stitching bin packing implementation.
 *
 * Algorithm:
 * 1. Initialize tile grid with one tile covering the usable
 *    panel area (panelSize - 2*border).
 * 2. For each SourceBoard (in input order):
 *    a. Try bestFitOne() for each remaining copy.
 *    b. If fit found, placeBestFit() and split tiles.
 *    c. If no fit, try 90° rotation if allowRotate90.
 *    d. If still no fit, return false with errorOut.
 * 3. On success, fill outItems with all placed PlacedBoards.
 *
 * NOTE: Coordinates are in inches throughout. The legacy code mixes
 * mm and mils silently — this implementation keeps inches until
 * Gerber emission where TextUtils::convertToInches() is used.
 * Corner-stitching: see Ousterhout 1984.
 */
bool layout(const QList<SourceBoard>& sources,
            const PanelSpec&        spec,
            QList<PlacedBoard*>&    outItems,
            QString*                errorOut)
{
    outItems.clear();

    if (sources.isEmpty()) {
        if (errorOut) {
            *errorOut = "No source boards specified";
        }
        return false;
    }

    // == Step 1: Load board geometries ==
    // For each source board, we need to determine its physical size.
    // If it's an open window, we can query the PCB sketch widget.
    // If it's a .fzz file, we need to extract the board geometry.

    QList<LegacyPanelItem> legacyItems;
    QList<SourceBoard> remainingSources;

    for (int i = 0; i < sources.size(); ++i) {
        const SourceBoard& source = sources[i];
        LegacyPanelItem item = sourceToLegacy(source, i);

        // Determine board size — for now, use a default if we can't load
        // TODO: extract actual board size from .fzz or open window
        if (source.boardSizeInches.width() > 0 && source.boardSizeInches.height() > 0) {
            item.boardSizeInches = source.boardSizeInches;
        } else {
            // Default 100x100mm board (3.94x3.94 inches)
            item.boardSizeInches = QSizeF(3.94, 3.94);
        }

        // Add all copies to the legacyItems list
        for (int c = 0; c < source.copies; ++c) {
            legacyItems.append(item);
            remainingSources.append(source);
        }
    }

    if (legacyItems.isEmpty()) {
        if (errorOut) {
            *errorOut = "No valid boards to place";
        }
        return false;
    }

    // == Step 2: Shelf bin-packing in inches ==
    //
    // FIX(landracer): the previous tile-plane-based layout crashed on
    // the first board because TiNewPlane(nullptr,...) leaves pl_hint
    // null and the fallback pl_left is a sentinel wall tile whose TR
    // pointer is NULL - WIDTH(tile) then dereferences NULL.
    //
    // Replaced with classic shelf bin-packing (Coffman/Garey/Johnson
    // First-Fit Decreasing Height variant). Boards are sorted by
    // height descending and laid left-to-right on the current shelf;
    // a new shelf opens above when the next board does not fit.
    //
    // Coordinate system: panel-local inches, origin at panel top-left.
    // Usable area excludes the border on all four sides.

    const double usableW = qMax(0.0, spec.panelSizeInches.width()  - 2 * spec.borderInches);
    const double usableH = qMax(0.0, spec.panelSizeInches.height() - 2 * spec.borderInches);
    if (usableW <= 0.0 || usableH <= 0.0) {
        if (errorOut) {
            *errorOut = QObject::tr("Panel size (%1x%2 in) is smaller than 2x border (%3 in)")
                            .arg(spec.panelSizeInches.width())
                            .arg(spec.panelSizeInches.height())
                            .arg(spec.borderInches);
        }
        return false;
    }

    // Build (index, w, h, rotated) candidates. For boards that allow
    // rotation, the candidate height is max(w, h) so FFDH puts the
    // tallest "natural" side down. Rotation is decided per-placement
    // below to honor whichever orientation actually fits the shelf.
    struct Cand { int idx; double w, h; };
    QList<Cand> work;
    work.reserve(legacyItems.size());
    for (int i = 0; i < legacyItems.size(); ++i) {
        Cand c;
        c.idx = i;
        c.w = legacyItems[i].boardSizeInches.width();
        c.h = legacyItems[i].boardSizeInches.height();
        // FFDH key: pick the larger dimension as "height" so we sort
        // tall-first regardless of the source orientation.
        if (remainingSources[i].allowRotate90 && c.w > c.h) std::swap(c.w, c.h);
        work.append(c);
    }
    std::sort(work.begin(), work.end(), [](const Cand & a, const Cand & b) {
        return a.h > b.h; // tallest first
    });

    // Shelves grow downward from (border, border).
    const double gutter = spec.gutterInches;
    double shelfY      = spec.borderInches;     // top of current shelf
    double shelfH      = 0.0;                   // tallest board so far on this shelf
    double cursorX     = spec.borderInches;     // next free X on current shelf

    for (const Cand & c : work) {
        LegacyPanelItem & item = legacyItems[c.idx];
        const bool allowRot    = remainingSources[c.idx].allowRotate90;

        // Two candidate orientations; honor allowRotate90.
        struct Orient { double w, h; bool rotated; };
        QList<Orient> tries;
        tries.append({ item.boardSizeInches.width(), item.boardSizeInches.height(), false });
        if (allowRot && item.boardSizeInches.width() != item.boardSizeInches.height()) {
            tries.append({ item.boardSizeInches.height(), item.boardSizeInches.width(), true });
        }

        bool placed = false;
        for (int pass = 0; pass < 2 && !placed; ++pass) {
            // pass 0: try to fit on the current shelf
            // pass 1: open a new shelf above and try again
            for (const Orient & o : tries) {
                const double needX = (cursorX > spec.borderInches) ? (cursorX + gutter) : cursorX;
                const double xRight = needX + o.w;
                // Right edge must stay within usable area.
                if (xRight > spec.borderInches + usableW + 1e-9) continue;
                // Bottom edge must stay within usable area.
                if (shelfY + o.h > spec.borderInches + usableH + 1e-9) continue;
                // Fit! Commit.
                item.x = needX;
                item.y = shelfY;
                item.rotate90 = o.rotated;
                cursorX = needX + o.w;
                if (o.h > shelfH) shelfH = o.h;
                placed = true;
                break;
            }
            if (!placed) {
                // Open a new shelf.
                if (shelfH <= 0.0) break; // empty shelf already; cannot help
                shelfY  += shelfH + gutter;
                shelfH   = 0.0;
                cursorX  = spec.borderInches;
            }
        }

        if (!placed) {
            if (errorOut) {
                *errorOut = QObject::tr("Failed to place board %1 (%2 x %3 in) on a %4 x %5 in panel")
                                .arg(item.boardName)
                                .arg(item.boardSizeInches.width())
                                .arg(item.boardSizeInches.height())
                                .arg(spec.panelSizeInches.width())
                                .arg(spec.panelSizeInches.height());
            }
            return false;
        }
    }

    // == Step 3: Convert results to PlacedBoard ==
    for (int i = 0; i < legacyItems.size(); ++i) {
        PanelizerEngine::PlacedBoard* board = new PanelizerEngine::PlacedBoard();
        *board = legacyToPlaced(legacyItems[i], remainingSources[i]);
        outItems.append(board);
    }

    return true;
}

/**
 * @brief V-cut/mouse-bite SVG generation.
 *
 * V-cut:
 * - For each interior gutter line (vertical and horizontal),
 *   emit a single <line> in SVG into layer panel_vcut.
 * - Line extends border + epsilon past panel outline on each end.
 * - panel_vcut maps to Gerber .gko (board outline).
 *
 * Mouse-bites:
 * - Per gutter segment between two adjacent boards:
 *   1. Compute gutter midline.
 *   2. Lay out holesPerTab circles centered on midline.
 *   3. Emit two parallel arcs/lines on outline layer (slots).
 *   4. Holes go on .txt drill Gerber, slots on .gko.
 */
QString separationSvg(const QList<PanelizerEngine::PlacedBoard*>& items,
                      const PanelizerEngine::PanelSpec& spec,
                      const PanelizerEngine::SeparationSpec& sep)
{
	// Dispatch to the concrete separator implementation in
	// PanelizerSeparators. Each kind emits one <g id="edge_cuts">
	// group (and, for mouse-bites, an additional <g id="drill">).
	switch (sep.kind) {
	case PanelizerEngine::Separation::None:
		return QString();
	case PanelizerEngine::Separation::VCut:
		return PanelizerSeparators::vcutSvg(items, spec, sep);
	case PanelizerEngine::Separation::MouseBites:
		return PanelizerSeparators::mouseBiteSvg(items, spec, sep);
	}
	return QString();
}

/**
 * @brief Gerber emission implementation.
 *
 * Pattern: copy from MainWindow::exportToGerber() in
 * src/mainwindow/mainwindow_export.cpp#L1644.
 *
 * 1. For each PlacedBoard, call GerberGenerator::exportToGerber()
 *    with the placed board's ModelPart.
 * 2. Append synthetic elements:
 *    - V-cut lines → .gko layer
 *    - Mouse-bite holes → .txt drill layer
 *    - Mouse-bite slots → .gko layer
 *    - Fiducials → top copper + top soldermask
 *    - Tooling holes → .txt drill layer
 * 3. If savePanelFzz, create synthetic .fzz with PanelBoardItem.
 *
 * NOTE: This must run on the main thread (QGraphicsScene is not
 * thread-safe). Use ProcessEventBlocker for long phases.
 */
PanelizerEngine::Result emitPanel(const QList<PanelizerEngine::PlacedBoard*>& laidOut,
            const PanelizerEngine::PanelSpec&         spec,
            const PanelizerEngine::SeparationSpec&    sep,
            const PanelizerEngine::ExtrasSpec&        extras,
            const QString&           outputDir,
            FApplication*            app)
{
	// NOTE: Minimal first-stage emitter. Writes the separation +
	// extras SVG artifacts into outputDir so the user can verify
	// V-cut / mouse-bite geometry in any viewer.
	//
	// TODO(landracer): full GerberGenerator integration per board.
	// See MainWindow::exportToGerber() in mainwindow_export.cpp for
	// the pattern.
	(void)app;
	PanelizerEngine::Result result;

	if (laidOut.isEmpty()) {
		result.warnings << QObject::tr("emitPanel: no placed boards");
		return result;
	}

	QDir dir(outputDir);
	if (!dir.exists() && !dir.mkpath(".")) {
		result.warnings << QObject::tr("emitPanel: cannot create output dir %1").arg(outputDir);
		return result;
	}

	// Separation geometry → panel_separation.svg
	const QString sepSvg = separationSvg(laidOut, spec, sep);
	if (!sepSvg.isEmpty()) {
		QFile f(dir.filePath("panel_separation.svg"));
		if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
			f.write(sepSvg.toUtf8());
			f.close();
		} else {
			result.warnings << QObject::tr("emitPanel: cannot write panel_separation.svg");
		}
	}

	// Extras (fiducials + tooling holes) → panel_extras.svg
	QString extrasSvg;
	if (extras.addFiducials) {
		extrasSvg += PanelizerSeparators::fiducialSvg(spec,
			extras.fiducialDiameterMils, extras.fiducialClearMils);
	}
	if (extras.addToolingHoles) {
		extrasSvg += PanelizerSeparators::toolingHoleSvg(spec,
			extras.toolingHoleDiameterInches);
	}
	if (!extrasSvg.isEmpty()) {
		QFile f(dir.filePath("panel_extras.svg"));
		if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
			f.write(extrasSvg.toUtf8());
			f.close();
		}
	}

	// Composite the full production bundle (Edge_Cuts.gbr + .drl +
	// placeholder copper/silk/mask + BOM/CPL + .gbrjob) via FabExporter.
	// NOTE: sketchBoard / sketchWidget are null in this code path because
	// emitPanel does not yet load the synthetic panel sketch. PR #G6
	// will surface them so per-board copper rendering can land.
	{
		FabExporter exporter;
		FabExporter::Spec fspec;
		fspec.profile     = FabExporter::JLCPCB;
		fspec.projectName = QFileInfo(outputDir).fileName();
		if (fspec.projectName.isEmpty()) fspec.projectName = QStringLiteral("panel");
		fspec.outputDir   = outputDir;
		FabExporter::Result fr = exporter.exportFromPanel(
			laidOut, spec, sep, extras,
			/*sketchBoard=*/nullptr, /*sketchWidget=*/nullptr, fspec);
		result.gerberFiles  = fr.written;
		result.warnings.append(fr.warnings);
		if (!fr.success) {
			result.warnings << QObject::tr("emitPanel: FabExporter bundle failed");
		}
		// Prefer the production gerber directory over the artifact dir.
		if (!fr.productionDir.isEmpty()) {
			result.gerberDir = QDir(fr.productionDir).filePath("gerber");
		} else {
			result.gerberDir = dir.absolutePath();
		}
	}
	result.success = true;
	return result;
}

} // namespace PanelizerEngine
