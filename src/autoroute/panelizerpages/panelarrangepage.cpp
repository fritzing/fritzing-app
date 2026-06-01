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

#include "panelarrangepage.h"

#include "../panelizerengine.h"
#include "../panelizerwizard.h"

#include <QLabel>
#include <QToolBar>
#include <QAction>
#include <QVBoxLayout>
#include <QWizard>

namespace {
constexpr double kMmPerInch = 25.4;
}

PanelArrangePage::PanelArrangePage(QWidget *parent)
	: QWizardPage(parent)
	, m_editor(nullptr)
	, m_toolbar(nullptr)
	, m_status(nullptr)
{
	setTitle(tr("Arrange Boards"));
	setSubTitle(tr("Drag each board to position it. Select a board and use "
	               "Rotate / Flip (or press R / F) to orient it individually. "
	               "Red boards overlap or stick out past the panel rail."));

	m_toolbar = new QToolBar(this);
	m_toolbar->addAction(tr("Rotate 90\u00B0"), this, [this]() {
		m_editor->rotateSelectedClockwise();
	});
	m_toolbar->addAction(tr("Flip"), this, [this]() {
		m_editor->flipSelectedHorizontal();
	});
	m_toolbar->addSeparator();
	m_toolbar->addAction(tr("Auto-arrange"), this, [this]() {
		m_editor->autoArrange();
	});
	m_toolbar->addAction(tr("Fit view"), this, [this]() {
		m_editor->zoomToFit();
	});
	m_toolbar->addSeparator();
	// Draggable alignment bars: a checkable toggle plus a one-shot
	// re-centre so the user can line boards up against a common edge.
	QAction * guidesAction = m_toolbar->addAction(tr("Guides"), this, [this](bool on) {
		m_editor->setGuidesVisible(on);
	});
	guidesAction->setCheckable(true);
	guidesAction->setChecked(true);
	m_toolbar->addAction(tr("Centre guides"), this, [this]() {
		m_editor->resetGuides();
	});
	m_toolbar->addSeparator();
	// Interactive Generate: overwrite the output Gerbers and (re)open a
	// live preview without leaving the wizard, so the user can rearrange
	// and re-generate repeatedly. The final save still happens on Finish.
	m_toolbar->addAction(tr("Generate preview"), this, [this]() {
		if (auto * w = qobject_cast<PanelizerWizard*>(wizard())) {
			w->generateInteractivePreview();
		}
	});

	m_editor = new PanelLayoutEditor(this);
	connect(m_editor, &PanelLayoutEditor::layoutChanged,
	        this, &PanelArrangePage::onLayoutChanged);

	m_status = new QLabel(this);
	m_status->setWordWrap(true);

	auto * outer = new QVBoxLayout(this);
	outer->setContentsMargins(0, 0, 0, 0);
	outer->addWidget(m_toolbar);
	outer->addWidget(m_editor, 1);
	outer->addWidget(m_status);
}

PanelArrangePage::~PanelArrangePage() = default;

void PanelArrangePage::initializePage()
{
	if (wizard() == nullptr || m_boardSizeInches.isEmpty()) {
		m_status->setText(tr("Board size not available yet."));
		return;
	}

	const double inToMm = kMmPerInch;
	const double mmToIn = 1.0 / kMmPerInch;

	const QSizeF panelMm(wizard()->field("panel.width").toDouble(),
	                     wizard()->field("panel.height").toDouble());
	const double borderMm = wizard()->field("panel.border").toDouble();
	const double gutterMm = wizard()->field("panel.gutter").toDouble();
	const int copies      = qMax(1, wizard()->field("panel.copies").toInt());
	const bool allowRot   = wizard()->field("panel.allowRotate").toBool();

	// Run the engine's auto-layout to seed the initial arrangement; the
	// user is free to override every position/orientation afterwards.
	PanelizerEngine::SourceBoard sb;
	sb.fzzPath         = QString();
	sb.openWindow      = nullptr;
	sb.copies          = copies;
	sb.allowRotate90   = allowRot;
	sb.boardSizeInches = m_boardSizeInches;
	QList<PanelizerEngine::SourceBoard> sources;
	sources.append(sb);

	PanelizerEngine::PanelSpec spec;
	spec.panelSizeInches = QSizeF(panelMm.width() * mmToIn, panelMm.height() * mmToIn);
	spec.borderInches    = borderMm * mmToIn;
	spec.gutterInches    = gutterMm * mmToIn;
	spec.addRails        = true;

	QList<PanelizerEngine::PlacedBoard*> placed;
	QString errIgnored;
	PanelizerEngine::layout(sources, spec, placed, &errIgnored);

	QList<QPointF> tops;
	QList<bool>    rots;
	if (placed.isEmpty()) {
		// Layout failed for the requested count — seed the boards in a
		// simple cascade inside the usable area so the user still has
		// something to drag rather than an empty panel.
		const QSizeF bMm(m_boardSizeInches.width() * inToMm,
		                 m_boardSizeInches.height() * inToMm);
		double x = borderMm, y = borderMm;
		for (int i = 0; i < copies; ++i) {
			tops.append(QPointF(x, y));
			rots.append(false);
			x += 4.0; y += 4.0; // visible cascade
		}
	} else {
		for (auto * pb : placed) {
			tops.append(QPointF(pb->positionInches.x() * inToMm,
			                    pb->positionInches.y() * inToMm));
			rots.append(pb->rotated90);
		}
	}
	qDeleteAll(placed);

	const QSizeF boardMm(m_boardSizeInches.width() * inToMm,
	                     m_boardSizeInches.height() * inToMm);
	// Snap to a fraction of the gutter so boards land on a sensible
	// grid; fall back to 1 mm if the gutter is zero.
	m_editor->setGridStepMm(gutterMm > 0.01 ? gutterMm : 1.0);
	m_editor->seed(panelMm, borderMm, boardMm, tops, rots);

	onLayoutChanged();
}

bool PanelArrangePage::isComplete() const
{
	// Gate Next on a physically valid arrangement so the user can't
	// ship a panel with overlapping or off-panel boards.
	return m_editor != nullptr && m_editor->isLayoutValid();
}

void PanelArrangePage::onLayoutChanged()
{
	const bool ok = m_editor->isLayoutValid();
	const int n = m_editor->placements().size();
	if (ok) {
		m_status->setText(tr("%n board(s) placed. Layout is valid \u2014 "
		                     "click Next to render.", "", n));
	} else {
		m_status->setText(tr("\u26A0 Some boards overlap or extend past the "
		                     "panel rail (shown in red). Fix them before "
		                     "continuing."));
	}
	emit completeChanged();
}

QList<PanelLayoutEditor::Placement> PanelArrangePage::placements() const
{
	return m_editor != nullptr ? m_editor->placements()
	                           : QList<PanelLayoutEditor::Placement>();
}
