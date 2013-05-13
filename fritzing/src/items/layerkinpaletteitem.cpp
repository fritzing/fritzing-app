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

$Revision: 6998 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-28 13:51:10 +0200 (So, 28. Apr 2013) $

********************************************************************/

#include "layerkinpaletteitem.h"
#include "../sketch/infographicsview.h"
#include "../debugdialog.h"
#include "../layerattributes.h"
#include "../utils/graphicsutils.h"
#include "../utils/textutils.h"
#include "../utils/folderutils.h"
#include "../svg/svgfilesplitter.h"

#include <qmath.h>

////////////////////////////////////////////////

static QString IDString("-_-_-text-_-_-%1");


////////////////////////////////////////////////

LayerKinPaletteItem::LayerKinPaletteItem(PaletteItemBase * chief, ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu* itemMenu)
	: PaletteItemBase(modelPart, viewID, viewGeometry, id, itemMenu)

{
    m_layerKinChief = chief;
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    m_modelPart->removeViewItem(this);  // we don't need to save layerkin
}

void LayerKinPaletteItem::init(LayerAttributes & layerAttributes, const LayerHash & viewLayers) {
    setViewLayerPlacement(layerAttributes.viewLayerPlacement);
	m_ok = setUpImage(m_modelPart, viewLayers, layerAttributes);
	//DebugDialog::debug(QString("lk accepts hover %1 %2 %3 %4 %5").arg(title()).arg(m_viewID).arg(m_id).arg(viewLayerID).arg(this->acceptHoverEvents()));
}

QVariant LayerKinPaletteItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
	//DebugDialog::debug(QString("lk item change %1 %2").arg(this->id()).arg(change));
	if (m_layerKinChief != NULL) {
	    if (change == ItemSelectedChange) {
	       	bool selected = value.toBool();
	    	if (m_blockItemSelectedChange && m_blockItemSelectedValue == selected) {
	    		m_blockItemSelectedChange = false;
	   		}
			else {
	        	m_layerKinChief->syncKinSelection(selected, this);
	       	}
	    }
	    //else if (change == ItemVisibleHasChanged && value.toBool()) {
	    	//this->setSelected(m_layerKinChief->syncSelected());
	    	//this->setPos(m_offset + m_layerKinChief->syncMoved());
	    //}
	    else if (change == ItemPositionHasChanged) {
	    	m_layerKinChief->syncKinMoved(this->m_offset, value.toPointF());
	   	}
   	}
    return PaletteItemBase::itemChange(change, value);
}

void LayerKinPaletteItem::setOffset(double x, double y) {
	m_offset.setX(x);
	m_offset.setY(y);
	this->setPos(this->pos() + m_offset);
}

ItemBase * LayerKinPaletteItem::layerKinChief() {
	return m_layerKinChief;
}

bool LayerKinPaletteItem::ok() {
	return m_ok;
}

void LayerKinPaletteItem::updateConnections(bool includeRatsnest, QList<ConnectorItem *> & already) {
    if (m_layerKinChief) {
	    m_layerKinChief->updateConnections(includeRatsnest, already);
    }
    else {
        DebugDialog::debug("chief deleted before layerkin");
    }
}

void LayerKinPaletteItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {

	//DebugDialog::debug("layer kin mouse press event");
	if (m_layerKinChief->lowerConnectorLayerVisible(this)) {
		// TODO: this code may be unnecessary
		DebugDialog::debug("LayerKinPaletteItem::mousePressEvent isn't obsolete");
		event->ignore();
		return;
	}

	m_layerKinChief->mousePressEvent(this, event);
    return;


    ItemBase::mousePressEvent(event);
}

void LayerKinPaletteItem::setHidden(bool hide) {
	ItemBase::setHidden(hide);
	m_layerKinChief->figureHover();
}

void LayerKinPaletteItem::setInactive(bool inactivate) {
	ItemBase::setInactive(inactivate);
	m_layerKinChief->figureHover();
}

void LayerKinPaletteItem::clearModelPart() {
	m_layerKinChief->clearModelPart();
}

ItemBase * LayerKinPaletteItem::lowerConnectorLayerVisible(ItemBase * itemBase) {
    //debugInfo("layerkin lowerconnector");
	return m_layerKinChief->lowerConnectorLayerVisible(itemBase);
}

bool LayerKinPaletteItem::stickyEnabled() {
	return m_layerKinChief->stickyEnabled();
}

bool LayerKinPaletteItem::isSticky() {
	return m_layerKinChief->isSticky();
}

bool LayerKinPaletteItem::isBaseSticky() {
	return m_layerKinChief->isBaseSticky();
}

void LayerKinPaletteItem::setSticky(bool s) 
{
	m_layerKinChief->setSticky(s);
}

void LayerKinPaletteItem::addSticky(ItemBase * sticky, bool stickem) {
	m_layerKinChief->addSticky(sticky, stickem);
}

ItemBase * LayerKinPaletteItem::stickingTo() {
	return m_layerKinChief->stickingTo();
}

QList< QPointer<ItemBase> > LayerKinPaletteItem::stickyList() {
	return m_layerKinChief->stickyList();
}

bool LayerKinPaletteItem::alreadySticking(ItemBase * itemBase) {
	return m_layerKinChief->alreadySticking(itemBase);
}

void LayerKinPaletteItem::resetID() {
	long offset = m_id % ModelPart::indexMultiplier;
	ItemBase::resetID();
	m_id += offset;
}

QString LayerKinPaletteItem::retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor) 
{
	return m_layerKinChief->retrieveSvg(viewLayerID, svgHash, blackOnly, dpi, factor);
}

ConnectorItem* LayerKinPaletteItem::newConnectorItem(Connector *connector) 
{
	return m_layerKinChief->newConnectorItem(this, connector);
}


bool LayerKinPaletteItem::isSwappable() {
	return m_layerKinChief->isSwappable();
}

void LayerKinPaletteItem::setSwappable(bool swappable) {
	m_layerKinChief->setSwappable(swappable);
}

bool LayerKinPaletteItem::inRotation() {
	return layerKinChief()->inRotation();
}

void LayerKinPaletteItem::setInRotation(bool val) {
    layerKinChief()->setInRotation(val);
}

//////////////////////////////////////////////////

SchematicTextLayerKinPaletteItem::SchematicTextLayerKinPaletteItem(PaletteItemBase * chief, ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu* itemMenu)
	: LayerKinPaletteItem(chief, modelPart, viewID, viewGeometry, id, itemMenu)

{
    m_flipped = false;
}

bool SchematicTextLayerKinPaletteItem::setUpImage(ModelPart * modelPart, const LayerHash & viewLayers, LayerAttributes & layerAttributes)
{
	bool result = PaletteItemBase::setUpImage(modelPart, viewLayers, layerAttributes);
    this->setProperty("textSvg", layerAttributes.loaded());
    return result;
}


void SchematicTextLayerKinPaletteItem::transformItem(const QTransform & currTransf, bool includeRatsnest) {
    Q_UNUSED(currTransf);
    Q_UNUSED(includeRatsnest);

    double rotation;
    QTransform chiefTransform = layerKinChief()->transform();      // assume chief already has rotation
    bool isFlipped = GraphicsUtils::isFlipped(chiefTransform.toAffine(), rotation);
    if (isFlipped) {
        if (!m_flipped) {
             makeFlipTextSvg();
        }
    }
    else if (m_flipped) {
        QString textSvg = this->property("textSvg").toString();
        reloadRenderer(textSvg, true);
        m_flipped = false;
    }

    QPointF p = layerKinChief()->sceneBoundingRect().topLeft();
    QTransform transform;
    QRectF bounds = boundingRect();
    transform.translate(bounds.width() / 2, bounds.height() / 2);
    transform.rotate(rotation);
    transform.translate(bounds.width() / -2, bounds.height() / -2);
    this->setTransform(transform);
}

bool SchematicTextLayerKinPaletteItem::makeFlipTextSvg() {
    QByteArray textSvg = this->property("textSvg").toByteArray();

    QDomDocument doc;
    QString errorStr;
	int errorLine;
	int errorColumn;
    if (!doc.setContent(textSvg, &errorStr, &errorLine, &errorColumn)) {
        DebugDialog::debug(QString("unable to parse schematic text: %1 %2 %3:\n%4").arg(errorStr).arg(errorLine).arg(errorColumn).arg(QString(textSvg)));
		return false;
    }

    QDomElement root = doc.documentElement();
    QDomNodeList nodeList = root.elementsByTagName("text");
    QList<QDomElement> texts;
    for (int i = 0; i < nodeList.count(); i++) {
        texts.append(nodeList.at(i).toElement());
    }

    int ix = 0;
    foreach (QDomElement text, texts) {
        text.setAttribute("id", IDString.arg(ix++));
    }

    positionTexts(doc, texts);

    //QSvgRenderer renderer;
    //renderer.load(doc.toByteArray());
    ix = 0;
    foreach (QDomElement text, texts) {
        QString id = IDString.arg(ix++);
        double x = this->property(id.toUtf8().constData()).toDouble();    
        // can't just use boundsOnElement() because it returns a null rectangle with <text> elements
        text.setAttribute("x", x);
    }

    QString debug = doc.toString();
    reloadRenderer(debug, true);
    m_flipped = true;
    return true;
}

#define MINMAX(mx, my)          \
    if (mx < minX) minX = mx;   \
    if (mx > maxX) maxX = mx;   \
    if (my < minY) minY = my;   \
    if (my > maxY) maxY = my;  


void SchematicTextLayerKinPaletteItem::positionTexts(QDomDocument & doc, QList<QDomElement> & texts) {
    QString id = IDString.arg(0);
    // TODO: reuse these values unless the pin labels have changed
    //if (this->property(id.toUtf8().constData()).isValid()) {
    //    // calculated this already
    //    return;
    //}

    foreach (QDomElement text, texts) {
        text.setTagName("g");
    }

    QRectF br = boundingRect();
    QImage image(qCeil(br.width()), qCeil(br.height()), QImage::Format_Mono);

    //bool hack = false;

    int ix = 0;
    foreach (QDomElement text, texts) {
        text.setTagName("text");
        id = IDString.arg(ix++);

        image.fill(0xffffffff);
        QSvgRenderer renderer(doc.toByteArray());
	    QPainter painter;
	    painter.begin(&image);
	    painter.setRenderHint(QPainter::Antialiasing, false);
	    renderer.render(&painter  /*, sourceRes */);
	    painter.end();

#ifndef QT_NO_DEBUG
	    image.save(FolderUtils::getUserDataStorePath("") + "/" + id + ".png");
#endif

        QRectF viewBox = renderer.viewBoxF();
        double x = text.attribute("x").toDouble();
        double y = text.attribute("y").toDouble();
        QPointF p(image.width() * x / viewBox.width(), image.height() * y / viewBox.height());
        QMatrix matrix = renderer.matrixForElement(id);
        QPointF q = matrix.map(p);
        QPoint iq((int) q.x(), (int) q.y());

        /*
        if (!hack) {
            hack = true;
            QDomElement rect = doc.createElement("rect");
            doc.documentElement().appendChild(rect);
            rect.setAttribute("x", 0);
            rect.setAttribute("y", 0);
            rect.setAttribute("width", viewBox.width());
            rect.setAttribute("height", viewBox.height());
            rect.setAttribute("stroke", "none");
            rect.setAttribute("stroke-width", "0");
            rect.setAttribute("fill", "red");
            rect.setAttribute("fill-opacity", 0.5);
        }
        */

        int minX = image.width() + 1;
        int maxX = -1;
        int minY = image.height() + 1;
        int maxY = -1;

        // spiral around q
        int limit = qMax(image.width(), image.height());
        for (int lim = 0; lim < limit; lim++) {
            int t = qMax(0, iq.y() - lim);
            int b = qMin(iq.y() + lim, image.height() - 1);
            int l = qMax(0, iq.x() - lim);
            int r = qMin(iq.x() + lim, image.width() - 1);

            for (int iy = t; iy <= b; iy++) {
                if (image.pixel(l, iy) == 0xff000000) {
                    MINMAX(l, iy);
                }
                if (image.pixel(r, iy) == 0xff000000) {
                    MINMAX(r, iy);
                }
            }

            for (int ix = l + 1; ix < r; ix++) {
                if (image.pixel(ix, t) == 0xff000000) {
                    MINMAX(ix, t);
                }
                if (image.pixel(ix, b) == 0xff000000) {
                    MINMAX(ix, b);
                }
            }
        }

        // TODO: assumes left-to-right text orientation
        QString anchor = TextUtils::findAnchor(text);
        int useX = maxX;
        if (anchor == "middle") {
            useX = (maxX + minX) / 2;
        }
        else if (anchor == "end") {
            useX = minX;
        }

        QMatrix inv = matrix.inverted();
        QPointF rp((image.width() - useX) * viewBox.width() / image.width(), (image.height() - maxY) * viewBox.height() / image.height());
        QPointF rq = inv.map(rp);
        this->setProperty(id.toUtf8().constData(), rq.x());
        text.setTagName("g");
    }

    foreach (QDomElement text, texts) {
        text.setTagName("text");
    }

}
