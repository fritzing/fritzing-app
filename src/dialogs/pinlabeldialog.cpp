/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2016 Fritzing

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

********************************************************************

$Revision: 6112 $:
$Author: cohen@irascible.com $:
$Date: 2012-06-28 00:18:10 +0200 (Do, 28. Jun 2012) $

********************************************************************/

#include "pinlabeldialog.h"
#include "../debugdialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
#include <QKeyEvent>

// TODO:
//
//		would be nice to update schematic view with new pin labels
//
//		reasons for not doing save:
//			undo would require a lot of work to implement and would require saving the old dom document
//			every instance of the part in all open documents would have to be updated

PinLabelUndoCommand::PinLabelUndoCommand(PinLabelDialog * pinLabelDialog, int index, QLineEdit * lineEdit, const QString & previous, const QString & next) : QUndoCommand() 
{
	m_pinLabelDialog = pinLabelDialog;
	m_previous = previous;
	m_next = next;
	m_index = index;
	m_lineEdit = lineEdit;
}

void PinLabelUndoCommand::undo() {
	m_pinLabelDialog->setLabelText(m_index, m_lineEdit, m_previous);
}

void PinLabelUndoCommand::redo() {
	m_pinLabelDialog->setLabelText(m_index, m_lineEdit, m_next);
}

/////////////////////////////////////////////////////////

PinLabelDialog::PinLabelDialog(const QStringList & labels, bool singleRow, const QString & chipLabel, bool isCore, QWidget *parent)
	: QDialog(parent)
{
	m_isCore = isCore;
	m_labels = labels;
	m_doSaveAs = true;
	this->setWindowTitle(QObject::tr("Pin Label Editor"));

	QVBoxLayout * vLayout = new QVBoxLayout(this);

	QScrollArea * scrollArea = new QScrollArea(this);
	scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	QFrame * frame = new QFrame(this);
	QHBoxLayout * hLayout = new QHBoxLayout(frame);

	QFrame * labelsFrame = initLabels(labels, singleRow, chipLabel);

	QFrame * textFrame = new QFrame();
	QVBoxLayout * textLayout = new QVBoxLayout(frame);

	QLabel * label = new QLabel("<html><body>" +
								tr("<p><h2>Pin Label Editor</h2></p>") +
								tr("<p>Click on a label next to a pin number to rename that pin.") + " " +
								tr("You can use the tab key to move through the labels in order.</p>") +
								"</body></html>");
	label->setMaximumWidth(150);
	label->setWordWrap(true);

	textLayout->addWidget(label);
	textLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));
	textFrame->setLayout(textLayout);

	hLayout->addWidget(labelsFrame);
	hLayout->addSpacing(15);
	hLayout->addWidget(textFrame);
	frame->setLayout(hLayout);

	scrollArea->setWidget(frame);	

	QDialogButtonBox * buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
	
	QPushButton * cancelButton = buttonBox->button(QDialogButtonBox::Cancel);
	cancelButton->setText(tr("Cancel"));
	cancelButton->setDefault(false);
	
	m_saveAsButton = buttonBox->button(QDialogButtonBox::Save);
	m_saveAsButton->setText(tr("Save"));
	m_saveAsButton->setEnabled(false);
	m_saveAsButton->setDefault(false);

	m_undoButton = new QPushButton(tr("Undo"));
	m_undoButton->setEnabled(false);
	m_undoButton->setDefault(false);

	m_redoButton = new QPushButton(tr("Redo"));
	m_redoButton->setEnabled(false);
	m_redoButton->setDefault(false);

    buttonBox->addButton(m_undoButton, QDialogButtonBox::ResetRole);
    buttonBox->addButton(m_redoButton, QDialogButtonBox::ResetRole);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	vLayout->addWidget(scrollArea);
	vLayout->addWidget(buttonBox);
	this->setLayout(vLayout);

	connect(buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));
	connect(&m_undoStack, SIGNAL(canRedoChanged(bool)), this, SLOT(undoChanged(bool)));
	connect(&m_undoStack, SIGNAL(canUndoChanged(bool)), this, SLOT(undoChanged(bool)));
	connect(&m_undoStack, SIGNAL(cleanChanged(bool)), this, SLOT(undoChanged(bool)));

}

PinLabelDialog::~PinLabelDialog()
{
}

QFrame * PinLabelDialog::initLabels(const QStringList & labels, bool singleRow, const QString & chipLabel)
{
	QFrame * frame = new QFrame();
	QVBoxLayout * vLayout = new QVBoxLayout();

	QLabel * label = new QLabel("<h2>" + chipLabel + "</h2>");
	label->setAlignment(Qt::AlignCenter);

	QFrame * subFrame = new QFrame();
	if (singleRow) {
		QGridLayout * gridLayout = new QGridLayout();
		gridLayout->setMargin(0);
		gridLayout->setSpacing(3);

		for (int i = 0; i < labels.count(); i++) {
			makeOnePinEntry(i, labels.at(i), Qt::AlignLeft, i, gridLayout);
		}

		subFrame->setLayout(gridLayout);
	}
	else {
		QHBoxLayout * hLayout = new QHBoxLayout();

		QFrame * lFrame = new QFrame();
		QGridLayout * lLayout = new QGridLayout;
		lLayout->setMargin(0);
		lLayout->setSpacing(3);
		for (int i = 0; i < labels.count() / 2; i++) {
			makeOnePinEntry(i, labels.at(i), Qt::AlignLeft, i, lLayout);
		}
		lFrame->setLayout(lLayout);

		QFrame * rFrame = new QFrame();
		QGridLayout * rLayout = new QGridLayout;
		rLayout->setMargin(0);
		rLayout->setSpacing(3);
		int row = labels.count() - 1;
		for (int i = labels.count() / 2; i < labels.count(); i++) {
			makeOnePinEntry(i, labels.at(i), Qt::AlignRight, row--, rLayout);
		}
		rFrame->setLayout(rLayout);

		hLayout->addWidget(lFrame);
		hLayout->addSpacing(15);
		hLayout->addWidget(rFrame);

		subFrame->setLayout(hLayout);
	}

	vLayout->addWidget(label);
	vLayout->addWidget(subFrame);
	frame->setLayout(vLayout);

	return frame;
}

void PinLabelDialog::makeOnePinEntry(int index, const QString & text, Qt::Alignment alignment, int row, QGridLayout * gridLayout) 
{
	QLineEdit * label = new QLineEdit();
	label->setText(QString::number(index + 1));
	label->setMaximumWidth(20);
	label->setMinimumWidth(20);
	label->setFrame(false);
	label->setEnabled(false);

	QLineEdit * lEdit = new QLineEdit();
	lEdit->setMaximumWidth(65);
	lEdit->setAlignment(alignment);
	lEdit->setText(text);
	lEdit->setProperty("index", index);
	lEdit->setProperty("prev", text);
	connect(lEdit, SIGNAL(editingFinished()), this, SLOT(labelChanged()));
	connect(lEdit, SIGNAL(textEdited(const QString &)), this, SLOT(labelEdited(const QString &)));

	if (alignment == Qt::AlignLeft) {
		label->setAlignment(Qt::AlignRight);
		gridLayout->addWidget(label, row, 0);
		gridLayout->addWidget(lEdit, row, 1);
	}
	else {
		label->setAlignment(Qt::AlignLeft);
		gridLayout->addWidget(lEdit, row, 0);
		gridLayout->addWidget(label, row, 1);
	}
}

void PinLabelDialog::labelChanged() {
	QLineEdit * lineEdit = qobject_cast<QLineEdit *>(sender());
	if (lineEdit == NULL) return;

	bool ok;
	int index = lineEdit->property("index").toInt(&ok);
	if (!ok) return;

	if (index < 0) return;
	if (index >= m_labels.count()) return;

	PinLabelUndoCommand * pluc = new PinLabelUndoCommand(this, index, lineEdit, lineEdit->property("prev").toString(), lineEdit->text());
	lineEdit->setProperty("prev", lineEdit->text());

	m_undoStack.push(pluc);
}

void PinLabelDialog::labelEdited(const QString &) 
{
	m_saveAsButton->setEnabled(true);
}

const QStringList & PinLabelDialog::labels() {
	return m_labels;
}

void PinLabelDialog::setLabelText(int index, QLineEdit * lineEdit, const QString & text) 
{
	m_labels.replace(index, text);
	lineEdit->setText(text);
	lineEdit->setProperty("prev", text);
}

void PinLabelDialog::buttonClicked(QAbstractButton * button) {

	if (button == m_undoButton) {
		m_undoStack.undo();
	}
	else if (button == m_redoButton) {
		m_undoStack.redo();
	}
}

bool PinLabelDialog::doSaveAs() {
	return m_doSaveAs;
}

void PinLabelDialog::undoChanged(bool) 
{
	bool redo = false;
	bool undo = false;
	bool saveAs = false;

	if (m_undoStack.canUndo()) {
		saveAs = undo = true;
	}
	if (m_undoStack.canRedo()) {
		redo = true;
	}

	m_saveAsButton->setEnabled(saveAs);
	m_undoButton->setEnabled(undo);
	m_redoButton->setEnabled(redo);
}

void PinLabelDialog::keyPressEvent(QKeyEvent *e)
{
	switch (e->key()) {
		case Qt::Key_Escape:
		case Qt::Key_Enter:
		case Qt::Key_Return:
			return;
		default:
			QDialog::keyPressEvent(e);
	}
}
