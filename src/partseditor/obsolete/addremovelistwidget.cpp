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


#include <QGridLayout>
#include "addremovelistwidget.h"
#include "../utils/misc.h"

#define BUTTON_SIZE 25

AddRemoveListWidget::AddRemoveListWidget(QString title, QWidget *parent) : QGroupBox(parent) {
	m_label = new QLabel(title+":");
	//setFixedWidth(parentWidget()->width());

	m_addButton = new QPushButton(QIcon(":/resources/images/add.png"),"",this);
	m_addButton->setFixedSize(BUTTON_SIZE,BUTTON_SIZE);
	connect(m_addButton,SIGNAL(clicked()),this,SLOT(addItem()));

	m_removeButton = new QPushButton(QIcon(":/resources/images/remove.png"),"",this);
	m_removeButton->setFixedSize(BUTTON_SIZE,BUTTON_SIZE);
	connect(m_removeButton,SIGNAL(clicked()),this,SLOT(removeSelectedItems()));

	m_list = new QListWidget(this);
	m_list->setSelectionMode(QAbstractItemView::ContiguousSelection);
	m_list->setFixedHeight(BUTTON_SIZE*3);
	m_list->setSortingEnabled(true);

	int row=0;
	QGridLayout *layout = new QGridLayout;
	layout->setSpacing(1);
	layout->setMargin(3);
	layout->addWidget(m_label,row,0);
    layout->addWidget(m_addButton,row,1);
    layout->addWidget(m_removeButton,row++,2);
    layout->addWidget(m_list,row,0,row,3);
	row++;
    setLayout(layout);
}

void AddRemoveListWidget::addItem() {
	addItem(m_label->text().toLower().remove("s:"));
}

void AddRemoveListWidget::addItem(QString itemText) {
	QListWidgetItem *item = new QListWidgetItem();
	item->setFlags(item->flags() | Qt::ItemIsEditable);
	item->setText(itemText);
	m_list->addItem(item);
	m_list->editItem(item);
}

void AddRemoveListWidget::removeSelectedItems() {
	QList<QListWidgetItem*> selitems = m_list->selectedItems();
	for(int i=0; i < selitems.size(); i++) {
		m_list->takeItem(m_list->row(selitems[i]));
	}
}

int AddRemoveListWidget::count() {
	if(m_list != NULL) {
		return m_list->count();
	} else {
		return 0;
	}
}

QListWidgetItem* AddRemoveListWidget::itemAt(int rowIdx) {
	if(m_list != NULL) {
		return m_list->item(rowIdx);
	} else {
		return NULL;
	}
}

QStringList& AddRemoveListWidget::getItemsText() {
	QStringList *retval = new QStringList();
	for(int i=0; i < count(); i++) {
		*retval << itemAt(i)->text();
	}
	return *retval;
}

void AddRemoveListWidget::setItemsText(const QStringList& texts) {
	for(int i=0; i < texts.count(); i++) {
		addItem(texts[i]);
	}
	m_list->sortItems();
}
