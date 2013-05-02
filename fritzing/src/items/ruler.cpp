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

$Revision: 6984 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-22 23:44:56 +0200 (Mo, 22. Apr 2013) $

********************************************************************/

#include "ruler.h"
#include "../utils/graphicsutils.h"
#include "../fsvgrenderer.h"
#include "../sketch/infographicsview.h"
#include "../svg/svgfilesplitter.h"
#include "moduleidnames.h"
#include "../utils/textutils.h"
#include "../utils/boundedregexpvalidator.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QRegExp>
#include <qmath.h>

static const int IndexCm = 0;
static const int IndexIn = 1;

static QString DefaultWidth = "";

Ruler::Ruler( ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: PaletteItem(modelPart, viewID, viewGeometry, id, itemMenu, doLabel)
{
	m_widthEditor = NULL;
	m_unitsEditor = NULL;
	m_widthValidator = NULL;
	QString w = modelPart->localProp("width").toString();
	if (w.isEmpty()) {
		if (DefaultWidth.isEmpty()) {
			DefaultWidth = modelPart->properties().value("width", "10cm");
		}
		m_modelPart->setLocalProp("width", DefaultWidth);
	}
}

Ruler::~Ruler() {
}

void Ruler::resizeMM(double magnitude, double unitsFlag, const LayerHash & viewLayers) {

	// note this really isn't resizeMM but resizeUnits

	Q_UNUSED(viewLayers);

	double w = TextUtils::convertToInches(prop("width"));
	QString units((unitsFlag == IndexCm) ? "cm" : "in");
	double newW = TextUtils::convertToInches(QString::number(magnitude) + units);
	if (w == newW) return;

	QString s = makeSvg(newW);

	bool result = resetRenderer(s);
	if (result) {
        modelPart()->setLocalProp("width", QVariant(QString::number(magnitude) + units));
	}
	//	DebugDialog::debug(QString("fast load result %1 %2").arg(result).arg(s));

}

QString Ruler::retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor) 
{
	double w = TextUtils::convertToInches(m_modelPart->localProp("width").toString());
	if (w != 0) {
		QString xml;
		switch (viewLayerID) {
			case ViewLayer::BreadboardRuler:
			case ViewLayer::SchematicRuler:
			case ViewLayer::PcbRuler:
				xml = makeSvg(w);
				break;
			default:
				break;
		}

		if (!xml.isEmpty()) {
			 return PaletteItemBase::normalizeSvg(xml, viewLayerID, blackOnly, dpi, factor);
		}
	}

	return PaletteItemBase::retrieveSvg(viewLayerID, svgHash, blackOnly, dpi, factor);
}

QString Ruler::makeSvg(double inches) {
	double cm = 1 / 2.54;
	double offset = 0.125;
	double mmW = inches * 25.4;
	QString svg = TextUtils::makeSVGHeader(GraphicsUtils::SVGDPI, GraphicsUtils::StandardFritzingDPI, (inches + offset + offset) * GraphicsUtils::SVGDPI, GraphicsUtils::SVGDPI);
	svg += "<g font-family='DroidSans' text-anchor='middle' font-size='100' stroke-width='1' stroke='black'>";
	int counter = 0;
	for (int i = 0; i <= qCeil(mmW); i++) {
		double h = cm / 4;
		double x = (offset + (i / 25.4)) * GraphicsUtils::StandardFritzingDPI;
		if (i % 10 == 0) {
			h = cm / 2;
			double y = (h + .1) * GraphicsUtils::StandardFritzingDPI;
			svg += QString("<text x='%1' y='%2'>%3</text>")
					.arg(x)
					.arg(y)
					.arg(QString::number(counter++));
			if (counter == 1) {
				svg += QString("<text x='%1' y='%2'>cm</text>").arg(x + 103).arg(y);
			}
		}
		else if (i % 5 == 0) {
			h = 3 * cm / 8;
		}
		svg += QString("<line x1='%1' y1='0' x2='%1' y2='%2' />\n")
			.arg(x)
			.arg(h * GraphicsUtils::StandardFritzingDPI);
	}
	counter = 0;
	for (int i = 0; i <= inches * 16; i++) {
		double h = 0.125;
		double x = (offset + (i / 16.0)) * GraphicsUtils::StandardFritzingDPI;
		if (i % 16 == 0) {
			h = .125 +  (3.0 / 16);
			double y = 1000 - ((h + .015) * GraphicsUtils::StandardFritzingDPI);
			svg += QString("<text x='%1' y='%2'>%3</text>")
					.arg(x)
					.arg(y)
					.arg(QString::number(counter++));
			if (counter == 1) {
				svg += QString("<text x='%1' y='%2'>in</text>").arg(x + 81).arg(y);
			}
		}
		else if (i % 8 == 0) {
			h = .125 +  (2.0 / 16);
		}
		else if (i % 4 == 0) {
			h = .125 +  (1.0 / 16);
		}
		svg += QString("<line x1='%1' y1='%2' x2='%1' y2='1000' />\n")
			.arg(x)
			.arg(1000 - (h * GraphicsUtils::StandardFritzingDPI));
	}

	for (int i = 0; i <= inches * 10; i++) {
		double x = (offset + (i / 10.0)) * GraphicsUtils::StandardFritzingDPI;
		double h = .125 + (3.0 / 16);
		double h2 = h - (cm / 4);
		if (i % 10 != 0) {
			if (i % 5 == 0) {
				h2 = .125 +  (2.0 / 16);
			}
			svg += QString("<line x1='%1' y1='%2' x2='%1' y2='%3' />\n")
				.arg(x)
				.arg(1000 - (h * GraphicsUtils::StandardFritzingDPI))
				.arg(1000 - (h2 * GraphicsUtils::StandardFritzingDPI));
		}
	}

    svg += QString("<line x1='%1' y1='%2' x2='%3' y2='%2' stroke-width='1' stroke='black' />\n")
        .arg(offset * GraphicsUtils::StandardFritzingDPI)
        .arg((GraphicsUtils::StandardFritzingDPI / 2) - 40)
        .arg((inches + offset) * GraphicsUtils::StandardFritzingDPI);
	svg += "<g font-size='40'>\n";
	svg += QString("<text x='%1' y='%2'>1/10</text>").arg((GraphicsUtils::StandardFritzingDPI * offset / 2.0) + 7).arg(780);
	svg += QString("<text x='%1' y='%2'>1/16</text>").arg((GraphicsUtils::StandardFritzingDPI * offset / 2.0) + 7).arg(990);
	svg += "</g>";


	svg += "</g></svg>";
	return svg;
}

bool Ruler::hasCustomSVG() {
	switch (m_viewID) {
		case ViewLayer::PCBView:
		case ViewLayer::SchematicView:
		case ViewLayer::BreadboardView:
			return true;
		default:
			return ItemBase::hasCustomSVG();
	}
}

bool Ruler::collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide)
{
	bool result = PaletteItem::collectExtraInfo(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget, hide);

	if (prop.compare("width", Qt::CaseInsensitive) == 0) {
		returnProp = tr("width");

		int units = m_modelPart->localProp("width").toString().contains("cm") ? IndexCm : IndexIn;
		QLineEdit * e1 = new QLineEdit();
		QDoubleValidator * validator = new QDoubleValidator(e1);
		validator->setRange(1.0, 20 * ((units == IndexCm) ? 2.54 : 1), 2);
		validator->setNotation(QDoubleValidator::StandardNotation);
		e1->setValidator(validator);
		e1->setEnabled(swappingEnabled);
		QString temp = m_modelPart->localProp("width").toString();
		temp.chop(2);
		e1->setText(temp);
		e1->setObjectName("infoViewLineEdit");	
        e1->setMaximumWidth(80);

		m_widthEditor = e1;
		m_widthValidator = validator;

		QComboBox * comboBox = new QComboBox(parent);
		comboBox->setEditable(false);
		comboBox->setEnabled(swappingEnabled);
		comboBox->addItem("cm");
		comboBox->addItem("in");
		comboBox->setCurrentIndex(units);
		m_unitsEditor = comboBox;
		comboBox->setObjectName("infoViewComboBox");	
        comboBox->setMinimumWidth(60);


		QHBoxLayout * hboxLayout = new QHBoxLayout();
		hboxLayout->setAlignment(Qt::AlignLeft);
		hboxLayout->setContentsMargins(0, 0, 0, 0);
		hboxLayout->setSpacing(0);
		hboxLayout->setMargin(0);


		hboxLayout->addWidget(e1);
		hboxLayout->addWidget(comboBox);

		QFrame * frame = new QFrame();
		frame->setLayout(hboxLayout);
		frame->setObjectName("infoViewPartFrame");

		connect(e1, SIGNAL(editingFinished()), this, SLOT(widthEntry()));
		connect(comboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(unitsEntry(const QString &)));

        returnValue = temp + QString::number(units);
		returnWidget = frame;

		return true;
	}

	return result;
}

void Ruler::widthEntry() {
	QLineEdit * edit = qobject_cast<QLineEdit *>(sender());
	if (edit == NULL) return;

	QString t = edit->text();
	QString w = prop("width");
	w.chop(2);
	if (t.compare(w) == 0) {
		return;
	}

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		int units = (m_unitsEditor->currentText() == "cm") ? IndexCm : IndexIn;
		DefaultWidth = edit->text() + m_unitsEditor->currentText();
		infoGraphicsView->resizeBoard(edit->text().toDouble(), units, false);
	}
}

void Ruler::unitsEntry(const QString & units) {
	double inches = TextUtils::convertToInches(prop("width"));
	if (units == "in") {
        modelPart()->setLocalProp("width", QVariant(QString::number(inches) + "in"));
		m_widthEditor->setText(QString::number(inches));
		m_widthValidator->setTop(20);
	}
	else {
        modelPart()->setLocalProp("width", QVariant(QString::number(inches * 2.54) + "cm"));
		m_widthEditor->setText(QString::number(inches * 2.54));
		m_widthValidator->setTop(20 * 2.54);
	}
	DefaultWidth = prop("width");
}

bool Ruler::stickyEnabled() {
	return false;
}

bool Ruler::hasPartLabel() {
    return false;
}

ItemBase::PluralType Ruler::isPlural() {
	return Singular;
}


void Ruler::addedToScene(bool temporary)
{
	if (this->scene()) {
		LayerHash viewLayers;
		QString w = prop("width");
		modelPart()->setLocalProp("width", "");							// makes sure resizeMM will do the work
		double inches = TextUtils::convertToInches(w);
		if (w.endsWith("cm")) {
			resizeMM(inches * 2.54, IndexCm, viewLayers);
		}
		else {
			resizeMM(inches, IndexIn, viewLayers);
		}
	}

    return PaletteItem::addedToScene(temporary);
}

bool Ruler::hasPartNumberProperty()
{
	return false;
}

bool Ruler::canFindConnectorsUnder() {
	return false;
}

ViewLayer::ViewID Ruler::useViewIDForPixmap(ViewLayer::ViewID, bool) 
{
    return ViewLayer::IconView;
}
