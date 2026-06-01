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

#ifndef PANELLAYOUTEDITOR_H
#define PANELLAYOUTEDITOR_H

#include <QGraphicsView>
#include <QGraphicsObject>
#include <QList>
#include <QSizeF>
#include <QString>
#include <QPointF>

class QGraphicsScene;
class QGraphicsRectItem;

/**
 * @brief One board instance the user can drag, rotate, and flip inside
 *        the panel.
 *
 * The item lives in *scene* coordinates, where 1 scene unit == 1
 * millimetre (we deliberately pick mm so the on-screen ruler and the
 * spacing math read naturally; the editor converts to/from inches at
 * the engine boundary because PanelizerEngine is inch-native — see
 * AGENTS.md §10 on the unit footgun).
 *
 * The board's *un-rotated* footprint is (m_boardWmm x m_boardHmm). The
 * item draws itself with its local origin at the top-left of the
 * un-rotated footprint and applies rotation/flip via setRotation() +
 * a horizontal mirror on the painter, so the scene bounding rect always
 * reflects the post-transform footprint. That keeps drag math and
 * collision tests honest regardless of orientation.
 *
 * @note Not a QObject-with-signals on purpose: the parent
 *       PanelLayoutEditor observes geometry changes through
 *       itemChange() callbacks routed back via a raw back-pointer, so
 *       we avoid per-item signal overhead while dragging dozens of
 *       boards.
 */
class BoardPlacementItem : public QGraphicsObject
{
	Q_OBJECT

public:
	/**
	 * @brief Construct a placement.
	 * @param index   Stable index into the editor's placement list.
	 * @param label   Human label drawn on the board (e.g. "1", "R2").
	 * @param boardWmm Un-rotated board width in millimetres (> 0).
	 * @param boardHmm Un-rotated board height in millimetres (> 0).
	 */
	BoardPlacementItem(int index, const QString & label,
	                   double boardWmm, double boardHmm);

	enum { Type = UserType + 1 };
	int type() const override { return Type; }

	QRectF boundingRect() const override;
	void paint(QPainter * painter, const QStyleOptionGraphicsItem * option,
	           QWidget * widget) override;

	/// Rotation in degrees, normalised to one of {0, 90, 180, 270}.
	int rotationDegrees() const { return m_rotationDegrees; }
	/// True when the board is mirrored left-to-right (bottom-of-panel flip).
	bool flippedHorizontal() const { return m_flippedHorizontal; }

	/// Cycle rotation by +90 (clockwise) and keep the top-left anchored.
	void rotateClockwise();
	/// Toggle the horizontal mirror.
	void toggleFlipHorizontal();

	/// Un-rotated footprint, millimetres.
	QSizeF unrotatedSizeMm() const { return QSizeF(m_boardWmm, m_boardHmm); }
	/// Post-rotation footprint (width/height swap on 90/270), millimetres.
	QSizeF effectiveSizeMm() const;

	/// Top-left of the post-rotation footprint, scene (mm) coordinates.
	QPointF topLeftMm() const { return pos(); }

	/// Paint the board red while it overlaps a sibling or leaves the panel.
	void setInvalid(bool invalid);
	bool isInvalid() const { return m_invalid; }

	int placementIndex() const { return m_index; }

signals:
	/// Emitted after any geometry change (move/rotate/flip) settles.
	void geometryEdited();

protected:
	QVariant itemChange(GraphicsItemChange change, const QVariant & value) override;

private:
	int     m_index;
	QString m_label;
	double  m_boardWmm;
	double  m_boardHmm;
	int     m_rotationDegrees;     // 0/90/180/270
	bool    m_flippedHorizontal;
	bool    m_invalid;
};

/**
 * @brief Interactive panel arrangement surface.
 *
 * Shows the panel outline (with rail/border inset) and a draggable
 * board item per copy. The user repositions each board, rotates it in
 * 90° steps, and flips it independently — exactly the KiCad-style
 * arrange step that was missing from the wizard. The editor seeds its
 * layout from the engine's auto-placement but lets the user override
 * every position and orientation afterwards.
 *
 * Coordinates: scene units are millimetres. Inches are converted only
 * at seedFromInches()/placements() — the PanelizerEngine boundary.
 */
class PanelLayoutEditor : public QGraphicsView
{
	Q_OBJECT

public:
	/// One board's final placement, expressed in inches for the engine.
	struct Placement {
		QPointF topLeftInches;   // post-rotation footprint top-left, panel-local
		int     rotationDegrees; // 0/90/180/270
		bool    flippedHorizontal;
	};

	explicit PanelLayoutEditor(QWidget * parent = nullptr);
	~PanelLayoutEditor() override;

	/**
	 * @brief (Re)build the scene from an auto-layout result.
	 * @param panelMm        Outer panel size in millimetres.
	 * @param borderMm       Rail/border inset on every edge, millimetres.
	 * @param boardMm        Un-rotated board footprint, millimetres.
	 * @param seedTopLefts   Per-copy top-left positions (mm) from layout().
	 * @param seedRotated90  Per-copy 90°-rotation flags from layout().
	 *
	 * seedTopLefts and seedRotated90 must be the same length; that length
	 * is the number of board copies placed.
	 */
	void seed(const QSizeF & panelMm, double borderMm, const QSizeF & boardMm,
	          const QList<QPointF> & seedTopLefts, const QList<bool> & seedRotated90);

	/// Grid step for drag snapping, millimetres (<= 0 disables snapping).
	void setGridStepMm(double mm) { m_gridStepMm = mm; }
	double gridStepMm() const { return m_gridStepMm; }

	/// Current placements, inches, panel-local — ready for emitPanel().
	QList<Placement> placements() const;

	/// True when no board overlaps another and all stay inside the panel.
	bool isLayoutValid() const;

public slots:
	void rotateSelectedClockwise();
	void flipSelectedHorizontal();
	void autoArrange();          // re-seed from the last engine layout
	void zoomToFit();

	/// Show/hide the draggable alignment guides and reset them to the
	/// panel centre. Boards snap their nearest edge to an active guide.
	void setGuidesVisible(bool on);
	/// Re-centre both guides on the panel without changing visibility.
	void resetGuides();

signals:
	/// Emitted whenever validity may have changed (drag/rotate/flip).
	void layoutChanged();

protected:
	void wheelEvent(QWheelEvent * event) override;
	void keyPressEvent(QKeyEvent * event) override;
	void mousePressEvent(QMouseEvent * event) override;
	void mouseMoveEvent(QMouseEvent * event) override;
	void mouseReleaseEvent(QMouseEvent * event) override;
	void showEvent(QShowEvent * event) override;
	void resizeEvent(QResizeEvent * event) override;
	void drawBackground(QPainter * painter, const QRectF & rect) override;
	void drawForeground(QPainter * painter, const QRectF & rect) override;

private slots:
	void onItemGeometryEdited();

private:
	BoardPlacementItem * selectedBoard() const;
	void snapItem(BoardPlacementItem * item);
	void revalidate();
	void rebuildSeed();

	/// Which guide (if any) the user is currently dragging.
	enum class GuideDrag { None, Vertical, Horizontal };
	/// Pixel distance from @p widgetPos to a guide; returns which guide
	/// is within grab tolerance (None if neither).
	GuideDrag guideHit(const QPoint & widgetPos) const;

	QGraphicsScene *   m_scene;
	QGraphicsRectItem * m_panelRect;
	QGraphicsRectItem * m_usableRect;   // panel minus border
	QList<BoardPlacementItem*> m_boards;

	QSizeF m_panelMm;
	double m_borderMm;
	QSizeF m_boardMm;
	double m_gridStepMm;

	// Cached seed so autoArrange() can restore the engine layout.
	QList<QPointF> m_seedTopLefts;
	QList<bool>    m_seedRotated90;

	// Deferred-fit state: the seed runs before the view has its final
	// on-screen size, so a fitInView() there zooms to a 1×1 viewport and
	// the panel arrives microscopic. Instead we mark the view "dirty" and
	// fit on the next showEvent/resizeEvent — but only while the user has
	// not taken manual control of the zoom (wheel), so we never fight a
	// deliberate zoom-in.
	bool m_userZoomed = false;

	// Draggable alignment guides (one vertical, one horizontal). Scene
	// (mm) coordinates. Boards snap their nearest edge to an active
	// guide when dropped within tolerance.
	bool      m_guidesVisible = true;
	double    m_vGuideMm = 0.0;
	double    m_hGuideMm = 0.0;
	GuideDrag m_draggingGuide = GuideDrag::None;
	// Guards snapItem() against unbounded re-entrancy (see cpp).
	bool      m_snapping = false;
};

#endif // PANELLAYOUTEDITOR_H
