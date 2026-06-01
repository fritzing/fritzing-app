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

#ifndef PANELCANDIDATEPAGE_H
#define PANELCANDIDATEPAGE_H

#include "../panelpresets.h"

#include <QSizeF>
#include <QVector>
#include <QWizardPage>

class QListWidget;
class QListWidgetItem;
class QLabel;
class QPushButton;

/**
 * @brief Wizard page that lets the user pick from a list of
 *        candidate panels, each showing a contour preview of how
 *        their boards would lay out on it.
 *
 * Only inserted into the wizard flow when "Auto-fit panel to copies"
 * is enabled on the previous page (PanelSizePage). When entered, the
 * page runs PanelizerEngine::layout() once per preset against the
 * current source-board geometry + copy count, renders each result as
 * a contour pixmap (panel rect + placed board rects, scaled to a
 * fixed icon size), and presents them in a left-to-right scrollable
 * list. Clicking an item writes its size back into the wizard's
 * panel.width / panel.height fields so the downstream pipeline picks
 * up the user's choice.
 *
 * @note Pure preview: no Gerber rendering, no .fzz IO. Layout runs
 *       are fast (microseconds per preset) so the page initialises
 *       synchronously without a progress indicator.
 */
class PanelCandidatePage : public QWizardPage {
	Q_OBJECT

public:
	explicit PanelCandidatePage(QWidget *parent = nullptr);
	~PanelCandidatePage() override;

	void initializePage() override;
	bool isComplete() const override;

	/**
	 * @brief Board footprint to use for thumbnail layout, in inches.
	 *        Set by the wizard before initializePage() so the page
	 *        doesn't have to re-probe the .fzz.
	 */
	void setBoardSizeInches(const QSizeF &sz) { m_boardSizeInches = sz; }

	/// Copies count to lay out per candidate; set by wizard.
	void setRequestedCopies(int copies) { m_requestedCopies = copies; }

	/// Panel border (rail) in inches; matters for fit calculation.
	void setBorderInches(double in) { m_borderInches = in; }

	/// Panel gutter (board spacing) in inches.
	void setGutterInches(double in) { m_gutterInches = in; }

	/// Whether the layout engine is allowed to rotate boards 90°.
	void setAllowRotate90(bool on) { m_allowRotate90 = on; }

	/// True iff the user has selected a candidate (Next gated on this).
	bool hasSelection() const { return m_selectedIndex >= 0; }

	/// Selected panel size in mm; valid only when hasSelection().
	QSizeF selectedPanelMm() const;

private slots:
	void onItemSelected();

private:
	struct Candidate {
		PanelPresets::Preset preset;
		int  placedCount = 0;     // how many of m_requestedCopies actually fit
		bool fitsAll     = false; // placedCount == m_requestedCopies
		QVector<QRectF> boardRects; // in inches, panel-local
	};

	void buildCandidates();
	QPixmap renderContour(const Candidate &c, const QSize &iconSize) const;

	QListWidget *m_list;
	QLabel      *m_detail;
	QSizeF       m_boardSizeInches;
	double       m_borderInches;
	double       m_gutterInches;
	int          m_requestedCopies;
	bool         m_allowRotate90;
	int          m_selectedIndex;
	QVector<Candidate> m_candidates;
};

#endif // PANELCANDIDATEPAGE_H
