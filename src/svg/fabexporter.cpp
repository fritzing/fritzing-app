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

#include "fabexporter.h"

#include "svg2gerber.h"
#include "gerbergenerator.h"
#include "../autoroute/panelizer.h"
#include "../autoroute/panelizerengine.h"
#include "../autoroute/panelizerseparators.h"
#include "../debugdialog.h"
#include "../fapplication.h"
#include "../items/itembase.h"
#include "../mainwindow/mainwindow.h"
#include "../model/modelpart.h"
#include "../sketch/pcbsketchwidget.h"
#include "../utils/graphicsutils.h"
#include "../utils/textutils.h"
#include "../viewgeometry.h"
#include "../viewlayer.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGraphicsScene>
#include <QHash>
#include <QObject>
#include <QSet>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTransform>
#include <QtMath>

// ======================================================================
// FabExporter implementation.
//
// The bundle layout (under <outputDir>/production/):
//   gerber/
//     <project>-F_Cu.gbr           (top copper)         [placeholder]
//     <project>-B_Cu.gbr           (bottom copper)      [placeholder]
//     <project>-F_Mask.gbr         (top soldermask)     [placeholder]
//     <project>-B_Mask.gbr         (bottom soldermask)  [placeholder]
//     <project>-F_Silkscreen.gbr   (top silk)           [placeholder]
//     <project>-B_Silkscreen.gbr   (bottom silk)        [placeholder]
//     <project>-F_Paste.gbr        (top paste)          [placeholder]
//     <project>-B_Paste.gbr        (bottom paste)       [placeholder]
//     <project>-Edge_Cuts.gbr      (panel outline + V-cut/mouse-bite slots)
//     <project>.drl                (Excellon drill)
//     <project>-job.gbrjob         (Gerber X3 job file)
//   bom/<project>_bom.csv          (Comment / Designator / Footprint / LCSC #)
//   cpl/<project>_cpl.csv          (Designator / Val / Package / MidX / MidY / Rot / Layer)
//
// NOTE(landracer): the copper / silk / mask / paste layers are written
// as Gerber stubs (header + M02) for now. Real per-board rendering
// arrives in PR #G6; the bundle structure is already complete so the
// rest of the wizard chain (preview, upload, validation) can be wired.
// ======================================================================

namespace {

// Map a logical layer key to the JLC-flavored suffix.
struct LayerNaming {
	const char * key;
	const char * jlc;
	const char * generic;
};

static const LayerNaming kLayers[] = {
	{ "F_Cu",         "-F_Cu.gbr",         "_copperTop.gtl"      },
	{ "B_Cu",         "-B_Cu.gbr",         "_copperBottom.gbl"   },
	{ "F_Mask",       "-F_Mask.gbr",       "_maskTop.gts"        },
	{ "B_Mask",       "-B_Mask.gbr",       "_maskBottom.gbs"     },
	{ "F_Silkscreen", "-F_Silkscreen.gbr", "_silkTop.gto"        },
	{ "B_Silkscreen", "-B_Silkscreen.gbr", "_silkBottom.gbo"     },
	{ "F_Paste",      "-F_Paste.gbr",      "_pasteMaskTop.gtp"   },
	{ "B_Paste",      "-B_Paste.gbr",      "_pasteMaskBottom.gbp"},
	{ "Edge_Cuts",    "-Edge_Cuts.gbr",    "_contour.gm1"        },
	{ "Drill",        ".drl",              "_drill.txt"          }
};

constexpr int kNumLayers = sizeof(kLayers) / sizeof(kLayers[0]);

} // anonymous namespace

QString FabExporter::suffixFor(Profile p, const QString & layerKey) const
{
	for (int i = 0; i < kNumLayers; ++i) {
		if (layerKey == QLatin1String(kLayers[i].key)) {
			switch (p) {
				case JLCPCB:
				case PCBWay:
				case OshPark:
					return QString::fromLatin1(kLayers[i].jlc);
				case Generic:
					return QString::fromLatin1(kLayers[i].generic);
			}
		}
	}
	return QString();
}

bool FabExporter::convertSvgToGerber(const QString & svg,
                                     const QString & layerName,
                                     int forWhy,
                                     const QString & outPath,
                                     QStringList & warnings) const
{
	if (svg.isEmpty()) {
		return writePlaceholderGerber(outPath, layerName, warnings);
	}

	SVG2gerber converter;
	// Board size unknown to converter at this layer - safe to pass (0,0);
	// SVG2gerber re-reads viewBox from the svg string.
	const int rc = converter.convert(svg, false, layerName,
	                                 static_cast<SVG2gerber::ForWhy>(forWhy),
	                                 QSizeF(0, 0));
	if (rc < 0) {
		warnings << QObject::tr("FabExporter: SVG2gerber failed for %1 (rc=%2); writing placeholder")
		            .arg(layerName).arg(rc);
		return writePlaceholderGerber(outPath, layerName, warnings);
	}

	QFile f(outPath);
	if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
		warnings << QObject::tr("FabExporter: cannot open %1 for write").arg(outPath);
		return false;
	}
	f.write(converter.getGerber().toUtf8());
	f.close();
	return true;
}

QString FabExporter::compositeEdgeCuts(const QStringList & svgFragments) const
{
	// All fragments share the same viewBox (panel inches * DPI). We
	// concatenate the contents of their <g id="edge_cuts"> groups into
	// a single wrapping SVG. The trivial approach below relies on the
	// fragment format produced by panelizerseparators.cpp.

	// Find first non-empty fragment to extract viewBox / dimensions.
	// IMPORTANT: start the slice at the <svg open tag, not byte 0. Each
	// fragment carries its own <?xml ...?> prologue — including it here
	// would emit two prologues and SVG2gerber rejects that with
	// "invalid name for processing instruction".
	QString header;
	for (const QString & f : svgFragments) {
		if (f.isEmpty()) continue;
		const int svgStart = f.indexOf("<svg");
		if (svgStart < 0) continue;
		const int svgEnd = f.indexOf('>', svgStart);
		if (svgEnd < 0) continue;
		header = f.mid(svgStart, svgEnd - svgStart + 1);
		break;
	}
	if (header.isEmpty()) return QString();

	QStringList parts;
	parts << QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	parts << header;
	parts << QStringLiteral("  <g id=\"edge_cuts\" stroke=\"#000000\" stroke-width=\"10\" fill=\"none\">");
	// Lift the inner <line>/<rect>/<circle> elements of every input
	// fragment's edge_cuts group. We hunt by token; a full XML pass
	// here is overkill given the fragments are our own output.
	for (const QString & f : svgFragments) {
		if (f.isEmpty()) continue;
		const int gStart = f.indexOf("<g id=\"edge_cuts\"");
		if (gStart < 0) continue;
		const int contentStart = f.indexOf('>', gStart) + 1;
		const int gEnd = f.indexOf("</g>", contentStart);
		if (contentStart <= 0 || gEnd < 0) continue;
		parts << f.mid(contentStart, gEnd - contentStart);
	}
	parts << QStringLiteral("  </g>");
	parts << QStringLiteral("</svg>");
	return parts.join(QChar('\n'));
}

QString FabExporter::compositeDrill(const QStringList & svgFragments) const
{
	// Same pattern as compositeEdgeCuts but for the "drill" group.
	// Slice starting at <svg to avoid a double <?xml prologue.
	QString header;
	for (const QString & f : svgFragments) {
		if (f.isEmpty()) continue;
		const int svgStart = f.indexOf("<svg");
		if (svgStart < 0) continue;
		const int svgEnd = f.indexOf('>', svgStart);
		if (svgEnd < 0) continue;
		header = f.mid(svgStart, svgEnd - svgStart + 1);
		break;
	}
	if (header.isEmpty()) return QString();

	QStringList parts;
	parts << QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	parts << header;
	parts << QStringLiteral("  <g id=\"drill\" fill=\"#000000\" stroke=\"none\">");
	for (const QString & f : svgFragments) {
		if (f.isEmpty()) continue;
		const int gStart = f.indexOf("<g id=\"drill\"");
		if (gStart < 0) continue;
		const int contentStart = f.indexOf('>', gStart) + 1;
		const int gEnd = f.indexOf("</g>", contentStart);
		if (contentStart <= 0 || gEnd < 0) continue;
		parts << f.mid(contentStart, gEnd - contentStart);
	}
	parts << QStringLiteral("  </g>");
	parts << QStringLiteral("</svg>");
	return parts.join(QChar('\n'));
}

// ----------------------------------------------------------------------
// renderRealPanelLayers - the real Gerber pipeline.
//
// Pattern lifted from src/autoroute/panelizer.cpp (the legacy batch
// panelizer). For each unique source .fzz we:
//   1. Open a hidden MainWindow via FApplication::openWindowForService.
//   2. Locate the board ItemBase via pcbView()->findBoard().
//   3. Call Panelizer::makeSVGs(...) to render 9 per-layer SVGs onto
//      disk (one set non-rotated, one set rotated 90deg).
//
// Then, per panel layer, we concatenate all placed boards' per-layer
// SVGs (translated to the board's panel-local position) into a single
// panel-sized SVG and hand it to GerberGenerator::doEnd() - the same
// function single-board export uses, so output is gerbv/JLC-valid.
//
// The Edge_Cuts and Drill composites also receive the panel-synthesized
// frame + V-cut / mouse-bite / tooling-hole SVG fragments passed in as
// @p extraEdgeCutsSvg / @p extraDrillSvg.
// ----------------------------------------------------------------------

namespace {

// Build the same layer table the legacy batch panelizer uses (see
// panelizer.cpp:320-329). Centralised here so additions/reorderings
// stay in lockstep.
static QList<LayerThing> buildPanelLayerTable() {
	QList<LayerThing> t;
	t.append(LayerThing("outline",           ViewLayer::outlineLayers(),                        SVG2gerber::ForOutline,   GerberGenerator::OutlineSuffix));
	t.append(LayerThing("copper_top",        ViewLayer::copperLayers(ViewLayer::NewTop),        SVG2gerber::ForCopper,    GerberGenerator::CopperTopSuffix));
	t.append(LayerThing("copper_bottom",     ViewLayer::copperLayers(ViewLayer::NewBottom),     SVG2gerber::ForCopper,    GerberGenerator::CopperBottomSuffix));
	t.append(LayerThing("mask_top",          ViewLayer::maskLayers(ViewLayer::NewTop),          SVG2gerber::ForMask,      GerberGenerator::MaskTopSuffix));
	t.append(LayerThing("mask_bottom",       ViewLayer::maskLayers(ViewLayer::NewBottom),       SVG2gerber::ForMask,      GerberGenerator::MaskBottomSuffix));
	t.append(LayerThing("paste_mask_top",    ViewLayer::maskLayers(ViewLayer::NewTop),          SVG2gerber::ForPasteMask, GerberGenerator::PasteMaskTopSuffix));
	t.append(LayerThing("paste_mask_bottom", ViewLayer::maskLayers(ViewLayer::NewBottom),       SVG2gerber::ForPasteMask, GerberGenerator::PasteMaskBottomSuffix));
	t.append(LayerThing("silk_top",          ViewLayer::silkLayers(ViewLayer::NewTop),          SVG2gerber::ForSilk,      GerberGenerator::SilkTopSuffix));
	t.append(LayerThing("silk_bottom",       ViewLayer::silkLayers(ViewLayer::NewBottom),       SVG2gerber::ForSilk,      GerberGenerator::SilkBottomSuffix));
	t.append(LayerThing("drill",             ViewLayer::drillLayers(),                          SVG2gerber::ForDrill,     GerberGenerator::DrillSuffix));
	return t;
}

// Translate-and-strip one per-board layer SVG fragment, matching
// Panelizer::doOnePanelItem()'s wrapping, and apply the placement's
// orientation.
//
// The caller renders only two SVG sets per source board: a 0° set
// (norotate/) and a 90° set (rotate/). To realise the four rotations
// plus an optional horizontal mirror we pick the closest pre-rendered
// set and finish the orientation with an SVG <g transform>:
//
//   base set  = (rotation==90||270) ? the 90° set : the 0° set
//   extraSpin = (rotation==180||270) ? 180 : 0     // about the footprint centre
//   mirror    = flippedHorizontal ? scale(-1,1) about the centre
//
// Because 180° about the centre leaves the bounding box unchanged and
// the 90° set already swaps W/H, (effW,effH) below is the *placed*
// footprint for whichever base set we use, so the post-transform bbox
// top-left lands exactly on (xInches,yInches).
//
// @param oneSvg   stripped per-board layer SVG (already in the base set).
// @param xInches  placed footprint top-left X, panel-local inches.
// @param yInches  placed footprint top-left Y, panel-local inches.
// @param effWIn   placed footprint width  (inches) for the chosen set.
// @param effHIn   placed footprint height (inches) for the chosen set.
// @param extraSpin180  apply an extra 180° about the footprint centre.
// @param mirror   mirror left-to-right about the footprint centre.
static QString wrapForPanel(const QString & oneSvg,
                            double xInches, double yInches,
                            double effWIn, double effHIn,
                            bool extraSpin180, bool mirror) {
	if (oneSvg.isEmpty()) return QString();
	int left = oneSvg.indexOf("<svg");
	if (left < 0) return QString();
	left = oneSvg.indexOf('>', left + 1);
	if (left < 0) return QString();
	const int right = oneSvg.lastIndexOf("<");
	if (right <= left) return QString();

	const double dpi = GraphicsUtils::StandardFritzingDPI;
	const double tx = xInches * dpi;
	const double ty = yInches * dpi;
	const double ew = effWIn * dpi;
	const double eh = effHIn * dpi;
	const double cx = ew / 2.0;
	const double cy = eh / 2.0;

	// SVG transforms apply right-to-left: translate to the placement,
	// then (optionally) spin 180° about the footprint centre, then
	// (optionally) mirror about the footprint centre.
	QString xform = QStringLiteral("translate(%1,%2)").arg(tx).arg(ty);
	if (extraSpin180) {
		xform += QStringLiteral(" rotate(180,%1,%2)").arg(cx).arg(cy);
	}
	if (mirror) {
		// Mirror about the vertical centre line: x -> ew - x.
		xform += QStringLiteral(" translate(%1,0) scale(-1,1)").arg(ew);
	}

	return QStringLiteral("<g transform='%1'>\n").arg(xform)
	       + oneSvg.mid(left + 1, right - left - 1)
	       + QStringLiteral("</g>\n");
}

// Merge an "extra" geometry fragment into a layer composite. The extra
// fragments produced by panelizerseparators.cpp are full SVG documents
// in 1000-DPI units; the panel composite is in StandardFritzingDPI units.
// We strip outer <svg> tags and rescale the contents accordingly.
static QString wrapExtra(const QString & extraSvg) {
	if (extraSvg.isEmpty()) return QString();
	int left = extraSvg.indexOf("<svg");
	if (left < 0) return QString();
	left = extraSvg.indexOf('>', left + 1);
	if (left < 0) return QString();
	const int right = extraSvg.lastIndexOf("<");
	if (right <= left) return QString();
	// panelizerseparators emits at 1000 DPI; panel composite uses
	// StandardFritzingDPI. Apply a single uniform <g scale=...> wrap.
	const double scale = GraphicsUtils::StandardFritzingDPI / 1000.0;
	return QStringLiteral("<g transform='scale(%1)'>\n")
	         .arg(scale, 0, 'f', 6)
	       + extraSvg.mid(left + 1, right - left - 1)
	       + QStringLiteral("</g>\n");
}

} // anonymous namespace

bool FabExporter::renderRealPanelLayers(
		const QList<PanelizerEngine::PlacedBoard*> & laidOut,
		const PanelizerEngine::PanelSpec & panelSpec,
		const QString & extraEdgeCutsSvg,
		const QString & extraDrillSvg,
		const QString & gerberDir,
		const QString & projectName,
		QStringList & written,
		QStringList & warnings) const
{
	FApplication * app = qobject_cast<FApplication*>(qApp);
	if (app == nullptr) {
		warnings << QObject::tr("FabExporter: no FApplication; cannot open source sketches");
		return false;
	}
	if (laidOut.isEmpty()) {
		warnings << QObject::tr("FabExporter: no placed boards");
		return false;
	}

	const QList<LayerThing> layers = buildPanelLayerTable();

	// Scratch dirs for per-board renderings. norotate/ holds the SVGs
	// for un-rotated placements, rotate/ for the 90deg-rotated copies.
	QTemporaryDir tmp;
	if (!tmp.isValid()) {
		warnings << QObject::tr("FabExporter: cannot create temp dir for SVG scratch");
		return false;
	}
	QDir scratch(tmp.path());
	scratch.mkdir("norotate");
	scratch.mkdir("rotate");
	QDir norotateDir(scratch.absoluteFilePath("norotate"));
	QDir rotateDir(scratch.absoluteFilePath("rotate"));

	// Per-source-path -> board ItemBase ID. doOnePanelItem-style lookup
	// keys on (boardName, boardID), so every placed board copying the
	// same source resolves to the same SVG set.
	QHash<QString, long> boardIdByPath;
	QSet<QString> processed;

	for (const PanelizerEngine::PlacedBoard * pb : laidOut) {
		if (pb == nullptr) continue;
		const QString fzz = pb->source.fzzPath;
		if (fzz.isEmpty() || processed.contains(fzz)) continue;
		processed.insert(fzz);

		QFileInfo info(fzz);
		if (!info.exists()) {
			warnings << QObject::tr("FabExporter: source not found: %1").arg(fzz);
			continue;
		}

		// initialTab=3 is the PCB tab; matches Panelizer::openWindows.
		MainWindow * mw = app->openWindowForService(false, 3);
		if (mw == nullptr) {
			warnings << QObject::tr("FabExporter: openWindowForService returned null");
			continue;
		}
		mw->setCloseSilently(true);
		if (!mw->loadWhich(fzz, false, false, false, QString())) {
			warnings << QObject::tr("FabExporter: failed to load %1").arg(fzz);
			mw->close();
			delete mw;
			continue;
		}

		// Unlock all items so makeSVGs can render cleanly (matches
		// panelizer.cpp openWindows()).
		for (QGraphicsItem * gi : mw->pcbView()->scene()->items()) {
			ItemBase * ib = dynamic_cast<ItemBase*>(gi);
			if (ib != nullptr) ib->setMoveLock(false);
		}

		QList<ItemBase*> boards = mw->pcbView()->findBoard();
		if (boards.isEmpty()) {
			warnings << QObject::tr("FabExporter: no board found in %1").arg(fzz);
			mw->close();
			delete mw;
			continue;
		}
		ItemBase * board = boards.first();
		const QString boardName = info.completeBaseName();
		boardIdByPath.insert(fzz, board->id());

		// Render the un-rotated SVG set.
		Panelizer::makeSVGs(mw, board, boardName, const_cast<QList<LayerThing>&>(layers), norotateDir, info);

		// Render the 90deg-rotated SVG set. We rotate the whole sketch
		// in-place, then re-fetch the board (rotation may change the
		// item id under some conditions; in practice it doesn't, but
		// the legacy panelizer re-walks findBoard() too).
		mw->pcbView()->selectAllItems(true, false);
		mw->pcbView()->rotateX(90, false, nullptr);
		QList<ItemBase*> rotBoards = mw->pcbView()->findBoard();
		if (!rotBoards.isEmpty()) {
			Panelizer::makeSVGs(mw, rotBoards.first(), boardName, const_cast<QList<LayerThing>&>(layers), rotateDir, info);
		}

		mw->close();
		delete mw;
	}

	if (boardIdByPath.isEmpty()) {
		warnings << QObject::tr("FabExporter: no boards rendered; aborting");
		return false;
	}

	// Composite per-layer panel SVGs. One QString per layer, primed with
	// a panel-sized <svg> header in StandardFritzingDPI units.
	const double W = panelSpec.panelSizeInches.width();
	const double H = panelSpec.panelSizeInches.height();
	QStringList panelSvgs;
	for (int i = 0; i < layers.count(); ++i) {
		panelSvgs << TextUtils::makeSVGHeader(1, GraphicsUtils::StandardFritzingDPI, W, H);
	}

	for (const PanelizerEngine::PlacedBoard * pb : laidOut) {
		if (pb == nullptr) continue;
		if (!boardIdByPath.contains(pb->source.fzzPath)) continue;
		const long bid = boardIdByPath.value(pb->source.fzzPath);
		const QString boardName = QFileInfo(pb->source.fzzPath).completeBaseName();
		// Pick the pre-rendered base set closest to the requested angle:
		// the 90° set for 90/270, the 0° set for 0/180.
		const bool useRotatedSet = (pb->rotationDegrees == 90 || pb->rotationDegrees == 270)
		                           || pb->rotated90;
		const QDir & layerDir = useRotatedSet ? rotateDir : norotateDir;

		// Effective footprint for the chosen base set (the 90° set has
		// W/H swapped). 180°/270° keep the same bbox, applied as an extra
		// spin in wrapForPanel.
		const double w = pb->source.boardSizeInches.width();
		const double h = pb->source.boardSizeInches.height();
		const double effWIn = useRotatedSet ? h : w;
		const double effHIn = useRotatedSet ? w : h;
		const bool extraSpin180 = (pb->rotationDegrees == 180 || pb->rotationDegrees == 270);
		const bool mirror = pb->flippedHorizontal;

		for (int i = 0; i < layers.count(); ++i) {
			const QString fname = layerDir.absoluteFilePath(
				QStringLiteral("%1_%2_%3.svg").arg(boardName).arg(bid).arg(layers[i].name));
			QFile f(fname);
			if (!f.open(QFile::ReadOnly)) {
				// Some boards legitimately produce no content for a given
				// layer (e.g. no silk-bottom). Silent skip matches the
				// legacy panelizer's behaviour.
				continue;
			}
			const QString one = QString::fromUtf8(f.readAll());
			f.close();
			const QString wrapped = wrapForPanel(one,
				pb->positionInches.x(), pb->positionInches.y(),
				effWIn, effHIn, extraSpin180, mirror);
			if (!wrapped.isEmpty()) {
				panelSvgs[i] += wrapped;
			}
		}
	}

	// Merge the extra synthesized geometry into Edge_Cuts / Drill.
	for (int i = 0; i < layers.count(); ++i) {
		if (layers[i].name == QLatin1String("outline") && !extraEdgeCutsSvg.isEmpty()) {
			panelSvgs[i] += wrapExtra(extraEdgeCutsSvg);
		} else if (layers[i].name == QLatin1String("drill") && !extraDrillSvg.isEmpty()) {
			panelSvgs[i] += wrapExtra(extraDrillSvg);
		}
	}

	// Close each SVG and hand to GerberGenerator::doEnd().
	int emitted = 0;
	for (int i = 0; i < layers.count(); ++i) {
		panelSvgs[i] += QStringLiteral("</svg>");
		SVG2gerber::ForWhy fw = layers[i].forWhy;
		// doEnd's documented quirk (replicated from panelizer.cpp): mask
		// and paste-mask both ride the ForCopper conversion path because
		// they're emitted as pre-expanded black regions.
		if (fw == SVG2gerber::ForMask || fw == SVG2gerber::ForPasteMask) {
			fw = SVG2gerber::ForCopper;
		}
		const QSizeF svgSizePx(W * GraphicsUtils::StandardFritzingDPI,
		                       H * GraphicsUtils::StandardFritzingDPI);
		const int rc = GerberGenerator::doEnd(panelSvgs[i], 2, layers[i].name, fw,
		                                      svgSizePx, gerberDir, projectName,
		                                      layers[i].suffix, false);
		if (rc < 0) {
			warnings << QObject::tr("FabExporter: doEnd failed for %1 (rc=%2)")
			            .arg(layers[i].name).arg(rc);
			continue;
		}
		written << QDir(gerberDir).filePath(projectName + layers[i].suffix);
		++emitted;
	}
	return emitted > 0;
}

bool FabExporter::writePlaceholderGerber(const QString & outPath,
                                         const QString & layerName,
                                         QStringList & warnings) const
{
	// Minimum valid RS-274X: format spec + units + end-of-file.
	// JLC and other ingest pipelines accept an empty layer as long as
	// the magic header is present. We emit metric / leading zero
	// suppression to match what SVG2gerber writes for real layers.
	QFile f(outPath);
	if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
		warnings << QObject::tr("FabExporter: cannot open %1 for write").arg(outPath);
		return false;
	}
	QTextStream s(&f);
	s << "G04 Layer " << layerName << " (placeholder) - Fritzing panelizer*\n";
	s << "%FSLAX46Y46*%\n";
	s << "%MOMM*%\n";
	s << "%LPD*%\n";
	s << "M02*\n";
	f.close();
	return true;
}

bool FabExporter::writeGbrJob(const Result & r,
                              const PanelizerEngine::PanelSpec & panelSpec,
                              const Spec & spec) const
{
	// Minimal Gerber X3 job file (Ucamco spec). Just enough that JLC
	// recognizes the bundle as a "Gerber X3 project" rather than a
	// loose pile of files.
	const QString jobPath = QDir(r.productionDir).filePath("gerber/" + spec.projectName + "-job.gbrjob");
	QFile f(jobPath);
	if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return false;

	QTextStream s(&f);
	s << "{\n";
	s << "  \"Header\": {\n";
	s << "    \"GenerationSoftware\": {\n";
	s << "      \"Vendor\": \"Fritzing\",\n";
	s << "      \"Application\": \"Panelizer\",\n";
	s << "      \"Version\": \"1.0\"\n";
	s << "    },\n";
	s << "    \"CreationDate\": \"" << QDateTime::currentDateTime().toString(Qt::ISODate) << "\"\n";
	s << "  },\n";
	s << "  \"GeneralSpecs\": {\n";
	s << "    \"ProjectId\": { \"Name\": \"" << spec.projectName << "\", \"GUID\": \"\", \"Revision\": \"1\" },\n";
	s << "    \"Size\": { \"X\": " << panelSpec.panelSizeInches.width() * 25.4
	  << ", \"Y\": " << panelSpec.panelSizeInches.height() * 25.4 << " },\n";
	s << "    \"LayerNumber\": 2,\n";
	s << "    \"BoardThickness\": 1.6,\n";
	s << "    \"Finish\": \"HASL\"\n";
	s << "  },\n";
	s << "  \"FilesAttributes\": [\n";
	bool first = true;
	for (const QString & path : r.written) {
		const QString base = QFileInfo(path).fileName();
		QString function;
		if (base.endsWith("-F_Cu.gbr"))         function = "Copper,L1,Top";
		else if (base.endsWith("-B_Cu.gbr"))    function = "Copper,L2,Bot";
		else if (base.endsWith("-F_Mask.gbr"))  function = "Soldermask,Top";
		else if (base.endsWith("-B_Mask.gbr"))  function = "Soldermask,Bot";
		else if (base.endsWith("-F_Silkscreen.gbr")) function = "Legend,Top";
		else if (base.endsWith("-B_Silkscreen.gbr")) function = "Legend,Bot";
		else if (base.endsWith("-F_Paste.gbr")) function = "Paste,Top";
		else if (base.endsWith("-B_Paste.gbr")) function = "Paste,Bot";
		else if (base.endsWith("-Edge_Cuts.gbr")) function = "Profile,P";
		else continue; // drill file not listed under FilesAttributes
		if (!first) s << ",\n";
		s << "    { \"Path\": \"gerber/" << base << "\", \"FileFunction\": \"" << function << "\" }";
		first = false;
	}
	s << "\n  ]\n";
	s << "}\n";
	f.close();
	return true;
}

// ----------------------------------------------------------------------
// BOM / CPL writers
//
// The part-walking approach mirrors GerberGenerator::exportPickAndPlace:
// scene().collidingItems(board) gives every QGraphicsItem touching the
// board rect; we keep one entry per layerKinChief() to avoid double-
// counting the same component across top/bottom views.
//
// BOM aggregates by (value, package) - one row per unique part type.
// CPL emits one row per placement (one row per designator).
// ----------------------------------------------------------------------

namespace {

struct PartWalk {
	QString designator;
	QString value;
	QString package;
	QPointF centerScene;   // scene px, 1000 DPI
	double  rotationDeg;   // CCW positive
	bool    topLayer;
	bool    notInBom;
};

// Lift the same idea as GerberGenerator::exportPickAndPlace, but
// returning a vector of normalized rows instead of writing to a stream.
static QVector<PartWalk> collectPlacedParts(ItemBase * board, PCBSketchWidget * pcbView)
{
	QVector<PartWalk> rows;
	if (board == nullptr || pcbView == nullptr || pcbView->scene() == nullptr) return rows;

	QSet<ItemBase *> seen;
	const auto colliding = pcbView->scene()->collidingItems(board);
	for (QGraphicsItem * gi : colliding) {
		auto * ib = dynamic_cast<ItemBase *>(gi);
		if (ib == nullptr) continue;
		if (ib == board) continue;
		if (ib->itemType() == ModelPart::Wire) continue;
		ib = ib->layerKinChief();
		if (!ib->isEverVisible()) continue;
		if (ib == board) continue;
		if (seen.contains(ib)) continue;
		seen.insert(ib);

		PartWalk p;
		p.designator = ib->instanceTitle();
		// value lookup - same keys as exportPickAndPlace
		static const QStringList valueKeys =
			{ "resistance", "capacitance", "inductance", "voltage", "current", "power" };
		for (const QString & k : valueKeys) {
			p.value = ib->modelPart()->localProp(k).toString();
			if (!p.value.isEmpty()) break;
			p.value = ib->modelPart()->properties().value(k);
			if (!p.value.isEmpty()) break;
		}
		p.package = ib->modelPart()->properties().value("package");
		p.centerScene = ib->sceneBoundingRect().center();
		const QTransform t = ib->transform();
		p.rotationDeg = std::atan2(t.m12(), t.m11()) * 180.0 / M_PI;
		if (p.rotationDeg < 0) p.rotationDeg += 360.0;
		p.topLayer = (ib->viewLayerID() == ViewLayer::Copper1);
		// DNP / not-in-bom: Fritzing doesn't have a first-class flag,
		// but parts marked "notinbom" via properties are excluded.
		p.notInBom = ib->modelPart()->properties().value("notinbom").toLower() == "yes";
		rows.append(p);
	}
	return rows;
}

} // anonymous namespace

bool FabExporter::writeBom(ItemBase * board,
                           PCBSketchWidget * pcbView,
                           const QList<PanelizerEngine::PlacedBoard*> & laidOut,
                           const Spec & spec,
                           QStringList & warnings,
                           QString & outBomPath) const
{
	const QString dir = QDir(spec.outputDir).filePath("production/bom");
	QDir().mkpath(dir);
	outBomPath = QDir(dir).filePath(spec.projectName + "_bom.csv");

	QFile f(outBomPath);
	if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
		warnings << QObject::tr("FabExporter: cannot write %1").arg(outBomPath);
		return false;
	}
	QTextStream s(&f);
	// JLC BOM header (column order matters to their parser).
	s << "Comment,Designator,Footprint,LCSC Part #\n";

	if (board == nullptr || pcbView == nullptr) {
		f.close();
		return true; // empty BOM is fine as a placeholder
	}

	// How many panel placements share this source board? Each placement
	// gets its own designator suffix (`R1_1`, `R1_2`, ...) so the
	// assembler can map a row back to a physical location alongside
	// the CPL coordinates emitted below. Falls back to 1 for the
	// no-laidOut case (single-board export reusing this writer).
	const QString sourceKey = QFileInfo(
		board->modelPart() ? board->modelPart()->path() : QString()).absoluteFilePath();
	int copyCount = 0;
	for (const auto * pb : laidOut) {
		if (pb == nullptr) continue;
		if (QFileInfo(pb->source.fzzPath).absoluteFilePath() == sourceKey) ++copyCount;
	}
	if (copyCount <= 0) copyCount = 1;

	// Aggregate by (value + package); designators get joined with commas.
	const QVector<PartWalk> parts = collectPlacedParts(board, pcbView);
	QMap<QString, QStringList> designatorsByKey;
	QMap<QString, QPair<QString, QString>> labelsByKey; // key -> (value, package)
	for (const PartWalk & p : parts) {
		if (p.notInBom) continue;
		const QString key = p.value + "|" + p.package;
		if (!labelsByKey.contains(key)) labelsByKey[key] = qMakePair(p.value, p.package);
		if (copyCount == 1) {
			designatorsByKey[key].append(p.designator);
		} else {
			// Expand one row per copy so the user can audit which copy
			// each designator belongs to. Keeps JLC happy (they accept
			// either aggregated or expanded designator lists).
			for (int i = 1; i <= copyCount; ++i) {
				designatorsByKey[key].append(QStringLiteral("%1_%2").arg(p.designator).arg(i));
			}
		}
	}
	for (auto it = designatorsByKey.cbegin(); it != designatorsByKey.cend(); ++it) {
		QStringList dlist = it.value();
		dlist.sort();
		const auto & label = labelsByKey[it.key()];
		s << '"' << label.first << "\",\""
		  << dlist.join(',') << "\",\""
		  << label.second << "\",\"\"\n";
	}
	f.close();
	return true;
}

bool FabExporter::writeCpl(ItemBase * board,
                           PCBSketchWidget * pcbView,
                           const QList<PanelizerEngine::PlacedBoard*> & laidOut,
                           const Spec & spec,
                           QStringList & warnings,
                           QString & outCplPath) const
{
	const QString dir = QDir(spec.outputDir).filePath("production/cpl");
	QDir().mkpath(dir);
	outCplPath = QDir(dir).filePath(spec.projectName + "_cpl.csv");

	QFile f(outCplPath);
	if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
		warnings << QObject::tr("FabExporter: cannot write %1").arg(outCplPath);
		return false;
	}
	QTextStream s(&f);
	s << "Designator,Val,Package,Mid X,Mid Y,Rotation,Layer\n";

	if (board == nullptr || pcbView == nullptr) {
		f.close();
		return true;
	}

	// Source-board metrics in mm, plus part offsets relative to the
	// board's bottom-left (Gerber/CPL math-positive Y-up frame).
	const QRectF boardScene = board->sceneBoundingRect();
	const QPointF originScene = boardScene.bottomLeft();
	const double boardWmm = GraphicsUtils::pixels2mm(boardScene.width(),  GraphicsUtils::SVGDPI);
	const double boardHmm = GraphicsUtils::pixels2mm(boardScene.height(), GraphicsUtils::SVGDPI);

	const QVector<PartWalk> parts = collectPlacedParts(board, pcbView);

	struct PartMm {
		QString designator;
		QString value;
		QString package;
		double  offXmm;       // offset from board's bottom-left (Y up)
		double  offYmm;
		double  rotationDeg;
		bool    topLayer;
	};
	QVector<PartMm> partsMm;
	partsMm.reserve(parts.size());
	for (const PartWalk & p : parts) {
		if (p.notInBom) continue; // DNP excluded from CPL per JLC spec
		PartMm m;
		m.designator = p.designator;
		m.value      = p.value;
		m.package    = p.package;
		m.offXmm     = GraphicsUtils::pixels2mm(p.centerScene.x() - originScene.x(), GraphicsUtils::SVGDPI);
		m.offYmm     = GraphicsUtils::pixels2mm(originScene.y() - p.centerScene.y(), GraphicsUtils::SVGDPI);
		m.rotationDeg = p.rotationDeg;
		m.topLayer    = p.topLayer;
		partsMm.append(m);
	}

	// Collect placements for THIS source board. If the caller passed an
	// empty laidOut (e.g. single-board export reusing this writer), fall
	// back to one synthetic placement at (0,0) with no rotation so the
	// pre-panelizer behaviour is preserved.
	QList<const PanelizerEngine::PlacedBoard*> mine;
	const QString sourceKey = QFileInfo(
		board->modelPart() ? board->modelPart()->path() : QString()).absoluteFilePath();
	for (const auto * pb : laidOut) {
		if (pb == nullptr) continue;
		if (QFileInfo(pb->source.fzzPath).absoluteFilePath() == sourceKey) mine.append(pb);
	}
	const double inToMm = 25.4;

	if (mine.isEmpty()) {
		// Single-board fallback: emit at origin, un-suffixed designators.
		for (const PartMm & m : partsMm) {
			s << '"' << m.designator << "\",\""
			  << m.value << "\",\""
			  << m.package << "\","
			  << QString::number(m.offXmm, 'f', 4) << "mm,"
			  << QString::number(m.offYmm, 'f', 4) << "mm,"
			  << QString::number(m.rotationDeg, 'f', 2) << ","
			  << (m.topLayer ? "top" : "bottom") << "\n";
		}
		f.close();
		return true;
	}

	// Per-placement emission. For each PlacedBoard:
	//   - positionInches is the board's bottom-left corner on the panel
	//     (panel Y-up). We add the part's in-board offset to that.
	//   - rotated90: the renderPanel SVG pipeline pre-rotates the board
	//     CCW 90° around its own bottom-left ("renderRealPanelLayers"
	//     reads from rotateDir, which holds the pre-rotated SVGs). After
	//     that rotation the rotated board's width = original height and
	//     vice-versa; the bottom-left corner is unchanged. So a part
	//     originally at (ox, oy) ends up at (-oy, ox) relative to the
	//     bottom-left, then shifted right by boardHmm to keep the
	//     footprint in the positive quadrant aligned with positionInches.
	int copyIdx = 0;
	for (const auto * pb : mine) {
		++copyIdx;
		const double panelOriginXmm = pb->positionInches.x() * inToMm;
		const double panelOriginYmm = pb->positionInches.y() * inToMm;
		for (const PartMm & m : partsMm) {
			double localX = m.offXmm;
			double localY = m.offYmm;
			double rot    = m.rotationDeg;
			if (pb->rotated90) {
				// CCW 90° about bottom-left, then shift +boardHmm in X so
				// the corner sits back at (0,0) in the rotated frame.
				const double rx = -localY + boardHmm;
				const double ry =  localX;
				localX = rx;
				localY = ry;
				rot   += 90.0;
				if (rot >= 360.0) rot -= 360.0;
			}
			const double midXmm = panelOriginXmm + localX;
			const double midYmm = panelOriginYmm + localY;
			const QString designator = (mine.size() == 1)
				? m.designator
				: QStringLiteral("%1_%2").arg(m.designator).arg(copyIdx);
			s << '"' << designator << "\",\""
			  << m.value << "\",\""
			  << m.package << "\","
			  << QString::number(midXmm, 'f', 4) << "mm,"
			  << QString::number(midYmm, 'f', 4) << "mm,"
			  << QString::number(rot, 'f', 2) << ","
			  << (m.topLayer ? "top" : "bottom") << "\n";
		}
	}
	// boardWmm is intentionally captured but only used for symmetry with
	// boardHmm if a CW-rotation variant is added; reference once to
	// silence -Wunused-but-set-variable on stricter toolchains.
	(void) boardWmm;
	f.close();
	return true;
}

// ----------------------------------------------------------------------
// exportFromPanel - the entry point.
// ----------------------------------------------------------------------

FabExporter::Result FabExporter::exportFromPanel(
		const QList<PanelizerEngine::PlacedBoard*> & laidOut,
		const PanelizerEngine::PanelSpec & panelSpec,
		const PanelizerEngine::SeparationSpec & sep,
		const PanelizerEngine::ExtrasSpec & extras,
		ItemBase * sketchBoard,
		PCBSketchWidget * sketchWidget,
		const Spec & spec)
{
	Result r;

	if (spec.outputDir.isEmpty()) {
		r.warnings << QObject::tr("FabExporter: no outputDir");
		return r;
	}
	if (spec.projectName.isEmpty()) {
		r.warnings << QObject::tr("FabExporter: no projectName");
		return r;
	}

	// Layout: <outputDir>/production/{gerber,bom,cpl}/
	r.productionDir = QDir(spec.outputDir).filePath("production");
	const QString gerberDir = QDir(r.productionDir).filePath("gerber");
	QDir().mkpath(gerberDir);

	// 1) Build the Edge_Cuts SVG by compositing:
	//      frame outline + V-cut OR mouse-bite slot cuts
	QStringList edgeCutsParts;
	edgeCutsParts << PanelizerSeparators::frameOutlineSvg(panelSpec);
	if (sep.kind == PanelizerEngine::Separation::VCut) {
		edgeCutsParts << PanelizerSeparators::vcutSvg(laidOut, panelSpec, sep);
	} else if (sep.kind == PanelizerEngine::Separation::MouseBites) {
		edgeCutsParts << PanelizerSeparators::mouseBiteSvg(laidOut, panelSpec, sep);
	}
	const QString edgeSvg = compositeEdgeCuts(edgeCutsParts);

	// 2) Build the Drill SVG by compositing:
	//      mouse-bite holes + tooling holes
	QStringList drillParts;
	if (sep.kind == PanelizerEngine::Separation::MouseBites) {
		drillParts << PanelizerSeparators::mouseBiteSvg(laidOut, panelSpec, sep);
	}
	if (extras.addToolingHoles) {
		drillParts << PanelizerSeparators::toolingHoleSvg(panelSpec, extras.toolingHoleDiameterInches);
	}
	const QString drillSvg = compositeDrill(drillParts);

	// 3) Fiducial SVG (top copper + top mask) - emitted directly into
	//    the placeholder layers via composite below (next milestone).
	//    For now, we attach it as a separate diagnostic SVG so the
	//    user can verify positions even though the layers are stubs.
	if (extras.addFiducials) {
		const QString fidSvg = PanelizerSeparators::fiducialSvg(panelSpec,
		                          extras.fiducialDiameterMils, extras.fiducialClearMils);
		if (!fidSvg.isEmpty()) {
			const QString fidPath = QDir(gerberDir).filePath(spec.projectName + "_fiducials.svg");
			QFile f(fidPath);
			if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
				f.write(fidSvg.toUtf8());
				f.close();
				r.written << fidPath;
			}
		}
	}

	// 4) Real per-board rendering: open every source .fzz, render its
	//    9 layers, composite each onto a panel-sized SVG, and run them
	//    through GerberGenerator::doEnd() - the same code path the
	//    single-board export uses. The synthesized frame / V-cut /
	//    mouse-bite / tooling-hole geometry is merged into the
	//    appropriate composite (outline -> Edge_Cuts, drill -> .drl).
	const bool realOK = renderRealPanelLayers(laidOut, panelSpec,
	                                          edgeSvg, drillSvg,
	                                          gerberDir, spec.projectName,
	                                          r.written, r.warnings);
	if (!realOK) {
		// Hard failure (no boards opened / no FApplication). Fall back to
		// the panel-frame-only Edge_Cuts + Drill so the user still gets
		// something the viewer can render.
		for (int i = 0; i < kNumLayers; ++i) {
			const QString key = QLatin1String(kLayers[i].key);
			const QString suffix = suffixFor(spec.profile, key);
			const QString outPath = QDir(gerberDir).filePath(spec.projectName + suffix);
			bool ok = false;
			if (key == QLatin1String("Edge_Cuts")) {
				ok = convertSvgToGerber(edgeSvg, key, static_cast<int>(SVG2gerber::ForOutline), outPath, r.warnings);
			} else if (key == QLatin1String("Drill")) {
				ok = convertSvgToGerber(drillSvg, key, static_cast<int>(SVG2gerber::ForDrill), outPath, r.warnings);
			} else {
				ok = writePlaceholderGerber(outPath, key, r.warnings);
			}
			if (ok) r.written << outPath;
		}
	}

	// 5) BOM + CPL CSVs.
	QString bomPath, cplPath;
	if (writeBom(sketchBoard, sketchWidget, laidOut, spec, r.warnings, bomPath) && !bomPath.isEmpty())
		r.written << bomPath;
	if (writeCpl(sketchBoard, sketchWidget, laidOut, spec, r.warnings, cplPath) && !cplPath.isEmpty())
		r.written << cplPath;

	// 6) Job file (X3 metadata).
	if (writeGbrJob(r, panelSpec, spec)) {
		r.written << QDir(r.productionDir).filePath("gerber/" + spec.projectName + "-job.gbrjob");
	}

	r.success = true;
	DebugDialog::debug(QString("FabExporter: emitted %1 files to %2")
	                   .arg(r.written.size()).arg(r.productionDir),
	                   DebugDialog::Info);
	return r;
}
