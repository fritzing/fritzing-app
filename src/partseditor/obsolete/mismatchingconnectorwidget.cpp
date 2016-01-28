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
#include <QPushButton>
#include "mismatchingconnectorwidget.h"
#include "connectorinforemovebutton.h"
#include "../debugdialog.h"

QList<ViewLayer::ViewIdentifier> MismatchingConnectorWidget::AllViews;

//TODO Mariano: looks like an abstracteditable, perhaps can be one
MismatchingConnectorWidget::MismatchingConnectorWidget(ConnectorsInfoWidget *topLevelContainer, ViewLayer::ViewIdentifier viewId, const QString &connId, QWidget *parent, bool isInView, Connector* conn)
	: AbstractConnectorInfoWidget(topLevelContainer, parent)
{
	if(AllViews.size() == 0) {
		AllViews << ViewLayer::BreadboardView << ViewLayer::SchematicView << ViewLayer::PCBView;
	}

	m_prevConn = conn;
	m_connId = connId;

	m_connIdLabel = new QLabel(m_connId+":", this);
	m_connIdLabel->setObjectName("mismatchConnId");

	m_connMsgLabel = new QLabel(viewsString(),this);
	m_connMsgLabel->setObjectName("mismatchConnMsg");

	QLabel *errorImg = new QLabel(this);
	errorImg->setPixmap(QPixmap(":/resources/images/error_x_small.png"));

	if(isInView) {
		m_missingViews << ViewLayer::BreadboardView << ViewLayer::SchematicView << ViewLayer::PCBView;
		addViewPresence(viewId);
	} else {
		removeViewPresence(viewId);
	}

	QPushButton *completeConnBtn = new QPushButton(tr("fix this!"),this);
	connect(completeConnBtn,SIGNAL(clicked()),this,SLOT(emitCompletion()));

	QHBoxLayout *lo = new QHBoxLayout();
	lo->addWidget(errorImg);
	lo->addSpacerItem(new QSpacerItem(5,0));
	lo->addWidget(m_connIdLabel);
	lo->addWidget(m_connMsgLabel);
	lo->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::Expanding,QSizePolicy::Expanding));
	lo->addWidget(completeConnBtn);
	lo->addSpacerItem(new QSpacerItem(10,0,QSizePolicy::Minimum,QSizePolicy::Expanding));
	lo->addWidget(m_removeButton);
	lo->setMargin(3);
	lo->setSpacing(3);
	this->setLayout(lo);

	//updateGeometry();
	setMaximumHeight(SingleConnectorHeight);
}

MismatchingConnectorWidget::~MismatchingConnectorWidget() {
}

void MismatchingConnectorWidget::setSelected(bool selected, bool doEmitChange) {
	AbstractConnectorInfoWidget::setSelected(selected, doEmitChange);

	if(selected && !m_connId.isNull()) {
		emit tellViewsMyConnectorIsNewSelected(m_connId);
	}
}

bool MismatchingConnectorWidget::onlyMissingThisView(ViewLayer::ViewIdentifier viewId) {
	return m_missingViews.size() == 1 && m_missingViews[0] == viewId;
}

void MismatchingConnectorWidget::addViewPresence(ViewLayer::ViewIdentifier viewId) {
	m_missingViews.removeAll(viewId);
	m_connMsgLabel->setText(viewsString());
}

void MismatchingConnectorWidget::removeViewPresence(ViewLayer::ViewIdentifier viewId) {
	m_missingViews << viewId;
	m_connMsgLabel->setText(viewsString());
}

bool MismatchingConnectorWidget::presentInAllViews() {
	return m_missingViews.size() == 0;
}

const QString &MismatchingConnectorWidget::connId() {
	return m_connId;
}

Connector *MismatchingConnectorWidget::prevConn() {
	return m_prevConn;
}

QList<ViewLayer::ViewIdentifier> MismatchingConnectorWidget::views() {
	QList<ViewLayer::ViewIdentifier> list = AllViews;
	for(int i=0; i < m_missingViews.size(); i++) {
		list.removeAll(m_missingViews[i]);
	}
	return list;
}

QList<ViewLayer::ViewIdentifier> MismatchingConnectorWidget::missingViews() {
	return m_missingViews;
}

QString MismatchingConnectorWidget::viewsString() {
	QString retval = tr("In ");
	bool notFirst = false;
	for(int i=0; i < AllViews.size(); i++) {
		ViewLayer::ViewIdentifier viewId = AllViews[i];
		if(!m_missingViews.contains(viewId)) {
			if(notFirst) {
				retval += tr("and ");
			}
			retval += ViewLayer::viewIdentifierNaturalName(viewId)+" ";
			notFirst = true;
		}
	}
	retval += tr("view only");
	return retval;
}

void MismatchingConnectorWidget::mousePressEvent(QMouseEvent * event) {
	if(!isSelected()) {
		setSelected(true);
	}
	QFrame::mousePressEvent(event);
}

void MismatchingConnectorWidget::emitCompletion() {
	emit completeConn(this);
}
