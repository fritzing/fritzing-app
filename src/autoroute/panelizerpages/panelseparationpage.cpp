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

#include "panelseparationpage.h"

#include <QRadioButton>
#include <QButtonGroup>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>

PanelSeparationPage::PanelSeparationPage(QWidget *parent)
	: QWizardPage(parent)
	, m_methodGroup(nullptr)
	, m_noneRadio(nullptr)
	, m_vcutRadio(nullptr)
	, m_mousebitesRadio(nullptr)
	, m_vcutGroup(nullptr)
	, m_mousebitesGroup(nullptr)
	, m_vcutLineWidth(nullptr)
	, m_vcutLayerName(nullptr)
	, m_tabWidth(nullptr)
	, m_holesPerTab(nullptr)
	, m_holeDiameter(nullptr)
	, m_holePitch(nullptr)
	, m_tabPreview(nullptr)
	, m_selectedMethod(1)
{
	setTitle(tr("Separation"));
	setSubTitle(tr("Choose how boards will be separated from the panel"));

	m_noneRadio       = new QRadioButton(tr("None"), this);
	m_vcutRadio       = new QRadioButton(tr("V-cut"), this);
	m_mousebitesRadio = new QRadioButton(tr("Mouse-bites"), this);
	m_vcutRadio->setChecked(true);

	auto *methodLayout = new QVBoxLayout;
	methodLayout->addWidget(m_noneRadio);
	methodLayout->addWidget(m_vcutRadio);
	methodLayout->addWidget(m_mousebitesRadio);
	m_methodGroup = new QGroupBox(tr("Method"), this);
	m_methodGroup->setLayout(methodLayout);

	// V-cut sub-group.
	m_vcutLineWidth = new QDoubleSpinBox(this);
	m_vcutLineWidth->setRange(1.0, 100.0);
	m_vcutLineWidth->setValue(10.0);
	m_vcutLineWidth->setSuffix(tr(" mils"));
	m_vcutLayerName = new QLineEdit("Edge_Cuts", this);
	auto *vcutForm = new QFormLayout;
	vcutForm->addRow(tr("Line width:"), m_vcutLineWidth);
	vcutForm->addRow(tr("Edge layer:"), m_vcutLayerName);
	m_vcutGroup = new QGroupBox(tr("V-cut options"), this);
	m_vcutGroup->setLayout(vcutForm);

	// Mouse-bites sub-group.
	m_tabWidth = new QDoubleSpinBox(this);
	m_tabWidth->setRange(1.0, 20.0);
	m_tabWidth->setValue(3.0);
	m_tabWidth->setSuffix(" mm");
	m_holesPerTab = new QSpinBox(this);
	m_holesPerTab->setRange(2, 12);
	m_holesPerTab->setValue(5);
	m_holeDiameter = new QDoubleSpinBox(this);
	m_holeDiameter->setRange(0.1, 5.0);
	m_holeDiameter->setValue(0.5);
	m_holeDiameter->setSuffix(" mm");
	m_holePitch = new QDoubleSpinBox(this);
	m_holePitch->setRange(0.1, 5.0);
	m_holePitch->setValue(0.8);
	m_holePitch->setSuffix(" mm");
	auto *mbForm = new QFormLayout;
	mbForm->addRow(tr("Tab width:"),     m_tabWidth);
	mbForm->addRow(tr("Holes per tab:"), m_holesPerTab);
	mbForm->addRow(tr("Hole diameter:"), m_holeDiameter);
	mbForm->addRow(tr("Hole pitch:"),    m_holePitch);
	m_mousebitesGroup = new QGroupBox(tr("Mouse-bite options"), this);
	m_mousebitesGroup->setLayout(mbForm);

	auto *outer = new QVBoxLayout;
	outer->addWidget(m_methodGroup);
	outer->addWidget(m_vcutGroup);
	outer->addWidget(m_mousebitesGroup);
	outer->addStretch();
	setLayout(outer);

	registerField("panel.sep.none",          m_noneRadio);
	registerField("panel.sep.vcut",          m_vcutRadio);
	registerField("panel.sep.mb",            m_mousebitesRadio);
	registerField("panel.sep.vcutWidth",     m_vcutLineWidth, "value", SIGNAL(valueChanged(double)));
	registerField("panel.sep.vcutLayer",     m_vcutLayerName);
	registerField("panel.sep.tabWidth",      m_tabWidth,    "value", SIGNAL(valueChanged(double)));
	registerField("panel.sep.holesPerTab",   m_holesPerTab);
	registerField("panel.sep.holeDiameter",  m_holeDiameter,"value", SIGNAL(valueChanged(double)));
	registerField("panel.sep.holePitch",     m_holePitch,   "value", SIGNAL(valueChanged(double)));

	auto *group = new QButtonGroup(this);
	group->addButton(m_noneRadio,       0);
	group->addButton(m_vcutRadio,       1);
	group->addButton(m_mousebitesRadio, 2);
	connect(group, SIGNAL(buttonClicked(int)), this, SLOT(onMethodChanged(int)));
	onMethodChanged(1);
}

PanelSeparationPage::~PanelSeparationPage()
{
}

bool PanelSeparationPage::isComplete() const
{
	return m_noneRadio->isChecked()
		|| m_vcutRadio->isChecked()
		|| m_mousebitesRadio->isChecked();
}

void PanelSeparationPage::onMethodChanged(int id)
{
	m_selectedMethod = id;
	m_vcutGroup->setEnabled(id == 1);
	m_mousebitesGroup->setEnabled(id == 2);
	emit completeChanged();
}

void PanelSeparationPage::updatePreview()
{
}

