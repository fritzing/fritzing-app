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


#include "connectorsinfowidget.h"
#include "singleconnectorinfowidget.h"
#include "connectorinforemovebutton.h"
#include "../connectors/connectorshared.h"
#include "../debugdialog.h"

static const QString PadString = "Pad";

ConnectorTypeWidget::ConnectorTypeWidget(Connector::ConnectorType type, QWidget *parent)
	: QFrame(parent)
{
	m_isSelected = false;
	m_noEditionModeWidget = new QLabel(this);
	m_editionModeWidget = new QPushButton(this);
	connect(
		m_editionModeWidget, SIGNAL(clicked()),
		this, SLOT(toggleValue())
	);
	m_noEditionModeWidget->setFixedWidth(30);
	m_editionModeWidget->setFixedWidth(30);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setMargin(2);
	layout->setSpacing(0);
	layout->addWidget(m_noEditionModeWidget);
	layout->addWidget(m_editionModeWidget);

	setType(type);
}

Connector::ConnectorType ConnectorTypeWidget::type() {
	if(text()==FemaleSymbolString) {
		return Connector::connectorTypeFromName("female");
	} else if(text()==MaleSymbolString) {
		return Connector::connectorTypeFromName("male");
	}
	else if (text()==PadString) {
		return Connector::connectorTypeFromName("pad");
	}
	return Connector::Unknown;
}

const QString &ConnectorTypeWidget::typeAsStr() {
	return Connector::connectorNameFromType(type());
}

void ConnectorTypeWidget::setType(Connector::ConnectorType type) {
	if(m_isInEditionMode) {
		m_typeBackUp = this->type();
	}
	if(type == Connector::Female) {
		setText(FemaleSymbolString);
	} else if(type == Connector::Male) {
		setText(MaleSymbolString);
	}
	else if(type == Connector::Pad) {
		setText(PadString);
	}
	setToolTip(Connector::connectorNameFromType(type));
}

void ConnectorTypeWidget::setSelected(bool selected) {
	m_isSelected = selected;
}

void ConnectorTypeWidget::setText(const QString &text) {
	m_text = text;
	m_noEditionModeWidget->setText(text);
	m_editionModeWidget->setText(text);
}

const QString &ConnectorTypeWidget::text() {
	return m_text;
}

void ConnectorTypeWidget::setInEditionMode(bool edition) {
	m_isInEditionMode = edition;
	m_noEditionModeWidget->setVisible(!edition);
	m_editionModeWidget->setVisible(edition);
}

void ConnectorTypeWidget::cancel() {
	setType(m_typeBackUp);
}

void ConnectorTypeWidget::toggleValue() {
	if(m_isSelected && m_isInEditionMode) {
		if(type() == Connector::Female) {
			setType(Connector::Pad);
		} else if(type() == Connector::Male) {
			setType(Connector::Female);
		} else if (type() == Connector::Pad) {
			setType(Connector::Male);
		}
	}
}



SingleConnectorInfoWidget::SingleConnectorInfoWidget(ConnectorsInfoWidget *topLevelContainer, WaitPushUndoStack *undoStack, Connector* connector, QWidget *parent)
	: AbstractConnectorInfoWidget(topLevelContainer,parent)
{
	static QString EMPTY_CONN_NAME = QObject::tr("no name yet");
	static QString EMPTY_CONN_DESC = QObject::tr("no description yet");
	static Connector::ConnectorType EMPTY_CONN_TYPE = Connector::Male;

	QString name;
	QString description;
	Connector::ConnectorType type;

	m_undoStack = undoStack;
	m_connector = connector;

	if(connector && connector->connectorShared()) {
		m_id = connector->connectorSharedID();
		name = connector->connectorSharedName();
		if (name.isEmpty()) name = EMPTY_CONN_NAME;
		description = connector->connectorSharedDescription();
		if (description.isEmpty()) description = EMPTY_CONN_DESC;
		type = connector->connectorType();
		if (type == Connector::Unknown) type = EMPTY_CONN_TYPE;
	} else {
		name = EMPTY_CONN_NAME;
		description = EMPTY_CONN_DESC;
		type = EMPTY_CONN_TYPE;
	}

	m_nameLabel = new QLabel(name,this);
	m_nameDescSeparator = new QLabel(" - ",this);
	m_descLabel = new QLabel(description,this);
	m_descLabel->setObjectName("description");

	m_type = new ConnectorTypeWidget(type, this);

	m_nameEdit = NULL;
	m_descEdit = NULL;

	m_nameEditContainer = new QFrame(this);
	QHBoxLayout *nameLO = new QHBoxLayout(m_nameEditContainer);
	nameLO->setSpacing(0);
	nameLO->setMargin(0);
	m_nameEditContainer->hide();

	m_descEditContainer = new QFrame(this);
	QVBoxLayout *descLO = new QVBoxLayout(m_descEditContainer);
	descLO->setSpacing(0);
	descLO->setMargin(0);
	m_descEditContainer->hide();

	m_acceptButton = NULL;
	m_cancelButton = NULL;

	setSelected(false);


	QLayout *layout = new QVBoxLayout(this);
	layout->setMargin(1);

	toStandardMode();
}

QString SingleConnectorInfoWidget::id() {
	if (m_connector) {
		return m_connector->connectorSharedID();
	}

	return m_id;
}
QString SingleConnectorInfoWidget::name() {
	return m_nameLabel->text();
}

QString SingleConnectorInfoWidget::description() {
	return m_descLabel->text();
}

QString SingleConnectorInfoWidget::type() {
	return m_type->typeAsStr();
}

void SingleConnectorInfoWidget::startEdition() {
	createInputs();

	m_nameEdit->setText(m_nameLabel->text());
	m_descEdit->setText(m_descLabel->text());
	toEditionMode();

	emit editionStarted();
}

void SingleConnectorInfoWidget::createInputs() {
	if(!m_nameEdit) {
		m_nameEdit = new QLineEdit(this);
		m_nameEditContainer->layout()->addWidget(new QLabel(tr("Name: "),this));
		m_nameEditContainer->layout()->addWidget(m_nameEdit);
	}
	if(!m_descEdit) {
		m_descEdit = new QTextEdit(this);
		m_descEditContainer->layout()->addWidget(new QLabel(tr("Description:"),this));
		m_descEditContainer->layout()->addWidget(m_descEdit);
	}
}

void SingleConnectorInfoWidget::editionCompleted() {
	if(m_isInEditionMode) {
		m_undoStack->push(new QUndoCommand("Dummy parts editor command"));
		m_nameLabel->setText(m_nameEdit->text());
		m_descLabel->setText(m_descEdit->toPlainText());
		toStandardMode();

		emit editionFinished();
	}
}

void SingleConnectorInfoWidget::editionCanceled() {
	m_type->cancel();
	toStandardMode();

	emit editionFinished();
}


void SingleConnectorInfoWidget::toStandardMode() {
	hide();

	setInEditionMode(false);

	hideIfNeeded(m_nameEditContainer);
	hideIfNeeded(m_descEditContainer);
	hideIfNeeded(m_acceptButton);
	hideIfNeeded(m_cancelButton);

	QHBoxLayout *hbLayout = new QHBoxLayout();
	hbLayout->addWidget(m_type);
	hbLayout->addSpacerItem(new QSpacerItem(10,0));
	hbLayout->addWidget(m_nameLabel);
	m_nameLabel->show();
	hbLayout->addWidget(m_nameDescSeparator);
	m_nameDescSeparator->show();
	hbLayout->addWidget(m_descLabel);
	m_descLabel->show();
	hbLayout->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::Expanding));
	hbLayout->addWidget(m_removeButton);

	QVBoxLayout *layout = (QVBoxLayout*)this->layout();
	layout->addLayout(hbLayout);

	setFixedHeight(SingleConnectorHeight);
	setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
	updateGeometry();

	show();
	setFocus();
}

void SingleConnectorInfoWidget::toEditionMode() {
	hide();

	setInEditionMode(true);

	hideIfNeeded(m_nameLabel);
	hideIfNeeded(m_nameDescSeparator);
	hideIfNeeded(m_descLabel);

	createInputs();

	// first row
	QHBoxLayout *firstRowLayout = new QHBoxLayout();
	firstRowLayout->addWidget(m_type);
	firstRowLayout->addSpacerItem(new QSpacerItem(10,0));
	firstRowLayout->addWidget(m_nameEditContainer);
	firstRowLayout->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::Expanding));
	firstRowLayout->addWidget(m_removeButton);
	m_nameEditContainer->show();

	// second row
	m_descEditContainer->show();

	// third row
	if(!m_acceptButton) {
		m_acceptButton = new QPushButton(QObject::tr("Accept"),this);
		connect(m_acceptButton,SIGNAL(clicked()),this,SLOT(editionCompleted()));
	}
	m_acceptButton->show();

	if(!m_cancelButton) {
		m_cancelButton = new QPushButton(QObject::tr("Cancel"),this);
		connect(m_cancelButton,SIGNAL(clicked()),this,SLOT(editionCanceled()));
	}
	m_cancelButton->show();

	QHBoxLayout *thirdRowLayout = new QHBoxLayout();
	thirdRowLayout->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::Expanding));
	thirdRowLayout->addWidget(m_acceptButton);
	thirdRowLayout->addWidget(m_cancelButton);


	QVBoxLayout *layout = (QVBoxLayout*)this->layout();
	layout->addLayout(firstRowLayout);
	layout->addWidget(m_descEditContainer);
	layout->addLayout(thirdRowLayout);


	setFixedHeight(SingleConnectorHeight*4);
	setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
	updateGeometry();

	show();
	setFocus();

	emit editionStarted();
}

void SingleConnectorInfoWidget::hideIfNeeded(QWidget* w) {
	if(w) {
		w->hide();
		layout()->removeWidget(w);
	}
}


QSize SingleConnectorInfoWidget::maximumSizeHint() const {
	return sizeHint();
}

QSize SingleConnectorInfoWidget::minimumSizeHint() const {
	return sizeHint();
}

QSize SingleConnectorInfoWidget::sizeHint() const {
	QSize retval;
	if(m_isInEditionMode) {
		retval = QSize(width(),SingleConnectorHeight*4);
	} else {
		retval = QSize(width(),SingleConnectorHeight);
	}

	return retval;
}

void SingleConnectorInfoWidget::setSelected(bool selected, bool doEmitChange) {
	AbstractConnectorInfoWidget::setSelected(selected, doEmitChange);
	m_type->setSelected(selected);

	if(selected && m_connector) {
		emit tellViewsMyConnectorIsNewSelected(m_connector->connectorSharedID());
	}
}

void SingleConnectorInfoWidget::setInEditionMode(bool inEditionMode) {
	m_isInEditionMode = inEditionMode;
	m_type->setInEditionMode(inEditionMode);
}

bool SingleConnectorInfoWidget::isInEditionMode() {
	return m_isInEditionMode;
}

void SingleConnectorInfoWidget::mousePressEvent(QMouseEvent * event) {
	if(isSelected() && !isInEditionMode()) {
		startEdition();
	} else if(!isSelected()) {
		setSelected(true);
	}
	QFrame::mousePressEvent(event);
}

Connector *SingleConnectorInfoWidget::connector() {
	return m_connector;
}

MismatchingConnectorWidget *SingleConnectorInfoWidget::toMismatching(ViewLayer::ViewIdentifier missingViewId) {
	MismatchingConnectorWidget *mcw = new MismatchingConnectorWidget(m_topLevelContainer,missingViewId, m_connector->connectorSharedID(), (QWidget*)parent(), false, m_connector);
	return mcw;
}

Connector::ConnectorType SingleConnectorInfoWidget::connectorType() {
	if (m_type == NULL) return Connector::Unknown;

	return m_type->type();
}

void SingleConnectorInfoWidget::setConnectorType(Connector::ConnectorType type) {
	if (m_type) {
		m_type->setType(type);
	}
}

void SingleConnectorInfoWidget::setName(const QString & name)
{
	m_nameLabel->setText(name);
	if (m_nameEdit) {
		m_nameEdit->setText(name);
	}
}

