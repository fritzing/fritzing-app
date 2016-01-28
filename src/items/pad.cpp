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

// TODO:
//		flip for one-sided board

#include "pad.h"

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

static QString PadTemplate = "";
static double OriginalWidth = 28;
static double OriginalHeight = 32;
static double TheOffset = 4;

Pad::Pad( ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: ResizableBoard(modelPart, viewID, viewGeometry, id, itemMenu, doLabel)
{
	m_decimalsAfter = 2;
    m_copperBlocker = false;
}

Pad::~Pad() {

}

QString Pad::makeLayerSvg(ViewLayer::ViewLayerID viewLayerID, double mmW, double mmH, double milsW, double milsH) 
{
	Q_UNUSED(milsW);
	Q_UNUSED(milsH);

 	switch (viewLayerID) {
		case ViewLayer::Copper0:
		case ViewLayer::Copper1:
			break;
		default:
			return "";
	}

	double wpx = mmW > 0 ? GraphicsUtils::mm2pixels(mmW) : OriginalWidth;
	double hpx = mmH > 0 ? GraphicsUtils::mm2pixels(mmH) : OriginalHeight;

	QString connectAt = m_modelPart->localProp("connect").toString();
	QRectF terminal;
	double minW = qMin((double) 1.0, wpx / 3);
	double minH = qMin((double) 1.0, hpx / 3);
	if (connectAt.compare("center", Qt::CaseInsensitive) == 0) {
		terminal.setRect(2, 2, wpx, hpx);
	}
	else if (connectAt.compare("north", Qt::CaseInsensitive) == 0) {
		terminal.setRect(2, 2, wpx, minH);
	}
	else if (connectAt.compare("south", Qt::CaseInsensitive) == 0) {
		terminal.setRect(2, 2 + hpx - minH, wpx, minH);
	}
	else if (connectAt.compare("east", Qt::CaseInsensitive) == 0) {
		terminal.setRect(2 + wpx - minW, 2, minW, hpx);
	}
	else if (connectAt.compare("west", Qt::CaseInsensitive) == 0) {
		terminal.setRect(2, 2, minW, hpx);
	}

    QString blockerColor = (viewLayerID == ViewLayer::Copper0) ? "#A26A00" : "#aF8B33";
    QString copperColor = (viewLayerID == ViewLayer::Copper0) ? ViewLayer::Copper0Color : ViewLayer::Copper1Color;
	QString svg = QString("<svg version='1.1' xmlns='http://www.w3.org/2000/svg'  x='0px' y='0px' width='%1px' height='%2px' viewBox='0 0 %1 %2'>\n"
							"<g id='%5'>\n"
							"<rect  id='%8pad' x='2' y='2' fill='%6' fill-opacity='%7' stroke='%9' stroke-width='%10' width='%3' height='%4'/>\n"
							)
					.arg(wpx + TheOffset)
					.arg(hpx + TheOffset)
					.arg(wpx)
					.arg(hpx)
					.arg(ViewLayer::viewLayerXmlNameFromID(viewLayerID))
                    .arg(copperBlocker() ? blockerColor : copperColor)
                    .arg(copperBlocker() ? 0.0 : 1.0)
                    .arg(copperBlocker() ? "zzz" : "connector0")
                    .arg(copperBlocker() ? blockerColor : "none")
                    .arg(copperBlocker() ? TheOffset : 0)
					;

    if (copperBlocker()) {
        svg += QString("<line linecap='butt' stroke='%5' stroke-width='1' x1='%1' y1='%2' x2='%3'  y2='%4'/>\n")
                    .arg(TheOffset)
                    .arg(TheOffset)
                    .arg(wpx)
                    .arg(hpx)
                    .arg(blockerColor)
                   ;
        svg += QString("<line linecap='butt' stroke='%5' stroke-width='1' x1='%1' y1='%2' x2='%3'  y2='%4'/>\n")
                    .arg(wpx)
                    .arg(TheOffset)
                    .arg(TheOffset)
                    .arg(hpx)
                    .arg(blockerColor)
                   ;

    }
    else {
        svg += QString("<rect  id='connector0terminal' x='%1' y='%2' fill='none' stroke='none' stroke-width='0' width='%3' height='%4'/>\n")
					.arg(terminal.left())
					.arg(terminal.top())
					.arg(terminal.width())
					.arg(terminal.height())
                    ;

    }

    svg += "</g>\n</svg>";

	//DebugDialog::debug("pad svg: " + svg);
	return svg;
}

QString Pad::makeNextLayerSvg(ViewLayer::ViewLayerID viewLayerID, double mmW, double mmH, double milsW, double milsH) 
{
	Q_UNUSED(mmW);
	Q_UNUSED(mmH);
	Q_UNUSED(milsW);
	Q_UNUSED(milsH);
	Q_UNUSED(viewLayerID);

	return "";
}

QString Pad::makeFirstLayerSvg(double mmW, double mmH, double milsW, double milsH) {
	return makeLayerSvg(moduleID().compare(ModuleIDNames::Copper0PadModuleIDName) == 0 ? ViewLayer::Copper0 : ViewLayer::Copper1, mmW, mmH, milsW, milsH);
}

ViewLayer::ViewID Pad::theViewID() {
	return ViewLayer::PCBView;
}

double Pad::minWidth() {
	return 1;
}

double Pad::minHeight() {
	return 1;
}

QString Pad::retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor)
{
	return ResizableBoard::retrieveSvg(viewLayerID, svgHash, blackOnly, dpi, factor);
}

QStringList Pad::collectValues(const QString & family, const QString & prop, QString & value) {
    QStringList values = ResizableBoard::collectValues(family, prop, value);

    QStringList newValues;
	if (prop.compare("layer", Qt::CaseInsensitive) == 0) {
        foreach (QString xmlName, values) {
            newValues << Board::convertFromXmlName(xmlName);
        }
        value = Board::convertFromXmlName(value);
        return newValues;
    }

    return values;
}

bool Pad::collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide) 
{
	if (prop.compare("shape", Qt::CaseInsensitive) == 0) {
		returnWidget = setUpDimEntry(false, false, false, returnWidget);
        returnWidget->setEnabled(swappingEnabled);
		returnProp = tr("shape");
		return true;
	}

    if (!copperBlocker()) {
	    if (prop.compare("connect to", Qt::CaseInsensitive) == 0) {
		    QComboBox * comboBox = new QComboBox();
		    comboBox->setObjectName("infoViewComboBox");
		    comboBox->setEditable(false);
		    comboBox->setEnabled(swappingEnabled);
		    comboBox->addItem(tr("center"), "center");
		    comboBox->addItem(tr("north"), "north");
		    comboBox->addItem(tr("east"), "east");
		    comboBox->addItem(tr("south"), "south");
		    comboBox->addItem(tr("west"), "west");
		    QString connectAt = m_modelPart->localProp("connect").toString();
		    for (int i = 0; i < comboBox->count(); i++) {
			    if (comboBox->itemData(i).toString().compare(connectAt) == 0) {
				    comboBox->setCurrentIndex(i);
				    break;
			    }
		    }

		    connect(comboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(terminalPointEntry(const QString &)));

		    returnWidget = comboBox;
		    returnProp = tr("connect to");
		    return true;
	    }
    }

	bool result = PaletteItem::collectExtraInfo(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget, hide);

    if (prop.compare("layer") == 0 && returnWidget != NULL) {
        bool disabled = true;
        InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
        if (infoGraphicsView && infoGraphicsView->boardLayers() == 2) disabled = false;
        returnWidget->setDisabled(disabled);
    }

    return result;
}

void Pad::setProp(const QString & prop, const QString & value) 
{	
	if (prop.compare("connect to", Qt::CaseInsensitive) == 0) {
		modelPart()->setLocalProp("connect", value);
		resizeMMAux(m_modelPart->localProp("width").toDouble(), m_modelPart->localProp("height").toDouble());
		return;
	}

	ResizableBoard::setProp(prop, value);
}

bool Pad::canEditPart() {
	return false;
}

bool Pad::hasPartLabel() {
	return true;
}

bool Pad::stickyEnabled() {
	return true;
}

ItemBase::PluralType Pad::isPlural() {
	return Plural;
}

bool Pad::rotationAllowed() {
	return true;
}

bool Pad::rotation45Allowed() {
	return true;
}

bool Pad::freeRotationAllowed(Qt::KeyboardModifiers modifiers) {
	if ((modifiers & altOrMetaModifier()) == 0) return false;
	if (!isSelected()) return false;

    return true;
}

bool Pad::freeRotationAllowed() {
    return true;
}

bool Pad::hasPartNumberProperty()
{
	return false;
}

void Pad::setInitialSize() {
	double w = m_modelPart->localProp("width").toDouble();
	if (w == 0) {
		// set the size so the infoGraphicsView will display the size as you drag
		modelPart()->setLocalProp("width", GraphicsUtils::pixels2mm(OriginalWidth, GraphicsUtils::SVGDPI)); 
		modelPart()->setLocalProp("height", GraphicsUtils::pixels2mm(OriginalHeight, GraphicsUtils::SVGDPI)); 
		modelPart()->setLocalProp("connect", "center"); 
	}
}

void Pad::resizeMMAux(double mmW, double mmH) {
	ResizableBoard::resizeMMAux(mmW, mmH);
	resetConnectors(NULL, NULL);
}

void Pad::addedToScene(bool temporary)
{
	if (this->scene()) {
		setInitialSize();
		resizeMMAux(m_modelPart->localProp("width").toDouble(), m_modelPart->localProp("height").toDouble());
	}

    return PaletteItem::addedToScene(temporary);
}

void Pad::terminalPointEntry(const QString &) {
	QComboBox * comboBox = qobject_cast<QComboBox *>(sender());
	if (comboBox == NULL) return;

	QString value = comboBox->itemData(comboBox->currentIndex()).toString();
	QString connectAt = m_modelPart->localProp("connect").toString();
	if (connectAt.compare(value) == 0) return;

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->setProp(this, "connect to", tr("connect to"), connectAt, value, true);
	}
}

ResizableBoard::Corner Pad::findCorner(QPointF scenePos, Qt::KeyboardModifiers modifiers) {
	//return ResizableBoard::NO_CORNER;
	ResizableBoard::Corner corner = ResizableBoard::findCorner(scenePos, modifiers);
	if (corner == ResizableBoard::NO_CORNER) return corner;

	if (modifiers & altOrMetaModifier()) {
		// free rotate
		setCursor(*CursorMaster::RotateCursor);
		return ResizableBoard::NO_CORNER;
	}

	return corner;
}

void Pad::paintHover(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	PaletteItem::paintHover(painter, option, widget);
}

bool Pad::copperBlocker() {
    return m_copperBlocker;
}

//////////////////////////////////////////

CopperBlocker::CopperBlocker( ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: Pad(modelPart, viewID, viewGeometry, id, itemMenu, doLabel)
{
    m_copperBlocker = true;
}

CopperBlocker::~CopperBlocker() {
}

bool CopperBlocker::hasPartLabel() {
	return false;
}

QPainterPath CopperBlocker::hoverShape() const
{
    if (m_viewID != ViewLayer::PCBView) {
        return PaletteItem::hoverShape();
     }

    QRectF r = boundingRect();

	QPainterPath path; 
    double half = TheOffset / 2;
    path.moveTo(half, half);
    path.lineTo(r.width() - half, half);
    path.lineTo(r.width() - half, r.height() - half);
    path.lineTo(half, r.height() - half);
    path.closeSubpath();
    path.moveTo(0, 0);
    path.lineTo(r.width(), r.height());
    path.moveTo(r.width(), 0);
    path.lineTo(0, r.height());
    path.moveTo(0, 0);

	QPen pen;
	pen.setCapStyle(Qt::RoundCap);
	return GraphicsUtils::shapeFromPath(path, pen, TheOffset, false);
}

QPainterPath CopperBlocker::shape() const
{
	return hoverShape();
}

QString CopperBlocker::makeFirstLayerSvg(double mmW, double mmH, double milsW, double milsH) {

	return makeLayerSvg(moduleID().compare(ModuleIDNames::Copper1BlockerModuleIDName) == 0 ? ViewLayer::Copper1 : ViewLayer::Copper0, mmW, mmH, milsW, milsH);
}