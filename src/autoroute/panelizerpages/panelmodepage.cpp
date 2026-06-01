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

#include "panelmodepage.h"

#include <QRadioButton>
#include <QGroupBox>
#include <QVBoxLayout>

PanelModePage::PanelModePage(QWidget *parent)
	: QWizardPage(parent)
	, m_stepAndRepeat(nullptr)
	, m_blendProjects(nullptr)
	, m_legacyXml(nullptr)
	, m_modeGroup(nullptr)
{
	setTitle(tr("Panel Mode"));
	setSubTitle(tr("Choose how to panelize your PCB"));

	m_stepAndRepeat = new QRadioButton(tr("Step-and-repeat current sketch"), this);
	m_stepAndRepeat->setChecked(true);
	m_blendProjects = new QRadioButton(tr("Blend multiple projects"), this);
	m_legacyXml     = new QRadioButton(tr("Use legacy panelizer.xml"), this);

	auto *groupLayout = new QVBoxLayout;
	groupLayout->addWidget(m_stepAndRepeat);
	groupLayout->addWidget(m_blendProjects);
	groupLayout->addWidget(m_legacyXml);

	m_modeGroup = new QGroupBox(tr("Mode"), this);
	m_modeGroup->setLayout(groupLayout);

	auto *outer = new QVBoxLayout;
	outer->addWidget(m_modeGroup);
	outer->addStretch();
	setLayout(outer);

	// Expose mode selection to the wizard via field("panel.mode").
	// Each radio registers as a separate bool field; collectMode()
	// downstream picks the active one.
	registerField("panel.modeStepAndRepeat", m_stepAndRepeat);
	registerField("panel.modeBlend",         m_blendProjects);
	registerField("panel.modeLegacyXml",     m_legacyXml);

	connect(m_stepAndRepeat, &QRadioButton::toggled, this, &QWizardPage::completeChanged);
	connect(m_blendProjects, &QRadioButton::toggled, this, &QWizardPage::completeChanged);
	connect(m_legacyXml,     &QRadioButton::toggled, this, &QWizardPage::completeChanged);
}

PanelModePage::~PanelModePage()
{
}

bool PanelModePage::isComplete() const
{
	return m_stepAndRepeat->isChecked()
		|| m_blendProjects->isChecked()
		|| m_legacyXml->isChecked();
}

PanelModePage::Mode PanelModePage::mode() const
{
	if (m_stepAndRepeat->isChecked()) return StepAndRepeat;
	if (m_blendProjects->isChecked()) return BlendProjects;
	return LegacyXml;
}

