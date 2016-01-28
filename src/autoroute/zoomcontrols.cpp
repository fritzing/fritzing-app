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

#include "zoomcontrols.h"
#include "../debugdialog.h"
#include "../utils/zoomslider.h"

ZoomButton::ZoomButton(QBoxLayout::Direction dir, ZoomButton::ZoomType type, ZoomableGraphicsView* view, QWidget *parent) : QLabel(parent)
{
	QString imgPath = ":/resources/images/icons/partsEditorZoom%1%2Button.png";
	QString typeStr = type==ZoomButton::ZoomIn? "In": "Out";
	QString dirStr;
	if(dir == QBoxLayout::LeftToRight || dir == QBoxLayout::RightToLeft) {
		dirStr = "Hor";
	} else if(dir == QBoxLayout::TopToBottom || dir == QBoxLayout::BottomToTop) {
		dirStr = "Ver";
	}
	imgPath = imgPath.arg(typeStr).arg(dirStr);
	m_step = 5*ZoomSlider::ZoomStep;
	m_type = type;

	m_owner = view;
	connect(this, SIGNAL(clicked()), this, SLOT(zoom()) );
	setPixmap(QPixmap(imgPath));
}

void ZoomButton::zoom() {
	int inOrOut = m_type == ZoomButton::ZoomIn? 1: -1;
	m_owner->relativeZoom(inOrOut*m_step, false);
	m_owner->ensureFixedToBottomRightItems();
}

void ZoomButton::mousePressEvent(QMouseEvent *event) {
	//QLabel::mousePressEvent(event);
	Q_UNUSED(event);
	emit clicked();
}

void ZoomButton::enterEvent(QEvent *event) {
	QLabel::enterEvent(event);
}

void ZoomButton::leaveEvent(QEvent *event) {
	QLabel::leaveEvent(event);
}


///////////////////////////////////////////////////////////

ZoomControlsPrivate::ZoomControlsPrivate(ZoomableGraphicsView* view, QBoxLayout::Direction dir, QWidget *parent) : QFrame(parent)
{
	//setObjectName("zoomControls");

	m_zoomInButton = new ZoomButton(dir, ZoomButton::ZoomIn, view, this);
	m_zoomOutButton = new ZoomButton(dir, ZoomButton::ZoomOut, view, this);

	m_boxLayout = new QBoxLayout(dir,this);
	m_boxLayout->addWidget(m_zoomInButton);
	m_boxLayout->addWidget(m_zoomOutButton);
	m_boxLayout->setMargin(2);
	m_boxLayout->setSpacing(2);

	setStyleSheet("background-color: transparent;");
}

///////////////////////////////////////////////////////////

ZoomControls::ZoomControls(ZoomableGraphicsView *view, QWidget *parent)
	: ZoomControlsPrivate(view, QBoxLayout::RightToLeft, parent)
{
	m_zoomLabel = new QLabel(this);
	m_zoomLabel->setFixedWidth(35);
	connect(view, SIGNAL(zoomChanged(double)),this,SLOT(updateLabel(double)));
	m_boxLayout->insertWidget(1,m_zoomLabel); // in the middle
	m_boxLayout->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::Expanding,QSizePolicy::Minimum)); // at the beginning
	updateLabel(view->currentZoom());
}

void ZoomControls::updateLabel(double zoom) {
	m_zoomLabel->setText(QString("%1%").arg((int)zoom));
}
