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

#ifndef PANELSEPARATIONPAGE_H
#define PANELSEPARATIONPAGE_H

#include <QWizardPage>

// Final destination: src/autoroute/panelizerpages/panelseparationpage.h
// Staged in: src/panelizer/panelizerpages/panelseparationpage.h

class QRadioButton;
class QGroupBox;
class QDoubleSpinBox;
class QSpinBox;
class QLabel;
class QLineEdit;
class QGraphicsView;

/**
 * @brief Page 4 of the panelizer wizard — Separation method.
 *
 * Radio: None / V-cut / Mouse-bites.
 *
 * V-cut sub-fields: line width (mils), edge layer name (default Edge_Cuts).
 * Mouse-bites sub-fields: tab width, holes per tab, hole diameter, pitch.
 * Show a small SVG preview of one tab.
 *
 * NOTE: V-cut lines must extend through the rails edge-to-edge
 * (that is what the fab actually scores against). Mouse-bites are
 * local to the gutter only.
 */
class PanelSeparationPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit PanelSeparationPage(QWidget *parent = nullptr);
    ~PanelSeparationPage() override;

    bool isComplete() const override;

    /**
     * @brief Returns the separation spec from this page.
     */
    // PanelizerEngine::SeparationSpec separationSpec() const;

private slots:
    void onMethodChanged(int id);
    void updatePreview();

private:
    QGroupBox *m_methodGroup;
    QRadioButton *m_noneRadio;
    QRadioButton *m_vcutRadio;
    QRadioButton *m_mousebitesRadio;
    QGroupBox *m_vcutGroup;
    QGroupBox *m_mousebitesGroup;
    QDoubleSpinBox *m_vcutLineWidth;
    QLineEdit *m_vcutLayerName;
    QDoubleSpinBox *m_tabWidth;
    QSpinBox *m_holesPerTab;
    QDoubleSpinBox *m_holeDiameter;
    QDoubleSpinBox *m_holePitch;
    QGraphicsView *m_tabPreview;
    int m_selectedMethod;
};

#endif // PANELSEPARATIONPAGE_H
