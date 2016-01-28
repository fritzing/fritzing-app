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

#include <QLineEdit>
#include <QKeyEvent>
#include <QFile>
#include <QTextStream>
#include <QTimer>
#include <QHBoxLayout>
#include <QValidator>
#include <QLabel>
#include <limits>

#include "zoomslider.h"
#include "../debugdialog.h"

double ZoomSlider::ZoomStep;
QList<double> ZoomSlider::ZoomFactors;

static const int MIN_VALUE = 10;
static const int STARTING_VALUE = 100;
static const int HEIGHT = 16;
static const int STEP = 100;

static int AutoRepeatDelay = 0;
static int AutoRepeatInterval = 0;

///////////////////////////////////////////////

ZoomLabel::ZoomLabel(QWidget * parent) : QLabel(parent)
{
	if (AutoRepeatDelay == 0) {
		QPushButton button;
		AutoRepeatInterval = button.autoRepeatInterval();
		AutoRepeatDelay = button.autoRepeatDelay();
	}
	m_autoRepeat = m_mouseIsDown = m_mouseIsIn = m_repeated = false;
	m_timer.setSingleShot(false);
	m_timer.setInterval(AutoRepeatInterval);
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    m_timer.setTimerType(Qt::PreciseTimer);
#endif
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(repeat()));
}

ZoomLabel::~ZoomLabel()
{
}

void ZoomLabel::setAutoRepeat(bool autoRepeat)
{
	m_autoRepeat = autoRepeat;
}

void ZoomLabel::setImages(const QString & normal, const QString & pressed)
{
	m_normal.load(normal);
	m_pressed.load(pressed);
	setPixmap(m_normal);
}


void ZoomLabel::repeat()
{
	if (m_mouseIsIn && m_mouseIsDown) {
		m_repeated = true;	
		emit clicked();
	}
}

void ZoomLabel::mousePressEvent(QMouseEvent * event)
{
	m_timer.stop();
	this->setMouseTracking(true);
	this->setPixmap(m_pressed);
	QLabel::mousePressEvent(event);
	m_mouseIsDown = m_mouseIsIn = true;
	m_repeated = false;
	if (m_autoRepeat) {
		m_timer.start(AutoRepeatDelay);
	}
}

void ZoomLabel::mouseMoveEvent(QMouseEvent * event)
{
	QLabel::mouseMoveEvent(event);
	if (m_mouseIsDown) {
		QPoint p = event->pos();
		bool mouseIsIn = p.x() >= 0 && p.y() >= 0 && p.x() <= width() && p.y() <= height();
		if (m_mouseIsIn && !mouseIsIn) {
			m_timer.stop();
			this->setPixmap(m_normal);
			m_mouseIsIn = false;
		}
		else if (!m_mouseIsIn && mouseIsIn) {
			this->setPixmap(m_pressed);
			m_mouseIsIn = true;
			m_timer.start(AutoRepeatDelay);
		}
	}
}

void ZoomLabel::mouseReleaseEvent(QMouseEvent * event)
{
	m_timer.stop();
	this->setMouseTracking(false);
	m_mouseIsDown = false;
	this->setPixmap(m_normal);
	QLabel::mouseReleaseEvent(event);
	if (!m_repeated && m_mouseIsIn) {
		emit clicked();
	}
}

/////////////////////////////////////////////

ZoomSlider::ZoomSlider(int maxValue, QWidget * parent) : QFrame(parent) 
{
	// layout doesn't seem to work: the slider appears too far down in the status bar
	// because the status bar layout is privileged for the message text

	m_firstTime = true;
	if (ZoomFactors.size() == 0) {
		loadFactors();
	}

	this->setObjectName("ZoomSliderFrame");


	m_lineEdit = new QLineEdit(this);
    m_lineEdit->setObjectName("ZoomSliderValue");
	m_lineEdit->setText(QString("%1").arg(STARTING_VALUE));
	m_lineEdit->setValidator(new QIntValidator(MIN_VALUE, maxValue + MIN_VALUE, this));
	m_lineEdit->setAttribute(Qt::WA_MacShowFocusRect, 0);
	m_lineEdit->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_suffix = new QLabel(tr("%"), this);
    m_suffix->setObjectName("ZoomSliderLabel");

    m_minusButton = new ZoomLabel(this);
	m_minusButton->setImages(":/resources/images/icons/zoomSliderMinus.png", ":/resources/images/icons/zoomSliderMinusPressed.png");
    m_minusButton->setAutoRepeat(true);
	m_minusButton->setObjectName("ZoomSliderButton");
	connect(m_minusButton, SIGNAL(clicked()), this, SLOT(minusClicked()));

    m_slider = new QSlider(this);
	m_slider->setObjectName("ZoomSliderSlider");
	m_slider->setOrientation(Qt::Horizontal);
	m_slider->setRange(MIN_VALUE, maxValue + MIN_VALUE);
	m_slider->setValue(STARTING_VALUE);
    m_slider->setTickPosition(QSlider::TicksBelow);
    m_slider->setTickInterval(500);

    m_plusButton = new ZoomLabel(this);
	m_plusButton->setImages(":/resources/images/icons/zoomSliderPlus.png", ":/resources/images/icons/zoomSliderPlusPressed.png");
    m_plusButton->setAutoRepeat(true);
	m_plusButton->setObjectName("ZoomSliderButton");
	connect(m_plusButton, SIGNAL(clicked()), this, SLOT(plusClicked()));

	connect(m_slider, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));
	connect(m_lineEdit, SIGNAL(textEdited(const QString &)), this, SLOT(sliderTextEdited(const QString &)));

	//m_userStillWriting = false;

	//m_valueBackup = editText();
	//m_indexBackup = itemIndex(m_valueBackup);
}

void ZoomSlider::loadFactors() {
	QFile file(":/resources/zoomfactors.txt");
	file.open(QFile::ReadOnly);
	QTextStream stream( &file );
	int lineNumber = 0;
	while(!stream.atEnd()) {
		QString line = stream.readLine();
		if(lineNumber != 0) {
			ZoomFactors << line.toDouble();
		} 
		else 
		{
			ZoomStep = line.toDouble();
		}
		lineNumber++;
	}
	file.close();
}


void ZoomSlider::setValue(double value) {
	QString newText = QString("%1").arg(qRound(value));
	m_lineEdit->setText(newText);
	sliderTextEdited(newText, false);
}

double ZoomSlider::value() {
	return m_lineEdit->text().toDouble();
}

void ZoomSlider::minusClicked() {
	step(-1);
}

void ZoomSlider::plusClicked() {
	step(1);
}

void ZoomSlider::step(int direction) {
	int minIndex = 0;
	double minDiff = std::numeric_limits<double>::max();
	double v = value();
	for (int i = 0; i < ZoomFactors.count(); i++) {
		double f = ZoomFactors[i];
		if (qAbs(f - v) < minDiff) {
			minDiff = qAbs(f - v);
			minIndex = i;
		}
	}

	minIndex += direction;
	if (minIndex < 0) {
		minIndex = 0;
	}
	else if (minIndex >= ZoomFactors.count()) {
		minIndex = ZoomFactors.count() - 1;
	}

	setValue(ZoomFactors[minIndex]);
	emit zoomChanged(ZoomFactors[minIndex]);
}

void ZoomSlider::sliderValueChanged(int newValue) {
	newValue = (newValue / 10) * 10;					// make it so moving the slider outputs nice rounded values
	QString newText = QString("%1").arg(newValue);
	if (newText.compare(m_lineEdit->text()) != 0) {
		m_lineEdit->setText(newText);
		emit zoomChanged(newValue);
	}
}

void ZoomSlider::sliderTextEdited(const QString & newText) {
	sliderTextEdited(newText, true);
}

void ZoomSlider::sliderTextEdited(const QString & newText, bool doEmit) 
{
	int value = newText.toInt();
	if (m_slider->value() != value) {
		disconnect(m_slider, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));
		m_slider->setValue(value);
		connect(m_slider, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));
		if (doEmit) emit zoomChanged(value);
	}
}

void ZoomSlider::zoomIn () {
	plusClicked();
}

void ZoomSlider::zoomOut () {
	minusClicked();
}

void ZoomSlider::showEvent(QShowEvent * event) 
{
	// can't get QHLayout to work, so shoving widgets into place here
	// because widths aren't set at constructor time
	QFrame::showEvent(event);
	if (m_firstTime) {
		m_firstTime = false;
		int soFar = 0;
		m_lineEdit->move(soFar, -2);
		soFar += m_lineEdit->width();
		m_suffix->move(soFar, 0);
		soFar += m_suffix->width();
		m_minusButton->move(soFar, 0);
		soFar += m_minusButton->width();
		m_slider->move(soFar, 0);
		soFar += m_slider->width();
		m_plusButton->move(soFar, 0);
	}
}
