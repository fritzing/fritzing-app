/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2016 Fritzing

Fritzing is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.a

Fritzing is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************

$Revision: 6991 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-26 15:24:41 +0200 (Fr, 26. Apr 2013) $

********************************************************************/

#include "drc.h"
#include "../connectors/svgidlayer.h"
#include "../sketch/pcbsketchwidget.h"
#include "../debugdialog.h"
#include "../items/virtualwire.h"
#include "../items/tracewire.h"
#include "../items/via.h"
#include "../utils/graphicsutils.h"
#include "../utils/folderutils.h"
#include "../utils/textutils.h"
#include "../connectors/connectoritem.h"
#include "../items/moduleidnames.h"
#include "../processeventblocker.h"
#include "../fsvgrenderer.h"
#include "../viewlayer.h"
#include "../processeventblocker.h"

#include <qmath.h>
#include <QApplication>
#include <QMessageBox>
#include <QPixmap>
#include <QSet>
#include <QSettings>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QRadioButton>

///////////////////////////////////////////
//
//
//  deal with invisible connectors
//
//  if a part is already overlapping, leave it out of future checking?
//
//
///////////////////////////////////////////

const QString DRC::AlsoNet = "alsoNet";
const QString DRC::NotNet = "notnet";
const QString DRC::Net = "net";

const uchar DRC::BitTable[] = { 128, 64, 32, 16, 8, 4, 2, 1 }; 

bool pixelsCollide(QImage * image1, QImage * image2, QImage * image3, int x1, int y1, int x2, int y2, uint clr, QList<QPointF> & points) {
    bool result = false;
    const uchar * bits1 = image1->constScanLine(0);
    const uchar * bits2 = image2->constScanLine(0);
    int bytesPerLine = image1->bytesPerLine();
	for (int y = y1; y < y2; y++) {
        int offset = y * bytesPerLine;
		for (int x = x1; x < x2; x++) {
			//QRgb p1 = image1->pixel(x, y);
			//if (p1 == 0xffffffff) continue;

			//QRgb p2 = image2->pixel(x, y);
			//if (p2 == 0xffffffff) continue;

            int byteOffset = (x >> 3) + offset;
            uchar mask = DRC::BitTable[x & 7];

            if (*(bits1 + byteOffset) & mask) continue;
            if (*(bits2 + byteOffset) & mask) continue;

            image3->setPixel(x, y, clr);
			//DebugDialog::debug(QString("p1:%1 p2:%2").arg(p1, 0, 16).arg(p2, 0, 16));
			result = true;
            if (points.count() < 1000) {
                points.append(QPointF(x, y));
            }
		}
	}

	return result;
}

QStringList getNames(CollidingThing * collidingThing) {
    QStringList names;
    QList<ItemBase *> itemBases;
    if (collidingThing->nonConnectorItem) {
        itemBases << collidingThing->nonConnectorItem->attachedTo();
    }
    foreach (ItemBase * itemBase, itemBases) {
        if (itemBase) {
            itemBase = itemBase->layerKinChief();
            QString name = QObject::tr("Part %1 '%2'").arg(itemBase->title()) .arg(itemBase->instanceTitle());
            names << name;
        }
    }
    
    return names;
}

void allGs(QDomElement & element) {
    element.setTagName("g");
    QDomElement child = element.firstChildElement();
    while (!child.isNull()) {
        allGs(child);
        child = child.nextSiblingElement();
    }
}

///////////////////////////////////////////////

DRCResultsDialog::DRCResultsDialog(const QString & message, const QStringList & messages, const QList<CollidingThing *> & collidingThings, 
                                    QGraphicsPixmapItem * displayItem, QImage * displayImage, PCBSketchWidget * sketchWidget, QWidget *parent) 
                 : QDialog(parent) 
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    m_messages = messages;
    m_sketchWidget = sketchWidget;
    m_displayItem = displayItem;
    if (m_displayItem) {
        m_displayItem->setFlags(0);
    }
    m_displayImage = displayImage;
    m_collidingThings = collidingThings;

	this->setWindowTitle(tr("DRC Results"));

	QVBoxLayout * vLayout = new QVBoxLayout(this);

    QLabel * label = new QLabel(message);
    label->setWordWrap(true);
	vLayout->addWidget(label);

    label = new QLabel(tr("Click on an item in the list to highlight of overlap it refers to."));
    label->setWordWrap(true);
	vLayout->addWidget(label);

    label = new QLabel(tr("Note: the list items and the red highlighting will not update as you edit your sketch--you must rerun the DRC. The highlighting will disappear when you close this dialog."));
    label->setWordWrap(true);
	vLayout->addWidget(label);

    QListWidget *listWidget = new QListWidget();
    for (int ix = 0; ix < messages.count(); ix++) {
        QListWidgetItem * item = new QListWidgetItem(messages.at(ix));
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        item->setData(Qt::UserRole, ix);
        listWidget->addItem(item);
    }
    vLayout->addWidget(listWidget);
	connect(listWidget, SIGNAL(itemPressed(QListWidgetItem *)), this, SLOT(pressedSlot(QListWidgetItem *)));
	connect(listWidget, SIGNAL(itemClicked(QListWidgetItem *)), this, SLOT(releasedSlot(QListWidgetItem *)));

	QDialogButtonBox * buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(close()));

	vLayout->addWidget(buttonBox);
	this->setLayout(vLayout);
}

DRCResultsDialog::~DRCResultsDialog() {
    if (m_displayItem != NULL && m_sketchWidget != NULL) {
        delete m_displayItem;
    }
    if (m_displayImage != NULL) {
        delete m_displayImage;
    }
    foreach (CollidingThing * collidingThing, m_collidingThings) {
        delete collidingThing;
    }
    m_collidingThings.clear();
}

void DRCResultsDialog::pressedSlot(QListWidgetItem * item) {
    if (item == NULL) return;

    int ix = item->data(Qt::UserRole).toInt();
    CollidingThing * collidingThing = m_collidingThings.at(ix);
    foreach (QPointF p, collidingThing->atPixels) {
        m_displayImage->setPixel(p.x(), p.y(), 2 /* 0xffffff00 */);
    }
    QPixmap pixmap = QPixmap::fromImage(*m_displayImage);
    m_displayItem->setPixmap(pixmap);
    if (collidingThing->nonConnectorItem) {
        m_sketchWidget->selectItem(collidingThing->nonConnectorItem->attachedTo());
    }
}

void DRCResultsDialog::releasedSlot(QListWidgetItem * item) {
    if (item == NULL) return;

    int ix = item->data(Qt::UserRole).toInt();
    CollidingThing * collidingThing = m_collidingThings.at(ix);
    foreach (QPointF p, collidingThing->atPixels) {
        m_displayImage->setPixel(p.x(), p.y(), 1 /* 0x80ff0000 */);
    }
    QPixmap pixmap = QPixmap::fromImage(*m_displayImage);
    m_displayItem->setPixmap(pixmap);
}

///////////////////////////////////////////

const QString DRC::KeepoutSettingName("DRC_Keepout");
const double DRC::KeepoutDefaultMils = 10;  

///////////////////////////////////////////////

static QString CancelledMessage;

///////////////////////////////////////////

DRC::DRC(PCBSketchWidget * sketchWidget, ItemBase * board)
{
    CancelledMessage = tr("DRC was cancelled.");

    m_cancelled = false;
	m_sketchWidget = sketchWidget;
    m_board = board;
    m_displayItem = NULL;
    m_displayImage = NULL;
    m_plusImage = NULL;
    m_minusImage = NULL;
}

DRC::~DRC(void)
{
    if (m_displayItem) {
        delete m_displayItem;
    }
    if (m_plusImage) {
        delete m_plusImage;
    }
    if (m_minusImage) {
        delete m_minusImage;
    }
    if (m_displayImage) {
        delete m_displayImage;
    }
    foreach (QDomDocument * doc, m_masterDocs) {
        delete doc;
    }
}

QStringList DRC::start(bool showOkMessage, double keepoutMils) {
	QString message;
    QStringList messages;
    QList<CollidingThing *> collidingThings;

    bool result = startAux(message, messages, collidingThings, keepoutMils);
    if (result) {
        if (messages.count() == 0) {
            message = tr("Your sketch is ready for production: there are no connectors or traces that overlap or are too close together.");
        }
        else {
            message = tr("The areas on your board highlighted in red are connectors and traces which may overlap or be too close together. ") +
					 tr("Reposition them and run the DRC again to find more problems");
        }
    }

#ifndef QT_NO_DEBUG
    m_displayImage->save(FolderUtils::getTopLevelUserDataStorePath() + "/testDRCDisplay.png");
#endif

    emit wantBothVisible();
    emit setProgressValue(m_maxProgress);
    emit hideProgress();


    if (showOkMessage) {
        if (messages.count() == 0) {
            QMessageBox::information(m_sketchWidget->window(), tr("Fritzing"), message);
        }
        else {
            DRCResultsDialog * dialog = new DRCResultsDialog(message, messages, collidingThings, m_displayItem, m_displayImage, m_sketchWidget, m_sketchWidget->window());
            dialog->show();
        }
    }
    else {}

        m_displayItem = NULL;
        m_displayImage = NULL;

    return messages;
}

bool DRC::startAux(QString & message, QStringList & messages, QList<CollidingThing *> & collidingThings, double keepoutMils) {
    bool bothSidesNow = m_sketchWidget->boardLayers() == 2;

    QList<ConnectorItem *> visited;
    QList< QList<ConnectorItem *> > equis;
    QList< QList<ConnectorItem *> > singletons;
    foreach (QGraphicsItem * item, m_sketchWidget->scene()->items()) {
        ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(item);
        if (connectorItem == NULL) continue;
        if (!connectorItem->attachedTo()->isEverVisible()) continue;
        if (connectorItem->attachedTo()->getRatsnest()) continue;
        if (visited.contains(connectorItem)) continue;

        QList<ConnectorItem *> equi;
        equi.append(connectorItem);
        ConnectorItem::collectEqualPotential(equi, bothSidesNow, (ViewGeometry::RatsnestFlag | ViewGeometry::NormalFlag | ViewGeometry::PCBTraceFlag | ViewGeometry::SchematicTraceFlag) ^ m_sketchWidget->getTraceFlag());
        visited.append(equi);
        
        if (equi.count() == 1) {
            singletons.append(equi);
            continue;
        }

        ItemBase * firstPart = connectorItem->attachedTo()->layerKinChief();
        bool gotTwo = false;
        foreach (ConnectorItem * equ, equi) {
            if (equ->attachedTo()->layerKinChief() != firstPart) {
                gotTwo = true;
                break;
            }
        }
        if (!gotTwo) {
            singletons.append(equi);
            continue;
        }

        equis.append(equi);
    }

    m_maxProgress = equis.count() + singletons.count() + 1;
    if (bothSidesNow) m_maxProgress *= 2;
    emit setMaximumProgress(m_maxProgress);
    int progress = 1;
    emit setProgressValue(progress);

	ProcessEventBlocker::processEvents();
    
    double dpi = qMax((double) 250, 1000 / keepoutMils);  // turns out making a variable dpi doesn't work due to vector-to-raster issues
    QRectF boardRect = m_board->sceneBoundingRect();
    QRectF sourceRes(0, 0, 
					 boardRect.width() * dpi / GraphicsUtils::SVGDPI, 
                     boardRect.height() * dpi / GraphicsUtils::SVGDPI);

    QSize imgSize(qCeil(sourceRes.width()), qCeil(sourceRes.height()));

    m_plusImage = new QImage(imgSize, QImage::Format_Mono);
    m_plusImage->fill(0xffffffff);

    m_minusImage = new QImage(imgSize, QImage::Format_Mono);
    m_minusImage->fill(0);

    m_displayImage = new QImage(imgSize, QImage::Format_Indexed8);
    m_displayImage->setColor(0, 0);
    m_displayImage->setColor(1, 0x80ff0000);
    m_displayImage->setColor(2, 0xffffff00);
    m_displayImage->fill(0);

    if (!makeBoard(m_minusImage, sourceRes)) {
        message = tr("Fritzing error: unable to render board svg.");
        return false;
    }

    extendBorder(1, m_minusImage);   // since the resolution = keepout, extend by 1

    QList<ViewLayer::ViewLayerPlacement> layerSpecs;
    layerSpecs << ViewLayer::NewBottom;
    if (bothSidesNow) layerSpecs << ViewLayer::NewTop;

    int emptyMasterCount = 0;
    foreach (ViewLayer::ViewLayerPlacement viewLayerPlacement, layerSpecs) {  
        if (viewLayerPlacement == ViewLayer::NewTop) {
            emit wantTopVisible();
            m_plusImage->fill(0xffffffff);
        }
        else emit wantBottomVisible();

	    LayerList viewLayerIDs = ViewLayer::copperLayers(viewLayerPlacement);
        viewLayerIDs.removeOne(ViewLayer::GroundPlane0);
        viewLayerIDs.removeOne(ViewLayer::GroundPlane1);
        RenderThing renderThing;
        renderThing.printerScale = GraphicsUtils::SVGDPI;
        renderThing.blackOnly = true;
        renderThing.dpi = GraphicsUtils::StandardFritzingDPI;
        renderThing.hideTerminalPoints = renderThing.selectedItems = renderThing.renderBlocker = false;
	    QString master = m_sketchWidget->renderToSVG(renderThing, m_board, viewLayerIDs);
        if (master.isEmpty()) {
            if (++emptyMasterCount == layerSpecs.count()) {
                message = tr("No traces or connectors to check");
                return false;
            }

            progress++;
            continue;
	    }
       
	    QDomDocument * masterDoc = new QDomDocument();
        m_masterDocs.insert(viewLayerPlacement, masterDoc);

	    QString errorStr;
	    int errorLine;
	    int errorColumn;
	    if (!masterDoc->setContent(master, &errorStr, &errorLine, &errorColumn)) {
            message = tr("Unexpected SVG rendering failure--contact fritzing.org");
		    return false;
	    }

	    ProcessEventBlocker::processEvents();
        if (m_cancelled) {
            message = CancelledMessage;
            return false;
        }

        QDomElement root = masterDoc->documentElement();
        SvgFileSplitter::forceStrokeWidth(root, 2 * keepoutMils, "#000000", true, false);

        ItemBase::renderOne(masterDoc, m_plusImage, sourceRes);

	    ProcessEventBlocker::processEvents();
        if (m_cancelled) {
            message = CancelledMessage;
            return false;
        }

        QList<QPointF> atPixels;
        if (pixelsCollide(m_plusImage, m_minusImage, m_displayImage, 0, 0, imgSize.width(), imgSize.height(), 1 /* 0x80ff0000 */, atPixels)) {
            CollidingThing * collidingThing = findItemsAt(atPixels, m_board, viewLayerIDs, keepoutMils, dpi, true, NULL);
            QString msg = tr("Too close to a border (%1 layer)")
                .arg(viewLayerPlacement == ViewLayer::NewTop ? ItemBase::TranslatedPropertyNames.value("top") : ItemBase::TranslatedPropertyNames.value("bottom"))
                ;
            emit setProgressMessage(msg);
            messages << msg;
            collidingThings << collidingThing;
            updateDisplay();
        }

        emit setProgressValue(progress++);

	    ProcessEventBlocker::processEvents();
        if (m_cancelled) {
            message = CancelledMessage;
            return false;
        }

    }

    // we are checking all the singletons at once
    // but the DRC will miss it if any of them overlap each other

    while (singletons.count() > 0) {
        QList<ConnectorItem *> combined;
        QList<ConnectorItem *> singleton = singletons.takeFirst();
        ItemBase * chief = singleton.at(0)->attachedTo()->layerKinChief();
        combined.append(singleton);
        for (int ix = singletons.count() - 1; ix >= 0; ix--) {
            QList<ConnectorItem *> candidate = singletons.at(ix);
            if (candidate.at(0)->attachedTo()->layerKinChief() == chief) {
                combined.append(candidate);
                singletons.removeAt(ix);
            }
        }

        equis.append(combined);
    }

    int index = 0;
    foreach (ViewLayer::ViewLayerPlacement viewLayerPlacement, layerSpecs) {    
        if (viewLayerPlacement == ViewLayer::NewTop) emit wantTopVisible();
        else emit wantBottomVisible();

        QDomDocument * masterDoc = m_masterDocs.value(viewLayerPlacement, NULL);
        if (masterDoc == NULL) continue;

	    LayerList viewLayerIDs = ViewLayer::copperLayers(viewLayerPlacement);
        viewLayerIDs.removeOne(ViewLayer::GroundPlane0);
        viewLayerIDs.removeOne(ViewLayer::GroundPlane1);

        foreach (QList<ConnectorItem *> equi, equis) {
            bool inLayer = false;
            foreach (ConnectorItem * equ, equi) {
                if (viewLayerIDs.contains(equ->attachedToViewLayerID())) {
                    inLayer = true;
                    break;
                }
            }
            if (!inLayer) {
                progress++;
                continue;
            }

            // we have a net;
            m_plusImage->fill(0xffffffff);
            m_minusImage->fill(0xffffffff);
            splitNet(masterDoc, equi, m_minusImage, m_plusImage, sourceRes, viewLayerPlacement, index++, keepoutMils);
            
            QHash<ConnectorItem *, QRectF> rects;
            QList<Wire *> wires;
            foreach (ConnectorItem * equ, equi) {
                if (viewLayerIDs.contains(equ->attachedToViewLayerID())) {
                    if (equ->attachedToItemType() == ModelPart::Wire) {
                        Wire * wire = qobject_cast<Wire *>(equ->attachedTo());
                        if (!wires.contains(wire)) {
                            wires.append(wire);
                            // could break diagonal wires into a series of rects
                            rects.insert(equ, wire->sceneBoundingRect());
                        }
                    }
                    else {
                        rects.insert(equ, equ->sceneBoundingRect());
                    }
                }
            }

	        ProcessEventBlocker::processEvents();
            if (m_cancelled) {
                message = CancelledMessage;
                return false;
            }

            foreach (ConnectorItem * equ, rects.keys()) {
                QRectF rect = rects.value(equ).intersected(boardRect);
                double l = (rect.left() - boardRect.left()) * dpi / GraphicsUtils::SVGDPI;
                double t = (rect.top() - boardRect.top()) * dpi / GraphicsUtils::SVGDPI;
                double r = (rect.right() - boardRect.left()) * dpi / GraphicsUtils::SVGDPI;
                double b = (rect.bottom() - boardRect.top()) * dpi / GraphicsUtils::SVGDPI;
                //DebugDialog::debug(QString("l:%1 t:%2 r:%3 b:%4").arg(l).arg(t).arg(r).arg(b));
                QList<QPointF> atPixels;
                if (pixelsCollide(m_plusImage, m_minusImage, m_displayImage, l, t, r, b, 1 /* 0x80ff0000 */, atPixels)) {

                    #ifndef QT_NO_DEBUG
                        m_plusImage->save(FolderUtils::getTopLevelUserDataStorePath() + QString("/collidePlus%1_%2.png").arg(viewLayerPlacement).arg(index));
                        m_minusImage->save(FolderUtils::getTopLevelUserDataStorePath() + QString("/collideMinus%1_%2.png").arg(viewLayerPlacement).arg(index));
                    #endif

                    CollidingThing * collidingThing = findItemsAt(atPixels, m_board, viewLayerIDs, keepoutMils, dpi, false, equ);
                    QStringList names = getNames(collidingThing);
                    QString name0 = names.at(0);
                    QString msg = tr("%1 is overlapping (%2 layer)")
                        .arg(name0)
                        .arg(viewLayerPlacement == ViewLayer::NewTop ? ItemBase::TranslatedPropertyNames.value("top") : ItemBase::TranslatedPropertyNames.value("bottom"))
                        ;
                    messages << msg;
                    collidingThings << collidingThing;
                    emit setProgressMessage(msg);
                    updateDisplay();
                }
            }

            emit setProgressValue(progress++);

	        ProcessEventBlocker::processEvents();
            if (m_cancelled) {
                message = CancelledMessage;
                return false;
            }
        }
    }
    checkHoles(messages, collidingThings,  dpi);
    checkCopperBoth(messages, collidingThings, dpi);

    return true;
}

bool DRC::makeBoard(QImage * image, QRectF & sourceRes) {
	LayerList viewLayerIDs;
	viewLayerIDs << ViewLayer::Board;
    RenderThing renderThing;
    renderThing.printerScale = GraphicsUtils::SVGDPI;
    renderThing.blackOnly = true;
    renderThing.dpi = GraphicsUtils::StandardFritzingDPI;
    renderThing.hideTerminalPoints = renderThing.selectedItems = renderThing.renderBlocker = false;
	QString boardSvg = m_sketchWidget->renderToSVG(renderThing, m_board, viewLayerIDs);
	if (boardSvg.isEmpty()) {
		return false;
	}

    QByteArray boardByteArray;
    QString tempColor("#ffffff");
    QStringList exceptions;
	exceptions << "none" << "";
    if (!SvgFileSplitter::changeColors(boardSvg, tempColor, exceptions, boardByteArray)) {
		return false;
	}

	QSvgRenderer renderer(boardByteArray);
	QPainter painter;
	painter.begin(image);
	painter.setRenderHint(QPainter::Antialiasing, false);
	DebugDialog::debug("boardbounds", sourceRes);
	renderer.render(&painter  /*, sourceRes */);
	painter.end();

    // board should be white, borders should be black

#ifndef QT_NO_DEBUG
    image->save(FolderUtils::getTopLevelUserDataStorePath() + "/testDRCBoard.png");
#endif

    return true;
}

void DRC::splitNet(QDomDocument * masterDoc, QList<ConnectorItem *> & equi, QImage * minusImage, QImage * plusImage, QRectF & sourceRes, ViewLayer::ViewLayerPlacement viewLayerPlacement, int index, double keepoutMils) {
    // deal with connectors on the same part, even though they are not on the same net
    // in other words, make sure there are no overlaps of connectors on the same part
    QList<QDomElement> net;
    QList<QDomElement> alsoNet;
    QList<QDomElement> notNet;
    Markers markers;
    markers.outID = AlsoNet;
    markers.inTerminalID = markers.inSvgID = markers.inSvgAndID = markers.inNoID = Net;
    splitNetPrep(masterDoc, equi, markers, net, alsoNet, notNet, true);
    foreach (QDomElement element, notNet) element.setTagName("g");
    foreach (QDomElement element, alsoNet) element.setTagName("g");
    foreach (QDomElement element, net) {
        // want the normal size
        SvgFileSplitter::forceStrokeWidth(element, -2 * keepoutMils, "#000000", false, false);
    }

    ItemBase::renderOne(masterDoc, plusImage, sourceRes);

    foreach (QDomElement element, net) {
        // restore to keepout size
        SvgFileSplitter::forceStrokeWidth(element, 2 * keepoutMils, "#000000", false, false);
    }

    #ifndef QT_NO_DEBUG
        plusImage->save(FolderUtils::getTopLevelUserDataStorePath() + QString("/splitNetPlus%1_%2.png").arg(viewLayerPlacement).arg(index));
    #else
        Q_UNUSED(viewLayerPlacement);
        Q_UNUSED(index);
    #endif

    // now want notnet
    foreach (QDomElement element, net) {
        element.removeAttribute("net");
        element.setTagName("g");
    }
    foreach (QDomElement element, alsoNet) {
        element.setTagName(element.attribute("former"));
        element.removeAttribute("net");
    }
    foreach (QDomElement element, notNet) {
        element.setTagName(element.attribute("former"));
        element.removeAttribute("net");
    }

    ItemBase::renderOne(masterDoc, minusImage, sourceRes);
    #ifndef QT_NO_DEBUG
        minusImage->save(FolderUtils::getTopLevelUserDataStorePath() + QString("/splitNetMinus%1_%2.png").arg(viewLayerPlacement).arg(index));
    #endif

     // master doc restored to original state
    foreach (QDomElement element, net) {
        element.setTagName(element.attribute("former"));
    }

}

void DRC::splitNetPrep(QDomDocument * masterDoc, QList<ConnectorItem *> & equi, const Markers & markers, QList<QDomElement> & net, QList<QDomElement> & alsoNet, QList<QDomElement> & notNet, bool checkIntersection) 
{
    QMultiHash<QString, QString> partSvgIDs;
    QMultiHash<QString, QString> partTerminalIDs;
    QHash<QString, QString> bothIDs;
    QMultiHash<QString, ItemBase *> itemBases;
    QSet<QString> wireIDs;
    foreach (ConnectorItem * equ, equi) {
        ItemBase * itemBase = equ->attachedTo();
        if (itemBase == NULL) continue;

        if (itemBase->itemType() == ModelPart::Wire) {
            wireIDs.insert(QString::number(itemBase->id()));
        }

        if (equ->connector() == NULL) {
            // this shouldn't happen
            itemBase->debugInfo("!!!!!!!!!!!!!!!!!!!!!!!!!!!!! missing connector !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
            equ->debugInfo("missing connector");
            continue;
        }
        
        QString sid = QString::number(itemBase->id());
        SvgIdLayer * svgIdLayer = equ->connector()->fullPinInfo(itemBase->viewID(), itemBase->viewLayerID());
        partSvgIDs.insert(sid, svgIdLayer->m_svgId);
        if (!svgIdLayer->m_terminalId.isEmpty()) {
            partTerminalIDs.insert(sid, svgIdLayer->m_terminalId);
            bothIDs.insert(sid + svgIdLayer->m_svgId, svgIdLayer->m_terminalId);
        }
        itemBases.insert(sid, itemBase);
    }

    QList<QDomElement> todo;
    todo << masterDoc->documentElement();
    bool firstTime = true;
    while (!todo.isEmpty()) {
        QDomElement element = todo.takeFirst();
        //QString string;
        //QTextStream stream(&string);
        //element.save(stream, 0);

        QDomElement child = element.firstChildElement();
        while (!child.isNull()) {
            todo << child;
            child = child.nextSiblingElement();
        }
        if (firstTime) {
            // don't include the root <svg> element
            firstTime = false;
            continue;
        }

        QString partID = element.attribute("partID");
        if (!partID.isEmpty()) {
            QStringList svgIDs = partSvgIDs.values(partID);
            QStringList terminalIDs = partTerminalIDs.values(partID);
            if (svgIDs.count() == 0) {
                markSubs(element, NotNet);
            }
            else if (wireIDs.contains(partID)) {
                markSubs(element, Net);
            }
            else {
                splitSubs(masterDoc, element, partID, markers, svgIDs, terminalIDs, itemBases.values(partID), bothIDs, checkIntersection);
            }
        }

        if (element.tagName() == "g") {
            element.removeAttribute("net");
            continue;
        }

        element.setAttribute("former", element.tagName());
        if (element.attribute("net") == Net) {
            net.append(element);
        }
        else if (element.attribute("net") == AlsoNet) {
            alsoNet.append(element);
        }
        else {
            // assume the element belongs to notNet
            notNet.append(element);
        }
    }
}

void DRC::markSubs(QDomElement & root, const QString & mark) {
    QList<QDomElement> todo;
    todo << root;
    while (!todo.isEmpty()) {
        QDomElement element = todo.takeFirst();
        element.setAttribute("net", mark);
        QDomElement child = element.firstChildElement();
        while (!child.isNull()) {
            todo << child;
            child = child.nextSiblingElement();
        }
    }
}

void DRC::splitSubs(QDomDocument * doc, QDomElement & root, const QString & partID, const Markers & markers, const QStringList & svgIDs, const QStringList & terminalIDs, const QList<ItemBase *> & itemBases, QHash<QString, QString> & bothIDs, bool checkIntersection)
{
    //QString string;
    //QTextStream stream(&string);
    //root.save(stream, 0);

    QStringList notSvgIDs;   
    QStringList notTerminalIDs;
    if (checkIntersection && itemBases.count() > 0) {
        ItemBase * itemBase = itemBases.at(0);
        foreach (ConnectorItem * connectorItem, itemBase->cachedConnectorItems()) {
            SvgIdLayer * svgIdLayer = connectorItem->connector()->fullPinInfo(itemBase->viewID(), itemBase->viewLayerID());
            if (!svgIDs.contains(svgIdLayer->m_svgId)) {
                notSvgIDs.append(svgIdLayer->m_svgId);
            }
            if (!svgIdLayer->m_terminalId.isEmpty()) {
                if (!terminalIDs.contains(svgIdLayer->m_terminalId)) {
                    notTerminalIDs.append(svgIdLayer->m_terminalId);
                }
            }
        }
    }

    // split subelements of a part into separate nets
    QList<QDomElement> todo;
    QList<QDomElement> netElements;
    todo << root;
    while (!todo.isEmpty()) {
        QDomElement element = todo.takeFirst();
        QString svgID = element.attribute("id");
        if (!svgID.isEmpty()) {
            if (svgIDs.contains(svgID)) {
                if (bothIDs.value(partID + svgID).isEmpty()) {
                    // no terminal point
                    markSubs(element, markers.inSvgID);
                }
                else {
                    // privilege the terminal point over the pin/pad
                    markSubs(element, markers.inSvgAndID);
                }
                netElements << element;         // save these for intersection checks
                // all children are marked so don't add these to todo
                continue;
            }
            else if (notSvgIDs.contains(svgID)) {
                markSubs(element, markers.outID);
                // all children are marked so don't add these to todo
                continue;            
            }
            else if (terminalIDs.contains(svgID)) {
                markSubs(element, markers.inTerminalID);
                continue;
            }
            else if (notTerminalIDs.contains(svgID)) {
                markSubs(element, markers.outID);
                continue;
            }
        }

        QDomElement child = element.firstChildElement();
        while (!child.isNull()) {
            todo << child;
            child = child.nextSiblingElement();
        }
    }

    QList<QDomElement> toCheck;

    todo << root;
    while (!todo.isEmpty()) {
        QDomElement element = todo.takeFirst();
        QString net = element.attribute("net");
        if (net == AlsoNet) continue;
        else if (net == Net) continue;
        else if (net == NotNet) continue;
        else if (!checkIntersection) {
            element.setAttribute("net", markers.outID);
        }
        else if (element.tagName() != "g") {
            element.setAttribute("oldid", element.attribute("id"));
            element.setAttribute("id", QString("_____toCheck_____%1").arg(toCheck.count()));
            toCheck << element;
        }

        QDomElement child = element.firstChildElement();
        while (!child.isNull()) {
            todo << child;
            child = child.nextSiblingElement();
        }
    }

    if (toCheck.count() > 0) {
        // deal with elements that are effectively part of a connector, but aren't separately labeled
        // such as a rect which overlaps a circle
        QDomElement masterRoot = doc->documentElement();
        QString svg = QString("<svg width='%1' height='%2' viewBox='%3'>\n")
            .arg(masterRoot.attribute("width"))
            .arg(masterRoot.attribute("height"))
            .arg(masterRoot.attribute("viewBox"));
        QString string;
        QTextStream stream(&string);
        root.save(stream, 0);
        svg += string;
        svg += "</svg>";
        QSvgRenderer renderer;
        renderer.load(svg.toUtf8());
        QList<QRectF> netRects;
        foreach (QDomElement element, netElements) {
            QString id = element.attribute("id");
            QRectF r = renderer.matrixForElement(id).mapRect(renderer.boundsOnElement(id));         
            netRects << r;
        }
        QList<QRectF> checkRects;
        foreach (QDomElement element, toCheck) {
            QString id = element.attribute("id");
            QRectF r = renderer.matrixForElement(id).mapRect(renderer.boundsOnElement(id));         
            checkRects << r;
            element.setAttribute("id", element.attribute("oldid"));
        }
        for (int i = 0; i < checkRects.count(); i++) {
            QRectF checkr = checkRects.at(i);
            QDomElement checkElement = toCheck.at(i);
            bool gotOne = false;
            double carea = checkr.width() * checkr.height();
            foreach (QRectF netr, netRects) {
                QRectF sect = netr.intersected(checkr);
                if (sect.isEmpty()) continue;

                double area = sect.width() * sect.height();
                double netArea = netr.width() * netr.height();
                if ((area > netArea * .5) && (netArea * 2 > carea)) {
                    markSubs(checkElement, markers.inNoID);
                    gotOne = true;
                    break;
                }
            }
            if (!gotOne) {
                markSubs(checkElement, markers.outID);
            }
        }
    }
}

void DRC::updateDisplay() {
    QPixmap pixmap = QPixmap::fromImage(*m_displayImage);
    if (m_displayItem == NULL) {
        m_displayItem = new QGraphicsPixmapItem(pixmap);
        m_displayItem->setPos(m_board->sceneBoundingRect().topLeft());
        m_sketchWidget->scene()->addItem(m_displayItem);
        m_displayItem->setZValue(5000);
        m_displayItem->setScale(m_board->sceneBoundingRect().width() / m_displayImage->width());   // GraphicsUtils::SVGDPI / dpi
        m_displayItem->setVisible(true);
    }
    else {
        m_displayItem->setPixmap(pixmap);
    }
    ProcessEventBlocker::processEvents();
}

void DRC::cancel() {
	m_cancelled = true;
}

CollidingThing * DRC::findItemsAt(QList<QPointF> & atPixels, ItemBase * board, const LayerList & viewLayerIDs, double keepoutMils, double dpi, bool skipHoles, ConnectorItem * already) {
    Q_UNUSED(board);
    Q_UNUSED(viewLayerIDs);
    Q_UNUSED(keepoutMils);
    Q_UNUSED(dpi);
    Q_UNUSED(skipHoles);
    
    CollidingThing * collidingThing = new CollidingThing;
    collidingThing->nonConnectorItem = already;
    collidingThing->atPixels = atPixels;

    return collidingThing;
}

void DRC::extendBorder(double keepout, QImage * image) {
    // TODO: scanlines

    // keepout in terms of the board grid size

    QImage copy = image->copy();

    int h = image->height();
    int w = image->width();
    int ikeepout = qCeil(keepout);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (copy.pixel(x, y) != 0xff000000) {
                continue;
            }

            for (int dy = y - ikeepout; dy <= y + ikeepout; dy++) {
                if (dy < 0) continue;
                if (dy >= h) continue;
                for (int dx = x - ikeepout; dx <= x + ikeepout; dx++) {
                    if (dx < 0) continue;
                    if (dx >= w) continue;

                    // extend border by keepout
                    image->setPixel(dx, dy, 0);
                }
            }
        }
    }
}

void DRC::checkHoles(QStringList & messages, QList<CollidingThing *> & collidingThings, double dpi) {
    QRectF boardRect = m_board->sceneBoundingRect();
    foreach (QGraphicsItem * item, m_sketchWidget->scene()->collidingItems(m_board)) {
        NonConnectorItem * nci = dynamic_cast<NonConnectorItem *>(item);
        if (nci == NULL) continue;

        QRectF ncibr = nci->sceneBoundingRect();
        if (boardRect.contains(ncibr)) continue;

        if (nci->attachedToItemType() == ModelPart::Wire) continue;
        if (nci->attachedToItemType() == ModelPart::CopperFill) continue;

        // TODO: skip pad part, logo item, smds

        QRectF ir = boardRect.intersected(ncibr);
        int x = qFloor((ir.left() - boardRect.left()) * dpi / GraphicsUtils::SVGDPI);
        if (x < 0) x = 0;
        int y = qFloor((ir.top() - boardRect.top()) * dpi / GraphicsUtils::SVGDPI);
        if (y < 0) y = 0;
        int w = qCeil(ir.width() * dpi / GraphicsUtils::SVGDPI);
        if (x + w > m_displayImage->width()) w = m_displayImage->width() - x;
        int h = qCeil(ir.height() * dpi / GraphicsUtils::SVGDPI);
        if (y + h > m_displayImage->height()) h = m_displayImage->height() - y;

        CollidingThing * collidingThing = new CollidingThing;
        collidingThing->nonConnectorItem = nci;
        for (int iy = 0; iy < h; iy++) {
            for (int ix = 0; ix < w; ix++) {
                QPoint p(ix + x, iy + y);
                m_displayImage->setPixel(p, 1);
                collidingThing->atPixels << p;
            }
        }
        QStringList names = getNames(collidingThing);
        QString name0 = names.at(0);
        QString msg = tr("A hole in %1 may lie outside the border of the board and would be clipped.")
            .arg(name0)
            ;
        messages << msg;
        collidingThings << collidingThing;
        emit setProgressMessage(msg);
        updateDisplay();
    }
}

void DRC::checkCopperBoth(QStringList & messages, QList<CollidingThing *> & collidingThings, double dpi) {
    QRectF boardRect = m_board->sceneBoundingRect();
    QList<ItemBase *> visited;
    foreach (QGraphicsItem * item, m_sketchWidget->scene()->items()) {
        ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
        if (itemBase == NULL) continue;
        if (!itemBase->isEverVisible()) continue;
        if (itemBase->modelPart()->isCore()) continue;

        itemBase = itemBase->layerKinChief();
        if (visited.contains(itemBase)) continue;

        visited << itemBase;

        QRectF ibr = item->sceneBoundingRect();
        if (!boardRect.intersects(ibr)) continue;

        QString fzpPath = itemBase->modelPart()->path();
        QFile file(fzpPath);
        if (!file.open(QFile::ReadOnly)) {
            DebugDialog::debug(QString("unable to open %1").arg(fzpPath));
            continue;
        }

        QString fzp = file.readAll();
        file.close();

        bool copper0 = fzp.contains("copper0");
        bool copper1 = fzp.contains("copper1");
        if (!copper1 && !copper0) continue;

        QString svgPath = itemBase->fsvgRenderer()->filename();
        QFile file2(svgPath);
        if (!file2.open(QFile::ReadOnly)) {
            DebugDialog::debug(QString("itembase svg file open failure %1").arg(itemBase->id()));
            continue;
        }

        QString svg = file2.readAll();
        file2.close();

        QDomDocument doc;
        QString errorStr;
	    int errorLine;
	    int errorColumn;
	    if (!doc.setContent(svg, &errorStr, &errorLine, &errorColumn)) {
            DebugDialog::debug(QString("itembase svg xml failure %1 %2 %3 %4").arg(itemBase->id()).arg(errorStr).arg(errorLine).arg(errorColumn));
            continue;
        }

        QSet<ConnectorItem *> missing;
        QDomElement root = doc.documentElement();
    
        if (copper1) {
            foreach(ConnectorItem * ci, missingCopper("copper1", ViewLayer::Copper1, itemBase, root)) {
                missing << ci;
            }
        }

        if (copper0) {
            foreach (ConnectorItem * ci, missingCopper("copper0", ViewLayer::Copper0, itemBase, root)) {
                missing << ci;
            }
        }

        foreach (ConnectorItem * ci, missing) {
            QRectF ir = boardRect.intersected(ci->sceneBoundingRect());
            int x = qFloor((ir.left() - boardRect.left()) * dpi / GraphicsUtils::SVGDPI);
            if (x < 0) x = 0;
            int y = qFloor((ir.top() - boardRect.top()) * dpi / GraphicsUtils::SVGDPI);
            if (y < 0) y = 0;
            int w = qCeil(ir.width() * dpi / GraphicsUtils::SVGDPI);
            if (x + w > m_displayImage->width()) w = m_displayImage->width() - x;
            int h = qCeil(ir.height() * dpi / GraphicsUtils::SVGDPI);
            if (y + h > m_displayImage->height()) h = m_displayImage->height() - y;

            CollidingThing * collidingThing = new CollidingThing;
            collidingThing->nonConnectorItem = ci;
            for (int iy = 0; iy < h; iy++) {
                for (int ix = 0; ix < w; ix++) {
                    QPoint p(ix + x, iy + y);
                    m_displayImage->setPixel(p, 1);
                    collidingThing->atPixels << p;
                }
            }
            QStringList names = getNames(collidingThing);
            QString name0 = names.at(0);
            QString msg = tr("Connector %1 on %2 should have both copper top and bottom layers, but the svg only specifies one layer.")
                .arg(ci->connectorSharedName())
                .arg(name0)
                ;
            messages << msg;
            collidingThings << collidingThing;
            emit setProgressMessage(msg);
            updateDisplay();
        }
    }

}

QList<ConnectorItem *> DRC::missingCopper(const QString & layerName, ViewLayer::ViewLayerID viewLayerID, ItemBase * itemBase, const QDomElement & root) 
{
    QDomElement copperElement = TextUtils::findElementWithAttribute(root, "id", layerName);
    QList<ConnectorItem *> missing;
    
    foreach (ConnectorItem * connectorItem, itemBase->cachedConnectorItems()) {
        SvgIdLayer * svgIdLayer = connectorItem->connector()->fullPinInfo(itemBase->viewID(), viewLayerID);
        if (svgIdLayer == NULL) {
            DebugDialog::debug(QString("missing pin info for %1").arg(itemBase->id()));
            missing << connectorItem;
            continue;
        }

        QDomElement element = TextUtils::findElementWithAttribute(copperElement, "id", svgIdLayer->m_svgId);
        if (element.isNull()) {
            missing << connectorItem;
        }
    }

    return missing;
}
