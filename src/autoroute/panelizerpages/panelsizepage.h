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

#ifndef PANELSIZEPAGE_H
#define PANELSIZEPAGE_H

#include <QWizardPage>

// Final destination: src/autoroute/panelizerpages/panelsizepage.h
// Staged in: src/panelizer/panelizerpages/panelsizepage.h

class QComboBox;
class QDoubleSpinBox;
class QCheckBox;
class QLabel;

/**
 * @brief Page 3 of the panelizer wizard — Panel size and border.
 *
 * Preset combo: 100x100, 100x150, 150x100, 160x100, 200x150,
 * JLCPCB max (400x500), Custom...
 *
 * Custom: two QDoubleSpinBox (width, height) + unit toggle (mm/in).
 * Validate against min(board) + 2*border to refuse impossible sizes.
 *
 * Gutter spacing (mm): default 2.0.
 * Border / rail width (mm): default 5.0.
 * Checkbox: "Add top+bottom rails (recommended for V-cut)".
 *
 * Live "fits N boards" label — calls PanelizerEngine::layout()
 * in a non-modal QFutureWatcher and updates as you type.
 */
class PanelSizePage : public QWizardPage
{
    Q_OBJECT

public:
    explicit PanelSizePage(QWidget *parent = nullptr);
    ~PanelSizePage() override;

    bool isComplete() const override;

    /**
     * @brief Returns the panel spec from this page.
     */
    // PanelizerEngine::PanelSpec panelSpec() const;

    /**
     * @brief Sets the current sketch path for autosize functionality.
     * @param sketchPath The path to the current sketch.
     */
    void setSketchPath(const QString &sketchPath);

private slots:
    void onPresetChanged(int index);
    void onSizeChanged();
    void onFitsCountReady();
    void onUnitToggled(bool checked);
    void onAutosizeFromCurrent();
    void onAutoFitToggled(bool on);

private:
    QComboBox *m_presetCombo;
    QDoubleSpinBox *m_widthSpin;
    QDoubleSpinBox *m_heightSpin;
    QCheckBox *m_mmCheckbox;
    QCheckBox *m_railsCheckbox;
    QCheckBox *m_autoFitCheckbox;
    QDoubleSpinBox *m_gutterSpin;
    QDoubleSpinBox *m_borderSpin;
    QLabel *m_fitsLabel;
    QPushButton *m_autosizeButton;
    QString m_currentPreset;
    QString m_sketchPath;
};

#endif // PANELSIZEPAGE_H
