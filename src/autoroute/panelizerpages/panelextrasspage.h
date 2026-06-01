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

#ifndef PANELEXTRASSPAGE_H
#define PANELEXTRASSPAGE_H

#include <QWizardPage>

// Final destination: src/autoroute/panelizerpages/panelextrasspage.h
// Staged in: src/panelizer/panelizerpages/panelextrasspage.h

class QCheckBox;
class QDoubleSpinBox;

/**
 * @brief Page 5 of the panelizer wizard — Extras (fiducials + tooling holes).
 *
 * Fiducials checkbox + diameter/clear spinners. Three corners (TL, TR,
 * BR) is the de-facto standard; expose as a 4-checkbox grid.
 *
 * Tooling holes checkbox + diameter spinner; place at all 4 rail
 * corners with a 5 mm offset.
 */
class PanelExtrasPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit PanelExtrasPage(QWidget *parent = nullptr);
    ~PanelExtrasPage() override;

    bool isComplete() const override;

    /**
     * @brief Returns the extras configuration from this page.
     */
    // struct ExtrasSpec {
    //     bool hasFiducials;
    //     double fiducialDiameter;
    //     double fiducialClear;
    //     bool hasToolingHoles;
    //     double toolingHoleDiameter;
    // } extrasSpec() const;

private:
    QCheckBox *m_fiducialsCheck;
    QDoubleSpinBox *m_fiducialDiameter;
    QDoubleSpinBox *m_fiducialClear;
    QCheckBox *m_toolingHolesCheck;
    QDoubleSpinBox *m_toolingHoleDiameter;
};

#endif // PANELEXTRASSPAGE_H
