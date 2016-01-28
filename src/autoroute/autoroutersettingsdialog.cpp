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


///////////////////////////////////////

// todo: 
//	save and reload as settings
//	enable/disable custom on radio presses
//	change wording on custom via
//	actually modify the autorouter
//	enable single vs. double-sided settings


///////////////////////////////////////

#include "autoroutersettingsdialog.h"

#include <QLabel>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSettings>
#include <QComboBox>

#include "../items/tracewire.h"
#include "../items/via.h"
#include "../fsvgrenderer.h"
#include "../sketch/pcbsketchwidget.h"
#include "../utils/textutils.h"
#include "../utils/graphicsutils.h"
#include "drc.h"


const QString AutorouterSettingsDialog::AutorouteTraceWidth = "autorouteTraceWidth";

AutorouterSettingsDialog::AutorouterSettingsDialog(QHash<QString, QString> & settings, QWidget *parent) : QDialog(parent) 
{
    m_traceWidth = settings.value(AutorouteTraceWidth).toInt();

	Via::initHoleSettings(m_holeSettings);
    m_holeSettings.ringThickness = settings.value(Via::AutorouteViaRingThickness);
    m_holeSettings.holeDiameter = settings.value(Via::AutorouteViaHoleSize);

	this->setWindowTitle(QObject::tr("Autorouter Settings"));

	QVBoxLayout * windowLayout = new QVBoxLayout();
	this->setLayout(windowLayout);

	QGroupBox * prodGroupBox = new QGroupBox(tr("Production type"), this);
	QVBoxLayout * prodLayout = new QVBoxLayout();
	prodGroupBox->setLayout(prodLayout);

	m_homebrewButton = new QRadioButton(tr("homebrew"), this); 
	connect(m_homebrewButton, SIGNAL(clicked(bool)), this, SLOT(production(bool)));

	m_professionalButton = new QRadioButton(tr("professional"), this); 
	connect(m_professionalButton, SIGNAL(clicked(bool)), this, SLOT(production(bool)));

	m_customButton = new QRadioButton(tr("custom"), this); 
	connect(m_customButton, SIGNAL(clicked(bool)), this, SLOT(production(bool)));

	m_customFrame = new QFrame(this);
	QHBoxLayout * customFrameLayout = new QHBoxLayout(this);
	m_customFrame->setLayout(customFrameLayout);

	customFrameLayout->addSpacing(5);

	QFrame * innerFrame = new QFrame(this);
	QVBoxLayout * innerFrameLayout = new QVBoxLayout(this);
	innerFrame->setLayout(innerFrameLayout);

    QWidget * traceWidget = createTraceWidget();
    QWidget * keepoutWidget = createKeepoutWidget(settings.value(DRC::KeepoutSettingName));
    QWidget * viaWidget = createViaWidget();

    QDialogButtonBox * buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
	buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	innerFrameLayout->addWidget(traceWidget);
	innerFrameLayout->addWidget(keepoutWidget);
	innerFrameLayout->addWidget(viaWidget);

	customFrameLayout->addWidget(innerFrame);

	prodLayout->addWidget(m_homebrewButton);
	prodLayout->addWidget(m_professionalButton);
	prodLayout->addWidget(m_customButton);
	prodLayout->addWidget(m_customFrame);

	windowLayout->addWidget(prodGroupBox);

	windowLayout->addSpacerItem(new QSpacerItem(1, 10, QSizePolicy::Preferred, QSizePolicy::Expanding));

	windowLayout->addWidget(buttonBox);

	enableCustom(initProductionType());	
}

AutorouterSettingsDialog::~AutorouterSettingsDialog() {
}

void AutorouterSettingsDialog::production(bool checked) {
	Q_UNUSED(checked);

	QString units;
	if (sender() == m_homebrewButton) {
		enableCustom(false);
		changeHoleSize(sender()->property("holesize").toString() + "," + sender()->property("ringthickness").toString());
		setTraceWidth(16);
        setDefaultKeepout();
	}
	else if (sender() == m_professionalButton) {
		enableCustom(false);
		changeHoleSize(sender()->property("holesize").toString() + "," + sender()->property("ringthickness").toString());
		setTraceWidth(24);
        setDefaultKeepout();
	}
	else if (sender() == m_customButton) {
		enableCustom(true);
	}	
}

void AutorouterSettingsDialog::enableCustom(bool enable) 
{
	m_customFrame->setVisible(enable);
}

bool AutorouterSettingsDialog::initProductionType() 
{
    m_homebrewButton->setChecked(false);
    m_professionalButton->setChecked(false);

    int custom = 0;

    QString keepoutString = getKeepoutString();
    double mils = TextUtils::convertToInches(keepoutString) * 1000;
    if (qAbs(mils - DRC::KeepoutDefaultMils) >= 1) {
        custom++;
    }

    double standard = GraphicsUtils::pixels2mils(Wire::STANDARD_TRACE_WIDTH, GraphicsUtils::SVGDPI);
    if (qAbs(m_traceWidth - standard) >= 1) {
        custom++;
    }

    custom++;  // assume the holesize/ringthickness won't match
    double rt = TextUtils::convertToInches(m_holeSettings.ringThickness);
    double hs = TextUtils::convertToInches(m_holeSettings.holeDiameter);
	foreach (QString name, m_holeSettings.holeThing->holeSizeKeys) {
        // have to loop through all values to set up the two buttons
		QStringList values = m_holeSettings.holeThing->holeSizes.value(name).split(",");
		QString ringThickness = values[1];
		QString holeSize = values[0];
		if (!name.isEmpty() && !ringThickness.isEmpty() && !holeSize.isEmpty()) {
			QRadioButton * button = NULL;
			if (name.contains("home", Qt::CaseInsensitive)) button = m_homebrewButton;
			else if (name.contains("standard", Qt::CaseInsensitive)) button = m_professionalButton;
			if (button) {
				button->setProperty("ringthickness", ringThickness);
				button->setProperty("holesize", holeSize);
                double krt = TextUtils::convertToInches(ringThickness);
                double khs = TextUtils::convertToInches(holeSize);
				if (qAbs(rt - krt) < 0.001 && qAbs(hs - khs) < 0.001) {
                    // holesize/ringthickness match after all
                    if (--custom == 0) {  
					    button->setChecked(true);
                    }
				}
			}
		}
	}	

    m_customButton->setChecked(custom > 0);
	return custom > 0;
}

void AutorouterSettingsDialog::widthEntry(const QString & text) {
	int w = TraceWire::widthEntry(text, sender());
	if (w == 0) return;

	m_traceWidth = w;
}

void AutorouterSettingsDialog::changeHoleSize(const QString & newSize) {
	QString s = newSize;
	PaletteItem::setHoleSize(s, false, m_holeSettings);
}

void AutorouterSettingsDialog::changeUnits(bool) 
{
	QString newVal = PaletteItem::changeUnits(m_holeSettings);
}

void AutorouterSettingsDialog::changeDiameter() 
{
	if (PaletteItem::changeDiameter(m_holeSettings, sender())) {
		QLineEdit * edit = qobject_cast<QLineEdit *>(sender());
		changeHoleSize(edit->text() + m_holeSettings.currentUnits() + "," + m_holeSettings.ringThickness);
	}
}

void AutorouterSettingsDialog::changeThickness() 
{
	if (PaletteItem::changeThickness(m_holeSettings, sender())) {
		QLineEdit * edit = qobject_cast<QLineEdit *>(sender());
		changeHoleSize(m_holeSettings.holeDiameter + "," + edit->text() + m_holeSettings.currentUnits());
	}	
}


void AutorouterSettingsDialog::setTraceWidth(int width)
{
	for (int i = 0; i > m_traceWidthComboBox->count(); i++) {
		if (m_traceWidthComboBox->itemData(i).toInt() == width) {
			m_traceWidthComboBox->setCurrentIndex(i);
			return;
		}
	}
}

QWidget * AutorouterSettingsDialog::createKeepoutWidget(const QString & keepoutString) {
	QGroupBox * keepoutGroupBox = new QGroupBox(tr("Keepout"), this);
	QVBoxLayout * vLayout = new QVBoxLayout();

	QLabel * label = new QLabel(tr("<b>Keepout</b> is the minimum distance between copper elements on different nets."));
    //label->setWordWrap(true);  // setting wordwrap here seems to break the layout
	vLayout->addWidget(label);

    label = new QLabel(tr("A keepout of 0.01 inch (0.254 mm) is a good default."));
	vLayout->addWidget(label);

	label = new QLabel(tr("Note: the smaller the keepout, the slower the DRC and Autorouter will run."));
	vLayout->addWidget(label);

    QFrame * frame = new QFrame;
    QHBoxLayout * frameLayout = new QHBoxLayout;

    m_keepoutSpinBox = new QDoubleSpinBox;
    m_keepoutSpinBox->setDecimals(4);
    m_keepoutSpinBox->setLocale(QLocale::C);
    connect(m_keepoutSpinBox, SIGNAL(valueChanged(double)), this, SLOT(keepoutEntry()));
    frameLayout->addWidget(m_keepoutSpinBox);

    m_inRadio = new QRadioButton("in");
    frameLayout->addWidget(m_inRadio);
    connect(m_inRadio, SIGNAL(clicked()), this, SLOT(toInches()));

    m_mmRadio = new QRadioButton("mm");
    frameLayout->addWidget(m_mmRadio);
    connect(m_mmRadio, SIGNAL(clicked()), this, SLOT(toMM()));

    m_keepoutMils = TextUtils::convertToInches(keepoutString) * 1000;
    if (keepoutString.endsWith("mm")) {
        toMM();
        m_mmRadio->setChecked(true);
    }
    else {
        toInches();
        m_inRadio->setChecked(true);
    }

    frame->setLayout(frameLayout);
    vLayout->addWidget(frame);

	keepoutGroupBox->setLayout(vLayout);

    return keepoutGroupBox;
}

QWidget * AutorouterSettingsDialog::createTraceWidget() {
	QGroupBox * traceGroupBox = new QGroupBox(tr("Trace width"), this);
	QBoxLayout * traceLayout = new QVBoxLayout();

	m_traceWidthComboBox = TraceWire::createWidthComboBox(m_traceWidth, traceGroupBox);
    connect(m_traceWidthComboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(widthEntry(const QString &)));

    traceLayout->addWidget(m_traceWidthComboBox);
	traceGroupBox->setLayout(traceLayout);

    return traceGroupBox;
}

QWidget * AutorouterSettingsDialog::createViaWidget() {
	QGroupBox * viaGroupBox = new QGroupBox("Via size", this);
	QVBoxLayout * viaLayout = new QVBoxLayout();

	QWidget * viaWidget = Hole::createHoleSettings(viaGroupBox, m_holeSettings, true, "", true);

	connect(m_holeSettings.sizesComboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(changeHoleSize(const QString &)));
	connect(m_holeSettings.mmRadioButton, SIGNAL(toggled(bool)), this, SLOT(changeUnits(bool)));
	connect(m_holeSettings.inRadioButton, SIGNAL(toggled(bool)), this, SLOT(changeUnits(bool)));
	connect(m_holeSettings.diameterEdit, SIGNAL(editingFinished()), this, SLOT(changeDiameter()));
	connect(m_holeSettings.thicknessEdit, SIGNAL(editingFinished()), this, SLOT(changeThickness()));

    viaLayout->addWidget(viaWidget);
	viaGroupBox->setLayout(viaLayout);

    return viaGroupBox;
}

void AutorouterSettingsDialog::toInches() {
    m_keepoutSpinBox->blockSignals(true);
    m_keepoutSpinBox->setRange(.001, 1);
    m_keepoutSpinBox->setSingleStep(.001);
    m_keepoutSpinBox->setValue(m_keepoutMils / 1000);
    m_keepoutSpinBox->blockSignals(false);
}

void AutorouterSettingsDialog::toMM() {
    m_keepoutSpinBox->blockSignals(true);
    m_keepoutSpinBox->setRange(.001 * 25.4, 1);
    m_keepoutSpinBox->setSingleStep(.01);
    m_keepoutSpinBox->setValue(m_keepoutMils * 25.4 / 1000);
    m_keepoutSpinBox->blockSignals(false);
}

void AutorouterSettingsDialog::keepoutEntry() {
    double k = m_keepoutSpinBox->value();
    if (m_inRadio->isChecked()) {
        m_keepoutMils = k * 1000;
    }
    else {
        m_keepoutMils = k * 1000 / 25.4;
    }
}

void AutorouterSettingsDialog::setDefaultKeepout() {
    m_keepoutMils = DRC::KeepoutDefaultMils;
    double inches = DRC::KeepoutDefaultMils / 1000;
    if (m_inRadio->isChecked()) {
        m_keepoutSpinBox->setValue(inches);
    }
    else {
        m_keepoutSpinBox->setValue(inches * 25.4);
    }
}

QHash<QString, QString> AutorouterSettingsDialog::getSettings() {
    QHash<QString, QString> settings;
    settings.insert(DRC::KeepoutSettingName, getKeepoutString());
	settings.insert(Via::AutorouteViaHoleSize, m_holeSettings.holeDiameter);
	settings.insert(Via::AutorouteViaRingThickness, m_holeSettings.ringThickness);
	settings.insert(AutorouteTraceWidth, QString::number(m_traceWidth));

    return settings;
}

QString AutorouterSettingsDialog::getKeepoutString() 
{
    double k = m_keepoutSpinBox->value();
    if (m_inRadio->isChecked()) {
        return QString("%1in").arg(k);
    }
    else {
        return QString("%1mm").arg(k);
    }
}
