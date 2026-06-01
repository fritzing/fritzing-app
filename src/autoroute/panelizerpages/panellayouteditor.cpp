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

#include "panellayouteditor.h"

#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QShowEvent>
#include <QWheelEvent>
#include <QtMath>

namespace {
// 1 scene unit == 1 mm. Inches <-> mm at the engine boundary only.
constexpr double kMmPerInch = 25.4;

// Board fill/stroke palette. Greenish "good", red "invalid", with a
// brighter selected state so the user can see which one the toolbar
// rotate/flip actions will hit.
const QColor kBoardFill     (150, 200, 150, 200);
const QColor kBoardFillSel  (120, 190, 255, 220);
const QColor kBoardStroke   ( 30,  90,  30);
const QColor kBoardStrokeSel ( 20,  90, 160);
const QColor kBoardInvalidFill  (235, 150, 150, 220);
const QColor kBoardInvalidStroke(170,  30,  30);

const QColor kPanelFill   (245, 245, 240);
const QColor kPanelStroke ( 40,  40,  40);
const QColor kUsableStroke(120, 120, 120);

// Alignment-guide colour + the pixel tolerance for grabbing one.
const QColor kGuideColor  (255, 140,   0, 220);
constexpr int kGuideGrabPx = 6;
} // namespace

// ======================================================================
// BoardPlacementItem
// ======================================================================

BoardPlacementItem::BoardPlacementItem(int index, const QString & label,
                                       double boardWmm, double boardHmm)
	: QGraphicsObject(nullptr)
	, m_index(index)
	, m_label(label)
	, m_boardWmm(boardWmm)
	, m_boardHmm(boardHmm)
	, m_rotationDegrees(0)
	, m_flippedHorizontal(false)
	, m_invalid(false)
{
	// Movable + selectable; we want geometry-change notifications so the
	// editor can snap + revalidate after every drag.
	setFlag(QGraphicsItem::ItemIsMovable, true);
	setFlag(QGraphicsItem::ItemIsSelectable, true);
	setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
	setCursor(Qt::OpenHandCursor);
	setZValue(1.0);
}

QSizeF BoardPlacementItem::effectiveSizeMm() const
{
	// 90/270 swap width and height; 0/180 keep them.
	const bool swap = (m_rotationDegrees == 90 || m_rotationDegrees == 270);
	return swap ? QSizeF(m_boardHmm, m_boardWmm) : QSizeF(m_boardWmm, m_boardHmm);
}

QRectF BoardPlacementItem::boundingRect() const
{
	// Local origin at the post-rotation footprint top-left so pos()
	// is always the footprint top-left. A 1 mm margin leaves room for
	// the selection stroke without clipping.
	const QSizeF s = effectiveSizeMm();
	return QRectF(-1.0, -1.0, s.width() + 2.0, s.height() + 2.0);
}

void BoardPlacementItem::paint(QPainter * painter,
                               const QStyleOptionGraphicsItem *, QWidget *)
{
	const QSizeF s = effectiveSizeMm();
	const QRectF body(0.0, 0.0, s.width(), s.height());

	const bool sel = isSelected();
	QColor fill   = m_invalid ? kBoardInvalidFill   : (sel ? kBoardFillSel   : kBoardFill);
	QColor stroke = m_invalid ? kBoardInvalidStroke : (sel ? kBoardStrokeSel : kBoardStroke);

	painter->setRenderHint(QPainter::Antialiasing, true);
	painter->setBrush(fill);
	QPen pen(stroke, sel ? 0.6 : 0.4);
	painter->setPen(pen);
	painter->drawRect(body);

	// Orientation marker: a small wedge in the corner that, on the
	// un-rotated board, sits at the top-left. It rotates with the board
	// so the user can read orientation at a glance, and a mirror flag
	// is shown as an "F".
	painter->save();
	// Move into the board centre and apply the board's own rotation +
	// optional horizontal mirror, so the wedge + label spin together.
	painter->translate(body.center());
	if (m_flippedHorizontal) painter->scale(-1.0, 1.0);
	painter->rotate(m_rotationDegrees);
	// Half-extents of the *un-rotated* board (because we rotated the
	// painter, not the geometry).
	const double hw = m_boardWmm / 2.0;
	const double hh = m_boardHmm / 2.0;
	// Corner wedge at the un-rotated top-left.
	const double wedge = qMin(qMin(hw, hh) * 0.5, 3.0);
	QPolygonF tri;
	tri << QPointF(-hw, -hh)
	    << QPointF(-hw + wedge, -hh)
	    << QPointF(-hw, -hh + wedge);
	painter->setBrush(stroke);
	painter->setPen(Qt::NoPen);
	painter->drawPolygon(tri);

	// Label, drawn upright relative to the (rotated) board.
	QFont f = painter->font();
	f.setPointSizeF(qMax(2.0, qMin(hw, hh) * 0.6));
	f.setBold(true);
	painter->setFont(f);
	painter->setPen(stroke);
	QString text = m_label;
	if (m_flippedHorizontal) {
		// Undo the mirror just for the glyphs so the number stays legible.
		painter->scale(-1.0, 1.0);
		text += QStringLiteral(" \u219F"); // up-down arrow hint = flipped
	}
	painter->drawText(QRectF(-hw, -hh, m_boardWmm, m_boardHmm),
	                  Qt::AlignCenter, text);
	painter->restore();
}

void BoardPlacementItem::rotateClockwise()
{
	prepareGeometryChange();
	m_rotationDegrees = (m_rotationDegrees + 90) % 360;
	update();
	emit geometryEdited();
}

void BoardPlacementItem::toggleFlipHorizontal()
{
	m_flippedHorizontal = !m_flippedHorizontal;
	update();
	emit geometryEdited();
}

void BoardPlacementItem::setInvalid(bool invalid)
{
	if (m_invalid == invalid) return;
	m_invalid = invalid;
	update();
}

QVariant BoardPlacementItem::itemChange(GraphicsItemChange change,
                                        const QVariant & value)
{
	if (change == ItemPositionHasChanged && scene() != nullptr) {
		emit geometryEdited();
	}
	return QGraphicsObject::itemChange(change, value);
}

// ======================================================================
// PanelLayoutEditor
// ======================================================================

PanelLayoutEditor::PanelLayoutEditor(QWidget * parent)
	: QGraphicsView(parent)
	, m_scene(new QGraphicsScene(this))
	, m_panelRect(nullptr)
	, m_usableRect(nullptr)
	, m_borderMm(5.0)
	, m_gridStepMm(1.0)
{
	setScene(m_scene);
	setRenderHint(QPainter::Antialiasing, true);
	setDragMode(QGraphicsView::RubberBandDrag);
	setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	// Y grows downward in panel-local coords, matching SVG/Gerber space.
	setBackgroundBrush(QColor(70, 72, 75));
}

PanelLayoutEditor::~PanelLayoutEditor() = default;

void PanelLayoutEditor::seed(const QSizeF & panelMm, double borderMm,
                             const QSizeF & boardMm,
                             const QList<QPointF> & seedTopLefts,
                             const QList<bool> & seedRotated90)
{
	m_panelMm  = panelMm;
	m_borderMm = borderMm;
	m_boardMm  = boardMm;
	m_seedTopLefts = seedTopLefts;
	m_seedRotated90 = seedRotated90;
	rebuildSeed();
}

void PanelLayoutEditor::rebuildSeed()
{
	m_scene->clear();
	m_boards.clear();
	m_panelRect = nullptr;
	m_usableRect = nullptr;

	// Panel substrate.
	m_panelRect = m_scene->addRect(QRectF(0, 0, m_panelMm.width(), m_panelMm.height()),
	                               QPen(kPanelStroke, 0.5), QBrush(kPanelFill));
	m_panelRect->setZValue(-2.0);

	// Usable area (panel minus border rails). Boards should stay inside.
	const QRectF usable(m_borderMm, m_borderMm,
	                    qMax(0.0, m_panelMm.width()  - 2 * m_borderMm),
	                    qMax(0.0, m_panelMm.height() - 2 * m_borderMm));
	QPen usablePen(kUsableStroke, 0.3, Qt::DashLine);
	m_usableRect = m_scene->addRect(usable, usablePen, Qt::NoBrush);
	m_usableRect->setZValue(-1.0);

	// One draggable board per seed entry.
	for (int i = 0; i < m_seedTopLefts.size(); ++i) {
		auto * b = new BoardPlacementItem(i, QString::number(i + 1),
		                                  m_boardMm.width(), m_boardMm.height());
		// Seed rotation: layout() only ever emits 0 or 90.
		if (i < m_seedRotated90.size() && m_seedRotated90[i]) {
			b->rotateClockwise();
		}
		m_scene->addItem(b);
		b->setPos(m_seedTopLefts[i]);
		connect(b, &BoardPlacementItem::geometryEdited,
		        this, &PanelLayoutEditor::onItemGeometryEdited);
		m_boards.append(b);
	}

	m_scene->setSceneRect(QRectF(-10, -10,
	                             m_panelMm.width() + 20, m_panelMm.height() + 20));

	// Centre the alignment guides on the fresh panel.
	m_vGuideMm = m_panelMm.width()  * 0.5;
	m_hGuideMm = m_panelMm.height() * 0.5;

	revalidate();
	// Defer the fit: at seed time (wizard initializePage) the view often
	// has no real size yet, so fitInView() would zoom to a degenerate
	// viewport and the panel would arrive microscopic. Re-enable auto-fit
	// and let showEvent/resizeEvent perform the fit once we are sized; if
	// we already have a usable viewport, fit right now.
	m_userZoomed = false;
	if (isVisible() && viewport()->width() > 2 && viewport()->height() > 2) {
		zoomToFit();
	}
	emit layoutChanged();
}

void PanelLayoutEditor::onItemGeometryEdited()
{
	auto * b = qobject_cast<BoardPlacementItem*>(sender());
	if (b != nullptr) snapItem(b);
	revalidate();
	emit layoutChanged();
}

void PanelLayoutEditor::snapItem(BoardPlacementItem * item)
{
	if (item == nullptr) return;
	// Re-entrancy guard: setPos() below re-enters via
	// itemChange(ItemPositionHasChanged) -> geometryEdited ->
	// onItemGeometryEdited -> snapItem. The snapped value is a fixed
	// point so it normally settles in one extra pass, but combining grid
	// + guide snapping on non-integer board sizes can oscillate by a
	// sub-pixel amount and recurse until the stack blows (observed as a
	// crash while dragging on large panels). A simple flag makes the
	// nested call a no-op and kills the recursion dead.
	if (m_snapping) return;
	m_snapping = true;

	// Snap the footprint top-left to the grid, then (if close) align the
	// nearest edge to an active alignment guide.
	const QPointF p = item->pos();
	double x = p.x();
	double y = p.y();

	if (m_gridStepMm > 0.0) {
		x = qRound(x / m_gridStepMm) * m_gridStepMm;
		y = qRound(y / m_gridStepMm) * m_gridStepMm;
	}

	// Guide snapping: pull whichever edge (top-left or bottom-right) is
	// nearest the guide onto it, within a tolerance scaled off the grid.
	if (m_guidesVisible) {
		const QSizeF s = item->effectiveSizeMm();
		const double tol = qMax(1.0, m_gridStepMm * 1.5);
		if (qAbs(x - m_vGuideMm) <= tol)               x = m_vGuideMm;
		else if (qAbs(x + s.width() - m_vGuideMm) <= tol) x = m_vGuideMm - s.width();
		if (qAbs(y - m_hGuideMm) <= tol)               y = m_hGuideMm;
		else if (qAbs(y + s.height() - m_hGuideMm) <= tol) y = m_hGuideMm - s.height();
	}

	if (!qFuzzyCompare(x, p.x()) || !qFuzzyCompare(y, p.y())) {
		item->setPos(x, y);
	}
	m_snapping = false;
}

void PanelLayoutEditor::revalidate()
{
	const QRectF usable = (m_usableRect != nullptr)
		? m_usableRect->rect() : QRectF(0, 0, m_panelMm.width(), m_panelMm.height());

	for (BoardPlacementItem * a : m_boards) {
		bool bad = false;
		const QRectF ar = a->sceneBoundingRect().adjusted(1, 1, -1, -1);
		// Out of the usable area?
		if (!usable.contains(ar)) bad = true;
		// Overlaps a sibling?
		if (!bad) {
			for (BoardPlacementItem * b : m_boards) {
				if (b == a) continue;
				const QRectF br = b->sceneBoundingRect().adjusted(1, 1, -1, -1);
				if (ar.intersects(br)) { bad = true; break; }
			}
		}
		a->setInvalid(bad);
	}
}

bool PanelLayoutEditor::isLayoutValid() const
{
	for (BoardPlacementItem * b : m_boards) {
		if (b->isInvalid()) return false;
	}
	return true;
}

BoardPlacementItem * PanelLayoutEditor::selectedBoard() const
{
	const QList<QGraphicsItem*> sel = m_scene->selectedItems();
	for (QGraphicsItem * gi : sel) {
		if (auto * b = qgraphicsitem_cast<BoardPlacementItem*>(gi)) return b;
	}
	// Fall back to the only board if there is just one.
	return (m_boards.size() == 1) ? m_boards.first() : nullptr;
}

void PanelLayoutEditor::rotateSelectedClockwise()
{
	for (QGraphicsItem * gi : m_scene->selectedItems()) {
		if (auto * b = qgraphicsitem_cast<BoardPlacementItem*>(gi)) {
			b->rotateClockwise();
			snapItem(b);
		}
	}
	if (m_scene->selectedItems().isEmpty() && m_boards.size() == 1) {
		m_boards.first()->rotateClockwise();
		snapItem(m_boards.first());
	}
	revalidate();
	emit layoutChanged();
}

void PanelLayoutEditor::flipSelectedHorizontal()
{
	for (QGraphicsItem * gi : m_scene->selectedItems()) {
		if (auto * b = qgraphicsitem_cast<BoardPlacementItem*>(gi)) {
			b->toggleFlipHorizontal();
		}
	}
	if (m_scene->selectedItems().isEmpty() && m_boards.size() == 1) {
		m_boards.first()->toggleFlipHorizontal();
	}
	emit layoutChanged();
}

void PanelLayoutEditor::autoArrange()
{
	rebuildSeed();
}

void PanelLayoutEditor::zoomToFit()
{
	if (m_scene->sceneRect().isEmpty()) return;
	fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
	// An explicit fit re-enables auto-fit on subsequent resizes; the user
	// can still take over again with the wheel.
	m_userZoomed = false;
}

void PanelLayoutEditor::setGuidesVisible(bool on)
{
	if (m_guidesVisible == on) return;
	m_guidesVisible = on;
	if (on) resetGuides();   // re-centre when (re)enabling
	if (viewport() != nullptr) viewport()->update();
}

void PanelLayoutEditor::resetGuides()
{
	m_vGuideMm = m_panelMm.width()  * 0.5;
	m_hGuideMm = m_panelMm.height() * 0.5;
	if (viewport() != nullptr) viewport()->update();
}

PanelLayoutEditor::GuideDrag
PanelLayoutEditor::guideHit(const QPoint & widgetPos) const
{
	if (!m_guidesVisible) return GuideDrag::None;
	// Project the guide's scene position to viewport pixels and compare.
	const QPoint vGuidePx = mapFromScene(QPointF(m_vGuideMm, m_hGuideMm));
	if (qAbs(widgetPos.x() - vGuidePx.x()) <= kGuideGrabPx) return GuideDrag::Vertical;
	if (qAbs(widgetPos.y() - vGuidePx.y()) <= kGuideGrabPx) return GuideDrag::Horizontal;
	return GuideDrag::None;
}

void PanelLayoutEditor::showEvent(QShowEvent * event)
{
	QGraphicsView::showEvent(event);
	// First real on-screen size — fit now unless the user already zoomed.
	if (!m_userZoomed) zoomToFit();
}

void PanelLayoutEditor::resizeEvent(QResizeEvent * event)
{
	QGraphicsView::resizeEvent(event);
	// Keep the panel framed as the wizard window grows/shrinks, unless the
	// user has taken manual control of the zoom.
	if (!m_userZoomed) zoomToFit();
}

QList<PanelLayoutEditor::Placement> PanelLayoutEditor::placements() const
{
	QList<Placement> out;
	out.reserve(m_boards.size());
	for (BoardPlacementItem * b : m_boards) {
		Placement p;
		p.topLeftInches      = b->topLeftMm() / kMmPerInch;
		p.rotationDegrees    = b->rotationDegrees();
		p.flippedHorizontal  = b->flippedHorizontal();
		out.append(p);
	}
	return out;
}

void PanelLayoutEditor::wheelEvent(QWheelEvent * event)
{
	// Ctrl+wheel zooms; plain wheel scrolls (default).
	if (event->modifiers().testFlag(Qt::ControlModifier)) {
		const double factor = (event->angleDelta().y() > 0) ? 1.15 : 1.0 / 1.15;
		scale(factor, factor);
		// The user has taken control of the zoom; stop auto-fitting on
		// resize so we don't snap their zoom away.
		m_userZoomed = true;
		event->accept();
		return;
	}
	QGraphicsView::wheelEvent(event);
}

void PanelLayoutEditor::mousePressEvent(QMouseEvent * event)
{
	// Grab an alignment guide only when the press is near a guide line
	// AND not on top of a draggable board, so board dragging always wins
	// the conflict (the user is far more likely to want to move a board).
	if (event->button() == Qt::LeftButton && m_guidesVisible
	    && itemAt(event->pos()) == nullptr) {
		const GuideDrag hit = guideHit(event->pos());
		if (hit != GuideDrag::None) {
			m_draggingGuide = hit;
			setCursor(hit == GuideDrag::Vertical ? Qt::SplitHCursor : Qt::SplitVCursor);
			event->accept();
			return;
		}
	}
	QGraphicsView::mousePressEvent(event);
}

void PanelLayoutEditor::mouseMoveEvent(QMouseEvent * event)
{
	if (m_draggingGuide != GuideDrag::None) {
		const QPointF scenePt = mapToScene(event->pos());
		if (m_draggingGuide == GuideDrag::Vertical) {
			// Clamp the guide to the panel so it can never wander off into
			// the grey margin where it would be useless for alignment.
			m_vGuideMm = qBound(0.0, scenePt.x(), m_panelMm.width());
		} else {
			m_hGuideMm = qBound(0.0, scenePt.y(), m_panelMm.height());
		}
		if (viewport() != nullptr) viewport()->update();
		event->accept();
		return;
	}
	// Hover feedback: show the split cursor when the pointer is over a
	// grabbable guide and no board is underneath.
	if (m_guidesVisible && (event->buttons() == Qt::NoButton)
	    && itemAt(event->pos()) == nullptr) {
		const GuideDrag hit = guideHit(event->pos());
		if (hit == GuideDrag::Vertical)        setCursor(Qt::SplitHCursor);
		else if (hit == GuideDrag::Horizontal) setCursor(Qt::SplitVCursor);
		else                                   unsetCursor();
	}
	QGraphicsView::mouseMoveEvent(event);
}

void PanelLayoutEditor::mouseReleaseEvent(QMouseEvent * event)
{
	if (m_draggingGuide != GuideDrag::None) {
		m_draggingGuide = GuideDrag::None;
		unsetCursor();
		// Re-snap boards now that the guide has settled so any board that
		// was sitting near the new guide line clicks onto it immediately.
		for (BoardPlacementItem * b : m_boards) snapItem(b);
		revalidate();
		emit layoutChanged();
		event->accept();
		return;
	}
	QGraphicsView::mouseReleaseEvent(event);
}

void PanelLayoutEditor::keyPressEvent(QKeyEvent * event)
{
	switch (event->key()) {
	case Qt::Key_R:
		rotateSelectedClockwise();
		event->accept();
		return;
	case Qt::Key_F:
		flipSelectedHorizontal();
		event->accept();
		return;
	default:
		break;
	}
	QGraphicsView::keyPressEvent(event);
}

void PanelLayoutEditor::drawBackground(QPainter * painter, const QRectF & rect)
{
	QGraphicsView::drawBackground(painter, rect);
	if (m_gridStepMm <= 0.0 || m_panelMm.isEmpty()) return;

	// Light grid over the panel only, so the user can eyeball spacing.
	// Skip if the grid would be denser than ~3 px to avoid a grey wash.
	const double pxPerMm = transform().m11();
	const double majorStep = m_gridStepMm * (m_gridStepMm * pxPerMm < 4.0 ? 10.0 : 1.0);
	if (majorStep * pxPerMm < 4.0) return;

	const QRectF panel(0, 0, m_panelMm.width(), m_panelMm.height());
	const QRectF area = rect.intersected(panel);
	if (area.isEmpty()) return;

	QPen grid(QColor(0, 0, 0, 30), 0.0);
	painter->setPen(grid);
	for (double x = 0; x <= m_panelMm.width() + 0.001; x += majorStep) {
		painter->drawLine(QPointF(x, 0), QPointF(x, m_panelMm.height()));
	}
	for (double y = 0; y <= m_panelMm.height() + 0.001; y += majorStep) {
		painter->drawLine(QPointF(0, y), QPointF(m_panelMm.width(), y));
	}
}

void PanelLayoutEditor::drawForeground(QPainter * painter, const QRectF & rect)
{
	QGraphicsView::drawForeground(painter, rect);
	if (!m_guidesVisible || m_panelMm.isEmpty()) return;

	// Draw the alignment guides on top of everything. Cosmetic pen so the
	// line stays 1.5 px wide at any zoom; a dash pattern distinguishes the
	// guides from board strokes.
	painter->save();
	QPen gp(kGuideColor, 1.5, Qt::DashLine);
	gp.setCosmetic(true);
	painter->setPen(gp);
	// Span the visible area so the guide reads as a full-length bar even
	// when the user has scrolled the panel partly out of view.
	painter->drawLine(QPointF(m_vGuideMm, rect.top()),
	                  QPointF(m_vGuideMm, rect.bottom()));
	painter->drawLine(QPointF(rect.left(),  m_hGuideMm),
	                  QPointF(rect.right(), m_hGuideMm));

	// Solid grab handles at the panel's top/left edge so the draggable
	// bars are discoverable. Drawn in device space so they keep a fixed
	// pixel size regardless of zoom.
	painter->restore();
	painter->save();
	painter->resetTransform();
	const QPoint vTop = mapFromScene(QPointF(m_vGuideMm, 0.0));
	const QPoint hLeft = mapFromScene(QPointF(0.0, m_hGuideMm));
	painter->setRenderHint(QPainter::Antialiasing, true);
	painter->setPen(Qt::NoPen);
	painter->setBrush(kGuideColor);
	// Down-pointing tab for the vertical guide.
	QPolygon vTab;
	vTab << QPoint(vTop.x() - 5, 0) << QPoint(vTop.x() + 5, 0) << QPoint(vTop.x(), 9);
	painter->drawPolygon(vTab);
	// Right-pointing tab for the horizontal guide.
	QPolygon hTab;
	hTab << QPoint(0, hLeft.y() - 5) << QPoint(0, hLeft.y() + 5) << QPoint(9, hLeft.y());
	painter->drawPolygon(hTab);
	painter->restore();
}
