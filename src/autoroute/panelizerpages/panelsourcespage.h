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

#ifndef PANELSOURCESPAGE_H
#define PANELSOURCESPAGE_H

#include <QWizardPage>

// Final destination: src/autoroute/panelizerpages/panelsourcespage.h
// Staged in: src/panelizer/panelizerpages/panelsourcespage.h

class QListWidget;
class QSpinBox;
class QCheckBox;
class QPushButton;
class QGroupBox;
class QLineEdit;

/**
 * @brief Page 2 of the panelizer wizard — Source boards.
 *
 * For step-and-repeat mode: shows current sketch's board as a
 * thumbnail; spinner for copies (1-100), checkbox for allow 90° rotation.
 *
 * For blend mode: QListWidget with Add .fzz, Remove, Up/Down buttons.
 * Per-row spinner for copies, per-row allowRotate90 checkbox.
 * Cap at 4 source files with 25 copies each (100 total).
 */
class PanelSourcesPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit PanelSourcesPage(QWidget *parent = nullptr);
    ~PanelSourcesPage() override;

    bool isComplete() const override;
    bool isBlendMode() const;

    /**
     * @brief Seeds the default output folder, called by the wizard
     *        once it knows the current sketch path. Picks
     *        <sketchDir>/panel as the default if the field is empty.
     */
    void setDefaultOutputDir(const QString &sketchPath);

    /**
     * @brief Output folder where the panel artifacts will be written.
     */
    QString outputDir() const;

    /**
     * @brief Returns the list of source boards from this page.
     */
    // QList<PanelizerEngine::SourceBoard> sources() const;

    /**
     * @brief Called by Qt every time the user enters this page.
     *
     * Toggles visibility of the SAR and blend groups based on
     * the panel mode selected on Page 1.
     */
    void initializePage() override;

    /**
     * @brief If Legacy XML mode was selected, skip all intermediate pages.
     */
    int nextId() const override;

private slots:
    void onAddSource();
    void onRemoveSource();
    void onMoveSourceUp();
    void onMoveSourceDown();
    void onBrowseOutput();
    void updateCopyCount();

private:
    QListWidget *m_sourceList;
    QPushButton *m_addButton;
    QPushButton *m_removeButton;
    QPushButton *m_upButton;
    QPushButton *m_downButton;
    QSpinBox *m_stepAndRepeatCopies;
    QCheckBox *m_allowRotation;
    QGroupBox *m_sarBox;
    QGroupBox *m_blendBox;
    QLineEdit *m_outputEdit;
    QPushButton *m_outputBrowse;
    int m_maxSources;
    int m_maxCopiesPerSource;
};

#endif // PANELSOURCESPAGE_H
