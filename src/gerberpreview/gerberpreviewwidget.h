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

#ifndef GERBERPREVIEWWIDGET_H
#define GERBERPREVIEWWIDGET_H

#include "excellonparser.h"
#include "gerberdocument.h"

#include <QColor>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <QTransform>
#include <QVector>
#include <QWidget>

/**
 * @brief Multi-layer Gerber + Excellon viewer.
 *
 * Drop-in replacement for the QGraphicsView placeholder previously
 * used by PanelPreviewPage. Classifies files by filename suffix,
 * parses each in-process via GerberParser / ExcellonParser, and
 * composites them with the fab-house colour palette documented in
 * homerun-gui.md §12.1.4.
 *
 * Zoom: mouse wheel (around cursor).
 * Pan:  left-button drag.
 * Fit:  zoomToFit() (called automatically by loadFiles()).
 */
class GerberPreviewWidget : public QWidget {
	Q_OBJECT

public:
	enum LayerKind {
		TopCopper, BottomCopper,
		TopMask,   BottomMask,
		TopSilk,   BottomSilk,
		TopPaste,  BottomPaste,
		EdgeCuts,
		Drill,
		Unknown
	};
	Q_ENUM(LayerKind)

	explicit GerberPreviewWidget(QWidget *parent = nullptr);
	~GerberPreviewWidget() override;

	/**
	 * @brief Replace the current layer stack with @p paths. Files
	 *        are classified by filename suffix (`-F_Cu.gbr` →
	 *        TopCopper, `.drl` → Drill, etc.). Composites in a
	 *        fixed bottom-up order for visual sanity.
	 */
	void loadFiles(const QStringList &paths);

	/// Show or hide one layer kind. Triggers a repaint.
	void setLayerVisible(LayerKind kind, bool on);

	/**
	 * @brief Override the render color (including alpha for opacity)
	 *        for every loaded layer of @p kind. No-op if no layer of
	 *        that kind is loaded. Triggers a repaint.
	 */
	void setLayerColor(LayerKind kind, const QColor &color);

	/**
	 * @brief Current render color for @p kind. If a layer of that
	 *        kind is loaded the live override is returned; otherwise
	 *        the palette default from colorFor().
	 */
	QColor layerColor(LayerKind kind) const;

	/**
	 * @brief List of layer kinds currently represented in the loaded
	 *        stack, in fixed bottom-up paint order (BottomCopper first,
	 *        Drill last). Used by the layer-manager widget to build
	 *        one row per actually-loaded layer.
	 */
	QVector<LayerKind> loadedKinds() const;

	/// Apply a 90° clockwise rotation to the view (around bounds center).
	void rotate90();
	/// Flip view horizontally around the bounds center.
	void flipHorizontal();
	/// Flip view vertically around the bounds center.
	void flipVertical();
	/// Reset rotation + flip (does not change zoom/pan).
	void resetViewTransform();

	/// Recompute the world→widget transform so the union of all
	/// loaded layers' bounds fits with a small margin.
	void zoomToFit();

	/// Number of currently loaded layers (Drill counts as one).
	int  layerCount() const;

	/**
	 * @brief Active interaction tool. Pan is the default. Measure
	 *        replaces left-drag panning with a rubber-band ruler
	 *        that reports distance/dx/dy in mm and inches.
	 */
	enum class Tool { Pan, Measure };
	Q_ENUM(Tool)

	/// Switch the active interaction tool. Clears any pending
	/// measurement when leaving Measure mode.
	void setTool(Tool t);
	Tool tool() const { return m_tool; }

	/// Clear the on-screen measurement line, if any.
	void clearMeasurement();

	/// Clear the highlighted aperture selection, if any.
	void clearApertureSelection();

signals:
	void loadFinished(int layerCount, const QStringList &warnings);

	/**
	 * @brief Emitted whenever the measurement ruler updates.
	 * @param distMm  Euclidean distance in millimeters; 0 if no
	 *                measurement is active.
	 * @param dxMm    Signed delta x in millimeters.
	 * @param dyMm    Signed delta y in millimeters.
	 */
	void measurementChanged(double distMm, double dxMm, double dyMm);

	/**
	 * @brief Emitted when the user clicks (without panning) on an
	 *        aperture flash/stroke. @p dcode is the D-code of the
	 *        clicked aperture; @p description is human-readable
	 *        (see GerberAperture::description); @p count is how many
	 *        commands on the same layer share that D-code.
	 *        Emitted with dcode=-1 to signal that selection was
	 *        cleared (e.g. user clicked empty space or pressed Esc).
	 */
	void apertureSelected(int dcode, const QString &description, int count);

	/**
	 * @brief Emitted when the user toggles a layer's visibility from
	 *        the on-canvas legend (the corner key). The owning dialog /
	 *        wizard page listens to this so it can persist the choice
	 *        and keep the side layer-manager checkbox in sync.
	 */
	void legendVisibilityToggled(LayerKind kind, bool visible);

	/**
	 * @brief Emitted when the user recolors a layer by clicking its
	 *        swatch in the on-canvas legend. The owning dialog / wizard
	 *        page persists the colour and updates the side manager.
	 */
	void legendColorChanged(LayerKind kind, const QColor &color);

protected:
	void paintEvent(QPaintEvent *event)         override;
	void wheelEvent(QWheelEvent *event)         override;
	void mousePressEvent(QMouseEvent *event)    override;
	void mouseMoveEvent(QMouseEvent *event)     override;
	void mouseReleaseEvent(QMouseEvent *event)  override;
	void keyPressEvent(QKeyEvent *event)        override;
	void resizeEvent(QResizeEvent *event)       override;

private:
	struct GerberLayer {
		LayerKind      kind     = Unknown;
		QString        path;
		GerberDocument doc;
		bool           visible  = true;
		QColor         color;
	};
	struct DrillLayer {
		QString                  path;
		ExcellonParser::Result   data;
		bool                     visible = true;
	};

	QVector<GerberLayer> m_gerberLayers;
	QVector<DrillLayer>  m_drillLayers;

	QTransform           m_world;     ///< mm → widget pixels
	QTransform           m_userXform; ///< user rotate/flip in mm space, around bounds center
	QPoint               m_dragLast;
	QRectF               m_worldBounds; ///< Union of every layer's bounds (mm)
	QStringList          m_warnings;

	/// Off-white board substrate painted on the drawing canvas only.
	/// Kept separate from the widget palette so it never leaks onto the
	/// surrounding dialog/window chrome (dark-theme readability fix).
	QColor               m_canvasColor;

	// Measure-tool state. World coords are in mm (pre-userXform space
	// is fine because m_userXform is identity for a fresh measurement
	// session that doesn't outlive a flip/rotate — clearMeasurement()
	// is called by the tool transitions that would invalidate it).
	Tool                 m_tool         = Tool::Pan;
	bool                 m_measureActive = false; ///< user picked a start point
	QPointF              m_measureStartMm;
	QPointF              m_measureEndMm;

	// Hit-test / DCode-highlight state. Pan-mode left-click without
	// drag picks the topmost flash/stroke/region under the cursor and
	// remembers (layer-index, aperture-code) so paintEvent can overlay
	// a highlight pass on every command of the same D-code.
	QPoint               m_pressPos;
	int                  m_selectedLayerIdx = -1;
	int                  m_selectedDcode    = -1;

	// On-canvas legend ("key"). Drawn in widget space in a corner, it
	// lists every loaded layer with a clickable colour swatch (recolor)
	// and a clickable name (toggle visibility). The hit rectangles are
	// recomputed every paint so mouse handling stays in lockstep with
	// what is actually on screen.
	struct LegendHit {
		LayerKind kind;
		QRect     swatch; ///< click → recolor
		QRect     label;  ///< click → toggle visibility
	};
	bool                 m_showLegend = true;
	QVector<LegendHit>   m_legendHits;

	static LayerKind classify(const QString &filename);
	static QColor   colorFor(LayerKind kind);

	/// Short human-readable name for a layer kind, used by the legend.
	static QString  legendLabelFor(LayerKind kind);

	/// True if at least one loaded layer of @p kind is currently visible.
	bool            isLayerVisible(LayerKind kind) const;

	/// Draw the on-canvas legend in a corner and refresh m_legendHits.
	/// Must be called with the painter's transform reset to widget space.
	void            paintLegend(QPainter &p);

	/// Handle a left-button press at widget-space @p pos against the
	/// legend. Returns true (and acts) if the press hit a swatch or a
	/// label, so the caller can swallow the event.
	bool            handleLegendPress(const QPoint &pos);

	QRectF          computeBounds() const;
	QRectF          effectiveBounds() const;   ///< m_userXform applied to m_worldBounds
	void            composeUserXform(const QTransform &op);

	/// Maps a widget-space point to world (mm) coords using the
	/// combined m_userXform * m_world transform. Returns (0,0) if the
	/// combined transform isn't invertible (degenerate viewport).
	QPointF         widgetToWorldMm(const QPoint &widget) const;
	/// Inverse of widgetToWorldMm.
	QPointF         worldMmToWidget(const QPointF &worldMm) const;

	/// Hit-test a world-mm point against all visible Gerber layers,
	/// topmost first. Updates m_selectedLayerIdx/m_selectedDcode and
	/// emits apertureSelected. Returns true if something was hit.
	bool            hitTest(const QPointF &worldMm);
};

#endif // GERBERPREVIEWWIDGET_H
