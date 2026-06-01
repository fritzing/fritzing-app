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

#ifndef LAYERMANAGERWIDGET_H
#define LAYERMANAGERWIDGET_H

#include "gerberpreviewwidget.h"

#include <QColor>
#include <QHash>
#include <QVector>
#include <QWidget>

class QCheckBox;
class QGridLayout;
class QLabel;
class QSlider;
class QToolButton;

/**
 * @brief Per-layer manager docked next to GerberPreviewWidget.
 *
 * One row per loaded layer kind:
 *   [✓ visibility]  [■ color swatch]  layer-name  [opacity slider]
 *
 * Clicking the swatch opens a QColorDialog (preserving the current
 * alpha). Adjusting the opacity slider re-emits the swatch color
 * with a new alpha component. Right-clicking a row opens a context
 * menu with "Show only this layer / Show all / Hide all".
 *
 * The widget is purely a controller — it never touches the renderer
 * directly. The owning dialog wires its signals up to
 * GerberPreviewWidget::setLayerVisible() / setLayerColor().
 *
 * Structure inspired by KiCad's widgets/layer_widget.cpp (GPL-3,
 * Jean-Pierre Charras + contributors). No code is copied; the
 * row-per-layer + swatch + slider pattern is.
 * See docs/design/gerbview-port-reference.md §2 Phase 1.
 */
class LayerManagerWidget : public QWidget {
	Q_OBJECT

public:
	explicit LayerManagerWidget(QWidget *parent = nullptr);
	~LayerManagerWidget() override;

	/**
	 * @brief Rebuild the row stack for the given set of layer kinds.
	 * @param kinds   The kinds to expose, in display order.
	 * @param colors  Initial color (with alpha) per kind. Missing
	 *                entries fall back to the renderer default.
	 */
	void setLayers(const QVector<GerberPreviewWidget::LayerKind> &kinds,
	               const QHash<GerberPreviewWidget::LayerKind, QColor> &colors);

	/// @return current visibility map; useful for QSettings persistence.
	QHash<GerberPreviewWidget::LayerKind, bool> visibilityMap() const;
	/// @return current color map (alpha = opacity).
	QHash<GerberPreviewWidget::LayerKind, QColor> colorMap() const;

	/**
	 * @brief Set initial visibility from QSettings without re-emitting.
	 *        Only applies to kinds already present in the manager.
	 */
	void setInitialVisibility(GerberPreviewWidget::LayerKind kind, bool on);

	/**
	 * @brief Update a row's swatch color without re-emitting
	 *        colorChanged. Used to keep the side panel in sync when the
	 *        user recolors a layer from the on-canvas legend. No-op if
	 *        the kind is not currently shown.
	 */
	void setRowColor(GerberPreviewWidget::LayerKind kind, const QColor &color);

signals:
	void visibilityChanged(GerberPreviewWidget::LayerKind kind, bool on);
	void colorChanged(GerberPreviewWidget::LayerKind kind, const QColor &color);

private slots:
	void onSwatchClicked();
	void onOpacityChanged(int alpha);
	void onVisibilityToggled(bool on);
	void onContextMenu(const QPoint &pos);

private:
	struct Row {
		GerberPreviewWidget::LayerKind kind;
		QCheckBox   *visible;
		QToolButton *swatch;
		QLabel      *name;
		QSlider     *opacity;
	};

	void clearRows();
	void appendRow(GerberPreviewWidget::LayerKind kind, const QColor &color);
	void paintSwatch(QToolButton *btn, const QColor &color);
	void emitShowOnly(GerberPreviewWidget::LayerKind only);
	void emitShowAll(bool on);
	static QString labelFor(GerberPreviewWidget::LayerKind kind);

	QGridLayout      *m_grid;
	QVector<Row>      m_rows;
};

#endif // LAYERMANAGERWIDGET_H
