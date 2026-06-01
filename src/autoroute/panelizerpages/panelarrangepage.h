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

#ifndef PANELARRANGEPAGE_H
#define PANELARRANGEPAGE_H

#include "panellayouteditor.h"

#include <QSizeF>
#include <QWizardPage>

class QLabel;
class QToolBar;

/**
 * @brief Wizard page that lets the user arrange individual boards on
 *        the panel — drag to reposition, rotate in 90° steps, and flip
 *        each board independently.
 *
 * This is the interactive "arrange" step the original wizard lacked.
 * On entry it seeds an auto-layout (PanelizerEngine::layout()) into the
 * embedded PanelLayoutEditor, then hands control to the user. The final
 * per-board placements (position + rotation + flip) are read back by the
 * wizard via placements() and fed straight into emitPanel(), so what the
 * user arranges is exactly what the fab receives.
 *
 * @note Board geometry (size) and the panel/border/gutter parameters
 *       are pushed in by the wizard before initializePage() so the page
 *       never re-opens the .fzz itself (the wizard caches the probe).
 */
class PanelArrangePage : public QWizardPage {
	Q_OBJECT

public:
	explicit PanelArrangePage(QWidget *parent = nullptr);
	~PanelArrangePage() override;

	void initializePage() override;
	bool isComplete() const override;

	/// Un-rotated board footprint in inches; set by the wizard.
	void setBoardSizeInches(const QSizeF &sz) { m_boardSizeInches = sz; }

	/// Final, user-arranged placements (inches, panel-local).
	QList<PanelLayoutEditor::Placement> placements() const;

private slots:
	void onLayoutChanged();

private:
	PanelLayoutEditor *m_editor;
	QToolBar          *m_toolbar;
	QLabel            *m_status;
	QSizeF             m_boardSizeInches;
};

#endif // PANELARRANGEPAGE_H
