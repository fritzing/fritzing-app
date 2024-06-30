/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2019 Fritzing

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

#include "peutils.h"
#include "hashpopulatewidget.h"
#include "../debugdialog.h"
#include "../utils/graphicsutils.h"
#include "utils/misc.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QRadioButton>
#include <QLabel>
#include <QTextStream>

QString PEUtils::Units = "mm";
const int PEUtils::Spacing = 10;


/////////////////////////////////////////////////////////

QString PEUtils::convertUnitsStr(double val)
{
	return QString::number(PEUtils::convertUnits(val));
}

double PEUtils::convertUnits(double val)
{
	if (Units.compare("in") == 0) {
		return val / GraphicsUtils::SVGDPI;
	}
	else if (Units.compare("mm") == 0) {
		return val * 25.4 / GraphicsUtils::SVGDPI;
	}

	return val;
}

double PEUtils::unconvertUnits(double val)
{
	if (Units.compare("in") == 0) {
		return val * GraphicsUtils::SVGDPI;
	}
	else if (Units.compare("mm") == 0) {
		return val * GraphicsUtils::SVGDPI / 25.4;
	}

	return val;
}

QWidget * PEUtils::makeConnectorForm(const QDomElement & connector, int index, QObject * slotHolder, bool alternating)
{
	auto * frame = new QFrame();
	if (alternating) {
		frame->setObjectName(index % 2 == 0 ? "NewPartsEditorConnector0Frame" : "NewPartsEditorConnector1Frame");
	}
	else {
		frame->setObjectName("NewPartsEditorConnectorFrame");
	}
	auto * mainLayout = new QVBoxLayout();
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);

	auto * nameFrame = new QFrame();
	auto * nameLayout = new QHBoxLayout();

	auto * justLabel = new QLabel(QObject::tr("<b>Name:</b>"));
	justLabel->setObjectName("NewPartsEditorLabel");
	nameLayout->addWidget(justLabel);

	auto * nameEdit = new QLineEdit();
	nameEdit->setText(connector.attribute("name"));
	QObject::connect(nameEdit, SIGNAL(editingFinished()), slotHolder, SLOT(nameEntry()));
	nameEdit->setObjectName("NewPartsEditorLineEdit");
	nameEdit->setStatusTip(QObject::tr("Set the connectors's title"));
	nameEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
	nameEdit->setProperty("index", index);
	nameEdit->setProperty("type", "name");
	nameEdit->setProperty("id", connector.attribute("id"));
	nameLayout->addWidget(nameEdit);
	nameLayout->addSpacing(Spacing);

	auto * hashRemoveButton = new HashRemoveButton(nullptr, nullptr, nullptr);
	hashRemoveButton->setProperty("index", index);
	QObject::connect(hashRemoveButton, SIGNAL(clicked(HashRemoveButton *)), slotHolder, SLOT(removeConnector()));
	nameLayout->addWidget(hashRemoveButton);

	nameFrame->setLayout(nameLayout);
	mainLayout->addWidget(nameFrame);



	auto * descriptionFrame = new QFrame();
	auto * descriptionLayout = new QHBoxLayout();

	justLabel = new QLabel(QObject::tr("<b>Description:</b>"));
	justLabel->setObjectName("NewPartsEditorLabel");
	descriptionLayout->addWidget(justLabel);

	auto * descriptionEdit = new QLineEdit();
	QDomElement description = connector.firstChildElement("description");
	descriptionEdit->setText(description.text());
	QObject::connect(descriptionEdit, SIGNAL(editingFinished()), slotHolder, SLOT(descriptionEntry()));
	descriptionEdit->setObjectName("NewPartsEditorLineEdit");
	descriptionEdit->setStatusTip(QObject::tr("Set the connectors's description"));
	descriptionEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
	descriptionEdit->setProperty("index", index);
	descriptionEdit->setProperty("type", "description");
	descriptionLayout->addWidget(descriptionEdit);

	descriptionFrame->setLayout(descriptionLayout);
	mainLayout->addWidget(descriptionFrame);



	auto * idFrame = new QFrame();
	auto * idLayout = new QHBoxLayout();

	justLabel = new QLabel(QObject::tr("<b>id:</b>"));
	justLabel->setObjectName("NewPartsEditorLabel");
	idLayout->addWidget(justLabel);

	justLabel = new QLabel(connector.attribute("id"));
	justLabel->setObjectName("NewPartsEditorLabel");
	idLayout->addWidget(justLabel);
	idLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));

	Connector::ConnectorType ctype = Connector::connectorTypeFromName(connector.attribute("type"));

	justLabel = new QLabel(QObject::tr("<b>type:</b>"));
	justLabel->setObjectName("NewPartsEditorLabel");
	idLayout->addWidget(justLabel);

	auto * radioButton = new QRadioButton(MaleSymbolString);
	QObject::connect(radioButton, SIGNAL(clicked()), slotHolder, SLOT(typeEntry()));
	radioButton->setObjectName("NewPartsEditorRadio");
	if (ctype == Connector::Male) radioButton->setChecked(true);
	radioButton->setProperty("value", Connector::Male);
	radioButton->setProperty("index", index);
	radioButton->setProperty("type", "radio");
	idLayout->addWidget(radioButton);

	radioButton = new QRadioButton(FemaleSymbolString);
	QObject::connect(radioButton, SIGNAL(clicked()), slotHolder, SLOT(typeEntry()));
	radioButton->setObjectName("NewPartsEditorRadio");
	if (ctype == Connector::Female) radioButton->setChecked(true);
	radioButton->setProperty("value", Connector::Female);
	radioButton->setProperty("index", index);
	radioButton->setProperty("type", "radio");
	idLayout->addWidget(radioButton);

	radioButton = new QRadioButton(QObject::tr("Pad"));
	QObject::connect(radioButton, SIGNAL(clicked()), slotHolder, SLOT(typeEntry()));
	radioButton->setObjectName("NewPartsEditorRadio");
	if (ctype == Connector::Pad) radioButton->setChecked(true);
	radioButton->setProperty("value", Connector::Pad);
	idLayout->addWidget(radioButton);
	radioButton->setProperty("index", index);
	radioButton->setProperty("type", "radio");
	idLayout->addSpacing(Spacing);

	idFrame->setLayout(idLayout);
	mainLayout->addWidget(idFrame);

	frame->setLayout(mainLayout);
	return frame;
}

bool PEUtils::fillInMetadata(int senderIndex, QWidget * parent, ConnectorMetadata & cmd)
{
	bool result = false;
	QList<QWidget *> widgets = parent->findChildren<QWidget *>();
	Q_FOREACH (QWidget * widget, widgets) {
		bool ok;
		int index = widget->property("index").toInt(&ok);
		if (!ok) continue;

		if (index != senderIndex) continue;

		QString type = widget->property("type").toString();
		if (type == "name") {
			auto * lineEdit = qobject_cast<QLineEdit *>(widget);
			if (lineEdit == nullptr) continue;

			cmd.connectorName = lineEdit->text();
			cmd.connectorID = widget->property("id").toString();
			result = true;
		}
		else if (type == "radio") {
			auto * radioButton = qobject_cast<QRadioButton *>(widget);
			if (radioButton == nullptr) continue;
			if (!radioButton->isChecked()) continue;

			cmd.connectorType = (Connector::ConnectorType) radioButton->property("value").toInt();
		}
		else if (type == "description") {
			auto * lineEdit = qobject_cast<QLineEdit *>(widget);
			if (lineEdit == nullptr) continue;

			cmd.connectorDescription = lineEdit->text();
		}

	}
	return result;
}
