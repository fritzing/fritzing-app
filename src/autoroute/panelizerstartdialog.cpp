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

#include "panelizerstartdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QListWidget>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>

PanelizerStartDialog::PanelizerStartDialog(QWidget * parent, const QString & currentSketchTitle)
	: QDialog(parent)
{
	buildUi(currentSketchTitle);
	updateBlendEnabled();
}

/**
 * @brief Lay out the compact start UI.
 *
 * Order top-to-bottom: copy count, a one-line rotation toggle, then the
 * mode selector with the (initially disabled) blend list. The dialog is
 * intentionally non-resizable-tall so it reads as a quick prompt.
 */
void PanelizerStartDialog::buildUi(const QString & currentSketchTitle)
{
	setWindowTitle(tr("Panelize"));

	QVBoxLayout * root = new QVBoxLayout(this);

	// --- Quantity ---------------------------------------------------
	const QString boardName = currentSketchTitle.isEmpty()
		? tr("the current board")
		: currentSketchTitle;
	QLabel * intro = new QLabel(
		tr("How many copies of %1 do you want on the panel?").arg(boardName), this);
	intro->setWordWrap(true);
	root->addWidget(intro);

	QFormLayout * form = new QFormLayout();
	m_copiesSpin = new QSpinBox(this);
	m_copiesSpin->setRange(1, 1000);
	m_copiesSpin->setValue(8);
	form->addRow(tr("Quantity:"), m_copiesSpin);
	root->addLayout(form);

	m_rotateCheck = new QCheckBox(tr("Allow 90° rotation for a tighter fit"), this);
	m_rotateCheck->setChecked(true);
	root->addWidget(m_rotateCheck);

	// --- Blend mode -------------------------------------------------
	QGroupBox * modeBox = new QGroupBox(tr("Boards on the panel"), this);
	QVBoxLayout * modeLayout = new QVBoxLayout(modeBox);

	m_modeStepRepeat = new QRadioButton(tr("Just this board (step && repeat)"), modeBox);
	m_modeStepRepeat->setChecked(true);
	m_modeBlend = new QRadioButton(tr("Blend in other boards too"), modeBox);
	modeLayout->addWidget(m_modeStepRepeat);
	modeLayout->addWidget(m_modeBlend);

	m_blendList = new QListWidget(modeBox);
	m_blendList->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_blendList->setMinimumHeight(80);
	modeLayout->addWidget(m_blendList);

	QHBoxLayout * blendButtons = new QHBoxLayout();
	m_addButton = new QPushButton(tr("Add boards…"), modeBox);
	m_removeButton = new QPushButton(tr("Remove"), modeBox);
	blendButtons->addWidget(m_addButton);
	blendButtons->addWidget(m_removeButton);
	blendButtons->addStretch();
	modeLayout->addLayout(blendButtons);

	root->addWidget(modeBox);

	// --- OK / Cancel ------------------------------------------------
	QDialogButtonBox * bb = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	bb->button(QDialogButtonBox::Ok)->setText(tr("Continue →"));
	root->addWidget(bb);

	connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(m_modeStepRepeat, &QRadioButton::toggled, this, &PanelizerStartDialog::updateBlendEnabled);
	connect(m_modeBlend, &QRadioButton::toggled, this, &PanelizerStartDialog::updateBlendEnabled);
	connect(m_addButton, &QPushButton::clicked, this, &PanelizerStartDialog::addBlendBoards);
	connect(m_removeButton, &QPushButton::clicked, this, &PanelizerStartDialog::removeBlendBoards);

	setMinimumWidth(380);
}

void PanelizerStartDialog::updateBlendEnabled()
{
	// The blend list only makes sense in blend mode; grey it out
	// otherwise so the small dialog stays unambiguous.
	const bool blend = m_modeBlend->isChecked();
	m_blendList->setEnabled(blend);
	m_addButton->setEnabled(blend);
	m_removeButton->setEnabled(blend);
}

void PanelizerStartDialog::addBlendBoards()
{
	const QStringList files = QFileDialog::getOpenFileNames(
		this, tr("Add boards to blend"), QString(),
		tr("Fritzing Sketch (*.fzz)"));
	for (const QString & f : files) {
		// Skip duplicates: the same .fzz twice would just stack copies,
		// which the quantity field already covers.
		bool exists = false;
		for (int i = 0; i < m_blendList->count(); ++i) {
			if (m_blendList->item(i)->data(Qt::UserRole).toString() == f) {
				exists = true;
				break;
			}
		}
		if (exists) continue;
		QListWidgetItem * item = new QListWidgetItem(QFileInfo(f).fileName(), m_blendList);
		item->setData(Qt::UserRole, f);
		item->setToolTip(f);
	}
}

void PanelizerStartDialog::removeBlendBoards()
{
	qDeleteAll(m_blendList->selectedItems());
}

int PanelizerStartDialog::copies() const
{
	return m_copiesSpin->value();
}

bool PanelizerStartDialog::allowRotate() const
{
	return m_rotateCheck->isChecked();
}

QStringList PanelizerStartDialog::blendPaths() const
{
	QStringList paths;
	if (!m_modeBlend->isChecked()) return paths;
	for (int i = 0; i < m_blendList->count(); ++i) {
		paths << m_blendList->item(i)->data(Qt::UserRole).toString();
	}
	return paths;
}
