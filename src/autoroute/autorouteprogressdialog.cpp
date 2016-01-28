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

$Revision: 6904 $:
$Author: irascibl@gmail.com $:
$Date: 2013-02-26 16:26:03 +0100 (Di, 26. Feb 2013) $

********************************************************************/

#include "autorouteprogressdialog.h"
#include "../debugdialog.h"
#include "zoomcontrols.h"

#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QLocale>
#include <QGroupBox>
#include <QScrollBar>

static const int ScrollAmount = 40;

ArrowButton::ArrowButton(int scrollX, int scrollY, ZoomableGraphicsView * view, const QString & path) : QLabel() {
	m_scrollX = scrollX;
	m_scrollY = scrollY;
	m_view = view;
	setPixmap(QPixmap(path));
}

void ArrowButton::mousePressEvent(QMouseEvent *event) {
	Q_UNUSED(event);
	if (m_scrollX != 0) {
		QScrollBar * scrollBar = m_view->horizontalScrollBar();
		scrollBar->setValue(scrollBar->value() - m_scrollX);
	}
	else if (m_scrollY != 0) {
		QScrollBar * scrollBar = m_view->verticalScrollBar();
		scrollBar->setValue(scrollBar->value() - m_scrollY);
	}
}

/////////////////////////////////////

AutorouteProgressDialog::AutorouteProgressDialog(const QString & title, bool zoomAndPan, bool stopButton, bool bestButton, bool spin, ZoomableGraphicsView * view, QWidget *parent) : QDialog(parent) 
{
	Qt::WindowFlags flags = windowFlags();
	flags ^= Qt::WindowCloseButtonHint;
	flags ^= Qt::WindowContextHelpButtonHint;
	setWindowFlags(flags);

	this->setWindowTitle(title);

	QVBoxLayout * vLayout = new QVBoxLayout(this);

	m_progressBar = new QProgressBar(this);
	vLayout->addWidget(m_progressBar);

	m_spinLabel = NULL;
	m_spinBox = NULL;

	if (spin) {
		QFrame * frame = new QFrame(this);
		m_spinLabel = new QLabel(this);
		m_spinBox = new QSpinBox(this);
		m_spinBox->setMinimum(1);
		m_spinBox->setMaximum(99999);
		connect(m_spinBox, SIGNAL(valueChanged(int)), this, SLOT(internalSpinChange(int)));
		QHBoxLayout * hBoxLayout = new QHBoxLayout(frame);
		hBoxLayout->addStretch();
		hBoxLayout->addWidget(m_spinLabel);
		hBoxLayout->addWidget(m_spinBox);
		vLayout->addWidget(frame);
	}

	m_message = new QLabel(this);
	vLayout->addWidget(m_message);
	m_message2 = new QLabel(this);
	vLayout->addWidget(m_message2);
	
	if (zoomAndPan) {
		QGroupBox * groupBox = new QGroupBox(tr("zoom and pan controls"));
		QHBoxLayout *lo2 = new QHBoxLayout(groupBox);
		lo2->setSpacing(1);
		lo2->setMargin(0);

		//TODO: use the zoom slider instead
		lo2->addWidget(new ZoomControls(view, groupBox));

		lo2->addSpacerItem(new QSpacerItem ( 10, 0, QSizePolicy::Expanding));

		QFrame * frame = new QFrame();
		QGridLayout *gridLayout = new QGridLayout(frame);

		QString imgPath = ":/resources/images/icons/arrowButton%1.png";
		ArrowButton * label = new ArrowButton(0, -ScrollAmount, view, imgPath.arg("Up"));
		gridLayout->addWidget(label, 0, 1);

		label = new ArrowButton(0, ScrollAmount, view, imgPath.arg("Down"));
		gridLayout->addWidget(label, 2, 1);

		label = new ArrowButton(-ScrollAmount, 0, view, imgPath.arg("Left"));
		gridLayout->addWidget(label, 0, 0, 3, 1);

		label = new ArrowButton(ScrollAmount, 0, view, imgPath.arg("Right"));
		gridLayout->addWidget(label, 0, 2, 3, 1);


		lo2->addWidget(frame);

		vLayout->addSpacing(7);
		vLayout->addWidget(groupBox);
		vLayout->addSpacing(7);
	}

	//QPushButton * button = new QPushButton(tr("Skip current trace"), this);
	//connect(button, SIGNAL(clicked()), this, SLOT(sendSkip()));
	//vLayout->addWidget(button);

	m_buttonBox = new QDialogButtonBox(stopButton ? QDialogButtonBox::Ok | QDialogButtonBox::Cancel : QDialogButtonBox::Cancel);
	if (stopButton) {
		m_buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Stop Now"));
		connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(sendStop()));
	}
    if (bestButton) {
        QPushButton * best = new QPushButton(tr("Best So Far"));
        m_buttonBox->addButton(best, QDialogButtonBox::ActionRole);
        connect(best, SIGNAL(clicked()), this, SLOT(sendBest()));
    }


	m_buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
	connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(sendCancel()));

	vLayout->addWidget(m_buttonBox);

	this->setLayout(vLayout);
}

AutorouteProgressDialog::~AutorouteProgressDialog() {
}

void AutorouteProgressDialog::setMinimum(int minimum) {
	m_progressBar->setMinimum(minimum);
}

void AutorouteProgressDialog::setMaximum(int maximum) {
	m_progressBar->setMaximum(maximum);
}

void AutorouteProgressDialog::setValue(int value) {
	m_progressBar->setValue(value);
}

void AutorouteProgressDialog::sendSkip() {
	emit skip();
}

void AutorouteProgressDialog::sendCancel() {
	emit cancel();
}

void AutorouteProgressDialog::sendStop() {
	emit stop();
}

void AutorouteProgressDialog::sendBest() {
	emit best();
}

void AutorouteProgressDialog::closeEvent(QCloseEvent *event)
{
	sendCancel();
	QDialog::closeEvent(event);
}

void AutorouteProgressDialog::setMessage(const QString & text)
{
	m_message->setText(text);
}

void AutorouteProgressDialog::setMessage2(const QString & text)
{
	m_message2->setText(text);
}

void AutorouteProgressDialog::setSpinLabel(const QString & text)
{
	m_spinLabel->setText(text);
}

void AutorouteProgressDialog::setSpinValue(int value)
{
	m_spinBox->setValue(value);
}

void AutorouteProgressDialog::internalSpinChange(int value) {
	emit spinChange(value);
}

void AutorouteProgressDialog::disableButtons() {
	m_buttonBox->setEnabled(false);
}
