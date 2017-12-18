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

#include "logoitem.h"
#include "../utils/graphicsutils.h"
#include "../utils/folderutils.h"
#include "../utils/textutils.h"
#include "../utils/fmessagebox.h"
#include "../fsvgrenderer.h"
#include "../sketch/infographicsview.h"
#include "../svg/svgfilesplitter.h"
#include "../svg/gerbergenerator.h"
#include "moduleidnames.h"
#include "../svg/groundplanegenerator.h"
#include "../utils/cursormaster.h"
#include "../debugdialog.h"

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
#include <QApplication>
#include <QColor>
#include <QColorDialog>
#include <QBuffer>

static QStringList LogoImageNames;
static QStringList Logo0ImageNames;
static QStringList NewLogoImageNames;
static QStringList Copper0ImageNames;
static QStringList Copper1ImageNames;

LogoItem::LogoItem( ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: ResizableBoard(modelPart, viewID, viewGeometry, id, itemMenu, doLabel)
{
	if (LogoImageNames.count() == 0) {
        // TODO: load these names from somewhere

		LogoImageNames << "new Made with Fritzing" << "new Fritzing icon" << "OHANDA logo" << "OSHW logo";
		Logo0ImageNames << "new Made with Fritzing 0" << "new Fritzing icon 0" << "OHANDA logo 0" << "OSHW logo 0";
	}

    m_svgOnly = false;
    m_standardizeColors = true;
	m_inLogoEntry = QTime::currentTime().addSecs(-10);
	m_aspectRatioCheck = NULL;
	m_keepAspectRatio = true;
	m_hasLogo = (modelPart->moduleID().endsWith(ModuleIDNames::LogoTextModuleIDName));
	m_logo = modelPart->localProp("logo").toString();
	if (m_hasLogo && m_logo.isEmpty()) {
		m_logo = modelPart->properties().value("logo", "logo");
		modelPart->setLocalProp("logo", m_logo);
	}

    if (m_modelPart->moduleID() == ModuleIDNames::LogoImageModuleIDName) {
        m_modelPart->modelPartShared()->setModuleID("new" + ModuleIDNames::LogoImageModuleIDName);
    }
    else if (m_modelPart->moduleID() == ModuleIDNames::Silkscreen0LogoImageModuleIDName) {
        m_modelPart->modelPartShared()->setModuleID("new" + ModuleIDNames::Silkscreen0LogoImageModuleIDName);
    }
    else if (m_modelPart->moduleID() == ModuleIDNames::Copper1LogoImageModuleIDName) {
        m_modelPart->modelPartShared()->setModuleID("new" + ModuleIDNames::Copper1LogoImageModuleIDName);
    }
    else if (m_modelPart->moduleID() == ModuleIDNames::Copper0LogoImageModuleIDName) {
        m_modelPart->modelPartShared()->setModuleID("new" + ModuleIDNames::Copper0LogoImageModuleIDName);
    }
    else if (m_modelPart->moduleID() == ModuleIDNames::SchematicLogoImageModuleIDName) {
        m_modelPart->modelPartShared()->setModuleID("new" + ModuleIDNames::SchematicLogoImageModuleIDName);
    }
    else if (m_modelPart->moduleID() == ModuleIDNames::BreadboardLogoImageModuleIDName) {
        m_modelPart->modelPartShared()->setModuleID("new" + ModuleIDNames::BreadboardLogoImageModuleIDName);
    }
}

LogoItem::~LogoItem() {
}

bool LogoItem::hasLogo() {
    return m_hasLogo;
}

QString LogoItem::getShapeForRenderer(const QString & svg) {
    return svg;
}

void LogoItem::addedToScene(bool temporary)
{
	if (this->scene()) {
		setInitialSize();
		m_aspectRatio.setWidth(this->boundingRect().width());
		m_aspectRatio.setHeight(this->boundingRect().height());
		m_originalFilename = filename();
		QString svg = prop("shape");
		if (!svg.isEmpty()) {
            QString arString = modelPart()->localProp("aspectratio").toString();
            if (!arString.isEmpty()) {
			    m_aspectRatio =  modelPart()->localProp("aspectratio").toSizeF();
            }
            else {
                bool okw, okh;
                double w = m_modelPart->localProp("width").toDouble(&okw);
		        double h = m_modelPart->localProp("height").toDouble(&okh);
                if (okw && okh) {
                    m_aspectRatio = QSizeF(w, h);
                }
            }
			if (resetRenderer(getShapeForRenderer(svg))) {
			}
			else {
				DebugDialog::debug("bad shape in " + m_originalFilename + " " + svg);
			}
		}
		else {
			QFile f(m_originalFilename);
			if (f.open(QFile::ReadOnly)) {
				QString svg = f.readAll();
				f.close();
				modelPart()->setLocalProp("shape", svg);
				modelPart()->setLocalProp("lastfilename", m_originalFilename);
				initImage();
			}
		}
	}

    return ResizableBoard::addedToScene(temporary);
}

bool LogoItem::canRetrieveLayer(ViewLayer::ViewLayerID viewLayerID) {
    return viewLayerID == layer();
}

QStringList & LogoItem::getImageNames() {
	return isBottom() ? Logo0ImageNames : LogoImageNames;
}

QStringList & LogoItem::getNewImageNames() {
	return NewLogoImageNames;
}

QString LogoItem::retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor)
{
	if (canRetrieveLayer(viewLayerID)) {
		QString svg = prop("shape");
		if (!svg.isEmpty()) {
			QString xmlName = ViewLayer::viewLayerXmlNameFromID(viewLayerID);
			SvgFileSplitter splitter;
			bool result = splitter.splitString(svg, xmlName);
			if (!result) {
				return "";
			}

			double scaleX = 1;
			double scaleY = 1;
			if (m_hasLogo) {
				QDomDocument doc = splitter.domDocument();
				QDomElement root = doc.documentElement();
				QDomElement g = root.firstChildElement("g");
				QDomElement text = g.firstChildElement("text");

				// TODO: this is really a hack and resizing should change a scale factor rather than the <text> coordinates
				// but it's not clear how to deal with existing sketches
	
				QString viewBox = root.attribute("viewBox");
				double w = TextUtils::convertToInches(root.attribute("width"));
				double h = TextUtils::convertToInches(root.attribute("height"));
				QStringList coords = viewBox.split(" ", QString::SkipEmptyParts);
				double sx = w / coords.at(2).toDouble();
				double sy = h / coords.at(3).toDouble();
				if (qAbs(sx - sy) > .001) {
					// change vertical dpi to match horizontal dpi
					// y coordinate is really intended in relation to font size so leave it be
					scaleY = sy / sx;
					root.setAttribute("viewBox", QString("0 0 %1 %2").arg(coords.at(2)).arg(h / sx));
				}
			}

			result = splitter.normalize(dpi, xmlName, blackOnly, factor);
			if (!result) {
				return "";
			}

			QString string = splitter.elementString(xmlName);

			if (scaleY == 1) return string;

			QTransform t;
			t.scale(scaleX, scaleY);
			return TextUtils::svgTransform(string, t, false, "");
		}
	}

	return PaletteItemBase::retrieveSvg(viewLayerID, svgHash, blackOnly, dpi, factor);
}

QStringList LogoItem::collectValues(const QString & family, const QString & prop, QString & value) {
    QStringList values = ResizableBoard::collectValues(family, prop, value);

    bool copperTopOK = true;
    InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
    if (infoGraphicsView == NULL) copperTopOK = false;
    else if (infoGraphicsView->boardLayers() == 1) copperTopOK = false;

    QStringList newValues;
	if (prop.compare("layer", Qt::CaseInsensitive) == 0) {
        foreach (QString xmlName, values) {
            if (xmlName == ViewLayer::viewLayerXmlNameFromID(ViewLayer::Copper1) && copperTopOK == false) continue;

            newValues << Board::convertFromXmlName(xmlName);
        }
        value = Board::convertFromXmlName(value);
        return newValues;
    }

    return values;
}

bool LogoItem::collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide) 
{
	if (m_hasLogo) {
		if (prop.compare("logo", Qt::CaseInsensitive) == 0) {
			returnProp = tr("text");
			returnValue = m_logo;

			QLineEdit * edit = new QLineEdit(parent);
			edit->setObjectName("infoViewLineEdit");

			edit->setText(m_logo);
			edit->setEnabled(swappingEnabled);
			connect(edit, SIGNAL(editingFinished()), this, SLOT(logoEntry()));

			returnWidget = edit;
			return true;
		}
	}
	else {
		if (prop.compare("filename", Qt::CaseInsensitive) == 0) {
            setupLoadImage(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget);
			return true;
		}
	}

	if (prop.compare("shape", Qt::CaseInsensitive) == 0) {
		returnWidget = setUpDimEntry(true, !m_hasLogo, false, returnWidget);
        returnWidget->setEnabled(swappingEnabled);
		returnProp = tr("shape");
		return true;
	}

	return PaletteItem::collectExtraInfo(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget, hide);
}

bool LogoItem::checkImage(const QString & filename) {
    Q_UNUSED(filename);
    return true;
}

void LogoItem::prepLoadImageAux(const QString & fileName, bool addName)
{
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->loadLogoImage(this, prop("shape"), m_aspectRatio, prop("lastfilename"), fileName, addName);
	}
}

bool LogoItem::reloadImage(const QString & incomingSvg, const QSizeF & aspectRatio, const QString & fileName, bool addName) 
{
    QString svg = incomingSvg;
	if (isBottom()) {
		if (!svg.contains("_flipped_")) {
			svg = flipSvg(svg);
		}
	}

    QString shape = getShapeForRenderer(svg);
	bool result = resetRenderer(shape);
	if (result) {
		if (aspectRatio == QSizeF(0, 0)) {
			QRectF r = fsvgRenderer()->viewBoxF();
			m_aspectRatio.setWidth(r.width());
			m_aspectRatio.setHeight(r.height());
		}
		else {
			m_aspectRatio = aspectRatio;
		}
		modelPart()->setLocalProp("aspectratio", m_aspectRatio);
		modelPart()->setLocalProp("shape", svg);
		modelPart()->setLocalProp("logo", "");
		modelPart()->setLocalProp("lastfilename", fileName);

        QSizeF size = fsvgRenderer()->defaultSizeF();

        double mmW = GraphicsUtils::pixels2mm(size.width(), GraphicsUtils::SVGDPI);
        double mmH = GraphicsUtils::pixels2mm(size.height(), GraphicsUtils::SVGDPI);
        modelPart()->setLocalProp("width", mmW);
		modelPart()->setLocalProp("height", mmH);
        modelPart()->setLocalProp("originalWidth", mmW);
		modelPart()->setLocalProp("originalHeight", mmH);

		if (addName) {
			if (!getNewImageNames().contains(fileName, Qt::CaseInsensitive)) {
				getNewImageNames().append(fileName);
                if (m_fileNameComboBox) {
				    bool wasBlocked = m_fileNameComboBox->blockSignals(true);
				    while (m_fileNameComboBox->count() > 0) {
					    m_fileNameComboBox->removeItem(0);
				    }
				    setFileNameItems();
				    m_fileNameComboBox->blockSignals(wasBlocked);
                }
			}
		}
		m_logo = "";

        setWidthAndHeight(qRound(mmW * 10) / 10.0, qRound(mmH * 10) / 10.0);
        return true;
	}
	else {
		// restore previous (not sure whether this is necessary)
		resetRenderer(getShapeForRenderer(prop("shape")));
		unableToLoad(fileName, tr("due to a rendering error"));
        return false;
	}
}

void LogoItem::loadImage(const QString & fileName, bool addName)
{
	QString svg;
	if (fileName.endsWith(".svg")) {
		QFile f(fileName);
		if (f.open(QFile::ReadOnly)) {
			svg = f.readAll();
		}
		if (svg.isEmpty()) {
			unableToLoad(fileName, tr("because the svg is empty"));
			return;
		}

		if (svg.contains("coreldraw", Qt::CaseInsensitive) && svg.contains("cdata", Qt::CaseInsensitive)) {
			unableToLoad(fileName, tr("because this appears to be a SVG file exported from CorelDRAW without choosing the 'presentation attributes' setting"));
			return;
		}

        TextUtils::fixMuch(svg, true);
		TextUtils::fixPixelDimensionsIn(svg);

		QString errorStr;
		int errorLine;
		int errorColumn;

		QDomDocument domDocument;

		if (!domDocument.setContent(svg, true, &errorStr, &errorLine, &errorColumn)) {
			unableToLoad(fileName, tr("due to an xml problem: %1 line:%2 column:%3").arg(errorStr).arg(errorLine).arg(errorColumn));
			return;
		}

		QDomElement root = domDocument.documentElement();
		if (root.isNull()) {
			unableToLoad(fileName, tr("because the file has no root element"));
			return;
		}

		if (root.tagName() != "svg") {
			unableToLoad(fileName, tr("because the file has no <svg> element"));
			return;
		}

        if (m_standardizeColors) {
		    QStringList exceptions;
		    exceptions << "none" << "";
		    QString toColor(colorString());
		    SvgFileSplitter::changeColors(root, toColor, exceptions);
            // todo: change opacity?
        }

        QRectF vb;
        double w, h;
		TextUtils::ensureViewBox(domDocument, GraphicsUtils::SVGDPI, vb, true, w, h, false);

        QDomElement layerElement = TextUtils::findElementWithAttribute(root, "id", layerName());
        if (layerElement.isNull()) {
		    QList<QDomNode> rootChildren;
		    QDomNode rootChild = root.firstChild();
		    while (!rootChild.isNull()) {
			    rootChildren.append(rootChild);
			    rootChild = rootChild.nextSibling();
		    }

		    QDomElement topG = domDocument.createElement("g");
		    topG.setAttribute("id", layerName());
		    root.appendChild(topG);
		    foreach (QDomNode node, rootChildren) {
			    topG.appendChild(node);
		    }
        }

		svg = TextUtils::removeXMLEntities(domDocument.toString());
	}
	else {
		QImage image(fileName);
		if (image.isNull()) {
			unableToLoad(fileName, tr("for unknown reasons--possibly the image file is corrupted"));
			return;
		}

		if (image.format() != QImage::Format_RGB32 && image.format() != QImage::Format_ARGB32) {
			image = image.convertToFormat(QImage::Format_Mono);
		}
		
        double res = image.dotsPerMeterX() / GraphicsUtils::InchesPerMeter;
        if (this->m_standardizeColors) {
		    GroundPlaneGenerator gpg;
		    gpg.setLayerName(layerName());
		    gpg.setMinRunSize(1, 1);
		    gpg.scanImage(image, image.width(), image.height(), 1, res, colorString(), false, false, QSizeF(0, 0), 0, QPointF(0, 0));
		    if (gpg.newSVGs().count() < 1) {
			    FMessageBox::information(
				    NULL,
				    tr("Unable to display"),
				    tr("Unable to display image from %1").arg(fileName)
			    );
			    return;
		    }

            svg = gpg.mergeSVGs("", layerName());
        }
        else {
            svg = TextUtils::makeSVGHeader(res, res, image.width(), image.height());
            int ix = svg.lastIndexOf(">");
            svg.replace(ix, 1, "xmlns:xlink='http://www.w3.org/1999/xlink'>");
            QString type = "png";
            QByteArray bytes;
            if (fileName.endsWith(".png") || fileName.endsWith(".jpg") || fileName.endsWith("jpeg")) {
                if (!fileName.endsWith(".png")) type = "jpg";
                QFile file(fileName);
                file.open(QFile::ReadOnly);
                bytes = file.readAll();
            }
            else {
                QBuffer buffer(&bytes);
                buffer.open(QIODevice::WriteOnly);
                image.save(&buffer, "PNG"); // writes image into bytes in PNG format
            }

            svg += QString("<image x='0' y='0' width='%1' height='%2' xlink:href='data:image/%3;base64,")
                .arg(image.width()).arg(image.height()).arg(type);

            int remaining = bytes.count();
            ix = 0;
            while (remaining > 0) {
                QByteArray sixty = bytes.mid(ix, qMin(remaining, 60));
                svg += QString(sixty.toBase64());
                svg += "\n";
                ix += 60;
                remaining -= 60;
            }

            svg += "'/>\n";
            svg += "</svg>";
        }
	}

	reloadImage(svg, QSizeF(0, 0), fileName, addName);
}

bool LogoItem::resizeMM(double mmW, double mmH, const LayerHash & viewLayers) 
{
	Q_UNUSED(viewLayers);

	if (mmW == 0 || mmH == 0) {
		return false;
	}

	// DebugDialog::debug(QString("resize mm %1 %2").arg(mmW).arg(mmH));

	QRectF r = this->boundingRect();
	if (qAbs(GraphicsUtils::pixels2mm(r.width(), GraphicsUtils::SVGDPI) - mmW) < .001 &&
		qAbs(GraphicsUtils::pixels2mm(r.height(), GraphicsUtils::SVGDPI) - mmH) < .001) 
	{
		return false;
	}

	double inW = GraphicsUtils::mm2mils(mmW) / 1000;
	double inH = GraphicsUtils::mm2mils(mmH) / 1000;

	// TODO: deal with aspect ratio

	QString svg = prop("shape");
	if (svg.isEmpty()) return false;

	QString errorStr;
	int errorLine;
	int errorColumn;

	QDomDocument domDocument;
	if (!domDocument.setContent(svg, &errorStr, &errorLine, &errorColumn)) {
		return false;
	}

	QDomElement root = domDocument.documentElement();
	if (root.isNull()) {
		return false;
	}

	if (root.tagName() != "svg") {
		return false;
	}

	root.setAttribute("width", QString::number(inW) + "in");
	root.setAttribute("height", QString::number(inH) + "in");

	svg = TextUtils::removeXMLEntities(domDocument.toString());	
    QString shape = getShapeForRenderer(svg);

	bool result = resetRenderer(shape);
	if (result) {
		modelPart()->setLocalProp("shape", svg);
		modelPart()->setLocalProp("width", mmW);
		modelPart()->setLocalProp("height", mmH);
	}

	setWidthAndHeight(qRound(mmW * 10) / 10.0, qRound(mmH * 10) / 10.0);
    return true;
}

void LogoItem::setProp(const QString & prop, const QString & value) {
	if (prop.compare("logo", Qt::CaseInsensitive) == 0) {
		setLogo(value, false);
		return;
	}

	ResizableBoard::setProp(prop, value);
}

void LogoItem::setLogo(QString logo, bool force) {
	if (!force && m_logo.compare(logo) == 0) return;

    if (!isEverVisible()) {
        return;
    }

	QString svg;
	QFile f(m_originalFilename);
	if (f.open(QFile::ReadOnly)) {
		svg = f.readAll();
	}

	if (svg.isEmpty()) return;

	QSizeF oldSize = m_size;
	QXmlStreamReader streamReader(svg);
	QSizeF oldSvgSize = fsvgRenderer() ? fsvgRenderer()->viewBoxF().size() : QSizeF(0, 0);
	
	DebugDialog::debug(QString("size %1 %2, %3 %4").arg(m_size.width()).arg(m_size.height()).arg(oldSvgSize.width()).arg(oldSvgSize.height()));

	svg = hackSvg(svg, logo);
	QXmlStreamReader newStreamReader(svg);

	bool ok = rerender(svg);

	m_logo = logo;
	modelPart()->setLocalProp("logo", logo);
	modelPart()->setLocalProp("shape", svg);
	if (ok && !force) {
		QSizeF newSvgSize = fsvgRenderer()->viewBoxF().size();
		QSizeF newSize = newSvgSize * oldSize.height() / oldSvgSize.height();
		DebugDialog::debug(QString("size %1 %2, %3 %4").arg(m_size.width()).arg(m_size.height()).arg(newSize.width()).arg(newSize.height()));
		
		// set the new text to approximately the same height as the original
		// if the text is non-proportional that will be lost
		LayerHash layerHash;
		resizeMM(GraphicsUtils::pixels2mm(newSize.width(), GraphicsUtils::SVGDPI),
				 GraphicsUtils::pixels2mm(newSize.height(), GraphicsUtils::SVGDPI),
				 layerHash);
		//DebugDialog::debug(QString("size %1 %2").arg(m_size.width()).arg(m_size.height()));
	}
}

bool LogoItem::rerender(const QString & svg)
{
    QString newSvg;
	bool result = resetRenderer(svg);
	if (result) {
		QRectF r = fsvgRenderer()->viewBoxF();
		m_aspectRatio.setWidth(r.width());
		m_aspectRatio.setHeight(r.height());
	}
	return result;
}

QString LogoItem::getProperty(const QString & key) {
	if (key.compare("logo", Qt::CaseInsensitive) == 0) {
		return m_logo;
	}

	return PaletteItem::getProperty(key);
}

const QString & LogoItem::logo() {
	return m_logo;
}

bool LogoItem::canEditPart() {
	return false;
}

bool LogoItem::hasPartLabel() {
	return false;
}

void LogoItem::logoEntry() {
	QLineEdit * edit = qobject_cast<QLineEdit *>(sender());
	if (edit == NULL) return;

    logoEntryAux(edit->text());
}

void LogoItem::logoEntryAux(const QString & text)
{
	if (text.compare(this->logo()) == 0) return;

	// m_inLogoEntry is a hack because a focus out event was being triggered on m_widthEntry when a user hit the return key in logoentry
	// this triggers a call to widthEntry() which causes all kinds of havoc.  (bug in version 0.7.1 and possibly earlier)
	m_inLogoEntry = QTime::currentTime().addSecs(1);

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->setProp(this, "logo", tr("text"), this->logo(), text, true);
	}
}

void LogoItem::initImage() {
	if (m_hasLogo) {
		setLogo(m_logo, true);
		return;
	}

	loadImage(m_originalFilename, false);
}

QString LogoItem::hackSvg(const QString & svg, const QString & logo) 
{
	QString errorStr;
	int errorLine;
	int errorColumn;
	QDomDocument doc;
	if (!doc.setContent(svg, &errorStr, &errorLine, &errorColumn)) return svg;

	QDomElement root = doc.documentElement();
	root.setAttribute("width", QString::number(logo.length() * 0.1) + "in");
	
	QString viewBox = root.attribute("viewBox");
	QStringList coords = viewBox.split(" ", QString::SkipEmptyParts);
	coords[2] = QString::number(logo.length() * 10);
	root.setAttribute("viewBox", coords.join(" "));

    QStringList exceptions;
	exceptions << "none" << "";
	QString toColor(colorString());
	SvgFileSplitter::changeColors(root, toColor, exceptions);

	QDomNodeList domNodeList = root.elementsByTagName("text");
	for (int i = 0; i < domNodeList.count(); i++) {
		QDomElement node = domNodeList.item(i).toElement();
		if (node.isNull()) continue;

		if (node.attribute("id").compare("label") != 0) continue;

		node.setAttribute("x", QString::number(logo.length() * 5));

		QDomNodeList childList = node.childNodes();
		for (int j = 0; j < childList.count(); j++) {
			QDomNode child = childList.item(i);
			if (child.isText()) {
				child.setNodeValue(logo);

				modelPart()->setLocalProp("width", logo.length() * 0.1 * 25.4);
				QString h = root.attribute("height");
				bool ok;
				modelPart()->setLocalProp("height", TextUtils::convertToInches(h, &ok, false) * 25.4);
                if (!isBottom()) return  doc.toString();
                return flipSvg(doc.toString());
			}
		}
	}

    if (!isBottom()) return svg;

	return flipSvg(svg);
}

void LogoItem::widthEntry() {
	if (QTime::currentTime() < m_inLogoEntry) return;

	QLineEdit * edit = qobject_cast<QLineEdit *>(sender());
	if (edit == NULL) return;

	double w = edit->text().toDouble();
	double oldW = m_modelPart->localProp("width").toDouble();
	if (w == oldW) return;

	double h = m_modelPart->localProp("height").toDouble();
	if (m_keepAspectRatio) {
		h = w * m_aspectRatio.height() / m_aspectRatio.width();
	}

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->resizeBoard(w, h, true);
	}
}

void LogoItem::heightEntry() {
	if (QTime::currentTime() < m_inLogoEntry) return;

	QLineEdit * edit = qobject_cast<QLineEdit *>(sender());
	if (edit == NULL) return;

	double h = edit->text().toDouble();
	double oldH =  m_modelPart->localProp("height").toDouble();
	if (h == oldH) return;

	setHeight(h);
}

void LogoItem::setHeight(double h)
{
	double w = m_modelPart->localProp("width").toDouble();
	if (m_keepAspectRatio) {
		w = h * m_aspectRatio.width() / m_aspectRatio.height();
	}

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->resizeBoard(w, h, true);
	}
}

bool LogoItem::stickyEnabled() {
	return true;
}

ItemBase::PluralType LogoItem::isPlural() {
	return Plural;
}

ViewLayer::ViewLayerID LogoItem::layer() {
	return  isBottom() ? ViewLayer::Silkscreen0 : ViewLayer::Silkscreen1;
}

QString LogoItem::colorString() {
    return isBottom() ? ViewLayer::Silkscreen0Color :  ViewLayer::Silkscreen1Color;
}

QString LogoItem::layerName() 
{
	return ViewLayer::viewLayerXmlNameFromID(layer());
}

double LogoItem::minWidth() {
    double zoom = 1;
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		zoom = infoGraphicsView->currentZoom() / 100;
        if (zoom == 0) zoom = 1;
	}
    
	return ResizableBoard::CornerHandleSize * 2 / zoom;
}

double LogoItem::minHeight() {
    double zoom = 1;
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		zoom = infoGraphicsView->currentZoom() / 100;
        if (zoom == 0) zoom = 1;
	}

	return ResizableBoard::CornerHandleSize * 2 / zoom;
}

bool LogoItem::freeRotationAllowed(Qt::KeyboardModifiers modifiers) {

	if ((modifiers & altOrMetaModifier()) == 0) return false;
	if (!isSelected()) return false;

	return true;
}

bool LogoItem::freeRotationAllowed() {
	return true;
}

ResizableBoard::Corner LogoItem::findCorner(QPointF scenePos, Qt::KeyboardModifiers modifiers) {
	ResizableBoard::Corner corner = ResizableBoard::findCorner(scenePos, modifiers);
	if (corner == ResizableBoard::NO_CORNER) return corner;

	if (modifiers & altOrMetaModifier()) {
		// free rotate
		setCursor(*CursorMaster::RotateCursor);
		return ResizableBoard::NO_CORNER;
	}

	return corner;
}

void LogoItem::paintHover(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	PaletteItem::paintHover(painter, option, widget);
}

bool LogoItem::isBottom() {
    QString layer = modelPart()->properties().value("layer");
	return layer.contains("0") || layer.contains("bottom");
}

QString LogoItem::flipSvg(const QString & svg)
{
	QString newSvg = svg;
    // need the end quote in the replace since the xml for silkscreen1 is just "silkscreen"
	newSvg.replace("\"" + ViewLayer::viewLayerXmlNameFromID(ViewLayer::Silkscreen1) + "\"", "\"" + ViewLayer::viewLayerXmlNameFromID(ViewLayer::Silkscreen0) + "\"");
	newSvg.replace("'" + ViewLayer::viewLayerXmlNameFromID(ViewLayer::Silkscreen1) + "'", "'" + ViewLayer::viewLayerXmlNameFromID(ViewLayer::Silkscreen0) + "'");
	newSvg.replace(ViewLayer::Silkscreen1Color, ViewLayer::Silkscreen0Color, Qt::CaseInsensitive);
    return flipSvgAux(newSvg);
}

QString LogoItem::flipSvgAux(QString & newSvg)
{
	QMatrix m;
	QSvgRenderer renderer(newSvg.toUtf8());
	QRectF bounds = renderer.viewBoxF();
	m.translate(bounds.center().x(), bounds.center().y());
	QMatrix mMinus = m.inverted();
    QMatrix cm = mMinus * QMatrix().scale(-1, 1) * m;
	int gix = newSvg.indexOf("<g");
	newSvg.replace(gix, 2, "<g _flipped_='1' transform='" + TextUtils::svgMatrix(cm) + "'");
	return newSvg;
}

QString LogoItem::getNewLayerFileName(const QString & newLayerName) {
    if (newLayerName.isEmpty()) return "";

    QString lastfilename = this->prop("lastfilename");
    if (QFile::exists(lastfilename)) return lastfilename;

    DebugDialog::debug(QString("logo item image '%1' doesn't exist").arg(lastfilename));
    return "";
}

QString LogoItem::getInspectorTitle() {
    if (hasLogo()) return m_logo;

    return ResizableBoard::getInspectorTitle();
}

void LogoItem::setInspectorTitle(const QString & oldText, const QString & newText) {
    if (!hasLogo()) return ResizableBoard::setInspectorTitle(oldText, newText);

    logoEntryAux(newText);
}

///////////////////////////////////////////////////////////////////////

SchematicLogoItem::SchematicLogoItem( ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: LogoItem(modelPart, viewID, viewGeometry, id, itemMenu, doLabel)
{
}

SchematicLogoItem::~SchematicLogoItem() {
}

ViewLayer::ViewLayerID SchematicLogoItem::layer() {
	return (m_hasLogo ? ViewLayer::SchematicText : ViewLayer::Schematic);
}

QString SchematicLogoItem::colorString() {
    return "black";
}

bool SchematicLogoItem::isBottom() {
    return false;
}

///////////////////////////////////////////////////////////////////////

BreadboardLogoItem::BreadboardLogoItem( ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: LogoItem(modelPart, viewID, viewGeometry, id, itemMenu, doLabel)
{
    m_color = "#787878";
	QString color = modelPart->localProp("color").toString();
    if (color.isEmpty()) {
	    color = modelPart->properties().value("color");
        if (color.isEmpty()) color = m_color;
		modelPart->setLocalProp("color", color);
	}
    m_color = color;
    m_standardizeColors = false;
}

BreadboardLogoItem::~BreadboardLogoItem() {
}

ViewLayer::ViewLayerID BreadboardLogoItem::layer() {
	return ViewLayer::Breadboard;
}

QString BreadboardLogoItem::colorString() {
    return m_color;
}

bool BreadboardLogoItem::collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide) 
{
	if (m_hasLogo) {
		if (prop.compare("color", Qt::CaseInsensitive) == 0) {
			returnProp = tr("color");
			returnValue = m_color;

		    QPushButton * button = new QPushButton(tr("Set text color"));
		    button->setObjectName("infoViewButton");
		    connect(button, SIGNAL(pressed()), this, SLOT(changeTextColor()));

			returnWidget = button;
			return true;
		}
	}

	return LogoItem::collectExtraInfo(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget, hide);
}

void BreadboardLogoItem::changeTextColor() {
    QColor color(m_color);
    QColor newColor = QColorDialog::getColor(color, NULL, tr("Select text color"));
    if (!newColor.isValid()) return;
    if (newColor.name().compare(m_color, Qt::CaseInsensitive) == 0) return;

    InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->setProp(this, "color", tr("color"), m_color, newColor.name(), true);
	}
}

void BreadboardLogoItem::setProp(const QString & prop, const QString & value) {
	if (m_hasLogo && prop.compare("color", Qt::CaseInsensitive) == 0) {
        m_color = value;
        modelPart()->setLocalProp("color", m_color);
		setLogo(m_logo, true);
		return;
	}

	LogoItem::setProp(prop, value);
}

bool BreadboardLogoItem::isBottom() {
    return false;
}

///////////////////////////////////////////////////////////////////////

CopperLogoItem::CopperLogoItem( ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: LogoItem(modelPart, viewID, viewGeometry, id, itemMenu, doLabel)
{
    // TODO: load these names from somewhere

	if (Copper1ImageNames.count() == 0) {
		Copper1ImageNames << "new Made with Fritzing copper1" << "new Fritzing icon copper1" << "OHANDA logo copper1" << "OSHW logo copper1";
	}

	if (Copper0ImageNames.count() == 0) {
		Copper0ImageNames << "new Made with Fritzing copper0" << "new Fritzing icon copper0" << "OHANDA logo copper0" << "OSHW logo copper1";
	}

	m_hasLogo = (modelPart->moduleID().endsWith(ModuleIDNames::LogoTextModuleIDName));
	m_logo = modelPart->localProp("logo").toString();
	if (m_hasLogo && m_logo.isEmpty()) {
		m_logo = modelPart->properties().value("logo", "logo");
		modelPart->setLocalProp("logo", m_logo);
	}
}

CopperLogoItem::~CopperLogoItem() {
}

ViewLayer::ViewLayerID CopperLogoItem::layer() {
	return isBottom() ? ViewLayer::Copper0 :  ViewLayer::Copper1;
}

QString CopperLogoItem::colorString() {
	return isBottom() ? ViewLayer::Copper0Color :  ViewLayer::Copper1Color;
}

QStringList & CopperLogoItem::getImageNames() {
	return isBottom() ? Copper0ImageNames : Copper1ImageNames;
}

QString CopperLogoItem::flipSvg(const QString & svg)
{
	QString newSvg = svg;
	newSvg.replace(ViewLayer::viewLayerXmlNameFromID(ViewLayer::Copper1), ViewLayer::viewLayerXmlNameFromID(ViewLayer::Copper0));
	newSvg.replace(ViewLayer::Copper1Color, ViewLayer::Copper0Color, Qt::CaseInsensitive);
    return flipSvgAux(newSvg);
}

////////////////////////////////////////////

// todo:
//
// don't mess with colors at all?

BoardLogoItem::BoardLogoItem(ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel) 
    : LogoItem(modelPart, viewID, viewGeometry, id, itemMenu, doLabel)
{
    m_hasLogo = false;
    m_svgOnly = true;
    m_standardizeColors = false;
	if (BoardImageNames.count() == 0) {
		BoardImageNames << "circle_pcb";
	}
}

BoardLogoItem::~BoardLogoItem() {
}

QString BoardLogoItem::getShapeForRenderer(const QString & svg) 
{
    return ResizableBoard::getShapeForRenderer(svg, m_viewLayerID);
}

ViewLayer::ViewLayerID BoardLogoItem::layer() {
	return ViewLayer::Board;
}

QString BoardLogoItem::colorString() {
	return ViewLayer::BoardColor;
}

QStringList & BoardLogoItem::getImageNames() {
	return BoardImageNames;
}

QStringList & BoardLogoItem::getNewImageNames() {
	return NewBoardImageNames;
}

bool BoardLogoItem::resizeMM(double mmW, double mmH, const LayerHash & viewLayers) 
{
	bool result = LogoItem::resizeMM(mmW, mmH, viewLayers);
    if (!result) return result;

    reloadLayerKin(mmW, mmH);
    return result;
}

void BoardLogoItem::reloadLayerKin(double mmW, double mmH)
{
	foreach (ItemBase * itemBase, m_layerKin) {
        if (itemBase->viewLayerID() == ViewLayer::Silkscreen1 || itemBase->viewLayerID() == ViewLayer::Silkscreen0) {
		    QString svg = ResizableBoard::getShapeForRenderer(prop("shape"), itemBase->viewLayerID());
		    if (!svg.isEmpty()) {
			    itemBase->prepareGeometryChange();
			    if (itemBase->resetRenderer(svg)) {
				    itemBase->modelPart()->setLocalProp("width", mmW);
				    itemBase->modelPart()->setLocalProp("height", mmH);
			    }
		    }
	    }
    }
}

bool BoardLogoItem::canRetrieveLayer(ViewLayer::ViewLayerID viewLayerID) {
    return LogoItem::canRetrieveLayer(viewLayerID) || viewLayerID == LogoItem::layer() || viewLayerID == ViewLayer::Silkscreen0 || viewLayerID == ViewLayer::Silkscreen1;
}

bool BoardLogoItem::reloadImage(const QString & svg, const QSizeF & aspectRatio, const QString & fileName, bool addName) 
{
    bool result;
    if (!svg.contains(GerberGenerator::MagicBoardOutlineID, Qt::CaseInsensitive)) {
        QString nsvg = setBoardOutline(svg);
        result = LogoItem::reloadImage(nsvg, aspectRatio, fileName, addName);
    }
    else {
        result = LogoItem::reloadImage(svg, aspectRatio, fileName, addName);
    }
    if (!result) return result;

    reloadLayerKin(prop("width").toDouble(), prop("height").toDouble());
    return result;
}

bool BoardLogoItem::checkImage(const QString & filename) {
    // override logoitem
    return ResizableBoard::checkImage(filename);
}

bool BoardLogoItem::collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide) 
{
	if (prop.compare("shape", Qt::CaseInsensitive) == 0) {
        Board::collectExtraInfo(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget, hide);
		returnWidget = setUpDimEntry(true, true, false, returnWidget);  
        returnWidget->setEnabled(swappingEnabled);

        m_aspectRatioCheck->setEnabled(false);
        m_aspectRatioCheck->setChecked(true);
        m_aspectRatioCheck->setVisible(false);
        m_aspectRatioLabel->setVisible(false);
		returnProp = tr("shape");
		return true;
	}

    return LogoItem::collectExtraInfo(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget, hide);
}

bool BoardLogoItem::isBottom() {
    return false;
}


