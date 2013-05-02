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

// TODO:
//
//	direct editing (eventually)

#include "schematicframe.h"
#include "../utils/graphicsutils.h"
#include "../utils/folderutils.h"
#include "../utils/textutils.h"
#include "../fsvgrenderer.h"
#include "../sketch/infographicsview.h"
#include "../svg/svgfilesplitter.h"
#include "moduleidnames.h"


#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QRegExp>
#include <QPushButton>
#include <QImageReader>
#include <QMessageBox>
#include <QImage>
#include <QLineEdit>
#include <QDateTimeEdit>
#include <QSpinBox>
#include <QHash>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextLayout>
#include <QTextLine>

static QString SchematicTemplate = "";
static int OriginalWidth = 0;
static int OriginalHeight = 0;
static int Margin = 200;
static double FontSize = 11;

QHash<QString, QString> FrameProps;
static const QString DisplayFormat("dd MMM yyyy hh:mm:ss");

SchematicFrame::SchematicFrame( ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: ResizableBoard(modelPart, viewID, viewGeometry, id, itemMenu, doLabel)
{
	if (FrameProps.count() == 0) {
		FrameProps.insert("descr", "");
		FrameProps.insert("project", tr("Project"));
		FrameProps.insert("filename", tr("Filename"));
		FrameProps.insert("date", tr("Date"));
		FrameProps.insert("sheet", tr("Sheet"));
		FrameProps.insert("rev", tr("Rev"));
	}

	foreach (QString prop, FrameProps.keys()) {
		if (modelPart->localProp(prop).toString().isEmpty()) {
			modelPart->setLocalProp(prop, modelPart->properties().value(prop));
		}
	}

	if (modelPart->localProp("date").toString().isEmpty()) {
		QDateTime dt = QDateTime::currentDateTime();
		modelPart->setLocalProp("date", QString::number(dt.toTime_t()));
	}

	if (modelPart->localProp("sheet").toString().isEmpty()) {
		modelPart->setLocalProp("sheet","1/1");
	}

	m_wrapInitialized = false;
	m_textEdit = new QTextEdit();
	m_textEdit->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
	m_textEdit->setAcceptRichText(false);
	m_textEdit->setLineWrapMode(QTextEdit::FixedPixelWidth);
}

SchematicFrame::~SchematicFrame() {
	if (m_textEdit) {
		delete m_textEdit;
	}
}

void SchematicFrame::loadTemplates() {
	if (SchematicTemplate.isEmpty()) {
		QFile file(":/resources/templates/schematic_frame_template.txt");
		file.open(QFile::ReadOnly);
		SchematicTemplate = file.readAll();
		file.close();

		QFile file2(":/resources/parts/svg/core/schematic/frame.svg");
		file2.open(QFile::ReadOnly);
		QString original = file2.readAll();
		file2.close();

		OriginalWidth = TextUtils::getViewBoxCoord(original, 2);
		OriginalHeight = TextUtils::getViewBoxCoord(original, 3);

		QString errorStr;
		int errorLine;
		int errorColumn;

		QDomDocument domDocument;
		if (!domDocument.setContent(SchematicTemplate, true, &errorStr, &errorLine, &errorColumn)) {
			return;
		}

		QDomElement root = domDocument.documentElement(); 
		QDomElement descr =TextUtils::findElementWithAttribute(root, "id", "descr");
		QString xString = descr.attribute("x");
		xString = xString.mid(1, xString.length() - 2); // remove the brackets;
		Margin = xString.toInt() * 2;
		FontSize = descr.attribute("font-size").toDouble();

	}
}

QString SchematicFrame::makeLayerSvg(ViewLayer::ViewLayerID viewLayerID, double mmW, double mmH, double milsW, double milsH) 
{
	Q_UNUSED(mmW);
	Q_UNUSED(mmH);

	if (SchematicTemplate.isEmpty()) return "";

	switch (viewLayerID) {
		case ViewLayer::SchematicFrame:
			break;
		default:
			return "";
	}

	if (!m_wrapInitialized) {
		m_wrapInitialized = true;
		QFont font = m_textEdit->font();
		font.setFamily("Droid Sans");
		font.setWeight(QFont::Normal);
		font.setPointSizeF(72.0 * FontSize / GraphicsUtils::StandardFritzingDPI);
		m_textEdit->setFont(font);
		m_textEdit->setLineWrapColumnOrWidth(GraphicsUtils::SVGDPI * (OriginalWidth - Margin) / GraphicsUtils::StandardFritzingDPI);  
	}


	if (milsW < OriginalWidth) milsW = OriginalWidth;
	if (milsH < OriginalHeight) milsH = OriginalHeight;
	QString svg = SchematicTemplate.arg(milsW / 1000).arg(milsH / 1000).arg(milsW).arg(milsH).arg(milsW - 8).arg(milsH - 8);
	svg = TextUtils::incrementTemplateString(svg, 1, milsW - OriginalWidth, TextUtils::incMultiplyPinFunction, TextUtils::noCopyPinFunction, NULL);
	svg.replace("{", "[");
	svg.replace("}", "]");
	svg = TextUtils::incrementTemplateString(svg, 1, milsH - OriginalHeight, TextUtils::incMultiplyPinFunction, TextUtils::noCopyPinFunction, NULL);
	QHash<QString, QString> hash;
	foreach (QString propp, FrameProps.keys()) {
		hash.insert(propp, prop(propp));
		QString label = FrameProps.value(propp);
		if (!label.isEmpty()) {
			hash.insert(propp + " label", label);
		}
	}

	// figure out the width and the font
	QString string = prop("descr");
	m_textEdit->setPlainText(string);
	QTextCursor textCursor = m_textEdit->cursorForPosition(QPoint(0,0));
	QTextLayout * textLayout = textCursor.block().layout();
	int lc2 = textLayout->lineCount();
	if (lc2 > 1) {
		QTextLine textLine = textLayout->lineAt(0);
		hash.insert("descr", string.left(textLine.textLength()));
		textLine = textLayout->lineAt(1);
		string = string.mid(textLine.textStart(), textLine.textLength());
		hash.insert("descr+", string);
	}


	QDateTime dt;
	dt.setTime_t(modelPart()->localProp("date").toUInt());
	hash.insert("date", dt.toString(DisplayFormat));

	return TextUtils::convertExtendedChars(TextUtils::replaceTextElements(svg, hash));
}

QString SchematicFrame::makeNextLayerSvg(ViewLayer::ViewLayerID viewLayerID, double mmW, double mmH, double milsW, double milsH) 
{
	Q_UNUSED(mmW);
	Q_UNUSED(mmH);
	Q_UNUSED(milsW);
	Q_UNUSED(milsH);
	Q_UNUSED(viewLayerID);

	return "";
}

QString SchematicFrame::makeFirstLayerSvg(double mmW, double mmH, double milsW, double milsH) {
	return makeLayerSvg(ViewLayer::SchematicFrame, mmW, mmH, milsW, milsH);
}

ViewLayer::ViewID SchematicFrame::theViewID() {
	return ViewLayer::SchematicView;
}

double SchematicFrame::minWidth() {
	return OriginalWidth * GraphicsUtils::SVGDPI / GraphicsUtils::StandardFritzingDPI;
}

double SchematicFrame::minHeight() {
	return OriginalHeight * GraphicsUtils::SVGDPI / GraphicsUtils::StandardFritzingDPI;
}

void SchematicFrame::addedToScene(bool temporary)
{
	if (prop("filename").isEmpty()) {
		InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
		if (infoGraphicsView != NULL) {
			modelPart()->setLocalProp("filename", infoGraphicsView->filenameIf());
		}
	}
    ResizableBoard::addedToScene(temporary);
	resizeMMAux(m_modelPart->localProp("width").toDouble(), m_modelPart->localProp("height").toDouble());
}

QString SchematicFrame::retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor)
{
	return ResizableBoard::retrieveSvg(viewLayerID, svgHash, blackOnly, dpi, factor);
}

bool SchematicFrame::makeLineEdit(QWidget * parent, const QString & family, const QString & propp, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget) 
{
	Q_UNUSED(value);
	Q_UNUSED(family);


	returnProp = ItemBase::TranslatedPropertyNames.value(propp.toLower());
	returnValue = prop(propp);

	QLineEdit * e1 = new QLineEdit(parent);
	e1->setObjectName("infoViewLineEdit");

	e1->setProperty("prop", QVariant(propp));

	e1->setText(returnValue);
	e1->setEnabled(swappingEnabled);
	connect(e1, SIGNAL(editingFinished()), this, SLOT(propEntry()));

	returnWidget = e1;
	return true;
}

bool SchematicFrame::collectExtraInfo(QWidget * parent, const QString & family, const QString & propp, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide) 
{
	if (propp.compare("shape", Qt::CaseInsensitive) == 0) {
		returnWidget = setUpDimEntry(false, false, true, returnWidget);
        returnWidget->setEnabled(swappingEnabled);
		returnProp = tr("shape");
		return true;
	}

    if (propp.compare("date", Qt::CaseInsensitive) == 0) {
		QDateTimeEdit * dateTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime(), parent);
		QString d = prop("date");
		if (!d.isEmpty()) {
			QDateTime dateTime;
			dateTime.setTime_t(d.toUInt());
			dateTimeEdit->setDateTime(dateTime);
		}
		//dateTimeEdit->setCalendarPopup(true);
		dateTimeEdit->setDisplayFormat(DisplayFormat);
		connect(dateTimeEdit, SIGNAL(dateTimeChanged(QDateTime)), this, SLOT(dateTimeEntry(QDateTime)));
		dateTimeEdit->setObjectName("infoViewDateEdit");
		dateTimeEdit->setEnabled(swappingEnabled);

		returnProp = ItemBase::TranslatedPropertyNames.value(propp.toLower());
		returnValue = prop(propp);
		returnWidget = dateTimeEdit;

		return true;
	}

	if (propp.compare("sheet", Qt::CaseInsensitive) == 0) {
		QString value = prop("sheet");
		QStringList strings = value.split("/");
		if (strings.count() != 2) {
			strings.clear();
			strings << "1" << "1";
		}
		QFrame * frame = new QFrame(parent);
		QSpinBox * spin1 = new QSpinBox(frame);
		spin1->setMinimum(1);
		spin1->setMaximum(999);
		spin1->setValue(strings[0].toInt());
		connect(spin1, SIGNAL(valueChanged(int)), this, SLOT(sheetEntry(int)));
		spin1->setObjectName("infoViewSpinner");
		spin1->setProperty("role", "numerator");
		spin1->setEnabled(swappingEnabled);

		QSpinBox * spin2 = new QSpinBox(frame);
		spin2->setMinimum(1);
		spin2->setMaximum(999);
		spin2->setValue(strings[1].toInt());
		connect(spin2, SIGNAL(valueChanged(int)), this, SLOT(sheetEntry(int)));
		spin2->setObjectName("infoViewSpinner");
		spin2->setProperty("role", "denominator");
		spin2->setEnabled(swappingEnabled);

		QLabel * label = new QLabel(parent);
		label->setText(tr("of"));
		label->setObjectName("infoViewOfLabel");
		label->setAlignment(Qt::AlignCenter);

		QHBoxLayout * hBoxLayout = new QHBoxLayout(frame);
		hBoxLayout->addWidget(spin1);
		hBoxLayout->addWidget(label);
		hBoxLayout->addWidget(spin2);
		hBoxLayout->addStretch();

		frame->setLayout(hBoxLayout);

		returnProp = ItemBase::TranslatedPropertyNames.value(propp.toLower());
		returnValue = prop(propp);
		returnWidget = frame;

		return true;
	}

	if (FrameProps.keys().contains(propp)) {
		return makeLineEdit(parent, family, propp, value, swappingEnabled, returnProp, returnValue, returnWidget);
	}

	return PaletteItem::collectExtraInfo(parent, family, propp, value, swappingEnabled, returnProp, returnValue, returnWidget, hide);
}

void SchematicFrame::setProp(const QString & prop, const QString & value) 
{	
	ResizableBoard::setProp(prop, value);

	if (FrameProps.keys().contains(prop)) {
		modelPart()->setLocalProp(prop, value);
		resizeMMAux(modelPart()->localProp("width").toDouble(), modelPart()->localProp("height").toDouble());
	}

}

bool SchematicFrame::canEditPart() {
	return false;
}

bool SchematicFrame::hasPartLabel() {
	return false;
}

bool SchematicFrame::stickyEnabled() {
	return false;
}

ItemBase::PluralType SchematicFrame::isPlural() {
	return Plural;
}

bool SchematicFrame::rotationAllowed() {
	return false;
}

bool SchematicFrame::rotation45Allowed() {
	return false;
}

bool SchematicFrame::hasPartNumberProperty()
{
	return false;
}

void SchematicFrame::setInitialSize() {
	double w = m_modelPart->localProp("width").toDouble();
	if (w == 0) {
		// set the size so the infoGraphicsView will display the size as you drag
		modelPart()->setLocalProp("width", 25.4 * OriginalWidth / GraphicsUtils::StandardFritzingDPI); 
		modelPart()->setLocalProp("height", 25.4 * OriginalHeight / GraphicsUtils::StandardFritzingDPI); 
	}
}

void SchematicFrame::propEntry() {
	QLineEdit * edit = qobject_cast<QLineEdit *>(sender());
	if (edit == NULL) return;

	QString propp = edit->property("prop").toString();
	if (propp.isEmpty()) return;

	QString current = prop(propp);

	if (edit->text().compare(current) == 0) return;

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->setProp(this, propp, ItemBase::TranslatedPropertyNames.value(propp), current, edit->text(), true);
	}
}

void SchematicFrame::dateTimeEntry(QDateTime dateTime) {
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->setProp(this, "date", tr("date"), prop("date"), QString::number(dateTime.toTime_t()), true);
	}
}

void SchematicFrame::sheetEntry(int value) {
	QString role = sender()->property("role").toString();
	QString sheet = prop("sheet");
	QStringList strings = sheet.split("/");
	if (strings.count() != 2) {
		strings.clear();
		strings << "1" << "1";
	}
	if (role.compare("numerator") == 0) {
		strings[0] = QString::number(value);
	}
	else if (role.compare("denominator") == 0) {
		strings[1] = QString::number(value);
	}
	else return;

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->setProp(this, "sheet", tr("sheet"), prop("sheet"), strings.at(0) + "/" + strings[1], true);
	}
}

ViewLayer::ViewID SchematicFrame::useViewIDForPixmap(ViewLayer::ViewID vid, bool) 
{
    if (vid == ViewLayer::SchematicView) {
        return ViewLayer::IconView;
    }

    return ViewLayer::UnknownView;
}
