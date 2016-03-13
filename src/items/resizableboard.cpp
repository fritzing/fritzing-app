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

#include "resizableboard.h"
#include "../utils/resizehandle.h"
#include "../utils/graphicsutils.h"
#include "../utils/folderutils.h"
#include "../utils/textutils.h"
#include "../fsvgrenderer.h"
#include "../sketch/infographicsview.h"
#include "../svg/svgfilesplitter.h"
#include "../commands.h"
#include "moduleidnames.h"
#include "../layerattributes.h"
#include "../debugdialog.h"
#include "../svg/gerbergenerator.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QImageReader>
#include <QMessageBox>
#include <QRegExp>
#include <qmath.h>
#include <qnumeric.h>

QList<QString> PaperSizeNames;
QList<QSizeF> PaperSizeDimensions;

static QHash<QString, QString> NamesToXmlNames;
static QHash<QString, QString> XmlNamesToNames;

static QHash<QString, QString> BoardLayerTemplates;
static QHash<QString, QString> SilkscreenLayerTemplates;
static QHash<QString, QString> Silkscreen0LayerTemplates;
static const int LineThickness = 8;
static const QRegExp HeightExpr("height=\\'\\d*px");
static QString StandardCustomBoardExplanation;

QStringList Board::BoardImageNames;
QStringList Board::NewBoardImageNames;

const double ResizableBoard::CornerHandleSize = 7.0;

static const double JND = (double) 0.01;

QString Board::OneLayerTranslated;
QString Board::TwoLayersTranslated;

Board::Board( ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: PaletteItem(modelPart, viewID, viewGeometry, id, itemMenu, doLabel)
{
    m_svgOnly = true;
	m_fileNameComboBox = NULL;
    if (isBoard(modelPart)) {
        if (modelPart->localProp("layers").isNull()) {
            modelPart->setLocalProp("layers", modelPart->properties().value("layers"));
        }
        if (itemType() == ModelPart::Board && !modelPart->properties().keys().contains("filename")) {
            // deal with old style custom boards
            modelPart->modelPartShared()->setProperty("filename", "", false);
        }
    }

    if (StandardCustomBoardExplanation.isEmpty()) {
        StandardCustomBoardExplanation = tr("\n\nA custom board svg typically has one or two silkscreen layers and one board layer.\n") +
                                            tr("Have a look at the circle_pcb.svg file in your Fritzing installation folder at parts/svg/core/pcb/.\n\n");
    }

    if (NamesToXmlNames.count() == 0) {
        NamesToXmlNames.insert("copper bottom", "copper0");
        NamesToXmlNames.insert("copper top", "copper1");
        NamesToXmlNames.insert("silkscreen bottom", "silkscreen0");
        NamesToXmlNames.insert("silkscreen top", "silkscreen");
        foreach (QString key, NamesToXmlNames.keys()) {
            XmlNamesToNames.insert(NamesToXmlNames.value(key), key);
        }
    }

}

Board::~Board() {
}

void Board::paintHover(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(widget);
	Q_UNUSED(option);
	painter->save();
	painter->setOpacity(0);
	painter->fillPath(this->hoverShape(), QBrush(HoverColor));
	painter->restore();
}

QStringList Board::collectValues(const QString & family, const QString & prop, QString & value) {
	if (prop.compare("layers", Qt::CaseInsensitive) == 0) {
        QString realValue = modelPart()->localProp("layers").toString();
        if (!realValue.isEmpty()) {
            value = realValue;
        }
		QStringList result;
		if (OneLayerTranslated.isEmpty()) {
			OneLayerTranslated = tr("one layer (single-sided)");
		}
		if (TwoLayersTranslated.isEmpty()) {
			TwoLayersTranslated = tr("two layers (double-sided)");
		}

		result.append(OneLayerTranslated);
		result.append(TwoLayersTranslated);

		if (value == "1") {
			value = OneLayerTranslated;
		}
		else if (value == "2") {
			value = TwoLayersTranslated;
		}

		return result;
	}


	QStringList result = PaletteItem::collectValues(family, prop, value);
    if (prop.compare("shape", Qt::CaseInsensitive) == 0 && isBoard(this) && !this->moduleID().contains(ModuleIDNames::BoardLogoImageModuleIDName)) {
        result.removeAll("Custom Shape");
    }

    return result;

}

bool Board::collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide) 
{
	if (prop.compare("filename", Qt::CaseInsensitive) == 0 && isBoard(this)) {
        setupLoadImage(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget);
		return true;
	}

	return PaletteItem::collectExtraInfo(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget, hide);
}

bool Board::rotation45Allowed() {
	return false;
}

bool Board::stickyEnabled() {
	return false;
}

ItemBase::PluralType Board::isPlural() {
	return Plural;
}

bool Board::canFindConnectorsUnder() {
	return false;
}


bool Board::isBoard(ItemBase * itemBase) {
    if (qobject_cast<Board *>(itemBase) == NULL) return false;

    return isBoard(itemBase->modelPart());
}

bool Board::isBoard(ModelPart * modelPart) {
    if (modelPart == NULL) return false;

    switch (modelPart->itemType()) {
        case ModelPart::Board:
            return true;
        case ModelPart::ResizableBoard:
            return true;
        case ModelPart::Logo:
            return modelPart->family().contains("pcb", Qt::CaseInsensitive);
        default:
            return false;
    }
}

void Board::setupLoadImage(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget) 
{
    Q_UNUSED(returnValue);
    Q_UNUSED(value);
    Q_UNUSED(prop);
    Q_UNUSED(family);
    Q_UNUSED(parent);

	returnProp = tr("image file");

	QFrame * frame = new QFrame();
	frame->setObjectName("infoViewPartFrame");
	QVBoxLayout * vboxLayout = new QVBoxLayout();
	vboxLayout->setContentsMargins(0, 0, 0, 0);
	vboxLayout->setSpacing(0);
	vboxLayout->setMargin(0);

	QComboBox * comboBox = new QComboBox();
	comboBox->setObjectName("infoViewComboBox");
	comboBox->setEditable(false);
	comboBox->setEnabled(swappingEnabled);
	m_fileNameComboBox = comboBox;

	setFileNameItems();

	connect(comboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(fileNameEntry(const QString &)));

	QPushButton * button = new QPushButton (tr("load image file"));
	button->setObjectName("infoViewButton");
	connect(button, SIGNAL(pressed()), this, SLOT(prepLoadImage()));
	button->setEnabled(swappingEnabled);

	vboxLayout->addWidget(comboBox);
	vboxLayout->addWidget(button);

	frame->setLayout(vboxLayout);
	returnWidget = frame;

	returnProp = "";
}

void Board::setFileNameItems() {
	if (m_fileNameComboBox == NULL) return;

	m_fileNameComboBox->addItems(getImageNames());
	m_fileNameComboBox->addItems(getNewImageNames());

	int ix = 0;
	foreach (QString name, getImageNames()) {
		if (prop("lastfilename").contains(name)) {
			m_fileNameComboBox->setCurrentIndex(ix);
			return;
		}
		ix++;
	}

	foreach (QString name, getNewImageNames()) {
		if (prop("lastfilename").contains(name)) {
			m_fileNameComboBox->setCurrentIndex(ix);
			return;
		}
		ix++;
	}
}

QStringList & Board::getImageNames() {
	return BoardImageNames;
}

QStringList & Board::getNewImageNames() {
	return NewBoardImageNames;
}

void Board::fileNameEntry(const QString & filename) {
	foreach (QString name, getImageNames()) {
		if (filename.compare(name) == 0) {
            QString f = FolderUtils::getAppPartsSubFolderPath("") + "/svg/core/pcb/" + filename + ".svg";
			prepLoadImageAux(f, false);
			return;
		}
	}

	prepLoadImageAux(filename, true);
}

void Board::prepLoadImage() {
	QString imagesStr = tr("Images");
	imagesStr += " (";
    if (!m_svgOnly) {
	    QList<QByteArray> supportedImageFormats = QImageReader::supportedImageFormats();
	    foreach (QByteArray ba, supportedImageFormats) {
		    imagesStr += "*." + QString(ba) + " ";
	    }
    }
	if (!imagesStr.contains("svg")) {
		imagesStr += "*.svg";
	}
	imagesStr += ")";
	QString fileName = FolderUtils::getOpenFileName(
		NULL,
		tr("Select an image file to load"),
		"",
		imagesStr
	);

	if (fileName.isEmpty()) return;

    if (!checkImage(fileName)) return;

	prepLoadImageAux(fileName, true);
}

bool Board::checkImage(const QString & filename) {
    QFile file(filename);
    
	QString errorStr;
	int errorLine;
	int errorColumn;

	QDomDocument domDocument;

	if (!domDocument.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
		unableToLoad(filename, tr("due to an xml problem: %1 line:%2 column:%3").arg(errorStr).arg(errorLine).arg(errorColumn));
		return false;
	}

    QDomElement root = domDocument.documentElement();
	if (root.tagName() != "svg") {
		unableToLoad(filename, tr("because the xml is not correctly formatted"));
		return false;
	}

    QList<QDomElement> elements;
    TextUtils::findElementsWithAttribute(root, "id", elements);
    int layers = 0;
    QList<QDomElement> boardElements;
    int silk0Layers = 0;
    int silk1Layers = 0;
    bool boardHasChildren = false;
    foreach (QDomElement element, elements) {
        QString id = element.attribute("id");
        ViewLayer::ViewLayerID viewLayerID = ViewLayer::viewLayerIDFromXmlString(id);
        if (viewLayerID != ViewLayer::UnknownLayer) {
            layers++;
            if (viewLayerID == ViewLayer::Board) {
                boardElements << element;
                if (element.childNodes().count() > 0) {
                    boardHasChildren = true;
                }
            }
            else if (viewLayerID == ViewLayer::Silkscreen1) {
                silk1Layers++;
            }
            else if (viewLayerID == ViewLayer::Silkscreen0) {
                silk0Layers++;
            }
        }
    }

    if (boardElements.count() == 1 && !boardHasChildren) {
        unableToLoad(filename, tr("the <board> element contains no shape elements") + StandardCustomBoardExplanation);
        return false;
    }

    if ((boardElements.count() == 1) && ((silk1Layers == 1) || (silk0Layers == 1))) {
        moreCheckImage(filename);
        return true;
    }

    if (boardElements.count() > 1) {
        unableToLoad(filename, tr("because there are multiple <board> layers") + StandardCustomBoardExplanation);
        return false;
    }

    if (silk1Layers > 1) {
        unableToLoad(filename, tr("because there are multiple <silkscreen> layers") + StandardCustomBoardExplanation);
        return false;
    }

    if (silk0Layers > 1) {
        unableToLoad(filename, tr("because there are multiple <silkscreen0> layers") + StandardCustomBoardExplanation);
        return false;
    }

    if (layers > 0 && boardElements.count() == 0) {
        unableToLoad(filename, tr("because there is no <board> layer") + StandardCustomBoardExplanation);
        return false;
    }

    if (layers == 0 && root.childNodes().count() == 0) {
        unableToLoad(filename, tr("the svg contains no shape elements") + StandardCustomBoardExplanation);
        return false;
    }

    if (layers == 0 || (boardElements.count() == 1 && silk1Layers == 0 && silk1Layers == 0)) {
        bool result = canLoad(filename, tr("but the pcb itself will have no silkscreen layer") + StandardCustomBoardExplanation);
        if (result) moreCheckImage(filename);
        return result;
    }

    unableToLoad(filename, tr("the svg doesn't fit the custom board format") + StandardCustomBoardExplanation);
    return false;
}

void Board::moreCheckImage(const QString & filename) {
    QFile file(filename);
    file.open(QFile::ReadOnly);
    QString svg = file.readAll();
    file.close();

    QString nsvg = setBoardOutline(svg);

	QString errorStr;
	int errorLine;
	int errorColumn;
	QDomDocument domDocument;
	domDocument.setContent(nsvg, &errorStr, &errorLine, &errorColumn);
    QDomElement element = TextUtils::findElementWithAttribute(domDocument.documentElement(), "id", GerberGenerator::MagicBoardOutlineID);

    int subpaths = 1;
    int mCount = 0;
    if (element.tagName() == "path") {
        QString originalPath = element.attribute("d", "").trimmed();
        if (GerberGenerator::MultipleZs.indexIn(originalPath) >= 0) {
            QStringList ds = element.attribute("d").split("z", QString::SkipEmptyParts);
            subpaths = ds.count();
            foreach (QString d, ds) {
                if (d.trimmed().startsWith("m", Qt::CaseInsensitive)) mCount++;
            }
        }
    }

    QString msg = tr("<b>The custom shape has been loaded, and you will see the new board shortly.</b><br/><br/>");
    msg += tr("Before actual PCB production we recommend that you test your custom shape by using the 'File > Export for Production > Extended Gerber' option. ");
    msg += tr("Check the resulting contour file with a Gerber-viewer application to make sure the shape came out as expected.<br/><br/>");

    msg += tr("The rest of this message concerns 'cutouts'. ");
    msg += tr("These are circular or irregularly-shaped holes that you can optionally incorporate into a custom PCB shape.<br/><br/>");
    if (subpaths == 1) {
        msg += tr("<b>The custom shape has no cutouts.</b>");
    }
    else {
        msg += tr("<b>The custom shape has %n cutouts.</b>", "", subpaths - 1);
        if (subpaths != mCount) {
            msg += tr("<br/>However, the cutouts may not be formatted correctly.");
        }
    }
    msg +=  tr("<br/><br/>If you intended your custom shape to have cutouts and you did not get the expected result, ");
    msg += tr("it is because Fritzing requires that you make cutouts using a shape 'subtraction' or 'difference' operation in your vector graphics editor.");
    QMessageBox::information(NULL, "Custom Shape", msg);
}

QString Board::setBoardOutline(const QString & svg) {
    QString errorStr;
	int errorLine;
	int errorColumn;

	QDomDocument domDocument;
	if (!domDocument.setContent(svg, true, &errorStr, &errorLine, &errorColumn)) {
		return svg;
	}

    QDomElement root = domDocument.documentElement();
	if (root.tagName() != "svg") {
		return svg;
	}

    QDomElement board = TextUtils::findElementWithAttribute(root, "id", "board");
    if (board.isNull()) {
        board = root;
    }

    QList<QDomElement> leaves;
    TextUtils::collectLeaves(board, leaves);

    if (leaves.count() == 1) {
        QDomElement leaf = leaves.at(0);
        leaf.setAttribute("id", GerberGenerator::MagicBoardOutlineID);
        return TextUtils::removeXMLEntities(domDocument.toString());
    }

    int ix = 0;
    QStringList ids;
    foreach (QDomElement leaf, leaves) {
        ids.append(leaf.attribute("id", ""));
        leaf.setAttribute("id", QString("____%1____").arg(ix++));
    }

    QSvgRenderer renderer(domDocument.toByteArray());

    ix = 0;
    foreach (QDomElement leaf, leaves) {    
        leaf.setAttribute("id", ids.at(ix++));
    }

    double maxArea = 0;
    int maxIndex = -1;
    for (int i = 0; i < leaves.count(); i++) {
        QRectF r = renderer.boundsOnElement(QString("____%1____").arg(i));
        if (r.width() * r.height() > maxArea) {
            maxArea = r.width() * r.height();
            maxIndex = i;
        }
    }

    if (maxIndex >= 0) {
        QDomElement leaf = leaves.at(maxIndex);
        leaf.setAttribute("id", GerberGenerator::MagicBoardOutlineID);
        return TextUtils::removeXMLEntities(domDocument.toString());
    }

    return svg;
}
void Board::unableToLoad(const QString & fileName, const QString & reason) {
	QMessageBox::information(
		NULL,
		tr("Unable to load"),
		tr("Unable to load image from %1 %2").arg(fileName).arg(reason)
	);
}

bool Board::canLoad(const QString & fileName, const QString & reason) {
    QMessageBox::StandardButton answer = QMessageBox::question(
		NULL,
		tr("Can load, but"),
		tr("The image from %1 can be loaded, but %2\nUse the file?").arg(fileName).arg(reason),
		QMessageBox::Yes | QMessageBox::No,
		QMessageBox::No
	);
	return answer == QMessageBox::Yes;
}

void Board::prepLoadImageAux(const QString & fileName, bool addName)
{
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->loadLogoImage(this, "", QSizeF(0,0), "", fileName, addName);
	}
}

ViewLayer::ViewID Board::useViewIDForPixmap(ViewLayer::ViewID vid, bool) 
{
    if (vid == ViewLayer::PCBView) {
        return ViewLayer::IconView;
    }

    return ViewLayer::UnknownView;
}

QString Board::convertToXmlName(const QString & name) {
    foreach (QString key, ItemBase::TranslatedPropertyNames.keys()) {
        if (name.compare(ItemBase::TranslatedPropertyNames.value(key), Qt::CaseInsensitive) == 0) {
            return NamesToXmlNames.value(key);
        }
    }

    return name;
}

QString Board::convertFromXmlName(const QString & xmlName) {
    QString result = ItemBase::TranslatedPropertyNames.value(XmlNamesToNames.value(xmlName));
    if (result.isEmpty()) return xmlName;
    return result;
}

///////////////////////////////////////////////////////////

ResizableBoard::ResizableBoard( ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: Board(modelPart, viewID, viewGeometry, id, itemMenu, doLabel)
{
	fixWH();

	m_keepAspectRatio = false;
	m_widthEditor = m_heightEditor = NULL;
    m_aspectRatioCheck = NULL;
    m_aspectRatioLabel = NULL;
    m_revertButton = NULL;
    m_paperSizeComboBox = NULL;

	m_corner = ResizableBoard::NO_CORNER;
	m_currentScale = 1.0;
	m_decimalsAfter = 1;
}

ResizableBoard::~ResizableBoard() {
}

void ResizableBoard::addedToScene(bool temporary) {

	loadTemplates();
	if (this->scene()) {
		setInitialSize();
	}

	Board::addedToScene(temporary);
}

void ResizableBoard::loadTemplates() {
    if (!BoardLayerTemplates.value(moduleID(), "").isEmpty()) return;

    QFile file(m_filename);
    if (!file.open(QIODevice::ReadOnly)) return;

    QString svg = file.readAll();
    if (svg.isEmpty()) return;

    BoardLayerTemplates.insert(moduleID(), getShapeForRenderer(svg, ViewLayer::Board));
    SilkscreenLayerTemplates.insert(moduleID(), getShapeForRenderer(svg, ViewLayer::Silkscreen1));
    Silkscreen0LayerTemplates.insert(moduleID(), getShapeForRenderer(svg, ViewLayer::Silkscreen0));
}

double ResizableBoard::minWidth() {
	return 0.25 * GraphicsUtils::SVGDPI;
}

double ResizableBoard::minHeight() {
	return 0.25 * GraphicsUtils::SVGDPI;
}

void ResizableBoard::mousePressEvent(QGraphicsSceneMouseEvent * event) 
{
	m_corner = ResizableBoard::NO_CORNER;

	if (m_spaceBarWasPressed) {
		Board::mousePressEvent(event);
		return;
	}

	double right = m_size.width();
	double bottom = m_size.height();

	m_resizeMousePos = event->scenePos();
	m_resizeStartPos = pos();
	m_resizeStartSize = m_size;
	m_resizeStartTopLeft = mapToScene(0, 0);
	m_resizeStartTopRight = mapToScene(right, 0);
	m_resizeStartBottomLeft = mapToScene(0, bottom);
	m_resizeStartBottomRight = mapToScene(right, bottom);

	m_corner = findCorner(event->scenePos(), event->modifiers());
	switch (m_corner) {
		case ResizableBoard::NO_CORNER:
			Board::mousePressEvent(event);
			return;
        default:
                break;
	}

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView) {
		setInitialSize();
		infoGraphicsView->viewItemInfo(this);
	}
}

void ResizableBoard::mouseMoveEvent(QGraphicsSceneMouseEvent * event) {
	if (m_corner == ResizableBoard::NO_CORNER) {
		Board::mouseMoveEvent(event);
		return;
	}

	QPointF zero = mapToScene(0, 0);
	QPointF ds = mapFromScene(zero + event->scenePos() - m_resizeMousePos);
	QPointF newPos;
	QSizeF size = m_resizeStartSize;

	switch (m_corner) {
		case ResizableBoard::BOTTOM_RIGHT:
			size.setWidth(size.width() + ds.x());
			size.setHeight(size.height() + ds.y());
			break;
		case ResizableBoard::TOP_RIGHT:
			size.setWidth(size.width() + ds.x());
			size.setHeight(size.height() - ds.y());
			break;
		case ResizableBoard::BOTTOM_LEFT:
			size.setWidth(size.width() - ds.x());
			size.setHeight(size.height() + ds.y());
			break;
		case ResizableBoard::TOP_LEFT:
			size.setWidth(size.width() - ds.x());
			size.setHeight(size.height() - ds.y());
			break;
        default:
            break;
	}

	if (size.width() < minWidth()) {
        DebugDialog::debug("to min width");
        size.setWidth(minWidth());
    }
	if (size.height() < minHeight()) {
        DebugDialog::debug("to min height");
        size.setHeight(minHeight());
    }

	if (m_keepAspectRatio) {
		double cw = size.height() * m_aspectRatio.width() / m_aspectRatio.height();
		double ch = size.width() * m_aspectRatio.height() / m_aspectRatio.width();
		if (ch < minHeight()) {
            DebugDialog::debug("from min height");
			size.setWidth(cw);
		}
		else if (cw < minWidth()) {
            DebugDialog::debug("from min width");
			size.setHeight(ch);
		}
		else {
			// figure out which one changes the area least
			double a1 = cw * size.height();
			double a2 = ch * size.width();
			double ac = m_size.width() * m_size.height();
			if (qAbs(ac - a1) <= qAbs(ac - a2)) {
				size.setWidth(cw);
			}
			else {
				size.setHeight(ch);
			}
		}
	}

	bool changePos = (m_corner != ResizableBoard::BOTTOM_RIGHT);
	bool changeTransform = !this->transform().isIdentity();

	LayerHash lh;
	QSizeF oldSize = m_size;
	resizePixels(size.width(), size.height(), lh);

	if (changePos) {
		if (changeTransform) {
			QTransform oldT = transform();

			DebugDialog::debug(QString("t old m:%1 p:%2,%3 sz:%4,%5")
                .arg(TextUtils::svgMatrix(oldT))
				.arg(pos().x()).arg(pos().y())
				.arg(oldSize.width()).arg(oldSize.height()));

			double sw = size.width() / 2;
			double sh = size.height() / 2;	
			QMatrix m(oldT.m11(), oldT.m12(), oldT.m21(), oldT.m22(), 0, 0);
			ds = m.inverted().map(ds);
			QTransform newT = QTransform().translate(-sw, -sh) * QTransform(m) * QTransform().translate(sw, sh);

			QList<ItemBase *> kin;
			kin << this->layerKinChief();
			foreach (ItemBase * lk, this->layerKinChief()->layerKin()) {
				kin << lk;
			}
			foreach (ItemBase * itemBase, kin) {
				itemBase->getViewGeometry().setTransform(newT);
				itemBase->setTransform(newT);
			}
			
			QTransform t = transform();
			DebugDialog::debug(QString("t new m:%1 p:%2,%3 sz:%4,%5")
				.arg(TextUtils::svgMatrix(t))
				.arg(pos().x()).arg(pos().y())
				.arg(size.width()).arg(size.height()));
		}
	
		QPointF actual;
		QPointF desired;
		switch (m_corner) {
			case ResizableBoard::TOP_RIGHT:
				actual = mapToScene(0, size.height());
				desired = m_resizeStartBottomLeft;
				break;
			case ResizableBoard::BOTTOM_LEFT:
				actual = mapToScene(size.width(), 0);
				desired = m_resizeStartTopRight;
				break;
			case ResizableBoard::TOP_LEFT:
				actual = mapToScene(size.width(), size.height());
				desired = m_resizeStartBottomRight;
				break;
                        default:
                                break;
		}	

		setPos(pos() + desired - actual);
	}
}

void ResizableBoard::mouseReleaseEvent(QGraphicsSceneMouseEvent * event) {
	if (m_corner == ResizableBoard::NO_CORNER) {
		Board::mouseReleaseEvent(event);
		return;
	}

	event->accept();
	m_corner = ResizableBoard::NO_CORNER;
	setKinCursor(Qt::ArrowCursor);

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView) {
		infoGraphicsView->viewItemInfo(this);
	}
}

ViewLayer::ViewID ResizableBoard::theViewID() {
	return ViewLayer::PCBView;
}

void ResizableBoard::resizePixels(double w, double h, const LayerHash & viewLayers) {
	resizeMM(GraphicsUtils::pixels2mm(w, GraphicsUtils::SVGDPI), GraphicsUtils::pixels2mm(h, GraphicsUtils::SVGDPI), viewLayers);
}

bool ResizableBoard::resizeMM(double mmW, double mmH, const LayerHash & viewLayers) {
	if (mmW == 0 || mmH == 0) {
        LayerAttributes layerAttributes;
        this->initLayerAttributes(layerAttributes, m_viewID, m_viewLayerID, m_viewLayerPlacement, true, true);
		setUpImage(modelPart(), viewLayers, layerAttributes);
		modelPart()->setLocalProp("height", QVariant());
		modelPart()->setLocalProp("width", QVariant());
		// do the layerkin
		return false;
	}

	QRectF r = this->boundingRect();
	if (qAbs(GraphicsUtils::pixels2mm(r.width(), GraphicsUtils::SVGDPI) - mmW) < .001 &&
		qAbs(GraphicsUtils::pixels2mm(r.height(), GraphicsUtils::SVGDPI) - mmH) < .001) 
	{
		return false;
	}

	resizeMMAux(mmW, mmH);
    return true;
}


void ResizableBoard::resizeMMAux(double mmW, double mmH)
{
	double milsW = GraphicsUtils::mm2mils(mmW);
	double milsH = GraphicsUtils::mm2mils(mmH);

	QString s = makeFirstLayerSvg(mmW, mmH, milsW, milsH);

    bool result = resetRenderer(s);
    if (result) {

		modelPart()->setLocalProp("width", mmW);
		modelPart()->setLocalProp("height", mmH);

		double tens = pow(10.0, m_decimalsAfter);
		setWidthAndHeight(qRound(mmW * tens) / tens, qRound(mmH * tens) / tens);
	}
	//	DebugDialog::debug(QString("fast load result %1 %2").arg(result).arg(s));

	foreach (ItemBase * itemBase, m_layerKin) {
		QString s = makeNextLayerSvg(itemBase->viewLayerID(), mmW, mmH, milsW, milsH);
		if (!s.isEmpty()) {
            result = itemBase->resetRenderer(s);
            if (result) {
				itemBase->modelPart()->setLocalProp("width", mmW);
				itemBase->modelPart()->setLocalProp("height", mmH);
			}
		}
	}

}

void ResizableBoard::loadLayerKin( const LayerHash & viewLayers, ViewLayer::ViewLayerPlacement viewLayerPlacement) {

	loadTemplates();				
	Board::loadLayerKin(viewLayers, viewLayerPlacement);
	double w =  m_modelPart->localProp("width").toDouble();
	if (w != 0) {
		resizeMM(w, m_modelPart->localProp("height").toDouble(), viewLayers);
	}
}

void ResizableBoard::setInitialSize() {
	double w =  m_modelPart->localProp("width").toDouble();
	if (w == 0) {
		// set the size so the infoGraphicsView will display the size as you drag
		QSizeF sz = this->boundingRect().size();
		modelPart()->setLocalProp("width", GraphicsUtils::pixels2mm(sz.width(), GraphicsUtils::SVGDPI)); 
		modelPart()->setLocalProp("height", GraphicsUtils::pixels2mm(sz.height(), GraphicsUtils::SVGDPI)); 
	}
}

QString ResizableBoard::retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor)
{
	double w = m_modelPart->localProp("width").toDouble();
	if (w != 0) {
		double h = m_modelPart->localProp("height").toDouble();
		QString xml = makeLayerSvg(viewLayerID, w, h, GraphicsUtils::mm2mils(w), GraphicsUtils::mm2mils(h));
		if (!xml.isEmpty()) {
			return PaletteItemBase::normalizeSvg(xml, viewLayerID, blackOnly, dpi, factor);
		}
	}

	return Board::retrieveSvg(viewLayerID, svgHash, blackOnly, dpi, factor);
}

QSizeF ResizableBoard::getSizeMM() {
	double w = m_modelPart->localProp("width").toDouble();
	double h = m_modelPart->localProp("height").toDouble();
	return QSizeF(w, h);
}

QString ResizableBoard::makeLayerSvg(ViewLayer::ViewLayerID viewLayerID, double mmW, double mmH, double milsW, double milsH) 
{
	switch (viewLayerID) {
		case ViewLayer::Board:
			return makeBoardSvg(mmW, mmH, milsW, milsH);
		case ViewLayer::Silkscreen1:
		case ViewLayer::Silkscreen0:
			return makeSilkscreenSvg(viewLayerID, mmW, mmH, milsW, milsH);
			break;
		default:
			return "";
	}
}

QString ResizableBoard::makeNextLayerSvg(ViewLayer::ViewLayerID viewLayerID, double mmW, double mmH, double milsW, double milsH) {

	if (viewLayerID == ViewLayer::Silkscreen1) return makeSilkscreenSvg(viewLayerID, mmW, mmH, milsW, milsH);
	if (viewLayerID == ViewLayer::Silkscreen0) return makeSilkscreenSvg(viewLayerID, mmW, mmH, milsW, milsH);

	return "";
}

QString ResizableBoard::makeFirstLayerSvg(double mmW, double mmH, double milsW, double milsH) {
	return makeBoardSvg(mmW, mmH, milsW, milsH);
}

QString ResizableBoard::makeBoardSvg(double mmW, double mmH, double milsW, double milsH) {
    Q_UNUSED(milsW);
    Q_UNUSED(milsH);
    return makeSvg(mmW, mmH, BoardLayerTemplates.value(moduleID()));
}

QString ResizableBoard::makeSilkscreenSvg(ViewLayer::ViewLayerID viewLayerID, double mmW, double mmH, double milsW, double milsH) {
    Q_UNUSED(milsW);
    Q_UNUSED(milsH);
    if (viewLayerID == ViewLayer::Silkscreen0) return makeSvg(mmW, mmH, Silkscreen0LayerTemplates.value(moduleID()));
    return makeSvg(mmW, mmH, SilkscreenLayerTemplates.value(moduleID()));
}

QString ResizableBoard::makeSvg(double mmW, double mmH, const QString & layerTemplate)
{
    if (layerTemplate.isEmpty()) return "";

    QDomDocument doc;
    if (!doc.setContent(layerTemplate)) return "";

    QDomElement root = doc.documentElement();
    static const QString mmString("%1mm");
    root.setAttribute("width", mmString.arg(mmW));
    root.setAttribute("height", mmString.arg(mmH));
    root.setAttribute("viewBox", QString("0 0 %1 %2").arg(mmW).arg(mmH));
    QList<QDomElement> leaves;
    TextUtils::collectLeaves(root, leaves);
    if (leaves.count() > 1) return "";

    QDomElement leaf = leaves.at(0);

    bool ok;
    double strokeWidth = leaf.attribute("stroke-width").toDouble(&ok);
    if (!ok) return "";

    if (layerTemplate.contains("<ellipse")) {
        leaf.setAttribute("cx", QString::number(mmW / 2));
        leaf.setAttribute("cy", QString::number(mmH / 2));
        leaf.setAttribute("rx", QString::number((mmW - strokeWidth) / 2));
        leaf.setAttribute("ry", QString::number((mmH - strokeWidth) / 2));
    }
    else if (layerTemplate.contains("<rect")) {
        leaf.setAttribute("width", QString::number(mmW - strokeWidth));
        leaf.setAttribute("height", QString::number(mmH - strokeWidth));
    }
    
    return doc.toString();
}

void ResizableBoard::saveParams() {
	double w = modelPart()->localProp("width").toDouble();
	double h = modelPart()->localProp("height").toDouble();
	m_boardSize = QSizeF(w, h);
	m_boardPos = pos();
}

void ResizableBoard::getParams(QPointF & p, QSizeF & s) {
	p = m_boardPos;
	s = m_boardSize;
}

bool ResizableBoard::hasCustomSVG() {
	return theViewID() == m_viewID;
}

bool ResizableBoard::collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide)
{
	bool result = Board::collectExtraInfo(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget, hide);

	if (prop.compare("shape", Qt::CaseInsensitive) == 0) {

		returnProp = tr("shape");

		if (!m_modelPart->localProp("height").isValid()) { 
			// display uneditable width and height
			QFrame * frame = new QFrame();
			frame->setObjectName("infoViewPartFrame");		

			QVBoxLayout * vboxLayout = new QVBoxLayout();
			vboxLayout->setAlignment(Qt::AlignLeft);
			vboxLayout->setSpacing(0);
			vboxLayout->setMargin(0);
			vboxLayout->setContentsMargins(0, 3, 0, 0);

			double tens = pow(10.0, m_decimalsAfter);
			QRectF r = this->boundingRect();
			double w = qRound(GraphicsUtils::pixels2mm(r.width(), GraphicsUtils::SVGDPI) * tens) / tens;
			QLabel * l1 = new QLabel(tr("width: %1mm").arg(w));	
			l1->setMargin(0);
			l1->setObjectName("infoViewLabel");		

			double h = qRound(GraphicsUtils::pixels2mm(r.height(), GraphicsUtils::SVGDPI) * tens) / tens;
			QLabel * l2 = new QLabel(tr("height: %1mm").arg(h));
			l2->setMargin(0);
			l2->setObjectName("infoViewLabel");		

			if (returnWidget) vboxLayout->addWidget(qobject_cast<QWidget *>(returnWidget));
			vboxLayout->addWidget(l1);
			vboxLayout->addWidget(l2);

			frame->setLayout(vboxLayout);

			returnValue = l1->text() + "," + l2->text();
			returnWidget = frame;
			return true;
		}

		returnWidget = setUpDimEntry(false, false, false, returnWidget);
        returnWidget->setEnabled(swappingEnabled);
		return true;
	}

	return result;
}

QStringList ResizableBoard::collectValues(const QString & family, const QString & prop, QString & value) {
	return Board::collectValues(family, prop, value);
}

void ResizableBoard::paperSizeChanged(int index) {
    QComboBox * comboBox = qobject_cast<QComboBox *>(sender());
    if (comboBox == NULL) return;

    QModelIndex modelIndex = comboBox->model()->index(index,0);  
    QSizeF size = comboBox->model()->data(modelIndex, Qt::UserRole).toSizeF();
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->resizeBoard(size.width(), size.height(), true);
	}
}

void ResizableBoard::widthEntry() {
	QLineEdit * edit = qobject_cast<QLineEdit *>(sender());
	if (edit == NULL) return;

	double w = edit->text().toDouble();
	double oldW = m_modelPart->localProp("width").toDouble();
	if (w == oldW) return;

	double h =  m_modelPart->localProp("height").toDouble();

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->resizeBoard(w, h, true);
	}
}

void ResizableBoard::heightEntry() {
	QLineEdit * edit = qobject_cast<QLineEdit *>(sender());
	if (edit == NULL) return;

	double h = edit->text().toDouble();
	double oldH =  m_modelPart->localProp("height").toDouble();
	if (h == oldH) return;

	double w =  m_modelPart->localProp("width").toDouble();

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->resizeBoard(w, h, true);
	}
}

bool ResizableBoard::hasPartNumberProperty()
{
	return false;
}

void ResizableBoard::paintSelected(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if (m_hidden || m_layerHidden) return;

	Board::paintSelected(painter, option, widget);

	// http://www.gamedev.net/topic/441695-transform-matrix-decomposition/
	double m11 = painter->worldTransform().m11();
	double m12 = painter->worldTransform().m12();
	double scale = m_currentScale = qSqrt((m11 * m11) + (m12 * m12));   // assumes same scaling for both x and y

	double scalefull = CornerHandleSize / scale;
	double scalehalf = scalefull / 2;
	double bottom = m_size.height();
	double right = m_size.width();
	
	QPen pen;
	pen.setWidthF(1.0 / scale);
	pen.setColor(QColor(0, 0, 0));
	QBrush brush(QColor(255, 255, 255));
	painter->setPen(pen);
	painter->setBrush(brush);

	QPolygonF poly;

	// upper left
	poly.append(QPointF(0, 0));
	poly.append(QPointF(0, scalefull));
	poly.append(QPointF(scalehalf, scalefull));
	poly.append(QPointF(scalehalf, scalehalf));
	poly.append(QPointF(scalefull, scalehalf));
	poly.append(QPointF(scalefull, 0));
	painter->drawPolygon(poly);

	// upper right
	poly.clear();
	poly.append(QPointF(right, 0));
	poly.append(QPointF(right, scalefull));
	poly.append(QPointF(right - scalehalf, scalefull));
	poly.append(QPointF(right - scalehalf, scalehalf));
	poly.append(QPointF(right - scalefull, scalehalf));
	poly.append(QPointF(right - scalefull, 0));
	painter->drawPolygon(poly);

	// lower left
	poly.clear();
	poly.append(QPointF(0, bottom - scalefull));
	poly.append(QPointF(0, bottom));
	poly.append(QPointF(scalefull, bottom));
	poly.append(QPointF(scalefull, bottom - scalehalf));
	poly.append(QPointF(scalehalf, bottom - scalehalf));
	poly.append(QPointF(scalehalf, bottom - scalefull));
	painter->drawPolygon(poly);

	// lower right
	poly.clear();
	poly.append(QPointF(right, bottom - scalefull));
	poly.append(QPointF(right, bottom));
	poly.append(QPointF(right - scalefull, bottom));
	poly.append(QPointF(right - scalefull, bottom - scalehalf));
	poly.append(QPointF(right - scalehalf, bottom - scalehalf));
	poly.append(QPointF(right - scalehalf, bottom - scalefull));
	painter->drawPolygon(poly);
}

bool ResizableBoard::inResize() {
	return m_corner != ResizableBoard::NO_CORNER;
}

void ResizableBoard::figureHover() {
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(ALLMOUSEBUTTONS);
	foreach(ItemBase * lkpi, m_layerKin) {
		lkpi->setAcceptHoverEvents(false);
		lkpi->setAcceptedMouseButtons(Qt::NoButton);
	}
}

void ResizableBoard::hoverEnterEvent( QGraphicsSceneHoverEvent * event ) {
	Board::hoverEnterEvent(event);
}

void ResizableBoard::hoverMoveEvent( QGraphicsSceneHoverEvent * event ) {
	Board::hoverMoveEvent(event);

	m_corner = findCorner(event->scenePos(), event->modifiers());
	QCursor cursor;
	switch (m_corner) {
		case ResizableBoard::BOTTOM_RIGHT:
		case ResizableBoard::TOP_LEFT:
		case ResizableBoard::TOP_RIGHT:
		case ResizableBoard::BOTTOM_LEFT:
			//DebugDialog::debug("setting scale cursor");
			cursor = *CursorMaster::ScaleCursor;
			break;
		default:
			//DebugDialog::debug("setting other cursor");
			cursor = Qt::ArrowCursor;
			break;
	}
	setKinCursor(cursor);

}

void ResizableBoard::hoverLeaveEvent( QGraphicsSceneHoverEvent * event ) {
	setKinCursor(Qt::ArrowCursor);
	//DebugDialog::debug("setting arrow cursor");		
	Board::hoverLeaveEvent(event);
}

ResizableBoard::Corner ResizableBoard::findCorner(QPointF scenePos, Qt::KeyboardModifiers modifiers) {
	Q_UNUSED(modifiers);
		
	if (!this->isSelected()) return ResizableBoard::NO_CORNER;
    if (this->moveLock()) return ResizableBoard::NO_CORNER;

	double d = CornerHandleSize / m_currentScale;
	double d2 = d * d;
	double right = m_size.width();
	double bottom = m_size.height();
	//DebugDialog::debug(QString("size %1 %2").arg(right).arg(bottom));
	QPointF q = mapToScene(right, bottom);
	if (GraphicsUtils::distanceSqd(scenePos, q) <= d2) {
		return ResizableBoard::BOTTOM_RIGHT;
	}
	q = mapToScene(0, 0);
	if (GraphicsUtils::distanceSqd(scenePos, q) <= d2) {
		return ResizableBoard::TOP_LEFT;
	}
	q = mapToScene(right, 0);
	if (GraphicsUtils::distanceSqd(scenePos, q) <= d2) {
		return ResizableBoard::TOP_RIGHT;
	}
	q = mapToScene(0, bottom);
	if (GraphicsUtils::distanceSqd(scenePos, q) <= d2) {
		return ResizableBoard::BOTTOM_LEFT;
	}

	return ResizableBoard::NO_CORNER;
}

void ResizableBoard::setKinCursor(QCursor & cursor) {
	ItemBase * chief = this->layerKinChief();
	chief->setCursor(cursor);
	foreach (ItemBase * itemBase, chief->layerKin()) {
		itemBase->setCursor(cursor);
	}
}

void ResizableBoard::setKinCursor(Qt::CursorShape cursor) {
	ItemBase * chief = this->layerKinChief();
	chief->setCursor(cursor);
	foreach (ItemBase * itemBase, chief->layerKin()) {
		itemBase->setCursor(cursor);
	}
}

QFrame * ResizableBoard::setUpDimEntry(bool includeAspectRatio, bool includeRevert, bool includePaperSizes, QWidget * & returnWidget)
{
	double tens = pow(10.0, m_decimalsAfter);
	double w = qRound(m_modelPart->localProp("width").toDouble() * tens) / tens;	// truncate to 1 decimal point
	double h = qRound(m_modelPart->localProp("height").toDouble() * tens) / tens;  // truncate to 1 decimal point

	QFrame * frame = new QFrame();
	frame->setObjectName("infoViewPartFrame");
	QVBoxLayout * vboxLayout = new QVBoxLayout();
	vboxLayout->setAlignment(Qt::AlignLeft);
	vboxLayout->setSpacing(1);
	vboxLayout->setContentsMargins(0, 3, 0, 0);

	QFrame * subframe1 = new QFrame();
	QHBoxLayout * hboxLayout1 = new QHBoxLayout();
	hboxLayout1->setAlignment(Qt::AlignLeft);
	hboxLayout1->setContentsMargins(0, 0, 0, 0);
	hboxLayout1->setSpacing(2);

	QFrame * subframe2 = new QFrame();
	QHBoxLayout * hboxLayout2 = new QHBoxLayout();
	hboxLayout2->setAlignment(Qt::AlignLeft);
	hboxLayout2->setContentsMargins(0, 0, 0, 0);
	hboxLayout2->setSpacing(2);

	QLabel * l1 = new QLabel(tr("width(mm)"));	
	l1->setMargin(0);
	l1->setObjectName("infoViewLabel");
	QLineEdit * e1 = new QLineEdit();
	QDoubleValidator * validator = new QDoubleValidator(e1);
	validator->setRange(0.1, 999.9, m_decimalsAfter);
	validator->setNotation(QDoubleValidator::StandardNotation);
    validator->setLocale(QLocale::C);
	e1->setObjectName("infoViewLineEdit");
	e1->setValidator(validator);
	e1->setMaxLength(4 + m_decimalsAfter);
	e1->setText(QString::number(w));

	QLabel * l2 = new QLabel(tr("height(mm)"));
	l2->setMargin(0);
	l2->setObjectName("infoViewLabel");
	QLineEdit * e2 = new QLineEdit();
	validator = new QDoubleValidator(e1);
	validator->setRange(0.1, 999.9, m_decimalsAfter);
	validator->setNotation(QDoubleValidator::StandardNotation);
    validator->setLocale(QLocale::C);
	e2->setObjectName("infoViewLineEdit");
	e2->setValidator(validator);
	e2->setMaxLength(4 + m_decimalsAfter);
	e2->setText(QString::number(h));

	hboxLayout1->addWidget(l1);
	hboxLayout1->addWidget(e1);
	hboxLayout2->addWidget(l2);
	hboxLayout2->addWidget(e2);

	subframe1->setLayout(hboxLayout1);
	subframe2->setLayout(hboxLayout2);
	if (returnWidget != NULL) vboxLayout->addWidget(returnWidget);

	connect(e1, SIGNAL(editingFinished()), this, SLOT(widthEntry()));
	connect(e2, SIGNAL(editingFinished()), this, SLOT(heightEntry()));

	m_widthEditor = e1;
	m_heightEditor = e2;

	vboxLayout->addWidget(subframe1);
	vboxLayout->addWidget(subframe2);

	if (includeAspectRatio || includeRevert || includePaperSizes) {
		QFrame * subframe3 = new QFrame();
		QHBoxLayout * hboxLayout3 = new QHBoxLayout();
		hboxLayout3->setAlignment(Qt::AlignLeft);
		hboxLayout3->setContentsMargins(0, 0, 0, 0);
		hboxLayout3->setSpacing(0);
	
		if (includeAspectRatio) {
			QLabel * l3 = new QLabel(tr("keep aspect ratio"));
			l3->setMargin(0);
			l3->setObjectName("infoViewLabel");
			QCheckBox * checkBox = new QCheckBox();
			checkBox->setChecked(m_keepAspectRatio);
			checkBox->setObjectName("infoViewCheckBox");

			hboxLayout3->addWidget(l3);
			hboxLayout3->addWidget(checkBox);
			connect(checkBox, SIGNAL(toggled(bool)), this, SLOT(keepAspectRatio(bool)));
			m_aspectRatioCheck = checkBox;
			m_aspectRatioLabel = l3;
	    }
	    if (includeRevert) {
			QPushButton * pb = new QPushButton(tr("Revert"));
			pb->setObjectName("infoViewButton");
			hboxLayout3->addWidget(pb);
			connect(pb, SIGNAL(clicked(bool)), this, SLOT(revertSize(bool)));
			double w = modelPart()->localProp("width").toDouble();
			double ow = modelPart()->localProp("originalWidth").toDouble();
			double h = modelPart()->localProp("height").toDouble();
			double oh = modelPart()->localProp("originalHeight").toDouble();
			pb->setEnabled(qAbs(w - ow) > JND || qAbs(h - oh) > JND);
			m_revertButton = pb;
	    }
        if (includePaperSizes) {
            initPaperSizes();

			QLabel * l3 = new QLabel(tr("size"));
			l3->setMargin(0);
			l3->setObjectName("infoViewLabel");

            m_paperSizeComboBox = new QComboBox();
	        m_paperSizeComboBox->setObjectName("infoViewComboBox");
	        m_paperSizeComboBox->setEditable(false);
            m_paperSizeComboBox->setMinimumWidth(150);
            m_paperSizeComboBox->addItem(tr("custom"));
            for (int i = 0; i < PaperSizeNames.count(); i++) {
                QSizeF dim = PaperSizeDimensions.at(i);
                m_paperSizeComboBox->addItem(PaperSizeNames.at(i), dim);
            }

            QModelIndex modelIndex = m_paperSizeComboBox->model()->index(0,0);  
            m_paperSizeComboBox->model()->setData(modelIndex, 0, Qt::UserRole - 1);           // to make it selectable again use Qt::ItemIsSelectable | Qt::ItemIsEnabled)

            updatePaperSizes(w, h);
            connect(m_paperSizeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(paperSizeChanged(int)));

            hboxLayout3->addWidget(l3);
            hboxLayout3->addWidget(m_paperSizeComboBox);

        }

		subframe3->setLayout(hboxLayout3);

		vboxLayout->addWidget(subframe3);
	}

	frame->setLayout(vboxLayout);

	return frame;
}

void ResizableBoard::fixWH() {
	bool okw, okh;
	QString wstr = m_modelPart->localProp("width").toString();
	QString hstr = m_modelPart->localProp("height").toString();
	double w = wstr.toDouble(&okw);
	double h = hstr.toDouble(&okh);

	//DebugDialog::debug(QString("w:%1 %2 ok:%3 h:%4 %5 ok:%6")
					//.arg(wstr).arg(w).arg(okw)
					//.arg(hstr).arg(h).arg(okh));

	if ((!okw && !wstr.isEmpty()) || qIsNaN(w) || qIsInf(w) || (!okh && !hstr.isEmpty()) || qIsNaN(h) || qIsInf(h)) {
		DebugDialog::debug("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
		DebugDialog::debug("bad width or height in ResizableBoard or subclass " + wstr + " " + hstr);
		DebugDialog::debug("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
		m_modelPart->setLocalProp("width", "");
		m_modelPart->setLocalProp("height", "");
	}
}

void ResizableBoard::setWidthAndHeight(double w, double h)
{
	if (m_widthEditor) {
		m_widthEditor->setText(QString::number(w));
	}
	if (m_heightEditor) {
		m_heightEditor->setText(QString::number(h));
	}
    updatePaperSizes(w, h);
}

QString ResizableBoard::getShapeForRenderer(const QString & svg, ViewLayer::ViewLayerID viewLayerID) 
{
    QString xmlName = ViewLayer::viewLayerXmlNameFromID(viewLayerID);
	SvgFileSplitter splitter;
    QString xml = svg;
	bool result = splitter.splitString(xml, xmlName);
	if (result) {
        xml = splitter.elementString(xmlName);
    }
    else {
		xml = "";
	}

    QString header("<?xml version='1.0' encoding='UTF-8'?>\n"
                    "<svg ");
    QDomNamedNodeMap map = splitter.domDocument().documentElement().attributes();
    for (int i = 0; i < map.count(); i++) {
        QDomNode node = map.item(i);
        header += node.nodeName() + "='" + node.nodeValue() + "' ";
    }
    header += ">\n";

    header = header + xml + "\n</svg>";
    //DebugDialog::debug("resizableBoard " + header);
	return header;
}

void ResizableBoard::keepAspectRatio(bool checkState) {
	m_keepAspectRatio = checkState;
}

void ResizableBoard::revertSize(bool) {
    double ow = modelPart()->localProp("originalWidth").toDouble();
    double oh = modelPart()->localProp("originalHeight").toDouble();

    InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->resizeBoard(ow, oh, true);
        m_revertButton->setEnabled(false);
	}
}

void ResizableBoard::updatePaperSizes(double w, double h) {
    if (m_paperSizeComboBox ==  NULL) return;

    int currentIndex = 0;
    for (int i = 0; i < PaperSizeNames.count(); i++) {
        QSizeF dim = PaperSizeDimensions.at(i);
        if (qAbs(w - dim.width()) < 0.1 && qAbs(h - dim.height()) < 0.1) {
            currentIndex = i + 1;
        }
    }

    QString custom = tr("custom");
    if (currentIndex == 0) {
        custom =  QString("%1x%2").arg(w).arg(h);
    }
    m_paperSizeComboBox->setItemText(0, custom);
    m_paperSizeComboBox->setCurrentIndex(currentIndex);
}

void ResizableBoard::initPaperSizes() {
    if (PaperSizeNames.count() == 0) {
        PaperSizeNames << tr("A0 (1030x1456)") << tr("A1 (728x1030)") << tr("A2 (515x728)") << tr("A3 (364x515)") << tr("A4 (257x364)") << tr("A5 (182x257)") << tr("A6 (128x182)") 
            << tr("Letter (8.5x11)") << tr("Legal (8.5x14)") << tr("Ledger (17x11)") << tr("Tabloid (11x17)");
        PaperSizeDimensions << QSizeF(1030,1456) << QSizeF(728,1030) << QSizeF(515,728) << QSizeF(364,515) << QSizeF(257,364) << QSizeF(182,257) << QSizeF(128,182) 
            << QSizeF(215.9,279.4) << QSizeF(215.9,355.6) << QSizeF(432,279) << QSizeF(279,432);
    }
}
