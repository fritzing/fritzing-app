/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2012 Fachhochschule Potsdam - http://fh-potsdam.de

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
#include <QGraphicsScene>
#include <QFile>
#include <QTimer>
#include <QSettings>
#include <QPainter>

#include "sketchmainhelp.h"
#include "../utils/expandinglabel.h"

double SketchMainHelp::OpacityLevel = 0.5;

SketchMainHelpCloseButton::SketchMainHelpCloseButton(const QString &imagePath, QWidget *parent)
	:QLabel(parent)
{
	m_pixmap = QPixmap(
		QString(":/resources/images/inViewHelpCloseButton%1.png").arg(imagePath));
	setPixmap(m_pixmap);
	setFixedHeight(m_pixmap.height());
}

void SketchMainHelpCloseButton::mousePressEvent(QMouseEvent * event) {
	emit clicked();
	QLabel::mousePressEvent(event);
}

void SketchMainHelpCloseButton::doShow() {
	setPixmap(m_pixmap);
}

void SketchMainHelpCloseButton::doHide() {
	setPixmap(0);
}


//////////////////////////////////////////////////////////////

SketchMainHelpPrivate::SketchMainHelpPrivate (
		const QString &viewString,
		const QString &htmlText,
		SketchMainHelp *parent)
	: QFrame()
{
	setObjectName("sketchMainHelp"+viewString);
	m_parent = parent;

	QFrame *main = new QFrame(this);
	QHBoxLayout *mainLayout = new QHBoxLayout(main);

	QLabel *imageLabel = new QLabel(this);
	QLabel *imageLabelAux = new QLabel(imageLabel);
	imageLabelAux->setObjectName(QString("inviewHelpImage%1").arg(viewString));
	QPixmap pixmap(QString(":/resources/images/helpImage%1.png").arg(viewString));
	imageLabelAux->setPixmap(pixmap);
	imageLabel->setFixedWidth(pixmap.width());
	imageLabel->setFixedHeight(pixmap.height());
	imageLabelAux->setFixedWidth(pixmap.width());
	imageLabelAux->setFixedHeight(pixmap.height());

	ExpandingLabel *textLabel = new ExpandingLabel(this);
	textLabel->setLabelText(htmlText);
	textLabel->setFixedWidth(430 - 41 - pixmap.width());
	textLabel->allTextVisible();
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
	textLabel->setToolTip("");
	textLabel->setAlignment(Qt::AlignLeft);

	mainLayout->setSpacing(6);
	mainLayout->setMargin(2);
	mainLayout->addWidget(imageLabel);
	mainLayout->addWidget(textLabel);
	setFixedWidth(430);

	QVBoxLayout *layout = new QVBoxLayout(this);
	m_closeButton = new SketchMainHelpCloseButton(viewString,this);
	connect(m_closeButton, SIGNAL(clicked()), this, SLOT(doClose()));

	QFrame *bottomMargin = new QFrame(this);
	bottomMargin->setFixedHeight(m_closeButton->height());

	layout->addWidget(m_closeButton);
	layout->addWidget(main);
	layout->addWidget(bottomMargin);

	layout->setSpacing(0);
	layout->setMargin(2);

	m_shouldGetTransparent = false;
	//m_closeButton->doHide();

	QFile styleSheet(":/resources/styles/inviewhelp.qss");
    if (!styleSheet.open(QIODevice::ReadOnly)) {
		qWarning("Unable to open :/resources/styles/inviewhelp.qss");
	} else {
		setStyleSheet(styleSheet.readAll());
	}
}

void SketchMainHelpPrivate::doClose() {
	m_parent->doClose();
}

void SketchMainHelpPrivate::enterEvent(QEvent * event) {
	enterEventAux();
	QFrame::enterEvent(event);
}

void SketchMainHelpPrivate::enterEventAux() {
	if(m_shouldGetTransparent) {
		setWindowOpacity(1.0);
		QTimer::singleShot(2000, this, SLOT(setTransparent()));
	}
	//m_closeButton->doShow();
}

void SketchMainHelpPrivate::setTransparent() {
	setWindowOpacity(SketchMainHelp::OpacityLevel);
}

void SketchMainHelpPrivate::leaveEvent(QEvent * event) {
	leaveEventAux();
	QFrame::leaveEvent(event);
}

void SketchMainHelpPrivate::leaveEventAux() {
	if(m_shouldGetTransparent) {
		setTransparent();
	}
	//m_closeButton->doHide();
}

bool SketchMainHelpPrivate::forwardMousePressEvent(QMouseEvent * event)
{
	QPoint p = m_closeButton->mapFromParent(event->pos());
	if (m_closeButton->rect().contains(p)) {
		doClose();
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////

SketchMainHelp::SketchMainHelp (
		const QString &viewString,
		const QString &htmlText,
		bool doShow
	) : QGraphicsProxyWidget()
{
	m_mouseWithin = false;
	m_visible = true;
	m_pixmap = NULL;
	setObjectName("sketchMainHelp"+viewString);
	m_son = new SketchMainHelpPrivate(viewString, htmlText, this);
	setWidget(m_son);
	if(!doShow) loadState();
}

SketchMainHelp::~SketchMainHelp()
{
	if (m_pixmap != NULL) {
		delete m_pixmap;
		m_pixmap = NULL;
	}
}

void SketchMainHelp::doClose() {
	doSetVisible(false);
}

void SketchMainHelp::doSetVisible(bool visible) {
	m_visible = visible;
	setVisible(visible);
	saveState();
}

void SketchMainHelp::setTransparent() {
	m_son->setWindowOpacity(OpacityLevel);
	m_son->m_shouldGetTransparent = true;
}

void SketchMainHelp::saveState() {
	QSettings settings;
	QString prop = objectName()+"Visibility";
	settings.setValue(prop,QVariant::fromValue(m_visible));
}

void SketchMainHelp::loadState() {
	QSettings settings;
	QString prop = objectName()+"Visibility";
	bool visible = settings.contains(prop)
		? settings.value(prop).toBool()
		: true;
	doSetVisible(visible);
}

const QPixmap & SketchMainHelp::getPixmap() {
	if (m_pixmap == NULL) {
		m_pixmap = new QPixmap(m_son->size());
		m_son->render(m_pixmap);
	}

	return *m_pixmap;
}

bool SketchMainHelp::getVisible() {
	return m_visible;
}

bool SketchMainHelp::setMouseWithin(bool within) {
	if (within == m_mouseWithin) return false;

	if (m_pixmap) {
		delete m_pixmap;
		m_pixmap = NULL;
	}
	m_mouseWithin = within;
	(within) ? m_son->enterEventAux() : m_son->leaveEventAux();
	return true;
}

bool SketchMainHelp::forwardMousePressEvent(QMouseEvent * event) {
	bool result = m_son->forwardMousePressEvent(event);

	if (result) {
		if (m_pixmap) {
			delete m_pixmap;
			m_pixmap = NULL;
		}
	}

	return result;
}
