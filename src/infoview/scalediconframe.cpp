/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2023 Fritzing GmbH

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

#include "scalediconframe.h"

#include <QScrollArea>
#include <QScrollBar>
#include <QDebug>
#include <QFrame>
#include <QPainter>
#include <QResizeEvent>

const int ScaledIconFrame::STANDARD_ICON_IMG_WIDTH = 160;
const int ScaledIconFrame::STANDARD_ICON_IMG_HEIGHT = 160;
constexpr int IconSpace = 2;


ScaledIconFrame::ScaledIconFrame(QWidget *parent)
    : QFrame(parent)
{
	m_iconLayout = new QHBoxLayout(this);

	NoIcon = QPixmap(STANDARD_ICON_IMG_WIDTH, STANDARD_ICON_IMG_HEIGHT);
	NoIcon.fill(QColorConstants::White);

//	// Create a QPainter to draw on the QPixmap
//	QPainter painter(&NoIcon);

//	// Draw a red rectangle onto the QPixmap
//	painter.setBrush(QColor("red"));
//	painter.drawRect(0, 0, 160, 160);

//	// Draw a blue circle onto the QPixmap
//	painter.setBrush(QColor("blue"));
//	painter.drawEllipse(50, 50, 60, 60);

//	// End the painting
//	painter.end();

//	m_iconLayout->addSpacing(IconSpace);
	m_icon1 = addLabel(m_iconLayout, NoIcon);
	m_icon2 = addLabel(m_iconLayout, NoIcon);
	m_icon3 = addLabel(m_iconLayout, NoIcon);
	m_icon1->setToolTip(tr("Part breadboard view image"));
	m_icon2->setToolTip(tr("Part schematic view image"));
	m_icon3->setToolTip(tr("Part pcb view image"));

	m_iconLayout->addSpacerItem(new QSpacerItem(IconSpace, 1, QSizePolicy::Expanding));

	setLayout(m_iconLayout);
	setMinimumSize(QSize(1,1));
}

QLabel *ScaledIconFrame::addLabel(QHBoxLayout *hboxLayout, const QPixmap &pixmap)
{
	auto *label = new QLabel();
	label->setObjectName("iconLabel");
	label->setAutoFillBackground(true);
	label->setPixmap(pixmap);
	label->setFixedSize(pixmap.size());
	label->setScaledContents(true);
	hboxLayout->addWidget(label);
	hboxLayout->addSpacing(IconSpace);

	return label;
}

void ScaledIconFrame::setIcons(const QPixmap * icon1, const QPixmap * icon2, const QPixmap * icon3)
{
	if ((icon1 == nullptr) || (icon1->isNull()))
		m_icon1->setPixmap(NoIcon);
	else
		m_icon1->setPixmap(*icon1);

	if ((icon2 == nullptr) || (icon2->isNull()))
		m_icon2->setPixmap(NoIcon);
	else
		m_icon2->setPixmap(*icon2);

	if ((icon3 == nullptr) || (icon3->isNull()))
		m_icon3->setPixmap(NoIcon);
	else
		m_icon3->setPixmap(*icon3);

}

void ScaledIconFrame::resizeEvent(QResizeEvent *event)
{
//	QSize previousSize = event->oldSize();
//	QSize currentSize = event->size();

//	qDebug() << "scaled previous size: " << previousSize
//	         << ", current size: " << currentSize;

	QFrame::resizeEvent(event);
	updateLayout();
}

void ScaledIconFrame::updateLayout()
{
	if (!m_icon1 || !m_icon2 || !m_icon3) {
		return;
	}

	int availableWidth = width() - IconSpace * 4 - 20;
	int totalIconWidth = m_icon1->sizeHint().width() + m_icon2->sizeHint().width() + m_icon3->sizeHint().width();

	qreal scaleFactor;
	if (totalIconWidth > availableWidth) {
		scaleFactor = qreal(availableWidth) / qreal(totalIconWidth);
	} else {
		scaleFactor = 1.0;
	}

	m_icon1->setFixedSize(m_icon1->sizeHint() * scaleFactor);
	m_icon2->setFixedSize(m_icon2->sizeHint() * scaleFactor);
	m_icon3->setFixedSize(m_icon3->sizeHint() * scaleFactor);
}
