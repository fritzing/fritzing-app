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
#include "partinfowidget.h"
#include "../debugdialog.h"

PartInfoWidget::PartInfoWidget(QWidget *parent) : QWidget(parent) {
	m_version = new QLineEdit(this);
	m_author = new QLineEdit(this);
	m_title = new QLineEdit(this);
	//m_taxonomy = new QLineEdit(this);
	m_label = new QLineEdit(this);

	m_description = new QTextEdit(this);
	m_description->setFixedHeight(50);

	m_date = new QDateEdit(QDate::currentDate(), this);
	m_date->setDisplayFormat("yyyy-MM-dd");

	m_tagsList = new AddRemoveListWidget(tr("Tags"),this);
	m_propertyList = new AddRemoveListWidget(tr("Properties"),this);

	int row = 0;
	QGridLayout *layout = new QGridLayout();
	layout->setSpacing(4);
	layout->setMargin(3);

	layout->addWidget(new QLabel(tr("Version")), row, 0);
	layout->addWidget(m_version, row, 1);
	layout->addWidget(new QLabel(tr("Author")), row, 2);
	layout->addWidget(m_author, row++, 3);

	layout->addWidget(new QLabel(tr("Title")), row, 0);
	layout->addWidget(m_title, row++, 1, 1, 3);

	layout->addWidget(new QLabel(tr("Label")), row, 0);
	layout->addWidget(m_label, row, 1);

	layout->addWidget(new QLabel(tr("Description")), row, 0);
	layout->addWidget(m_description, row++, 1, 1, 3);

	/*layout->addWidget(new QLabel(tr("Taxonomy")), row, 0);
	layout->addWidget(m_taxonomy, row, 1);*/
	layout->addWidget(new QLabel(tr("Date")), row, 2);
	layout->addWidget(m_date, row++, 3);

	layout->addWidget(m_tagsList, row, 0, 1, 2);
	layout->addWidget(m_propertyList, row++, 2, 1, 2);

	setLayout(layout);
}

ModelPartShared* PartInfoWidget::modelPartShared() {
	ModelPartShared* mps = new ModelPartShared();
	mps->setVersion(m_version->text());
	mps->setAuthor(m_author->text());
	mps->setTitle(m_title->text());
	mps->setDate(m_date->date());
	//mps->setTaxonomy(m_taxonomy->text());
	mps->setLabel(m_label->text());
	mps->setDescription(m_description->toPlainText());

	mps->setTags(m_tagsList->getItemsText());
	//mps->setProperties(m_propertiesList->getItemsText());

	return mps;
}

void PartInfoWidget::updateInfo(ModelPart * modelPart) {
	ModelPartShared* mps = modelPart->modelPartShared();
	m_version->setText(mps->version());
	m_author->setText(mps->author());
	m_title->setText(mps->title());
	m_date->setDate(mps->date());
	//m_taxonomy->setText(mps->taxonomy());
	m_label->setText(mps->label());
	DebugDialog::debug("setting text: "+mps->description());
	m_description->setText(mps->description());
	m_tagsList->setItemsText(mps->tags());
	//m_propertiesList->setItemsText(mps->properties());
}


