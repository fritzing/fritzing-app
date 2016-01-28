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



#include "abstractconnectorinfowidget.h"
#include "connectorinforemovebutton.h"
#include "connectorsinfowidget.h"
#include "../utils/misc.h"

#include <QVariant>

int AbstractConnectorInfoWidget::SingleConnectorHeight = 40;

AbstractConnectorInfoWidget::AbstractConnectorInfoWidget(ConnectorsInfoWidget *topLevelContainer, QWidget *parent)
	: QFrame(parent)
{
	m_topLevelContainer = topLevelContainer;
	m_removeButton = new ConnectorInfoRemoveButton(this);

	connect(
		m_removeButton, SIGNAL(clicked(AbstractConnectorInfoWidget*)),
		topLevelContainer, SLOT(removeConnector(AbstractConnectorInfoWidget*))
	);

	setMinimumWidth(100);

	setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
	setMinimumHeight(SingleConnectorHeight);
}

void AbstractConnectorInfoWidget::setSelected(bool selected, bool doEmitChange) {
	m_isSelected = selected;
	setProperty("selected",m_isSelected);

	reapplyStyle();

	if(selected) {
		setFocus();
		if(doEmitChange) {
			emit tellSistersImNewSelected(this);
		}
	}
}

void AbstractConnectorInfoWidget::reapplyStyle() {
	QString path = ":/resources/styles/partseditor.qss";
	QFile styleSheet(path);

	if (!styleSheet.open(QIODevice::ReadOnly)) {
		qWarning("Unable to open :/resources/styles/partseditor.qss");
	} else {
		setStyleSheet(styleSheet.readAll());
	}
}

bool AbstractConnectorInfoWidget::isSelected() {
	return m_isSelected;
}
