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

#include "panelcandidatepage.h"

#include "../panelizerengine.h"

#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QSizeF>
#include <QVBoxLayout>
#include <QWizard>

namespace {
constexpr int kIconWidth  = 220;
constexpr int kIconHeight = 160;

const QColor kPanelOutline(40, 40, 40);
const QColor kPanelFill   (245, 245, 240);
const QColor kBoardOutlineFit(  0, 110,   0);
const QColor kBoardFillFit  (180, 230, 180, 200);
const QColor kBoardOutlinePartial(180, 130,   0);
const QColor kBoardFillPartial  (255, 230, 170, 200);
}

PanelCandidatePage::PanelCandidatePage(QWidget *parent)
	: QWizardPage(parent)
	, m_list(nullptr)
	, m_detail(nullptr)
	, m_borderInches(0.2)
	, m_gutterInches(0.08)
	, m_requestedCopies(1)
	, m_allowRotate90(true)
	, m_selectedIndex(-1)
{
	setTitle(tr("Pick a Panel"));
	setSubTitle(tr("Each thumbnail shows how your boards lay out on a candidate panel size. "
	               "Green = all copies fit. Amber = partial. Click one to use it."));

	m_list = new QListWidget(this);
	m_list->setViewMode(QListView::IconMode);
	m_list->setIconSize(QSize(kIconWidth, kIconHeight));
	m_list->setResizeMode(QListView::Adjust);
	m_list->setMovement(QListView::Static);
	m_list->setSpacing(8);
	m_list->setWordWrap(true);
	m_list->setUniformItemSizes(true);
	m_list->setGridSize(QSize(kIconWidth + 28, kIconHeight + 60));
	m_list->setSelectionMode(QAbstractItemView::SingleSelection);
	connect(m_list, &QListWidget::itemSelectionChanged,
	        this, &PanelCandidatePage::onItemSelected);

	m_detail = new QLabel(tr("(no selection)"), this);
	m_detail->setWordWrap(true);

	auto * outer = new QVBoxLayout(this);
	outer->addWidget(m_list, 1);
	outer->addWidget(m_detail);
}

PanelCandidatePage::~PanelCandidatePage() = default;

bool PanelCandidatePage::isComplete() const
{
	return hasSelection();
}

QSizeF PanelCandidatePage::selectedPanelMm() const
{
	if (!hasSelection() || m_selectedIndex >= m_candidates.size()) return QSizeF();
	const auto &p = m_candidates[m_selectedIndex].preset;
	return QSizeF(p.widthMm, p.heightMm);
}

void PanelCandidatePage::initializePage()
{
	// Re-read inputs from wizard fields so a Back+forward cycle picks
	// up any edits on the previous pages without the wizard having to
	// hand-push them in.
	if (wizard() != nullptr) {
		m_requestedCopies = qMax(1, wizard()->field("panel.copies").toInt());
		const double mmToIn = 1.0 / 25.4;
		m_borderInches = wizard()->field("panel.border").toDouble() * mmToIn;
		m_gutterInches = wizard()->field("panel.gutter").toDouble() * mmToIn;
		m_allowRotate90 = wizard()->field("panel.allowRotate").toBool();
	}
	buildCandidates();
}

void PanelCandidatePage::buildCandidates()
{
	m_list->clear();
	m_candidates.clear();
	m_selectedIndex = -1;

	if (m_boardSizeInches.isEmpty() || m_requestedCopies <= 0) {
		m_detail->setText(tr("Board size not available yet."));
		emit completeChanged();
		return;
	}

	// Build a single SourceBoard with the requested copies; we re-use
	// the same shape across every preset trial.
	PanelizerEngine::SourceBoard sb;
	sb.fzzPath         = QString();
	sb.openWindow      = nullptr;
	sb.copies          = m_requestedCopies;
	sb.allowRotate90   = m_allowRotate90;
	sb.boardSizeInches = m_boardSizeInches;
	QList<PanelizerEngine::SourceBoard> sources;
	sources.append(sb);

	const double mmToIn = 1.0 / 25.4;
	int firstFitIndex = -1;
	const auto presets = PanelPresets::list();

	for (int i = 0; i < presets.size(); ++i) {
		const auto &preset = presets[i];

		PanelizerEngine::PanelSpec spec;
		spec.panelSizeInches = QSizeF(preset.widthMm * mmToIn,
		                              preset.heightMm * mmToIn);
		spec.borderInches = m_borderInches;
		spec.gutterInches = m_gutterInches;
		spec.addRails     = true;

		Candidate c;
		c.preset = preset;

		// First try: full requested copies. If layout() fails, walk
		// downward to find how many actually fit so the user sees the
		// partial result instead of a blank "doesn't fit" tile.
		QList<PanelizerEngine::SourceBoard> trial = sources;
		QList<PanelizerEngine::PlacedBoard*> placed;
		QString errIgnored;
		bool ok = PanelizerEngine::layout(trial, spec, placed, &errIgnored);

		if (!ok) {
			qDeleteAll(placed);
			placed.clear();
			// Linear walk downward — cheap because layout() is fast and
			// the requested copies count is bounded by the heavy-load
			// confirmation upstream (>50 already prompts the user).
			for (int n = m_requestedCopies - 1; n >= 1; --n) {
				trial[0].copies = n;
				QList<PanelizerEngine::PlacedBoard*> partial;
				if (PanelizerEngine::layout(trial, spec, partial, &errIgnored)) {
					placed = partial;
					ok = true;
					break;
				}
				qDeleteAll(partial);
			}
		}

		c.placedCount = placed.size();
		c.fitsAll     = (c.placedCount == m_requestedCopies);
		c.boardRects.reserve(placed.size());
		for (auto * pb : placed) {
			QSizeF sz = m_boardSizeInches;
			if (pb->rotated90) sz = QSizeF(sz.height(), sz.width());
			c.boardRects.append(QRectF(pb->positionInches, sz));
		}
		qDeleteAll(placed);

		m_candidates.append(c);

		// List item: icon + label "100x100 mm (vendor) — 4 of 8 fit".
		auto * item = new QListWidgetItem(m_list);
		item->setIcon(QIcon(renderContour(c, QSize(kIconWidth, kIconHeight))));
		const QString fitText = c.fitsAll
			? tr("all %1 fit").arg(m_requestedCopies)
			: (c.placedCount > 0
				? tr("%1 of %2 fit").arg(c.placedCount).arg(m_requestedCopies)
				: tr("0 of %1 fit").arg(m_requestedCopies));
		item->setText(QStringLiteral("%1\n%2").arg(c.preset.label, fitText));
		item->setTextAlignment(Qt::AlignHCenter | Qt::AlignTop);
		item->setData(Qt::UserRole, i);
		// Disable totally-empty candidates so the user can't pick a
		// panel that wouldn't hold a single copy.
		if (c.placedCount == 0) {
			item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
		} else if (firstFitIndex < 0 && c.fitsAll) {
			firstFitIndex = i;
		}
	}

	// Pre-select the smallest panel that fits everything, so a hurried
	// user can just hit Next. Falls back to the first non-empty
	// candidate if no preset fits the full count.
	if (firstFitIndex >= 0) {
		m_list->setCurrentRow(firstFitIndex);
	} else {
		for (int i = 0; i < m_candidates.size(); ++i) {
			if (m_candidates[i].placedCount > 0) {
				m_list->setCurrentRow(i);
				break;
			}
		}
	}
	emit completeChanged();
}

QPixmap PanelCandidatePage::renderContour(const Candidate &c,
                                          const QSize &iconSize) const
{
	QPixmap pm(iconSize);
	pm.fill(Qt::transparent);

	const double pw = c.preset.widthMm  / 25.4; // inches
	const double ph = c.preset.heightMm / 25.4;
	if (pw <= 0 || ph <= 0) return pm;

	// Compute panel-to-icon transform (uniform scale, centered, small margin).
	const double margin = 6.0;
	const double availW = iconSize.width()  - 2 * margin;
	const double availH = iconSize.height() - 2 * margin;
	const double s = qMin(availW / pw, availH / ph);
	const double drawW = pw * s;
	const double drawH = ph * s;
	const double ox = (iconSize.width()  - drawW) / 2.0;
	const double oy = (iconSize.height() - drawH) / 2.0;

	QPainter p(&pm);
	p.setRenderHint(QPainter::Antialiasing, true);

	// Panel rectangle.
	p.setBrush(kPanelFill);
	p.setPen(QPen(kPanelOutline, 1.2));
	p.drawRect(QRectF(ox, oy, drawW, drawH));

	// Each placed board.
	const QColor outline = c.fitsAll ? kBoardOutlineFit : kBoardOutlinePartial;
	const QColor fill    = c.fitsAll ? kBoardFillFit    : kBoardFillPartial;
	p.setBrush(fill);
	p.setPen(QPen(outline, 1.0));
	for (const QRectF &r : c.boardRects) {
		const QRectF dst(ox + r.x() * s,
		                 oy + r.y() * s,
		                 r.width()  * s,
		                 r.height() * s);
		p.drawRect(dst);
	}

	// Watermark fit count when partial so it's legible at thumb size.
	if (!c.fitsAll && c.placedCount > 0) {
		p.setPen(outline);
		QFont f = p.font();
		f.setBold(true);
		p.setFont(f);
		p.drawText(QRectF(0, iconSize.height() - 18, iconSize.width(), 16),
		           Qt::AlignHCenter | Qt::AlignVCenter,
		           QString::number(c.placedCount));
	}

	return pm;
}

void PanelCandidatePage::onItemSelected()
{
	const auto items = m_list->selectedItems();
	if (items.isEmpty()) {
		m_selectedIndex = -1;
		m_detail->setText(tr("(no selection)"));
		emit completeChanged();
		return;
	}
	const int idx = items.first()->data(Qt::UserRole).toInt();
	if (idx < 0 || idx >= m_candidates.size()) return;
	m_selectedIndex = idx;

	const Candidate &c = m_candidates[idx];
	const double areaMm2 = c.preset.widthMm * c.preset.heightMm;
	const double usedMm2 = c.placedCount * m_boardSizeInches.width()
	                       * m_boardSizeInches.height() * 25.4 * 25.4;
	const double usePct = areaMm2 > 0.0 ? (100.0 * usedMm2 / areaMm2) : 0.0;
	m_detail->setText(tr("Selected: <b>%1</b> &mdash; %2 of %3 copies, "
	                     "%4% material utilization.")
		.arg(c.preset.label)
		.arg(c.placedCount).arg(m_requestedCopies)
		.arg(QString::number(usePct, 'f', 1)));

	// Push the choice straight into the wizard fields so the rest of
	// the pipeline reads it as if the user had typed it on PanelSizePage.
	if (wizard() != nullptr) {
		wizard()->setField("panel.width",  c.preset.widthMm);
		wizard()->setField("panel.height", c.preset.heightMm);
	}
	emit completeChanged();
}
