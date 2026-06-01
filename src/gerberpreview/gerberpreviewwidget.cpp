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

#include "gerberpreviewwidget.h"
#include "gerberparser.h"
#include "gerberrenderer.h"

#include <QFile>
#include <QFileInfo>
#include <QColorDialog>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPathStroker>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QTextStream>
#include <QWheelEvent>
#include <QtMath>

#include <cmath>

GerberPreviewWidget::GerberPreviewWidget(QWidget *parent)
	: QWidget(parent)
{
	setMinimumSize(320, 240);
	setMouseTracking(true);
	setAutoFillBackground(false);
	// Need keyboard focus for Escape-to-clear-measurement.
	setFocusPolicy(Qt::StrongFocus);

	// NOTE: we deliberately do NOT override the widget palette here.
	// Forcing QPalette::Window to an off-white substrate used to bleed
	// onto the surrounding dialog chrome (and the window title bar on
	// dark Linux themes), making the preview window unreadable. The
	// off-white "board" is now painted only on the drawing canvas in
	// paintEvent() via m_canvasColor, leaving the dialog free to follow
	// the user's theme.
	m_canvasColor = QColor("#F5F5F0");
}

GerberPreviewWidget::~GerberPreviewWidget() = default;

int GerberPreviewWidget::layerCount() const
{
	return m_gerberLayers.size() + (m_drillLayers.isEmpty() ? 0 : 1);
}

GerberPreviewWidget::LayerKind
GerberPreviewWidget::classify(const QString &filename)
{
	// Matching is case-insensitive. We use two signals, in priority
	// order: (1) the real file extension, which is the most reliable
	// discriminator for both Fritzing (`_copperTop.gtl`) and most fab
	// houses, and (2) name fragments as a fallback for the KiCad
	// convention (`-F_Cu.gbr`) where everything shares the `.gbr`
	// extension.
	//
	// NOTE: the previous implementation tested `completeBaseName()`
	// (which strips the extension) against strings like ".gtl" — those
	// comparisons could never match, so every Fritzing-exported layer
	// fell through to Unknown and the layer manager never populated.
	// That was the root cause of "I see the boards but not the layers".
	const QFileInfo fi(filename);
	const QString base = fi.completeBaseName().toLower(); // extension stripped
	const QString ext  = fi.suffix().toLower();

	// --- (1) Extension-based classification (Fritzing + common fab) ---
	if (ext == QLatin1String("gtl")) return TopCopper;
	if (ext == QLatin1String("gbl")) return BottomCopper;
	if (ext == QLatin1String("gts")) return TopMask;
	if (ext == QLatin1String("gbs")) return BottomMask;
	if (ext == QLatin1String("gto")) return TopSilk;
	if (ext == QLatin1String("gbo")) return BottomSilk;
	if (ext == QLatin1String("gtp")) return TopPaste;
	if (ext == QLatin1String("gbp")) return BottomPaste;
	if (ext == QLatin1String("gko") || ext == QLatin1String("gm1") ||
	    ext == QLatin1String("gm2") || ext == QLatin1String("gml")) return EdgeCuts;
	if (ext == QLatin1String("drl") || ext == QLatin1String("xln")) return Drill;

	// --- (2) Name-fragment fallback (KiCad `.gbr`, Fritzing drill) ---
	// Order matters: the more specific paste/mask/silk fragments must be
	// tested before the very generic copper `_top`/`_bottom`, and paste
	// before mask because "pasteMaskTop" contains "maskTop".
	if (base.contains(QLatin1String("pastemasktop")) || base.contains(QLatin1String("pastetop")) ||
	    base.endsWith(QLatin1String("-f_paste")))                   return TopPaste;
	if (base.contains(QLatin1String("pastemaskbottom")) || base.contains(QLatin1String("pastebottom")) ||
	    base.endsWith(QLatin1String("-b_paste")))                   return BottomPaste;
	if (base.contains(QLatin1String("masktop")) || base.endsWith(QLatin1String("-f_mask")))   return TopMask;
	if (base.contains(QLatin1String("maskbottom")) || base.endsWith(QLatin1String("-b_mask"))) return BottomMask;
	if (base.contains(QLatin1String("silktop")) || base.endsWith(QLatin1String("-f_silkscreen")) ||
	    base.endsWith(QLatin1String("-f_silk")))                    return TopSilk;
	if (base.contains(QLatin1String("silkbottom")) || base.endsWith(QLatin1String("-b_silkscreen")) ||
	    base.endsWith(QLatin1String("-b_silk")))                    return BottomSilk;
	if (base.contains(QLatin1String("coppertop")) || base.endsWith(QLatin1String("-f_cu")) ||
	    base.endsWith(QLatin1String("_top")))                       return TopCopper;
	if (base.contains(QLatin1String("copperbottom")) || base.endsWith(QLatin1String("-b_cu")) ||
	    base.endsWith(QLatin1String("_bottom")))                    return BottomCopper;
	if (base.contains(QLatin1String("contour")) || base.contains(QLatin1String("outline")) ||
	    base.contains(QLatin1String("edge_cuts")) || base.contains(QLatin1String("edgecuts")) ||
	    base.contains(QLatin1String("boardoutline")))              return EdgeCuts;

	// Fritzing drill is `<board>_drill.txt`; a bare `.txt` in a Gerber
	// folder is almost always the Excellon drill file.
	if (base.contains(QLatin1String("drill")) || ext == QLatin1String("txt")) return Drill;

	return Unknown;
}

QColor GerberPreviewWidget::colorFor(LayerKind kind)
{
	// Palette per homerun-gui.md §12.1.4.
	switch (kind) {
		case TopCopper:    return QColor(184, 115,  51, 200); // copper
		case BottomCopper: return QColor( 51, 153, 255, 180); // blue
		case TopMask:      return QColor(  0,  85,   0, 120); // green
		case BottomMask:   return QColor(  0,  85,   0, 120);
		case TopSilk:      return QColor(255, 255, 255, 230);
		case BottomSilk:   return QColor(255, 255, 255, 200);
		case TopPaste:     return QColor(170, 170, 170, 150);
		case BottomPaste:  return QColor(170, 170, 170, 150);
		case EdgeCuts:     return QColor(255, 255,   0, 255); // yellow
		case Drill:        return QColor( 60,  60,  60, 255);
		case Unknown:
		default:           return QColor(200,   0, 200, 180); // magenta = "what?"
	}
}

void GerberPreviewWidget::loadFiles(const QStringList &paths)
{
	m_gerberLayers.clear();
	m_drillLayers.clear();
	m_warnings.clear();
	// Stale highlight from previous file set would point at gone layers.
	m_selectedLayerIdx = -1;
	m_selectedDcode    = -1;

	for (const QString &path : paths) {
		const LayerKind kind = classify(path);
		QFile f(path);
		if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
			m_warnings << tr("could not open: %1").arg(path);
			continue;
		}
		const QString text = QString::fromUtf8(f.readAll());
		f.close();

		if (kind == Drill) {
			DrillLayer dl;
			dl.path = path;
			ExcellonParser ep;
			dl.data = ep.parse(text);
			m_warnings << dl.data.warnings;
			m_drillLayers.append(dl);
		} else {
			GerberLayer gl;
			gl.path  = path;
			gl.kind  = kind;
			gl.color = colorFor(kind);
			GerberParser gp;
			gl.doc   = gp.parse(text);
			m_warnings << gl.doc.warnings;
			m_gerberLayers.append(gl);
		}
	}

	m_worldBounds = computeBounds();
	zoomToFit();
	update();
	emit loadFinished(layerCount(), m_warnings);
}

void GerberPreviewWidget::setLayerVisible(LayerKind kind, bool on)
{
	if (kind == Drill) {
		for (auto &dl : m_drillLayers) dl.visible = on;
	} else {
		for (auto &gl : m_gerberLayers) {
			if (gl.kind == kind) gl.visible = on;
		}
	}
	update();
}

void GerberPreviewWidget::setLayerColor(LayerKind kind, const QColor &color)
{
	// Drill is rendered with a hardcoded contrast color today; treat
	// it as a no-op for now so the layer-manager swatch for drill
	// stays informational. TODO(landracer): plumb drill color through
	// renderDrills() once we have a use case for non-default drill
	// styling.
	if (kind == Drill) return;
	for (auto &gl : m_gerberLayers) {
		if (gl.kind == kind) gl.color = color;
	}
	update();
}

QColor GerberPreviewWidget::layerColor(LayerKind kind) const
{
	for (const auto &gl : m_gerberLayers) {
		if (gl.kind == kind) return gl.color;
	}
	return colorFor(kind);
}

QVector<GerberPreviewWidget::LayerKind>
GerberPreviewWidget::loadedKinds() const
{
	// Return one entry per kind present in the stack, in the same
	// bottom-up order used by paintEvent(). The manager UI uses this
	// to drive both row order and what swatches to display.
	static const LayerKind order[] = {
		BottomCopper, BottomMask, BottomSilk, BottomPaste,
		TopCopper,    TopMask,    TopSilk,    TopPaste,
		EdgeCuts
	};
	QVector<LayerKind> out;
	for (LayerKind k : order) {
		for (const auto &gl : m_gerberLayers) {
			if (gl.kind == k) { out.append(k); break; }
		}
	}
	if (!m_drillLayers.isEmpty()) out.append(Drill);
	return out;
}

void GerberPreviewWidget::composeUserXform(const QTransform &op)
{
	// All view ops happen around the bounds center in mm so that the
	// content visibly pivots in place. Op is expressed as a transform
	// at the origin; we wrap it with translate-to-center/translate-back.
	const QPointF c = m_worldBounds.center();
	QTransform pivot;
	pivot.translate(c.x(), c.y());
	pivot = op * pivot;
	QTransform back;
	back.translate(-c.x(), -c.y());
	const QTransform around = back * pivot;
	m_userXform = m_userXform * around;
	zoomToFit();
	update();
}

void GerberPreviewWidget::rotate90()
{
	QTransform r;
	r.rotate(90.0);
	composeUserXform(r);
}

void GerberPreviewWidget::flipHorizontal()
{
	QTransform f;
	f.scale(-1.0, 1.0);
	composeUserXform(f);
}

void GerberPreviewWidget::flipVertical()
{
	QTransform f;
	f.scale(1.0, -1.0);
	composeUserXform(f);
}

void GerberPreviewWidget::resetViewTransform()
{
	m_userXform = QTransform();
	zoomToFit();
	update();
}

QRectF GerberPreviewWidget::computeBounds() const
{
	QRectF b;
	for (const auto &gl : m_gerberLayers) {
		if (gl.doc.bounds.isNull()) continue;
		b = b.isNull() ? gl.doc.bounds : b.united(gl.doc.bounds);
	}
	for (const auto &dl : m_drillLayers) {
		if (dl.data.bounds.isNull()) continue;
		b = b.isNull() ? dl.data.bounds : b.united(dl.data.bounds);
	}
	if (b.isNull()) b = QRectF(0, 0, 100, 100);    // empty stack fallback
	return b;
}

QRectF GerberPreviewWidget::effectiveBounds() const
{
	// Bounds after the user rotate/flip is applied — needed so that
	// zoomToFit() sizes the visible footprint (e.g. a 100×65 board
	// rotated 90° becomes 65×100).
	if (m_userXform.isIdentity()) return m_worldBounds;
	return m_userXform.mapRect(m_worldBounds);
}

void GerberPreviewWidget::zoomToFit()
{
	const QRectF bounds = effectiveBounds();
	if (bounds.isNull() || width() <= 0 || height() <= 0) {
		m_world = QTransform();
		return;
	}
	const double margin = 0.05;
	const double w = bounds.width()  * (1.0 + 2 * margin);
	const double h = bounds.height() * (1.0 + 2 * margin);
	const double sx = width()  / w;
	const double sy = height() / h;
	const double s  = qMin(sx, sy);

	// Gerber Y goes UP; widget Y goes DOWN — flip Y.
	const QPointF c = bounds.center();
	QTransform t;
	t.translate(width() * 0.5, height() * 0.5);
	t.scale(s, -s);
	t.translate(-c.x(), -c.y());
	m_world = t;
}

void GerberPreviewWidget::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);
	zoomToFit();
}

void GerberPreviewWidget::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	// Paint the off-white board substrate on the canvas only (see ctor).
	p.fillRect(rect(), m_canvasColor);

	if (m_gerberLayers.isEmpty() && m_drillLayers.isEmpty()) {
		p.setPen(QColor(120, 120, 120));
		p.drawText(rect(), Qt::AlignCenter,
			tr("No Gerber files loaded.\n"
			   "Finish the wizard to generate a panel preview."));
		return;
	}

	p.setWorldTransform(m_userXform * m_world);

	// Fixed bottom-up composite order — visually mimics fab houses.
	static const LayerKind order[] = {
		BottomCopper, BottomMask, BottomSilk, BottomPaste,
		TopCopper,    TopMask,    TopSilk,    TopPaste,
		EdgeCuts
	};

	GerberRenderer renderer;
	for (LayerKind k : order) {
		for (const auto &gl : m_gerberLayers) {
			if (!gl.visible || gl.kind != k) continue;
			renderer.render(p, gl.doc, gl.color);
		}
	}
	// Unknown layers painted last in magenta so the user notices.
	for (const auto &gl : m_gerberLayers) {
		if (gl.visible && gl.kind == Unknown)
			renderer.render(p, gl.doc, gl.color);
	}
	// Drills are always last and on top.
	const QColor bg = m_canvasColor;
	for (const auto &dl : m_drillLayers) {
		if (!dl.visible) continue;
		renderer.renderDrills(p, dl.data, colorFor(Drill), bg);
	}

	// === DCode highlight pass ===
	// Re-render every command on the selected layer that shares the
	// selected D-code, with a thick high-contrast outline. Drawn in
	// world space (still under the current QPainter transform).
	if (m_selectedLayerIdx >= 0
	    && m_selectedLayerIdx < m_gerberLayers.size()
	    && m_selectedDcode   >= 0) {
		const GerberLayer &gl = m_gerberLayers[m_selectedLayerIdx];
		if (gl.visible) {
			auto apIt = gl.doc.apertures.constFind(m_selectedDcode);
			const QPainterPath apPath =
				(apIt != gl.doc.apertures.constEnd()) ? apIt.value().path()
				                                      : QPainterPath();
			const double strokeW =
				(apIt != gl.doc.apertures.constEnd()) ? apIt.value().strokeWidth()
				                                      : 0.1;

			// World-space pen widths: keep the highlight visible
			// regardless of zoom by sizing in world mm with a
			// modest minimum derived from the current scale.
			const double pxPerMm = std::hypot(m_world.m11(), m_world.m21());
			const double minMm = (pxPerMm > 0.0) ? (2.0 / pxPerMm) : 0.05;
			const double outlineW = qMax(minMm, strokeW * 0.15);
			QPen outlinePen(QColor(255, 255, 0, 230), outlineW);
			outlinePen.setCosmetic(false);
			outlinePen.setJoinStyle(Qt::RoundJoin);
			outlinePen.setCapStyle(Qt::RoundCap);

			p.save();
			p.setBrush(Qt::NoBrush);
			p.setPen(outlinePen);
			for (const GerberCommand &cmd : gl.doc.commands) {
				if (cmd.apertureCode != m_selectedDcode) continue;
				if (cmd.kind == GerberCommand::Flash) {
					p.drawPath(apPath.translated(cmd.b));
				} else if (cmd.kind == GerberCommand::Stroke) {
					QPen strokePen = outlinePen;
					// Trace the stroked centerline AND outline the
					// aperture-width band so thick traces are clear.
					strokePen.setWidthF(qMax(minMm, strokeW + minMm * 2));
					strokePen.setColor(QColor(255, 255, 0, 90));
					p.setPen(strokePen);
					p.drawLine(cmd.a, cmd.b);
					p.setPen(outlinePen);
					p.drawLine(cmd.a, cmd.b);
				} else if (cmd.kind == GerberCommand::Region) {
					p.drawPath(cmd.region);
				}
			}
			p.restore();
		}
	}

	// === Measurement overlay ===
	// Drawn in widget (pixel) coordinates so the line weight and the
	// readout label stay legible regardless of zoom level. We reset
	// the world transform here; the world points are projected back
	// to widget space via worldMmToWidget().
	if (m_measureActive) {
		p.resetTransform();
		const QPointF a = worldMmToWidget(m_measureStartMm);
		const QPointF b = worldMmToWidget(m_measureEndMm);

		// Bright yellow with a thin dark backing for contrast on any
		// substrate color the user may have picked.
		p.setRenderHint(QPainter::Antialiasing, true);
		p.setPen(QPen(QColor(0, 0, 0, 180), 3.0));
		p.drawLine(a, b);
		p.setPen(QPen(QColor(255, 220, 0), 1.4));
		p.drawLine(a, b);

		// Endpoint dots.
		p.setBrush(QColor(255, 220, 0));
		p.setPen(QPen(QColor(0, 0, 0, 200), 1.0));
		p.drawEllipse(a, 3.5, 3.5);
		p.drawEllipse(b, 3.5, 3.5);

		// Readout label.
		const QPointF d = m_measureEndMm - m_measureStartMm;
		const double distMm = std::hypot(d.x(), d.y());
		const double mmToIn = 1.0 / 25.4;
		const QString text = tr("%1 mm  (%2 in)\n\u0394x %3 mm  \u0394y %4 mm")
			.arg(distMm,    0, 'f', 3)
			.arg(distMm * mmToIn, 0, 'f', 4)
			.arg(d.x(),     0, 'f', 3)
			.arg(d.y(),     0, 'f', 3);

		QFont f = p.font();
		f.setPointSizeF(qMax(8.0, f.pointSizeF()));
		p.setFont(f);
		QFontMetrics fm(f);
		QRectF textRect = fm.boundingRect(QRect(0, 0, 600, 200),
			Qt::AlignLeft | Qt::TextDontClip, text);
		textRect.adjust(0, 0, 12, 8);
		// Place label just past the second endpoint, kept on-screen.
		QPointF anchor = b + QPointF(10, -textRect.height() - 6);
		if (anchor.x() + textRect.width() > width()) {
			anchor.setX(width() - textRect.width() - 4);
		}
		if (anchor.y() < 0) anchor.setY(b.y() + 12);
		textRect.moveTopLeft(anchor);

		p.setBrush(QColor(0, 0, 0, 190));
		p.setPen(QPen(QColor(255, 220, 0), 1.0));
		p.drawRoundedRect(textRect, 4, 4);
		p.setPen(Qt::white);
		p.drawText(textRect.adjusted(6, 4, -6, -4),
			Qt::AlignLeft | Qt::AlignVCenter, text);
	}

	// On-canvas legend / key, drawn last so it floats above everything.
	paintLegend(p);
}

bool GerberPreviewWidget::isLayerVisible(LayerKind kind) const
{
	if (kind == Drill) {
		for (const auto &dl : m_drillLayers) if (dl.visible) return true;
		return false;
	}
	for (const auto &gl : m_gerberLayers) {
		if (gl.kind == kind && gl.visible) return true;
	}
	return false;
}

QString GerberPreviewWidget::legendLabelFor(LayerKind kind)
{
	switch (kind) {
	case TopCopper:    return tr("Top Copper");
	case BottomCopper: return tr("Bottom Copper");
	case TopMask:      return tr("Top Mask");
	case BottomMask:   return tr("Bottom Mask");
	case TopSilk:      return tr("Top Silkscreen");
	case BottomSilk:   return tr("Bottom Silkscreen");
	case TopPaste:     return tr("Top Paste");
	case BottomPaste:  return tr("Bottom Paste");
	case EdgeCuts:     return tr("Board Outline");
	case Drill:        return tr("Drill");
	default:           return tr("Unknown");
	}
}

void GerberPreviewWidget::paintLegend(QPainter &p)
{
	m_legendHits.clear();
	if (!m_showLegend) return;

	const QVector<LayerKind> kinds = loadedKinds();
	if (kinds.isEmpty()) return;

	// Reset to widget space so the legend keeps a fixed size and corner
	// position regardless of the current zoom/pan/rotate transform.
	p.resetTransform();
	p.setRenderHint(QPainter::Antialiasing, true);

	// --- Geometry --------------------------------------------------
	QFont f = p.font();
	f.setPointSizeF(qMax(8.0, f.pointSizeF()));
	p.setFont(f);
	const QFontMetrics fm(f);

	const int pad    = 8;   // inner padding
	const int rowGap = 4;   // gap between rows
	const int swatch = 14;  // colour-square edge
	const int gap    = 8;   // swatch → label gap
	const int rowH   = qMax(swatch, fm.height());

	const QString title = tr("Layers");
	int textW = fm.horizontalAdvance(title);
	for (LayerKind k : kinds) {
		textW = qMax(textW, fm.horizontalAdvance(legendLabelFor(k)));
	}

	const int boxW = pad + swatch + gap + textW + pad;
	const int boxH = pad + fm.height() + rowGap
	               + kinds.size() * (rowH + rowGap) - rowGap + pad;

	const int margin = 10;
	const int x0 = qMax(margin, width() - boxW - margin);
	const int y0 = margin;
	const QRect box(x0, y0, boxW, boxH);

	// Background panel.
	p.setPen(QPen(QColor(0, 0, 0, 120), 1.0));
	p.setBrush(QColor(250, 250, 248, 230));
	p.drawRoundedRect(box, 6, 6);

	// Title.
	QFont tf = f; tf.setBold(true);
	p.setFont(tf);
	p.setPen(QColor(40, 40, 40));
	int y = y0 + pad;
	p.drawText(QRect(x0 + pad, y, boxW - 2 * pad, fm.height()),
	           Qt::AlignLeft | Qt::AlignVCenter, title);
	p.setFont(f);
	y += fm.height() + rowGap;

	// Rows: [swatch] [name]. Swatch click recolors, name click toggles.
	for (LayerKind k : kinds) {
		const QRect swatchRect(x0 + pad, y + (rowH - swatch) / 2, swatch, swatch);
		const QRect labelRect(swatchRect.right() + gap, y,
		                      x0 + boxW - pad - (swatchRect.right() + gap), rowH);

		const bool visible = isLayerVisible(k);
		const QColor col = layerColor(k);

		// Checker under the colour so alpha is visible, then frame.
		p.setPen(Qt::NoPen);
		p.setBrush(QColor(235, 235, 235));
		p.drawRect(swatchRect);
		p.setBrush(QColor(205, 205, 205));
		const int hs = swatch / 2;
		p.drawRect(swatchRect.left(), swatchRect.top(), hs, hs);
		p.drawRect(swatchRect.left() + hs, swatchRect.top() + hs, hs, hs);
		p.setBrush(col);
		p.drawRect(swatchRect);
		p.setBrush(Qt::NoBrush);
		p.setPen(QPen(visible ? QColor(40, 40, 40) : QColor(170, 170, 170), 1.0));
		p.drawRect(swatchRect);

		// Label, greyed + struck through when hidden.
		QFont lf = f; lf.setStrikeOut(!visible);
		p.setFont(lf);
		p.setPen(visible ? QColor(40, 40, 40) : QColor(150, 150, 150));
		p.drawText(labelRect, Qt::AlignLeft | Qt::AlignVCenter, legendLabelFor(k));
		p.setFont(f);

		m_legendHits.append({ k, swatchRect, labelRect });
		y += rowH + rowGap;
	}
}

bool GerberPreviewWidget::handleLegendPress(const QPoint &pos)
{
	for (const LegendHit &hit : m_legendHits) {
		if (hit.swatch.contains(pos)) {
			// Recolor via a picker seeded with the current colour;
			// keep the alpha channel so per-layer opacity is tunable.
			const QColor current = layerColor(hit.kind);
			const QColor picked = QColorDialog::getColor(
				current, this, tr("Choose layer color"),
				QColorDialog::ShowAlphaChannel);
			if (picked.isValid()) {
				setLayerColor(hit.kind, picked);
				emit legendColorChanged(hit.kind, picked);
				update();
			}
			return true;
		}
		if (hit.label.contains(pos)) {
			const bool newVis = !isLayerVisible(hit.kind);
			setLayerVisible(hit.kind, newVis);
			emit legendVisibilityToggled(hit.kind, newVis);
			update();
			return true;
		}
	}
	return false;
}

void GerberPreviewWidget::wheelEvent(QWheelEvent *event)
{
	const double zoom = (event->angleDelta().y() > 0) ? 1.15 : (1.0 / 1.15);
	const QPointF pos = event->position();

	// Zoom around the cursor: map cursor to world, scale, re-anchor.
	bool ok = false;
	const QTransform inv = m_world.inverted(&ok);
	if (!ok) return;
	const QPointF worldAt = inv.map(pos);

	QTransform t;
	t.translate(pos.x(), pos.y());
	t.scale(zoom, zoom);
	t.translate(-pos.x(), -pos.y());
	m_world = m_world * t;
	Q_UNUSED(worldAt);

	update();
	event->accept();
}

void GerberPreviewWidget::mousePressEvent(QMouseEvent *event)
{
	if (event->button() != Qt::LeftButton) return;

	// Legend interaction takes priority over pan/measure so the user can
	// always toggle visibility / recolor a layer regardless of the
	// currently-active tool.
	if (handleLegendPress(event->pos())) {
		event->accept();
		return;
	}

	if (m_tool == Tool::Measure) {
		// Capture the world-space (mm) anchor; reset the endpoint so the
		// next mouseMove (with or without button held — setMouseTracking
		// is on) starts producing a fresh measurement.
		m_measureStartMm = widgetToWorldMm(event->pos());
		m_measureEndMm   = m_measureStartMm;
		m_measureActive  = true;
		emit measurementChanged(0.0, 0.0, 0.0);
		update();
		return;
	}

	// Pan mode: remember both the running drag anchor (for live pan
	// deltas in mouseMove) AND the original press position so that
	// mouseRelease can distinguish a true click (release ~ press) from
	// a drag and run a hit-test only in the former case.
	m_dragLast = event->pos();
	m_pressPos = event->pos();
}

void GerberPreviewWidget::mouseMoveEvent(QMouseEvent *event)
{
	if (m_tool == Tool::Measure) {
		if (!m_measureActive) return;
		m_measureEndMm = widgetToWorldMm(event->pos());
		const QPointF d = m_measureEndMm - m_measureStartMm;
		const double dist = std::hypot(d.x(), d.y());
		emit measurementChanged(dist, d.x(), d.y());
		update();
		return;
	}

	if ((event->buttons() & Qt::LeftButton) && !m_dragLast.isNull()) {
		const QPoint delta = event->pos() - m_dragLast;
		m_dragLast = event->pos();
		QTransform t;
		t.translate(delta.x(), delta.y());
		m_world = m_world * t;
		update();
	}
}

void GerberPreviewWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		// Treat any release within a tiny pixel radius of the press as
		// a click — anything bigger was a pan/measurement drag.
		const bool wasClick = (m_tool == Tool::Pan)
			&& !m_pressPos.isNull()
			&& (event->pos() - m_pressPos).manhattanLength() <= 3;
		m_dragLast = QPoint();
		m_pressPos = QPoint();
		if (wasClick) {
			const QPointF worldMm = widgetToWorldMm(event->pos());
			hitTest(worldMm);
		}
	}
	// Measure-mode release keeps the line on screen on purpose —
	// matches gerbv/KiCad behavior where the user can read the value
	// after releasing. Press a new start (or hit Escape) to clear.
}

void GerberPreviewWidget::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Escape) {
		bool handled = false;
		if (m_measureActive)      { clearMeasurement();        handled = true; }
		if (m_selectedDcode >= 0) { clearApertureSelection();  handled = true; }
		if (handled) { event->accept(); return; }
	}
	QWidget::keyPressEvent(event);
}

void GerberPreviewWidget::setTool(Tool t)
{
	if (m_tool == t) return;
	m_tool = t;
	// Switching tools always clears any pending measurement — leaving
	// it on screen while the user pans would be visually confusing.
	clearMeasurement();
	// Selection (DCode highlight) is independent of tool mode; keep it
	// so the user can switch to Measure for a quick read without losing
	// the highlighted aperture.
	setCursor(t == Tool::Measure ? Qt::CrossCursor : Qt::ArrowCursor);
}

void GerberPreviewWidget::clearMeasurement()
{
	if (!m_measureActive) return;
	m_measureActive  = false;
	m_measureStartMm = QPointF();
	m_measureEndMm   = QPointF();
	emit measurementChanged(0.0, 0.0, 0.0);
	update();
}

QPointF GerberPreviewWidget::widgetToWorldMm(const QPoint &widget) const
{
	bool ok = false;
	const QTransform inv = (m_userXform * m_world).inverted(&ok);
	if (!ok) return QPointF();
	return inv.map(QPointF(widget));
}

QPointF GerberPreviewWidget::worldMmToWidget(const QPointF &worldMm) const
{
	return (m_userXform * m_world).map(worldMm);
}

void GerberPreviewWidget::clearApertureSelection()
{
	if (m_selectedDcode < 0 && m_selectedLayerIdx < 0) return;
	m_selectedLayerIdx = -1;
	m_selectedDcode    = -1;
	emit apertureSelected(-1, QString(), 0);
	update();
}

bool GerberPreviewWidget::hitTest(const QPointF &worldMm)
{
	// Walk visible Gerber layers in the same order paintEvent draws
	// them (bottom-up), reversed, so the topmost rendered layer wins.
	// Drills are intentionally excluded — they have no aperture table
	// in our schema and the user already gets visual confirmation from
	// the hole rendering.
	static const LayerKind order[] = {
		BottomCopper, BottomMask, BottomSilk, BottomPaste,
		TopCopper,    TopMask,    TopSilk,    TopPaste,
		EdgeCuts
	};

	for (int oi = int(sizeof(order)/sizeof(order[0])) - 1; oi >= 0; --oi) {
		const LayerKind k = order[oi];
		for (int li = 0; li < m_gerberLayers.size(); ++li) {
			const GerberLayer &gl = m_gerberLayers[li];
			if (!gl.visible || gl.kind != k) continue;
			// Scan commands in reverse so later-drawn (on-top) commands
			// win within a layer.
			for (int ci = gl.doc.commands.size() - 1; ci >= 0; --ci) {
				const GerberCommand &cmd = gl.doc.commands[ci];
				bool hit = false;
				if (cmd.kind == GerberCommand::Region) {
					hit = cmd.region.contains(worldMm);
				} else {
					auto apIt = gl.doc.apertures.constFind(cmd.apertureCode);
					if (apIt == gl.doc.apertures.constEnd()) continue;
					const GerberAperture &ap = apIt.value();
					if (cmd.kind == GerberCommand::Flash) {
						// Aperture path centred at (0,0); translate to flash pos.
						hit = ap.path().translated(cmd.b).contains(worldMm);
					} else { // Stroke
						QPainterPath line;
						line.moveTo(cmd.a);
						line.lineTo(cmd.b);
						QPainterPathStroker stroker;
						stroker.setWidth(ap.strokeWidth());
						stroker.setCapStyle(Qt::RoundCap);
						hit = stroker.createStroke(line).contains(worldMm);
					}
				}
				if (hit) {
					m_selectedLayerIdx = li;
					m_selectedDcode    = cmd.apertureCode;
					QString desc;
					int count = 0;
					auto apIt = gl.doc.apertures.constFind(cmd.apertureCode);
					if (apIt != gl.doc.apertures.constEnd()) {
						desc = apIt.value().description();
					}
					for (const auto &c2 : gl.doc.commands) {
						if (c2.apertureCode == cmd.apertureCode) ++count;
					}
					emit apertureSelected(cmd.apertureCode, desc, count);
					update();
					return true;
				}
			}
		}
	}

	// Miss: clear any previous selection so the user has an obvious
	// gesture (click empty space) for dismissing the highlight.
	clearApertureSelection();
	return false;
}
