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

$Revision: 6912 $:
$Author: irascibl@gmail.com $:
$Date: 2013-03-09 08:18:59 +0100 (Sa, 09. Mrz 2013) $

********************************************************************/

#include "peutils.h"
#include "petoolview.h"
#include "pegraphicsitem.h"
#include "../utils/textutils.h"
#include "../utils/graphicsutils.h"
#include "../debugdialog.h"

#include <QHBoxLayout>
#include <QTextStream>
#include <QSplitter>
#include <QPushButton>
#include <QLineEdit>
#include <QFile>
#include <QApplication>

//////////////////////////////////////

static QPixmap * CheckImage = NULL;
static QPixmap * NoCheckImage = NULL;

//////////////////////////////////////

PEDoubleSpinBox::PEDoubleSpinBox(QWidget * parent) : QDoubleSpinBox(parent)
{
}

void PEDoubleSpinBox::stepBy(int steps)
{
    double amount;
    emit getSpinAmount(amount);
    setSingleStep(amount);
    QDoubleSpinBox::stepBy(steps);
}

//////////////////////////////////////

PEToolView::PEToolView(QWidget * parent) : QFrame (parent)
{
    m_assignButton = NULL;

	if (CheckImage == NULL) CheckImage = new QPixmap(":/resources/images/icons/check.png");
	if (NoCheckImage == NULL) NoCheckImage = new QPixmap(":/resources/images/icons/nocheck.png");

    this->setObjectName("PEToolView");
    /*

    QFile styleSheet(":/resources/styles/newpartseditor.qss");
    if (!styleSheet.open(QIODevice::ReadOnly)) {
        DebugDialog::debug("Unable to open :/resources/styles/newpartseditor.qss");
    } else {
    	this->setStyleSheet(styleSheet.readAll());
    }
*/
    m_pegi = NULL;

    QVBoxLayout * mainLayout = new QVBoxLayout;
    mainLayout -> setObjectName("connectorFrame");
    QSplitter * splitter = new QSplitter(Qt::Vertical);
    mainLayout->addWidget(splitter);

    QFrame * connectorsFrame = new QFrame;

    QVBoxLayout * connectorsLayout = new QVBoxLayout;

    QLabel * label = new QLabel(tr("Connector List (a checkmark means the graphic was selected)"));
	connectorsLayout->addWidget(label);


    m_connectorListWidget = new QTreeWidget();
	m_connectorListWidget->setColumnCount(2);
	connect(m_connectorListWidget, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT(switchConnector(QTreeWidgetItem *, QTreeWidgetItem *)));
    connectorsLayout->addWidget(m_connectorListWidget);

    m_busModeBox = new QCheckBox(tr("Set Internal Connections"));
    m_busModeBox->setChecked(false);
    m_busModeBox->setToolTip(tr("Set this checkbox to edit internal connections by drawing wires"));
    connect(m_busModeBox, SIGNAL(clicked(bool)), this, SLOT(busModeChangedSlot(bool)));
    connectorsLayout->addWidget(m_busModeBox);

    connectorsFrame->setLayout(connectorsLayout);
    splitter->addWidget(connectorsFrame);

    m_connectorInfoGroupBox = new QGroupBox;
    m_connectorInfoLayout = new QVBoxLayout;

    m_connectorInfoWidget = new QFrame;             // a placeholder for PEUtils::connectorForm
    m_connectorInfoLayout->addWidget(m_connectorInfoWidget);             

	m_terminalPointGroupBox = new QGroupBox("Terminal point");
	m_terminalPointGroupBox->setToolTip(tr("Controls for setting the terminal point for a connector. The terminal point is where a wire will attach to the connector. You can also drag the crosshair of the current connector"));
    QVBoxLayout * anchorGroupLayout = new QVBoxLayout;

    QFrame * posRadioFrame = new QFrame;
    QHBoxLayout * posRadioLayout = new QHBoxLayout;

    QList<QString> positionNames;
    positionNames << "Center"  << "W" << "N" << "S" << "E";
    QList<QString> trPositionNames;
    trPositionNames << tr("Center") << tr("W") << tr("N") << tr("S") << tr("E");
    QList<QString> trLongNames;
    trLongNames << tr("center") << tr("west") << tr("north") << tr("south") << tr("east");
    for (int i = 0; i < positionNames.count(); i++) {
        QPushButton * button = new QPushButton(trPositionNames.at(i));
        button->setProperty("how", positionNames.at(i));
		button->setToolTip(tr("Sets the connector's terminal point to %1.").arg(trLongNames.at(i)));
        connect(button, SIGNAL(clicked()), this, SLOT(buttonChangeTerminalPoint()));
        posRadioLayout->addWidget(button);
        m_buttons.append(button);
    }

    posRadioLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));

    posRadioFrame->setLayout(posRadioLayout);
    anchorGroupLayout->addWidget(posRadioFrame);

    QFrame * posNumberFrame = new QFrame;
    QHBoxLayout * posNumberLayout = new QHBoxLayout;

    label = new QLabel("x");
    posNumberLayout->addWidget(label);

    m_terminalPointX = new PEDoubleSpinBox;
    m_terminalPointX->setDecimals(4);
    m_terminalPointX->setToolTip(tr("Modifies the x-coordinate of the terminal point"));
    posNumberLayout->addWidget(m_terminalPointX);
    connect(m_terminalPointX, SIGNAL(getSpinAmount(double &)), this, SLOT(getSpinAmountSlot(double &)), Qt::DirectConnection);
    connect(m_terminalPointX, SIGNAL(valueChanged(double)), this, SLOT(terminalPointEntry()));

    posNumberLayout->addSpacing(PEUtils::Spacing);

    label = new QLabel("y");
    posNumberLayout->addWidget(label);

    m_terminalPointY = new PEDoubleSpinBox;
    m_terminalPointY->setDecimals(4);
    m_terminalPointY->setToolTip(tr("Modifies the y-coordinate of the terminal point"));
    posNumberLayout->addWidget(m_terminalPointY);
    connect(m_terminalPointY, SIGNAL(getSpinAmount(double &)), this, SLOT(getSpinAmountSlot(double &)), Qt::DirectConnection);
    connect(m_terminalPointY, SIGNAL(valueChanged(double)), this, SLOT(terminalPointEntry()));

    posNumberLayout->addSpacing(PEUtils::Spacing);

    m_units = new QLabel();
    posNumberLayout->addWidget(m_units);
	posNumberLayout->addSpacing(PEUtils::Spacing);

	m_terminalPointDragState = new QLabel(tr("Dragging disabled"));
    posNumberLayout->addWidget(m_terminalPointDragState);

    posNumberLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));

    posNumberFrame->setLayout(posNumberLayout);
    anchorGroupLayout->addWidget(posNumberFrame);

    m_terminalPointGroupBox->setLayout(anchorGroupLayout);
    m_connectorInfoLayout->addWidget(m_terminalPointGroupBox);

	m_connectorInfoLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));

    m_connectorInfoGroupBox->setLayout(m_connectorInfoLayout);

	splitter->addWidget(m_connectorInfoGroupBox);

	//this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	//this->setWidget(splitter);

    this->setLayout(mainLayout);

    m_connectorListWidget->resize(m_connectorListWidget->width(), 0);

    enableConnectorChanges(false, false, false, false);

}

PEToolView::~PEToolView() 
{
}

void PEToolView::enableConnectorChanges(bool enableTerminalPointDrag, bool enableTerminalPointControls, bool enableInfo, bool enableAssign)
{
    if (m_assignButton) {
        m_assignButton->setEnabled(enableAssign);
    }
    if (m_terminalPointGroupBox) {
	    m_terminalPointGroupBox->setEnabled(enableTerminalPointControls);
    }
	if (m_connectorInfoWidget) {
		m_connectorInfoWidget->setEnabled(enableInfo);
	}

    if (m_terminalPointDragState) {
	    if (enableTerminalPointDrag) {
		    m_terminalPointDragState->setText(tr("<font color='black'>Dragging enabled</font>"));
		    m_terminalPointDragState->setEnabled(true);
	    }
	    else {
            m_terminalPointDragState->setText(tr("<font color='gray'>Dragging disabled</font>"));
		    m_terminalPointDragState->setEnabled(false);
	    }
    }
}

void PEToolView::initConnectors(QList<QDomElement> * connectorList) {
    m_connectorListWidget->blockSignals(true);

    m_connectorListWidget->clear();  // deletes QTreeWidgetItems
    m_connectorList = connectorList;

    for (int ix = 0; ix < connectorList->count(); ix++) {
		QDomElement connector = connectorList->at(ix);
		QTreeWidgetItem *item = new QTreeWidgetItem;
		item->setData(0, Qt::DisplayRole, connector.attribute("name"));
		item->setData(0, Qt::UserRole, ix);
		item->setData(1, Qt::UserRole, ix);
		item->setData(0, Qt::DecorationRole, *NoCheckImage);
		m_connectorListWidget->addTopLevelItem(item);
		QLabel * label = new QLabel("");
		//label->setPixmap(*NoCheckImage);
		m_connectorListWidget->setItemWidget(item, 1, label);
    }

    if (connectorList->count() > 0) {
        m_connectorListWidget->setCurrentItem(m_connectorListWidget->topLevelItem(0));
        switchConnector(m_connectorListWidget->currentItem(), NULL);
    }

    m_connectorListWidget->blockSignals(false);
}

void PEToolView::showAssignedConnectors(const QDomDocument * svgDoc, ViewLayer::ViewID viewID) {
	QDomElement svgRoot = svgDoc->documentElement();

	for (int i = 0; i < m_connectorListWidget->topLevelItemCount(); i++) {
		QTreeWidgetItem * item = m_connectorListWidget->topLevelItem(i);
		int index = item->data(0, Qt::UserRole).toInt();
		QDomElement connector = m_connectorList->at(index);
		QString svgID, terminalID;
        bool ok = ViewLayer::getConnectorSvgIDs(connector, viewID, svgID, terminalID);
		if (!ok) {
			continue;
		}

		QDomElement element = TextUtils::findElementWithAttribute(svgRoot, "id", svgID);
		if (element.isNull()) {
			item->setData(0, Qt::DecorationRole, *NoCheckImage);
		}
		else {
			item->setData(0, Qt::DecorationRole, *CheckImage);
		}
	}

	hideConnectorListStuff();
	m_connectorListWidget->repaint();
}

void PEToolView::switchConnector(QTreeWidgetItem * current, QTreeWidgetItem * previous) {
    Q_UNUSED(previous);

    QWidget * widget = QApplication::focusWidget();
    if (widget) {
        QList<QWidget *> children = m_connectorInfoWidget->findChildren<QWidget *>();
        if (children.contains(widget)) {
            widget->blockSignals(true);
        }
    }

    if (m_connectorInfoWidget) {
        delete m_connectorInfoWidget;
        m_connectorInfoWidget = NULL;
    }

    if (current == NULL) return;

    int index = current->data(0, Qt::UserRole).toInt();
    QDomElement element = m_connectorList->at(index);

    int pos = 99999;
    for (int ix = 0; ix < m_connectorInfoLayout->count(); ix++) {
        QLayoutItem * item = m_connectorInfoLayout->itemAt(ix);
        if (item->widget() == m_terminalPointGroupBox) {
            pos = ix;
            break;
        }
    }

    m_connectorInfoWidget = PEUtils::makeConnectorForm(element, index, this, false);
    m_connectorInfoLayout->insertWidget(pos, m_connectorInfoWidget);
    m_connectorInfoGroupBox->setTitle(tr("Connector %1").arg(element.attribute("name")));
    m_units->setText(QString("(%1)").arg(PEUtils::Units));

	hideConnectorListStuff();

    emit switchedConnector(index);
}

void PEToolView::busModeChangedSlot(bool state)
{
    emit busModeChanged(state);
}

void PEToolView::nameEntry() {
	changeConnector();
}

void PEToolView::typeEntry() {
	changeConnector();
}

void PEToolView::descriptionEntry() {
	changeConnector();
}

void PEToolView::changeConnector() {
	QTreeWidgetItem * item = m_connectorListWidget->currentItem();
    if (item == NULL) return;

    int index = item->data(0, Qt::UserRole).toInt();

    ConnectorMetadata cmd;
    if (!PEUtils::fillInMetadata(index, this, cmd)) return;

    emit connectorMetadataChanged(&cmd);
}

void PEToolView::setCurrentConnector(const QDomElement & newConnector) {
	for (int ix = 0; ix < m_connectorListWidget->topLevelItemCount(); ix++) {
		QTreeWidgetItem * item = m_connectorListWidget->topLevelItem(ix);
		int index = item->data(0, Qt::UserRole).toInt();
		QDomElement connector = m_connectorList->at(index);
		if (connector.attribute("id") == newConnector.attribute("id")) {
			m_connectorListWidget->setCurrentItem(item);
			hideConnectorListStuff();
			return;
		}
	}
}

int PEToolView::currentConnectorIndex() {
    QTreeWidgetItem * item = m_connectorListWidget->currentItem();
    if (item == NULL) return -1;

    int index = item->data(0, Qt::UserRole).toInt();
    return index;
}

void PEToolView::setTerminalPointCoords(QPointF p) {
    m_terminalPointX->blockSignals(true);
    m_terminalPointY->blockSignals(true);
    m_terminalPointX->setValue(PEUtils::convertUnits(p.x()));
    m_terminalPointY->setValue(PEUtils::convertUnits(p.y()));
    m_terminalPointX->blockSignals(false);
    m_terminalPointY->blockSignals(false);
}

void PEToolView::setTerminalPointLimits(QSizeF sz) {
    m_terminalPointX->setRange(0, sz.width());
    m_terminalPointY->setRange(0, sz.height());
}

void PEToolView::buttonChangeTerminalPoint() {
    QString how = sender()->property("how").toString();
    emit terminalPointChanged(how);
}

void PEToolView::terminalPointEntry()
{
    if (sender() == m_terminalPointX) {
        emit terminalPointChanged("x", PEUtils::unconvertUnits(m_terminalPointX->value()));
    }
    else if (sender() == m_terminalPointY) {
       emit terminalPointChanged("y", PEUtils::unconvertUnits(m_terminalPointY->value()));
    }
}

void PEToolView::getSpinAmountSlot(double & d) {
    emit getSpinAmount(d);
}


void PEToolView::removeConnector() {
    QTreeWidgetItem * item = m_connectorListWidget->currentItem();
    if (item == NULL) return;

    int index = item->data(0, Qt::UserRole).toInt();
    QDomElement element = m_connectorList->at(index);
    emit removedConnector(element);

}

void PEToolView::setChildrenVisible(bool vis)
{
	foreach (QWidget * widget, findChildren<QWidget *>()) {
		widget->setVisible(vis);
	}
}

void PEToolView::pickModeChangedSlot() {
	emit pickModeChanged(true);
}

void PEToolView::hideConnectorListStuff() {
	m_connectorListWidget->setHeaderHidden(true);
	QTreeWidgetItem * current = m_connectorListWidget->currentItem();

	for (int i = 0; i < m_connectorListWidget->topLevelItemCount(); i++) {
		QTreeWidgetItem * item = m_connectorListWidget->topLevelItem(i);
		QWidget * widget = m_connectorListWidget->itemWidget(item, 1);
		if (qobject_cast<QPushButton *>(widget)) {
			if (item == current) ;   // button is already there
			else {
				// remove the button and add the label
				m_connectorListWidget->removeItemWidget(item, 1);
				QLabel * label = new QLabel();
				//label->setPixmap(*NoCheckImage);
				m_connectorListWidget->setItemWidget(item, 1, label);
			}
		}
		else {
			if (item != current) ;		// label is already there
			else {
				m_assignButton = new QPushButton(tr("Select graphic"));
                m_assignButton->setMaximumWidth(150);
				connect(m_assignButton, SIGNAL(clicked()), this, SLOT(pickModeChangedSlot()), Qt::DirectConnection);
				m_assignButton->setToolTip(tr("Use the cursor location and mouse wheel to navigate to the SVG element which you want to assign to the current connector, then mouse down to select it."));
				m_connectorListWidget->setItemWidget(item, 1, m_assignButton);
			}
		}
	}
}

void PEToolView::setCurrentConnector(int ix) {
    m_connectorListWidget->setCurrentItem(m_connectorListWidget->topLevelItem(ix));
    switchConnector(m_connectorListWidget->currentItem(), NULL);
}
