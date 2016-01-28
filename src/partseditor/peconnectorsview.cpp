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

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QRadioButton>
#include <QMessageBox>
#include <QMutexLocker>
#include <QApplication>

#include "peconnectorsview.h"
#include "peutils.h"
#include "hashpopulatewidget.h"
#include "../debugdialog.h"

//////////////////////////////////////

PEConnectorsView::PEConnectorsView(QWidget * parent) : QFrame(parent)
{
	m_connectorCount = 0;

    this -> setObjectName("peConnectors");
    /*
    QFile styleSheet(":/resources/styles/newpartseditor.qss");
    if (!styleSheet.open(QIODevice::ReadOnly)) {
        DebugDialog::debug("Unable to open :/resources/styles/newpartseditor.qss");
    } else {
    	this->setStyleSheet(styleSheet.readAll());
    }
*/
	QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setSizeConstraint( QLayout::SetMinAndMaxSize );
	
    QLabel *explanation = new QLabel(tr("This is where you edit the connector metadata for the part"));
    mainLayout->addWidget(explanation);

    QFrame * numberFrame = new QFrame();
    QHBoxLayout * numberLayout = new QHBoxLayout();

    QLabel * label = new QLabel(tr("number of connectors:"));
    numberLayout->addWidget(label);

    m_numberEdit = new QLineEdit();
    QValidator *validator = new QIntValidator(1, 999, this);
    m_numberEdit->setValidator(validator);
    numberLayout->addWidget(m_numberEdit);
    connect(m_numberEdit, SIGNAL(editingFinished()), this, SLOT(connectorCountEntry()));

    numberLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));
    numberFrame->setLayout(numberLayout);
    mainLayout->addWidget(numberFrame);

    QFrame * typeFrame = new QFrame();
    QHBoxLayout * typeLayout = new QHBoxLayout();

    label = new QLabel(QObject::tr("Set all to:"));
	label->setObjectName("NewPartsEditorLabel");
    typeLayout->addWidget(label);

	m_radios.clear();
    QRadioButton * radioButton = new QRadioButton(MaleSymbolString); 
	QObject::connect(radioButton, SIGNAL(clicked()), this, SLOT(allTypeEntry()));
    radioButton->setObjectName("NewPartsEditorRadio");
    radioButton->setProperty("value", Connector::Male);
    typeLayout->addWidget(radioButton);
	m_radios.append(radioButton);

    radioButton = new QRadioButton(FemaleSymbolString); 
	QObject::connect(radioButton, SIGNAL(clicked()), this, SLOT(allTypeEntry()));
    radioButton->setObjectName("NewPartsEditorRadio");
    radioButton->setProperty("value", Connector::Female);
    typeLayout->addWidget(radioButton);
	m_radios.append(radioButton);

    radioButton = new QRadioButton(QObject::tr("Pad")); 
	QObject::connect(radioButton, SIGNAL(clicked()), this, SLOT(allTypeEntry()));
    radioButton->setObjectName("NewPartsEditorRadio");
    radioButton->setProperty("value", Connector::Pad);
    typeLayout->addWidget(radioButton);
	m_radios.append(radioButton);

	typeLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));
	typeFrame->setLayout(typeLayout);
	mainLayout->addWidget(typeFrame);

    QFrame * smdFrame = new QFrame();
    QHBoxLayout * smdLayout = new QHBoxLayout();

    m_tht = new QRadioButton(tr("Through-hole")); 
	QObject::connect(m_tht, SIGNAL(clicked()), this, SLOT(smdEntry()));
    m_tht->setObjectName("NewPartsEditorRadio");
    smdLayout->addWidget(m_tht);

    m_smd = new QRadioButton(tr("SMD")); 
	QObject::connect(m_smd, SIGNAL(clicked()), this, SLOT(smdEntry()));
    m_smd->setObjectName("NewPartsEditorRadio");
    smdLayout->addWidget(m_smd);

	smdLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));
	smdFrame->setLayout(smdLayout);
	mainLayout->addWidget(smdFrame);

	m_scrollArea = new QScrollArea;
	m_scrollArea->setWidgetResizable(true);
	m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	m_scrollFrame = new QFrame;
	m_scrollArea->setWidget(m_scrollFrame);

	mainLayout->addWidget(m_scrollArea);
	this->setLayout(mainLayout);

}

PEConnectorsView::~PEConnectorsView() {

}

void PEConnectorsView::initConnectors(QList<QDomElement> * connectorList) 
{
    QWidget * widget = QApplication::focusWidget();
    if (widget) {
        QList<QWidget *> children = m_scrollFrame->findChildren<QWidget *>();
        if (children.contains(widget)) {
            widget->blockSignals(true);
        }
    }

    if (m_scrollFrame) {
        m_scrollArea->setWidget(NULL);
        delete m_scrollFrame;
        m_scrollFrame = NULL;
    }

    m_connectorCount = connectorList->size();
    m_numberEdit->setText(QString::number(m_connectorCount));

	m_scrollFrame = new QFrame(this);
	m_scrollFrame->setObjectName("NewPartsEditorConnectors");
	QVBoxLayout *scrollLayout = new QVBoxLayout();

    int ix = 0;
    foreach (QDomElement connector, *connectorList) {
        QWidget * widget = PEUtils::makeConnectorForm(connector, ix++, this, true);
        scrollLayout->addWidget(widget);
    }

    scrollLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));
    m_scrollFrame->setLayout(scrollLayout);

    m_scrollArea->setWidget(m_scrollFrame);
}

void PEConnectorsView::nameEntry() {
	QLineEdit * lineEdit = qobject_cast<QLineEdit *>(sender());
	if (lineEdit != NULL && lineEdit->isModified()) {
		changeConnector();
        lineEdit->setModified(false);
	}
}

void PEConnectorsView::typeEntry() {
    changeConnector();
}

void PEConnectorsView::descriptionEntry() {
	QLineEdit * lineEdit = qobject_cast<QLineEdit *>(sender());
	if (lineEdit != NULL && lineEdit->isModified()) {
		changeConnector();
        lineEdit->setModified(false);
	}
}

void PEConnectorsView::connectorCountEntry() {
    if (!m_mutex.tryLock(1)) return;            // need the mutex because multiple editingFinished() signals can be triggered more-or-less at once
   
    QLineEdit * lineEdit = qobject_cast<QLineEdit *>(sender());
    if (lineEdit != NULL && lineEdit->isModified()) {
        int newCount = lineEdit->text().toInt();
        if (newCount != m_connectorCount) {
            m_connectorCount = newCount;
            emit connectorCountChanged(newCount);
            lineEdit->setModified(false);
        }
    }

    m_mutex.unlock();
}

void PEConnectorsView::removeConnector() {
    bool ok;
    int senderIndex = sender()->property("index").toInt(&ok);
    if (!ok) return;

    ConnectorMetadata cmd;
    if (!PEUtils::fillInMetadata(senderIndex, m_scrollFrame, cmd)) return;

    QList<ConnectorMetadata *> cmdList;
    cmdList.append(&cmd);
    emit removedConnectors(cmdList);
}

void PEConnectorsView::changeConnector() {
    bool ok;
    int senderIndex = sender()->property("index").toInt(&ok);
    if (!ok) return;

    ConnectorMetadata cmd;
    if (!PEUtils::fillInMetadata(senderIndex, m_scrollFrame, cmd)) return;

    emit connectorMetadataChanged(&cmd);
}

void PEConnectorsView::allTypeEntry() {
	QRadioButton * radio = qobject_cast<QRadioButton *>(sender());
	if (radio == NULL) return;

	bool ok;
	Connector::ConnectorType ct = (Connector::ConnectorType) radio->property("value").toInt(&ok);
	if (!ok) return;

	emit connectorsTypeChanged(ct);

	QTimer::singleShot(10, this, SLOT(uncheckRadios()));
}

void PEConnectorsView::smdEntry()
{
	QRadioButton * radio = qobject_cast<QRadioButton *>(sender());
	if (radio == NULL) return;

	emit smdChanged(radio == m_smd ? "smd" : "tht");
}

void PEConnectorsView::uncheckRadios() {
	// this doesn't work because the buttons are "autoexclusive"
	foreach (QRadioButton * radio, m_radios) {
		radio->setChecked(false);
	}
}

void PEConnectorsView::setSMD(bool smd)
{
	if (smd) m_smd->setChecked(true);
	else m_tht->setChecked(true);
}
