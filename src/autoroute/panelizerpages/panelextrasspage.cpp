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

#include "panelextrasspage.h"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QVBoxLayout>

PanelExtrasPage::PanelExtrasPage(QWidget *parent)
	: QWizardPage(parent)
	, m_fiducialsCheck(nullptr)
	, m_fiducialDiameter(nullptr)
	, m_fiducialClear(nullptr)
	, m_toolingHolesCheck(nullptr)
	, m_toolingHoleDiameter(nullptr)
{
	setTitle(tr("Extras"));
	setSubTitle(tr("Add fiducials and tooling holes to the panel"));

	m_fiducialsCheck = new QCheckBox(tr("Add fiducials (top copper + mask)"), this);
	m_fiducialsCheck->setChecked(true);
	m_fiducialDiameter = new QDoubleSpinBox(this);
	m_fiducialDiameter->setRange(10.0, 200.0);
	m_fiducialDiameter->setValue(40.0);
	m_fiducialDiameter->setSuffix(tr(" mils"));
	m_fiducialClear = new QDoubleSpinBox(this);
	m_fiducialClear->setRange(10.0, 400.0);
	m_fiducialClear->setValue(80.0);
	m_fiducialClear->setSuffix(tr(" mils"));

	auto *fidForm = new QFormLayout;
	fidForm->addRow(QString(),                m_fiducialsCheck);
	fidForm->addRow(tr("Pad diameter:"),     m_fiducialDiameter);
	fidForm->addRow(tr("Mask clearance:"),   m_fiducialClear);
	auto *fidBox = new QGroupBox(tr("Fiducials"), this);
	fidBox->setLayout(fidForm);

	m_toolingHolesCheck = new QCheckBox(tr("Add tooling holes at rail corners"), this);
	m_toolingHoleDiameter = new QDoubleSpinBox(this);
	m_toolingHoleDiameter->setRange(0.01, 0.5);
	m_toolingHoleDiameter->setValue(0.125);
	m_toolingHoleDiameter->setSuffix(tr(" in"));
	m_toolingHoleDiameter->setDecimals(3);

	auto *thForm = new QFormLayout;
	thForm->addRow(QString(),               m_toolingHolesCheck);
	thForm->addRow(tr("Hole diameter:"),   m_toolingHoleDiameter);
	auto *thBox = new QGroupBox(tr("Tooling holes"), this);
	thBox->setLayout(thForm);

	auto *outer = new QVBoxLayout;
	outer->addWidget(fidBox);
	outer->addWidget(thBox);
	outer->addStretch();
	setLayout(outer);

	registerField("panel.extras.fiducials",        m_fiducialsCheck);
	registerField("panel.extras.fiducialDiameter", m_fiducialDiameter,    "value", SIGNAL(valueChanged(double)));
	registerField("panel.extras.fiducialClear",    m_fiducialClear,       "value", SIGNAL(valueChanged(double)));
	registerField("panel.extras.toolingHoles",     m_toolingHolesCheck);
	registerField("panel.extras.toolingDiameter",  m_toolingHoleDiameter, "value", SIGNAL(valueChanged(double)));
}

PanelExtrasPage::~PanelExtrasPage()
{
}

bool PanelExtrasPage::isComplete() const
{
	return true;
}

