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

#include "panelsourcespage.h"

#include <QListWidget>
#include <QListWidgetItem>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

PanelSourcesPage::PanelSourcesPage(QWidget *parent)
	: QWizardPage(parent)
	, m_sourceList(nullptr)
	, m_addButton(nullptr)
	, m_removeButton(nullptr)
	, m_upButton(nullptr)
	, m_downButton(nullptr)
	, m_stepAndRepeatCopies(nullptr)
	, m_allowRotation(nullptr)
	, m_sarBox(nullptr)
	, m_blendBox(nullptr)
	, m_outputEdit(nullptr)
	, m_outputBrowse(nullptr)
	, m_maxSources(4)
	, m_maxCopiesPerSource(1000)
{
	setTitle(tr("Source Boards"));
	setSubTitle(tr("Select source boards and copy counts"));

	// Step-and-repeat controls. Cap raised to 1000 - the wizard's
	// preview page issues a confirmation prompt before running a
	// heavy-load (>50 copy) panelize, so the upper bound here is just
	// a sanity guard against runaway input, not a UX gate.
	m_stepAndRepeatCopies = new QSpinBox(this);
	m_stepAndRepeatCopies->setRange(1, 1000);
	m_stepAndRepeatCopies->setValue(8);
	m_allowRotation = new QCheckBox(tr("Allow 90° rotation"), this);
	m_allowRotation->setChecked(true);

	auto *sarLayout = new QHBoxLayout;
	sarLayout->addWidget(new QLabel(tr("Copies:"), this));
	sarLayout->addWidget(m_stepAndRepeatCopies);
	sarLayout->addWidget(m_allowRotation);
	sarLayout->addStretch();

	m_sarBox = new QGroupBox(tr("Step-and-repeat"), this);
	m_sarBox->setLayout(sarLayout);

	// Blend controls.
	m_sourceList   = new QListWidget(this);
	m_addButton    = new QPushButton(tr("Add .fzz..."), this);
	m_removeButton = new QPushButton(tr("Remove"), this);
	m_upButton     = new QPushButton(tr("Up"), this);
	m_downButton   = new QPushButton(tr("Down"), this);

	auto *btnLayout = new QVBoxLayout;
	btnLayout->addWidget(m_addButton);
	btnLayout->addWidget(m_removeButton);
	btnLayout->addWidget(m_upButton);
	btnLayout->addWidget(m_downButton);
	btnLayout->addStretch();

	auto *blendLayout = new QHBoxLayout;
	blendLayout->addWidget(m_sourceList, 1);
	blendLayout->addLayout(btnLayout);

	m_blendBox = new QGroupBox(tr("Blend (multiple .fzz files)"), this);
	m_blendBox->setLayout(blendLayout);

	auto *outer = new QVBoxLayout;
	outer->addWidget(m_sarBox);
	outer->addWidget(m_blendBox);

	// Output folder lives here (instead of on the preview page) so the
	// user picks where artifacts land *before* the long-running render
	// kicks off when entering the preview page. Default is seeded by
	// setDefaultOutputDir() from the wizard ctor.
	m_outputEdit   = new QLineEdit(this);
	m_outputBrowse = new QPushButton(tr("Browse..."), this);
	auto *outRow = new QHBoxLayout;
	outer->addWidget(new QLabel(tr("Output folder:"), this));
	outRow->addWidget(m_outputEdit, 1);
	outRow->addWidget(m_outputBrowse);
	outer->addLayout(outRow);
	setLayout(outer);

	registerField("panel.copies",      m_stepAndRepeatCopies);
	registerField("panel.allowRotate", m_allowRotation);
	registerField("panel.outputDir*",  m_outputEdit);

	connect(m_addButton,    &QPushButton::clicked, this, &PanelSourcesPage::onAddSource);
	connect(m_removeButton, &QPushButton::clicked, this, &PanelSourcesPage::onRemoveSource);
	connect(m_upButton,     &QPushButton::clicked, this, &PanelSourcesPage::onMoveSourceUp);
	connect(m_downButton,   &QPushButton::clicked, this, &PanelSourcesPage::onMoveSourceDown);
	connect(m_outputBrowse, &QPushButton::clicked, this, &PanelSourcesPage::onBrowseOutput);
	connect(m_outputEdit,   &QLineEdit::textChanged, this, &QWizardPage::completeChanged);
}

PanelSourcesPage::~PanelSourcesPage()
{
}

bool PanelSourcesPage::isComplete() const
{
	// Output folder is required regardless of mode.
	if (m_outputEdit == nullptr || m_outputEdit->text().trimmed().isEmpty()) return false;
	// For step-and-repeat mode, we don't need a list, just a copy count.
	// For blend mode, we need at least one source file.
	if (field("panel.modeStepAndRepeat").toBool()) {
		return true;  // SAR mode doesn't require a list
	}
	return m_sourceList && m_sourceList->count() > 0;
}

bool PanelSourcesPage::isBlendMode() const
{
	return m_sourceList && m_sourceList->count() > 0;
}

void PanelSourcesPage::onAddSource()
{
	if (m_sourceList->count() >= m_maxSources) return;
	const QString f = QFileDialog::getOpenFileName(this,
		tr("Select source .fzz"), QString(), tr("Fritzing sketch (*.fzz)"));
	if (f.isEmpty()) return;
	m_sourceList->addItem(f);
	emit completeChanged();
}

void PanelSourcesPage::onRemoveSource()
{
	delete m_sourceList->takeItem(m_sourceList->currentRow());
	emit completeChanged();
}

void PanelSourcesPage::onMoveSourceUp()
{
	const int row = m_sourceList->currentRow();
	if (row <= 0) return;
	QListWidgetItem *item = m_sourceList->takeItem(row);
	m_sourceList->insertItem(row - 1, item);
	m_sourceList->setCurrentRow(row - 1);
}

void PanelSourcesPage::onMoveSourceDown()
{
	const int row = m_sourceList->currentRow();
	if (row < 0 || row >= m_sourceList->count() - 1) return;
	QListWidgetItem *item = m_sourceList->takeItem(row);
	m_sourceList->insertItem(row + 1, item);
	m_sourceList->setCurrentRow(row + 1);
}

void PanelSourcesPage::updateCopyCount()
{
}

void PanelSourcesPage::onBrowseOutput()
{
	const QString dir = QFileDialog::getExistingDirectory(this,
		tr("Select output folder"), m_outputEdit->text());
	if (!dir.isEmpty()) m_outputEdit->setText(dir);
}

void PanelSourcesPage::setDefaultOutputDir(const QString &sketchPath)
{
	if (m_outputEdit == nullptr || sketchPath.isEmpty()) return;
	if (!m_outputEdit->text().isEmpty()) return; // don't clobber a user edit
	QFileInfo fi(sketchPath);
	const QString baseDir = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
	m_outputEdit->setText(baseDir + "/panel");
}

QString PanelSourcesPage::outputDir() const
{
	return m_outputEdit ? m_outputEdit->text().trimmed() : QString();
}

/**
 * @brief Called by Qt every time the user enters this page.
 *
 * Toggles visibility of the SAR and blend groups based on
 * the panel mode selected on Page 1.
 */
void PanelSourcesPage::initializePage()
{
	const bool sar   = field("panel.modeStepAndRepeat").toBool();
	const bool blend = field("panel.modeBlend").toBool();
	const bool legacy = field("panel.modeLegacyXml").toBool();

	// Show only the relevant group based on mode
	m_sarBox->setVisible(sar);
	m_blendBox->setVisible(blend);

	// For legacy XML, hide both groups since we'll skip to preview
	m_sarBox->setVisible(!legacy);
	m_blendBox->setVisible(!legacy);

	emit completeChanged();
}

/**
 * @brief If Legacy XML mode was selected, skip all intermediate pages.
 */
int PanelSourcesPage::nextId() const
{
	if (field("panel.modeLegacyXml").toBool()) return -1;  // Finish
	return QWizardPage::nextId();
}

