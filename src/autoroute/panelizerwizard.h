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

#ifndef PANELIZERWIZARD_H
#define PANELIZERWIZARD_H

#include <QSizeF>
#include <QWizard>

// Final destination: src/autoroute/panelizerwizard.h
// Staged in: src/panelizer/panelizerwizard.h

class PanelModePage;
class PanelSourcesPage;
class PanelSizePage;
class PanelCandidatePage;
class PanelSeparationPage;
class PanelExtrasPage;
class PanelArrangePage;
class PanelPreviewPage;
class GerberPreviewDialog;

/**
 * @brief QWizard for the panelizer v2 GUI flow.
 *
 * Six pages: Mode → Sources → Panel Size → Separation → Extras → Preview/Export.
 * The wizard collects all parameters and builds a PanelizerEngine::SourceBoard
 * list, PanelizerEngine::PanelSpec, and PanelizerEngine::SeparationSpec
 * that are passed to PanelizerEngine::layout() and emitPanel() on Finish.
 *
 * The wizard is invoked from File → Export → for Production → Panelize...
 * (added to the for Production submenu in mainwindow_menu.cpp).
 */
class PanelizerWizard : public QWizard
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the panelizer wizard.
     * @param parent The parent widget (typically MainWindow).
     * @param currentSketchPath Path to the current sketch .fzz (for step-and-repeat mode).
     */
    explicit PanelizerWizard(QWidget *parent = nullptr,
                             const QString &currentSketchPath = QString());

    ~PanelizerWizard() override;

    /**
     * @brief Called when the user clicks Finish.
     *
     * Collects parameters from all pages, calls PanelizerEngine::layout()
     * + emitPanel(), and pushes a PanelizeCommand onto WaitPushUndoStack.
     */
    void accept() override;

    /**
     * @brief Override of QWizard::initializePage().
     *
     * When the preview page is entered, runs the full panelize pipeline
     * (read params -> probe board sizes -> layout -> emitPanel) and pushes
     * the resulting Gerber paths into the preview widget so the user can
     * verify the panel before clicking Finish.
     */
    void initializePage(int id) override;

    /**
     * @brief Override of QWizard::nextId().
     *
     * Routes around PanelCandidatePage when the user is not in
     * auto-fit mode. PanelCandidatePage only makes sense when the
     * wizard is choosing the panel size for the user; otherwise the
     * user types width/height into PanelSizePage and the candidate
     * picker would be a noisy redundancy.
     */
    int nextId() const override;

    /**
     * @brief Returns true if panelization succeeded.
     * @note Set by accept() after emitPanel() completes.
     */
    bool panelizeSuccess() const { return m_panelizeSuccess; }

    /**
     * @brief Run the panelize pipeline now and (re)load a live, non-modal
     *        Gerber preview window, leaving the wizard open.
     *
     * Wired to the "Generate" button on the interactive Arrange page so
     * the user can iterate: rearrange boards, click Generate to overwrite
     * the output Gerbers and refresh the preview, repeat — without leaving
     * the wizard. The preview window is created once and reused on
     * subsequent calls. Final confirmation/save still happens on Finish.
     */
    void generateInteractivePreview();

private:
    void executePanelize();

    // Runs the full panelize pipeline (probe -> layout -> emitPanel) using
    // the current wizard field values. Populates @p outFiles with the
    // emitted Gerber paths on success. Returns false on any hard failure,
    // with a user-facing message in @p outErr.
    bool runPanelize(QStringList & outFiles, QString & outErr);

    /**
     * @brief Open the source .fzz in a hidden MainWindow and probe
     *        the PCB board's bounds to derive the board size in
     *        inches. Cached after first call so subsequent uses
     *        (candidate page + runPanelize) reuse the result.
     * @return Probed size; falls back to a sane 50x30 mm if probing
     *         fails (matches the legacy fallback in runPanelize).
     */
    QSizeF probedBoardSizeInches();

    PanelModePage *m_pageMode;
    PanelSourcesPage *m_pageSources;
    PanelSizePage *m_pagePanelSize;
    PanelCandidatePage *m_pageCandidate;
    PanelSeparationPage *m_pageSeparation;
    PanelExtrasPage *m_pageExtras;
    PanelArrangePage *m_pageArrange;
    PanelPreviewPage *m_pagePreview;
    // Page ids assigned by addPage(); needed by nextId() to route the
    // candidate page in/out of the flow without depending on the
    // ordering inside QWizard's internal page map.
    int m_idMode;
    int m_idSources;
    int m_idPanelSize;
    int m_idCandidate;
    int m_idSeparation;
    int m_idExtras;
    int m_idArrange;
    int m_idPreview;
    bool m_panelizeSuccess;
    bool m_previewRendered;       // true once runPanelize has populated preview
    QString m_currentSketchPath;
    QString m_lastOutputDir;      // remembered for the final accept() message
    QSizeF  m_cachedBoardSizeInches; // empty until first probedBoardSizeInches() call
    // Reused non-modal preview window for the interactive "Generate"
    // loop. Created lazily by generateInteractivePreview(); cleared via
    // QPointer semantics is unnecessary because we own it as a child and
    // null it on destruction signal.
    GerberPreviewDialog * m_livePreview = nullptr;
};

#endif // PANELIZERWIZARD_H
