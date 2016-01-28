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



#include <QFrame>
#include <QGridLayout>
#include <QScrollBar>

#include "partspecificationswidget.h"
#include "../debugdialog.h"

PartSpecificationsWidget::PartSpecificationsWidget(QList<QWidget*> widgets, QWidget *parent)
	: QScrollArea(parent)
{
	m_scrollContent = new QFrame(this);
	m_scrollContent->setObjectName("scroll");
	//m_scrollContent->setSizePolicy(QSizePolicy::M,QSizePolicy::Expanding);

	QGridLayout *layout = new QGridLayout(m_scrollContent);
	for(int i=0; i < widgets.size(); i++) {
		//widgets[i]->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
		widgets[i]->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
		layout->addWidget(widgets[i],i,0);
		if(widgets[i]->metaObject()->indexOfSignal(QMetaObject::normalizedSignature("editionStarted()")) > -1) {
			connect(widgets[i],SIGNAL(editionStarted()),this,SLOT(updateLayout()));
		}
		if(widgets[i]->metaObject()->indexOfSignal(QMetaObject::normalizedSignature("editionFinished()")) > -1) {
			connect(widgets[i],SIGNAL(editionFinished()),this,SLOT(updateLayout()));
		}
	}
	layout->setMargin(0);
	layout->setSpacing(10);

	setWidget(m_scrollContent);
	setMinimumWidth(m_scrollContent->width()+15); //scrollbar
	setWidgetResizable(true);

	QGridLayout *mylayout = new QGridLayout(this);
	mylayout->setMargin(0);
	mylayout->setSpacing(0);

	//resize(sizeHint());
}

void PartSpecificationsWidget::updateLayout() {
	m_scrollContent->adjustSize();
}

QSize PartSpecificationsWidget::sizeHint() {
	//return QSize(width(),600);
	return QScrollArea::sizeHint();
}

