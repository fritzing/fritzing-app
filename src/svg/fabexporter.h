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

/*
 * FabExporter - Production-ready Gerber/CSV bundle writer.
 *
 * Attribution: The JLCPCB layer naming convention used here matches the
 * Fabrication-Toolkit project (MIT, https://github.com/bennymeg/Fabrication-Toolkit).
 * No code was copied; the column ordering and filename suffix mapping are
 * facts of the JLC ingest spec and are reproduced under fair use.
 * The Gerber X3 job-file metadata schema follows IPC-2581 / Ucamco's
 * publicly-published JSON specification.
 */

#ifndef FABEXPORTER_H
#define FABEXPORTER_H

#include <QString>
#include <QStringList>

class ItemBase;
class PCBSketchWidget;

namespace PanelizerEngine {
	struct PanelSpec;
	struct SeparationSpec;
	struct ExtrasSpec;
	struct PlacedBoard;
}

/**
 * @brief Production-bundle writer for fabrication houses.
 *
 * FabExporter takes the artifacts produced by the panelizer engine
 * (separation SVG, extras SVG, frame outline SVG, and - eventually -
 * per-board copper / silk / mask SVGs) and emits a directory of
 * Gerber RS-274X + Excellon + CSV files using the filename
 * convention expected by the selected fab.
 *
 * Profiles:
 *   - JLCPCB:  hyphenated suffixes (-F_Cu, -F_Silkscreen, .drl, etc.)
 *   - PCBWay:  KiCad-compatible naming subset
 *   - OshPark: KiCad naming (their preferred input)
 *   - Generic: Fritzing legacy suffixes
 *
 * @note FabExporter does not yet render per-board copper / silk / mask
 *       layers - that requires loading each source .fzz into a hidden
 *       PCBSketchWidget and is the next milestone (PR #G6). The current
 *       implementation produces a complete frame + drill + CSV bundle
 *       that loads cleanly in any Gerber viewer.
 */
class FabExporter
{
public:
	enum Profile {
		JLCPCB,
		PCBWay,
		OshPark,
		Generic
	};

	struct Spec {
		Profile profile = JLCPCB;
		QString projectName;      // becomes file prefix (e.g. "MyPanel")
		QString outputDir;        // root; we create <outputDir>/production/ under it
	};

	struct Result {
		bool success = false;
		QString productionDir;    // <outputDir>/production
		QStringList written;      // absolute paths of every emitted file
		QStringList warnings;
	};

	FabExporter() = default;

	/**
	 * @brief Emit a complete production bundle for a panelized layout.
	 *
	 * Synthesizes panel-level Gerbers from the SVG fragments the engine
	 * has already generated (V-cut / mouse-bite / frame / fiducials /
	 * tooling holes) plus empty placeholders for copper / silk / mask
	 * layers, writes a Gerber X3 job file, and (when @p sketchWidget is
	 * non-null) walks the colliding ItemBases to emit BOM + CPL CSVs.
	 *
	 * @param laidOut       The panel's placed boards (for BOM/CPL fallback paths).
	 * @param panelSpec     Panel dimensions / frame style.
	 * @param sep           Separation config (V-cut vs mouse-bites).
	 * @param extras        Fiducials / tooling holes config.
	 * @param sketchBoard   Optional: the panel sketch's board ItemBase. May be null.
	 * @param sketchWidget  Optional: the panel sketch's PCBSketchWidget. May be null.
	 * @param spec          Output naming + destination.
	 * @return Result with success flag, written paths, and any warnings.
	 */
	Result exportFromPanel(const QList<PanelizerEngine::PlacedBoard*> & laidOut,
	                       const PanelizerEngine::PanelSpec & panelSpec,
	                       const PanelizerEngine::SeparationSpec & sep,
	                       const PanelizerEngine::ExtrasSpec & extras,
	                       ItemBase * sketchBoard,
	                       PCBSketchWidget * sketchWidget,
	                       const Spec & spec);

protected:
	// Per-layer filename suffix lookup (profile-aware).
	QString suffixFor(Profile p, const QString & layerKey) const;

	// Convert a single SVG document to a Gerber file in @p outPath.
	// Returns true on success; warning lines pushed into @p warnings.
	bool convertSvgToGerber(const QString & svg,
	                        const QString & layerName,
	                        int forWhy,
	                        const QString & outPath,
	                        QStringList & warnings) const;

	// Composite two or more SVGs that share the same coordinate system
	// into one Edge_Cuts SVG (frame outline + V-cut + mouse-bite slots).
	QString compositeEdgeCuts(const QStringList & svgFragments) const;

	// Composite two or more SVGs into one Drill SVG (mouse-bite holes +
	// tooling holes + fiducial NPTH if any).
	QString compositeDrill(const QStringList & svgFragments) const;

	// Write an empty/placeholder Gerber so the bundle has the full
	// layer set the fab expects (avoids "missing layer" errors on
	// upload). Headers only; no apertures, no draws.
	bool writePlaceholderGerber(const QString & outPath,
	                            const QString & layerName,
	                            QStringList & warnings) const;

	// Write a minimal Gerber X3 .gbrjob file describing the bundle.
	bool writeGbrJob(const Result & r,
	                 const PanelizerEngine::PanelSpec & panelSpec,
	                 const Spec & spec) const;

	// CSV helpers - walk sketchWidget for parts on @p sketchBoard.
	// @p laidOut is the panel layout: BOM quantities scale by the
	// number of placements that share @p sketchBoard's source path,
	// and CPL emits one row per placement with translated coordinates
	// (and rotation for rotated-90 placements).
	bool writeBom(ItemBase * sketchBoard,
	              PCBSketchWidget * sketchWidget,
	              const QList<PanelizerEngine::PlacedBoard*> & laidOut,
	              const Spec & spec,
	              QStringList & warnings,
	              QString & outBomPath) const;

	bool writeCpl(ItemBase * sketchBoard,
	              PCBSketchWidget * sketchWidget,
	              const QList<PanelizerEngine::PlacedBoard*> & laidOut,
	              const Spec & spec,
	              QStringList & warnings,
	              QString & outCplPath) const;

	// Open each unique source .fzz, render per-board per-layer SVGs via
	// Panelizer::makeSVGs(), composite them into panel-level SVGs with
	// the placed offsets, and emit one Gerber per layer via
	// GerberGenerator::doEnd(). Replaces the placeholder layer writer
	// for everything except the synthesized panel frame/separator/extras
	// geometry (which is concatenated onto Edge_Cuts and Drill).
	//
	// @param laidOut          Layout result; each PlacedBoard.source.fzzPath
	//                         identifies a sketch to load.
	// @param panelSpec        Panel size (inches).
	// @param extraEdgeCutsSvg Extra Edge_Cuts SVG fragment (frame + V-cut /
	//                         mouse-bite slots) to merge into the composite.
	//                         Pass empty string to skip.
	// @param extraDrillSvg    Extra drill SVG fragment (mouse-bite holes +
	//                         tooling holes) to merge into the composite.
	// @param gerberDir        Output directory for the .gbr/.drl files.
	// @param projectName      Filename prefix.
	// @param written          Appended with every successfully-written file.
	// @param warnings         Appended with any non-fatal issues.
	// @return true if at least one layer was emitted, false on hard failure.
	bool renderRealPanelLayers(const QList<PanelizerEngine::PlacedBoard*> & laidOut,
	                           const PanelizerEngine::PanelSpec & panelSpec,
	                           const QString & extraEdgeCutsSvg,
	                           const QString & extraDrillSvg,
	                           const QString & gerberDir,
	                           const QString & projectName,
	                           QStringList & written,
	                           QStringList & warnings) const;
};

#endif // FABEXPORTER_H
