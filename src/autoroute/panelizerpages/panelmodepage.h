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

#ifndef PANELMODEPAGE_H
#define PANELMODEPAGE_H

#include <QWizardPage>

// Final destination: src/autoroute/panelizerpages/panelmodepage.h
// Staged in: src/panelizer/panelizerpages/panelmodepage.h

class QRadioButton;
class QGroupBox;

/**
 * @brief Page 1 of the panelizer wizard — Mode selection.
 *
 * Three radio buttons:
 * 1. Step-and-repeat current sketch (default)
 * 2. Blend multiple projects (enables Page 2 file list)
 * 3. Use legacy panelizer.xml (open file dialog, hand off to
 *    existing Panelizer::panelize() unchanged)
 */
class PanelModePage : public QWizardPage
{
    Q_OBJECT

public:
    explicit PanelModePage(QWidget *parent = nullptr);
    ~PanelModePage() override;

    bool isComplete() const override;

    /**
     * @brief Returns the selected mode.
     */
    enum Mode {
        StepAndRepeat,
        BlendProjects,
        LegacyXml
    };
    Q_ENUM(Mode)

    Mode mode() const;

private:
    QRadioButton *m_stepAndRepeat;
    QRadioButton *m_blendProjects;
    QRadioButton *m_legacyXml;
    QGroupBox *m_modeGroup;
};

#endif // PANELMODEPAGE_H
