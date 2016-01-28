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

#include "groundfillseeddialog.h"
#include "../debugdialog.h"
#include "../connectors/connectoritem.h"
#include "../sketch/pcbsketchwidget.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QLabel>

/////////////////////////////////////////////////////////

GroundFillSeedDialog::GroundFillSeedDialog(PCBSketchWidget * sketchWidget, QList<class ConnectorItem *> & connectorItems, const QString & intro, QWidget *parent)
	: QDialog(parent)
{
	m_sketchWidget = sketchWidget;
	m_connectorItems = connectorItems;
	m_activeConnectorItem = NULL;
	m_doFill = false;

	this->setWindowTitle(QObject::tr("Ground Fill Seed Editor"));

	QVBoxLayout * vLayout = new QVBoxLayout(this);

	if (!intro.isEmpty()) {
		QLabel * label = new QLabel(intro);
		label->setWordWrap(true);
		vLayout->addWidget(label);
	}

	QLabel * label = new QLabel(tr("The difference between a 'ground fill' and plain 'copper fill' is that in a ground fill, "
								 "the flooded area includes traces and connectors that are connected to 'ground' connectors. "
								 "Ground connectors are usually labeled 'GND' or 'ground' but sometimes this is not the case. "
								 "It also may be that there are multiple nets with a ground connector, "
								 "and you might only want one of the nets to be filled.\n\n"

								 "This dialog collects only connectors labeled 'GND' or 'ground', as well as connectors already chosen as seeds.\n\n"
								 
								 "Click an item to highlight its connections in the sketch.\n\n"

								 "It is also possible to choose a connector as a ground fill seed by right-clicking a connector and "
								 "choosing the 'Set Ground Fill Seed' context menu option."));
	label->setWordWrap(true);
	vLayout->addWidget(label);
	
	m_listWidget = new QListWidget(this);
	int ix = 0;
	foreach (ConnectorItem * connectorItem, connectorItems) {
		QListWidgetItem *item = new QListWidgetItem;
		item->setData(Qt::DisplayRole, connectorItem->connectorSharedName());
		item->setData(Qt::CheckStateRole, connectorItem->isGroundFillSeed() ? Qt::Checked : Qt::Unchecked);
		item->setData(Qt::UserRole, ix++);
		connectorItem->updateTooltip();
		item->setToolTip(connectorItem->toolTip());
		m_listWidget->addItem(item);
	}

	connect(m_listWidget, SIGNAL(itemClicked(QListWidgetItem *)), this, SLOT(clickedSlot(QListWidgetItem *)));
	connect(m_listWidget, SIGNAL(itemChanged(QListWidgetItem *)), this, SLOT(changedSlot(QListWidgetItem *)));
	vLayout->addWidget(m_listWidget);

	QDialogButtonBox * buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Yes | QDialogButtonBox::Cancel);
	
	QPushButton * cancelButton = buttonBox->button(QDialogButtonBox::Cancel);
	cancelButton->setText(tr("Cancel"));
	cancelButton->setDefault(false);
	
	QPushButton * okButton = buttonBox->button(QDialogButtonBox::Ok);
	okButton->setText(tr("OK"));
	okButton->setDefault(true);

	m_doFillButton = buttonBox->button(QDialogButtonBox::Yes);
	m_doFillButton->setDefault(false);
	connect(m_doFillButton, SIGNAL(clicked(bool)), this, SLOT(doFill(bool)));

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	vLayout->addWidget(buttonBox);
	this->setLayout(vLayout);

	changedSlot(NULL);
}

GroundFillSeedDialog::~GroundFillSeedDialog()
{
}

void GroundFillSeedDialog::changedSlot(QListWidgetItem *) {
	bool checked = false;
	for (int i = 0; i < m_listWidget->count(); i++) {
		if (m_listWidget->item(i)->checkState() == Qt::Checked) {
			checked = true;
			break;
		}
	}

	m_doFillButton->setText(checked ? tr("OK and ground fill") : tr("OK and copper fill"));
}


void GroundFillSeedDialog::clickedSlot(QListWidgetItem * item) {
	int ix = -1;
	if (item != NULL) {
		ix = item->data(Qt::UserRole).toInt();
	}

	showEqualPotential(m_activeConnectorItem, false);
	m_activeConnectorItem = (ix >= 0 && ix < m_connectorItems.count()) ? m_connectorItems.at(ix) : NULL;
	showEqualPotential(m_activeConnectorItem, true);
}

void GroundFillSeedDialog::showEqualPotential(ConnectorItem * connectorItem, bool show)
{
	if (connectorItem) {
		QList<ConnectorItem *> connectorItems;
		connectorItems.append(connectorItem);
		ConnectorItem::collectEqualPotential(connectorItems, true, ViewGeometry::NoFlag);
	    QList<ConnectorItem *> visited;
		foreach (ConnectorItem * ci, connectorItems) {
			ci->showEqualPotential(show, visited);
		}
	}
}

void GroundFillSeedDialog::accept() 
{
	showEqualPotential(m_activeConnectorItem, false);
	QDialog::accept();
}

void GroundFillSeedDialog::reject() 
{
	showEqualPotential(m_activeConnectorItem, false);
	QDialog::reject();
}

void GroundFillSeedDialog::getResults(QList<bool> & results) {
	for (int i = 0; i < m_listWidget->count(); i++) {
		results.append(m_listWidget->item(i)->checkState() == Qt::Checked);
	}
}

bool GroundFillSeedDialog::getFill() {
	return m_doFill;
}

void GroundFillSeedDialog::doFill(bool) {
	m_doFill = true;
	accept();
}
