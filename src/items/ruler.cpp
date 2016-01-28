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

    // save local prop incase render fails so we can revert back to orignal.
    QString orignalProp = m_modelPart->localProp("width").toString();

    // set local prop so makeSvg has the current mag and units.
    modelPart()->setLocalProp("width", QVariant(QString::number(magnitude) + units));

	QString s = makeSvg(newW);

	bool result = resetRenderer(s);
    if (!result) {
        // if render error restore orginal prop
        modelPart()->setLocalProp("width", QVariant(orignalProp));
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
    const double cm = 1 / 2.54;
    const double offset = 0.125;
    const double mmW = inches * 25.4;// 1/10 centimeter constant
    const double mmmW = inches * 254;// 1/100 centimeter constant

    // get units
    int units = m_modelPart->localProp("width").toString().contains("cm") ? IndexCm : IndexIn;

    QString svg = TextUtils::makeSVGHeader(GraphicsUtils::SVGDPI, GraphicsUtils::StandardFritzingDPI, (inches + offset + offset) * GraphicsUtils::SVGDPI, GraphicsUtils::SVGDPI/2);
    int counter;

    if (units == IndexCm) {
        counter = 0;
        svg += "<g font-family='Droid Sans' text-anchor='middle' font-size='100' stroke-width='1' style='stroke: rgb(100,100,100)'>";

        // 1/100 centimeter spacing
        for (int i = 0; i <= qCeil(mmmW); i++) {
            double h = cm / 12;
            double h2 = h - (cm / 12);
            double x = (offset + ((double)i / 254)) * GraphicsUtils::StandardFritzingDPI;
            if (i % 5 == 0) {
                h = cm / 6;
            }
            if  (i % 10 != 0 ) {
                svg += QString("<line x1='%1' y1='0' x2='%1' y2='%2' />\n")
                        .arg(x)
                        .arg(h * GraphicsUtils::StandardFritzingDPI);
                // Uncomment if you want 1/100cm scale on both sides.
                /*svg += QString("<line x1='%1' y1='%2' x2='%1' y2='%3' />\n")
                        .arg(x)
                        .arg((GraphicsUtils::StandardFritzingDPI/2)-(h * GraphicsUtils::StandardFritzingDPI))
                        .arg((GraphicsUtils::StandardFritzingDPI/2)-(h2 * GraphicsUtils::StandardFritzingDPI));*/
            }
        }
        svg += "</g>";

        svg += "<g font-family='Droid Sans' text-anchor='middle' font-size='100' stroke-width='1' stroke='black'>";

        counter = 0;
        // centimeters and millimeters spacing and text
        for (int i = 0; i <= qCeil(mmW); i++) {
            double h = cm / 4;
            double h2 = h - (cm / 4);
            double x = (offset + (i / 25.4)) * GraphicsUtils::StandardFritzingDPI;
            if (i % 10 == 0) {
                h = cm / 2;
                double y = (GraphicsUtils::StandardFritzingDPI/4)+35;//(h + .101) * GraphicsUtils::StandardFritzingDPI;
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

            svg += QString("<line x1='%1' y1='%2' x2='%1' y2='%3' />\n")
                    .arg(x)
                    .arg((GraphicsUtils::StandardFritzingDPI/2)-(h * GraphicsUtils::StandardFritzingDPI))
                    .arg((GraphicsUtils::StandardFritzingDPI/2)-(h2 * GraphicsUtils::StandardFritzingDPI));
        }
        svg += "<g font-size='40'>\n";
        svg += QString("<text x='%1' y='%2'>1/100</text>").arg((GraphicsUtils::StandardFritzingDPI * offset / 2.0) + 7).arg(30);
        svg += "</g>";
    }
    else {
        svg += "<g font-family='Droid Sans' text-anchor='middle' font-size='100' stroke-width='1' stroke='black'>";
        counter = 0;

        // 1/16, 1/8, 1/4, & 1 inch spacing and text
        for (int i = 0; i <= inches * 16; i++) {
            double h = 0.085;
            double x = (offset + (i / 16.0)) * GraphicsUtils::StandardFritzingDPI;
            if (i % 16 == 0) {

                // I wonder if QT compiler would see that this is just a constant, and not perform the runtime math?
                // If not calculating out the value could save some cycles:)
                h = 0.196875;//.125 + (1.15 / 16);

                double y = (GraphicsUtils::StandardFritzingDPI/4)+35;//(h + .101) * GraphicsUtils::StandardFritzingDPI;
                svg += QString("<text x='%1' y='%2'>%3</text>")
                        .arg(x)
                        .arg(y)
                        .arg(QString::number(counter++));
                if (counter == 1) {
                    svg += QString("<text x='%1' y='%2'>in</text>").arg(x + 103).arg(y);
                }
            }
            else if (i % 8 == 0) {
                h = 0.17812;//.125 +  (.85 / 16);
            }
            else if (i % 4 == 0) {
                h = 0.14062;//.125 +  (.25 / 16);
            }

            svg += QString("<line x1='%1' y1='0' x2='%1' y2='%2' />\n")
                    .arg(x)
                    .arg(h * GraphicsUtils::StandardFritzingDPI);

            svg += QString("<line x1='%1' y1='%2' x2='%1' y2='%3' />\n")
                    .arg(x)
                    .arg((GraphicsUtils::StandardFritzingDPI/2)-(h * GraphicsUtils::StandardFritzingDPI))
                    .arg((GraphicsUtils::StandardFritzingDPI/2));
        }

        // 1/10 inch spacing and text
        for (int i = 0; i <= inches * 10; i++) {
            double x = (offset + (i / 10.0)) * GraphicsUtils::StandardFritzingDPI;
            double h = .125 + (.9 / 16);
            double h2 = h - (cm / 6);
            if (i % 10 != 0 && i % 5 != 0) {
                svg += QString("<line x1='%1' y1='%2' x2='%1' y2='%3' />\n")
                        .arg(x)
                        .arg((h * GraphicsUtils::StandardFritzingDPI))
                        .arg((h2 * GraphicsUtils::StandardFritzingDPI));
            }
        }
        svg += "<g font-size='40'>\n";
        svg += QString("<text x='%1' y='%2'>1/16</text>").arg((GraphicsUtils::StandardFritzingDPI * offset / 2.0) + 7).arg(30);
        svg += QString("<text x='%1' y='%2'>1/10</text>").arg((GraphicsUtils::StandardFritzingDPI * offset / 2.0) + 7).arg(140);
        svg += "</g>";
    }
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
        validator->setLocale(QLocale::C);
        e1->setValidator(validator);
        e1->setEnabled(swappingEnabled);
        QString temp = m_modelPart->localProp("width").toString();
        temp.chop(2);
        e1->setText(temp);
        e1->setObjectName("infoViewLineEdit");
        e1->setMaximumWidth(80);

        m_widthEditor = e1;
        m_widthValidator = validator;

        // Radio Buttons
        QRadioButton *radioCm = new QRadioButton(tr("&cm"));
        QRadioButton *radioIn = new QRadioButton(tr("&in"));

        // set radio button object names
        radioCm->setObjectName("cm");
        radioIn->setObjectName("in");

        // update pointer and set checked
        if (units == IndexIn) {
            radioIn->setChecked(true);
            m_unitsEditor = radioIn;
        }
        else {
            radioCm->setChecked(true);
            m_unitsEditor = radioCm;
        }

        // spacer to keep radio buttons together when resizing Inspector Window
        QSpacerItem *item = new QSpacerItem(1,1, QSizePolicy::Expanding, QSizePolicy::Fixed);

        QHBoxLayout * hboxLayout = new QHBoxLayout();
        hboxLayout->setAlignment(Qt::AlignRight);
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        hboxLayout->setSpacing(5);
        hboxLayout->setMargin(0);

        hboxLayout->addWidget(e1);
        hboxLayout->addWidget(radioCm);
        hboxLayout->addWidget(radioIn);
        hboxLayout->addSpacerItem(item);

        QFrame * frame = new QFrame();
        frame->setLayout(hboxLayout);
        frame->setObjectName("infoViewPartFrame");

        connect(e1, SIGNAL(editingFinished()), this, SLOT(widthEntry()));
        connect(radioCm, SIGNAL(clicked()), this, SLOT(unitsEntry()));
        connect(radioIn, SIGNAL(clicked()), this, SLOT(unitsEntry()));

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
        // get current object units
        int units = (m_unitsEditor->objectName() == "cm") ? IndexCm : IndexIn;
        DefaultWidth = edit->text() + m_unitsEditor->objectName();
        infoGraphicsView->resizeBoard(edit->text().toDouble(), units, false);
	}
}

void Ruler::unitsEntry() {
    // get clicked object
    QRadioButton * obj = qobject_cast<QRadioButton *>(sender());
    if (obj == NULL) return;

    // update pointer
    m_unitsEditor = obj;

    QString units = obj->objectName();

    double inches = TextUtils::convertToInches(prop("width"));

    // save local prop incase render fails so we can revert back to orignal.
    QString orignalProp = m_modelPart->localProp("width").toString();

	if (units == "in") {
        // set local prop so makeSvg has the current width and units.
        modelPart()->setLocalProp("width", QVariant(QString::number(inches) + "in"));

        QString s = makeSvg(inches);
        bool result = resetRenderer(s);
        if (result) {
            m_widthEditor->setText(QString::number(inches));
            m_widthValidator->setTop(20);
        }
        else {
            // if render error restore orginal prop
            modelPart()->setLocalProp("width", QVariant(orignalProp));
        }
	}
	else {
        // set local prop so makeSvg has the current width and units.
        modelPart()->setLocalProp("width", QVariant(QString::number(inches * 2.54) + "cm"));

        QString s = makeSvg(inches);
        bool result = resetRenderer(s);
        if (result) {
            m_widthEditor->setText(QString::number(inches * 2.54));
            m_widthValidator->setTop(20 * 2.54);
        }
        else {
            // if render error restore orginal prop
            modelPart()->setLocalProp("width", QVariant(orignalProp));
        }
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
