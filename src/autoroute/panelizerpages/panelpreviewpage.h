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

#ifndef PANELPREVIEWPAGE_H
#define PANELPREVIEWPAGE_H

#include <QWizardPage>
#include <QStringList>
#include <QPointer>

#include "../../gerberpreview/gerberpreviewwidget.h"

// Final destination: src/autoroute/panelizerpages/panelpreviewpage.h
// Staged in: src/panelizer/panelizerpages/panelpreviewpage.h

class QPushButton;
class QLineEdit;
class QCheckBox;
class QLabel;
class GerberPreviewWidget;
class GerberPreviewDialog;
class LayerManagerWidget;

/**
 * @brief Page 6 of the panelizer wizard — Preview & Export.
 *
 * Embedded QGraphicsView showing the laid-out panel SVG (using FSvgRenderer).
 * "Output folder" file picker.
 * Checkbox: "Also save panel as .fzz" (default on).
 *
 * Finish button:
 * 1. Push a PanelizeCommand onto WaitPushUndoStack.
 * 2. Call PanelizerEngine::layout() + emitPanel().
 * 3. On success, offer to open the .fzz panel in a new window.
 */
class PanelPreviewPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit PanelPreviewPage(QWidget *parent = nullptr);
    ~PanelPreviewPage() override;

    bool isComplete() const override;

    /**
     * @brief Sets the default output directory for the panel.
     * @param sketchPath The path to the current sketch.
     */
    void setDefaultOutputDir(const QString &sketchPath);

    /**
     * @brief Returns the output directory selected on this page.
     */
    QString outputDir() const;

    /**
     * @brief Returns whether to save the panel as .fzz.
     */
    bool savePanelFzz() const;

    /**
     * @brief Load the rendered Gerber/Excellon files into the
     *        preview widget. Call after the engine has finished
     *        writing the panel.
     * @param paths Absolute paths to the layer files (any subset).
     */
    void showGerbers(const QStringList &paths);

    /**
     * @brief Returns the layer file paths most recently loaded into
     *        the embedded preview, or an empty list if none.
     *
     * Used by the wizard to hand the same set off to the standalone
     * GerberPreviewDialog that pops on Finish.
     */
    QStringList lastPaths() const { return m_lastPaths; }

private slots:
    void onBrowseOutput();
    void onPreviewReady();
    void onLayerVisibilityChanged(GerberPreviewWidget::LayerKind kind, bool on);
    void onLayerColorChanged(GerberPreviewWidget::LayerKind kind, const QColor &color);
    void onOpenFullPreview();

private:
    // Persist/restore per-layer colour + visibility using the same
    // QSettings keys as GerberPreviewDialog, so a colour the user picks
    // in the inline preview also shows up in the standalone window.
    QString settingsKeyFor(GerberPreviewWidget::LayerKind kind,
                           const QString &suffix) const;

    GerberPreviewWidget *m_preview;
    QPushButton *m_browseButton;
    QLineEdit *m_outputEdit;
    QCheckBox *m_saveFzzCheck;
    LayerManagerWidget *m_layerManager;
    QLabel *m_statusLabel;
    QPushButton *m_openFullButton;
    QStringList m_lastPaths;
    // NOTE: QPointer auto-nulls when the dialog is destroyed (WA_DeleteOnClose).
    // Used to enforce a single Open-In-Full-Window instance even if the user
    // rage-clicks the button — see onOpenFullPreview().
    QPointer<GerberPreviewDialog> m_fullPreviewDialog;
};

#endif // PANELPREVIEWPAGE_H
