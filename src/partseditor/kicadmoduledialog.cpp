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

$Revision: 6385 $:
$Author: cohen@irascible.com $:
$Date: 2012-09-08 21:21:20 +0200 (Sa, 08. Sep 2012) $

********************************************************************/

#include "kicadmoduledialog.h"
#include "../debugdialog.h"
#include "../connectors/connectoritem.h"
#include "../sketch/pcbsketchwidget.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFormLayout>

/////////////////////////////////////////////////////////

KicadModuleDialog::KicadModuleDialog(const QString & partType, const QString & filename, const QStringList & modules, QWidget *parent) : QDialog(parent) 
{
	this->setWindowTitle(QObject::tr("Select %1").arg(partType));

	QVBoxLayout * vLayout = new QVBoxLayout(this);

	QFrame * frame = new QFrame(this);

	QFormLayout * formLayout = new QFormLayout();

	m_comboBox = new QComboBox(this);
	m_comboBox->addItems(modules);
	formLayout->addRow(QString("%1:").arg(partType), m_comboBox );

	frame->setLayout(formLayout);

	QLabel * label = new QLabel(QString("There are %1 %3 descriptions in '%2'.  Please select one.").arg(modules.count()).arg(filename).arg(partType));
	vLayout->addWidget(label);

	vLayout->addWidget(frame);

    QDialogButtonBox * buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
	buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	vLayout->addWidget(buttonBox);

	this->setLayout(vLayout);
}

KicadModuleDialog::~KicadModuleDialog() {
}

const QString KicadModuleDialog::selectedModule() {
	return m_comboBox->currentText();
}

