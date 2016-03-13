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
    m_passMouseEvents = false;
    m_layerKinChief = chief;
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    m_modelPart->removeViewItem(this);  // we don't need to save layerkin
}

void LayerKinPaletteItem::initLKPI(LayerAttributes & layerAttributes, const LayerHash & viewLayers) {
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
    m_passMouseEvents = m_layerKinChief->mousePressEventK(this, event);
    return;

    //ItemBase::mousePressEvent(event);
}

void LayerKinPaletteItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) { 
    if (m_passMouseEvents) {
        m_layerKinChief->mouseMoveEventK(this, event);
        return;
    }

    PaletteItemBase::mouseMoveEvent(event);
}

void LayerKinPaletteItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) { 
    if (m_passMouseEvents) {
        m_layerKinChief->mouseReleaseEventK(this, event);
        return;
    }

    PaletteItemBase::mouseReleaseEvent(event);
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
}

void SchematicTextLayerKinPaletteItem::cacheLoaded(const LayerAttributes & layerAttributes)
{
    this->setProperty("textSvg", layerAttributes.loaded());
}

void SchematicTextLayerKinPaletteItem::setTransform2(const QTransform & currTransf) {
    transformItem(currTransf, false);
}

void SchematicTextLayerKinPaletteItem::transformItem(const QTransform & currTransf, bool includeRatsnest) {
    Q_UNUSED(currTransf);
    Q_UNUSED(includeRatsnest);

    if (m_textThings.count() == 0) {
        initTextThings();
    }

    double rotation;
    QTransform chiefTransform = layerKinChief()->transform();      // assume chief already has rotation
    bool isFlipped = GraphicsUtils::isFlipped(chiefTransform.toAffine(), rotation);
    QString svg;
    if (isFlipped) {
        svg = makeFlipTextSvg();
    }
   
    if (svg.isEmpty()) {
        svg = this->property("textSvg").toByteArray();
    }

    if (rotation >= 135 && rotation <= 225) {
        svg = vflip(svg, isFlipped);
    }

    reloadRenderer(svg, true);

    QPointF p = layerKinChief()->sceneBoundingRect().topLeft();
    QTransform transform;
    QRectF bounds = boundingRect();
    transform.translate(bounds.width() / 2, bounds.height() / 2);
    transform.rotate(rotation);
    transform.translate(bounds.width() / -2, bounds.height() / -2);
    this->setTransform(transform);

    //QMatrix m1 = chiefTransform.toAffine();
    //layerKinChief()->debugInfo("chief " + TextUtils::svgMatrix(m1));
        
    //m1 = transform.toAffine();      
    //debugInfo("\t " + TextUtils::svgMatrix(m1));
}

void SchematicTextLayerKinPaletteItem::initTextThings() {
    QByteArray textSvg = this->property("textSvg").toByteArray();

    QDomDocument doc;
    QString errorStr;
	int errorLine;
	int errorColumn;
    if (!doc.setContent(textSvg, &errorStr, &errorLine, &errorColumn)) {
        DebugDialog::debug(QString("unable to parse schematic text: %1 %2 %3:\n%4").arg(errorStr).arg(errorLine).arg(errorColumn).arg(QString(textSvg)));
		return;
    }

    QDomElement root = doc.documentElement();
    QDomNodeList nodeList = root.elementsByTagName("text");
    QList<QDomElement> texts;
    for (int i = 0; i < nodeList.count(); i++) {
        texts.append(nodeList.at(i).toElement());
    }

    positionTexts(texts);
}

QString SchematicTextLayerKinPaletteItem::makeFlipTextSvg() {
    QByteArray textSvg = this->property("textSvg").toByteArray();

    QDomDocument doc;
    QString errorStr;
	int errorLine;
	int errorColumn;
    if (!doc.setContent(textSvg, &errorStr, &errorLine, &errorColumn)) {
        DebugDialog::debug(QString("unable to parse schematic text: %1 %2 %3:\n%4").arg(errorStr).arg(errorLine).arg(errorColumn).arg(QString(textSvg)));
		return "";
    }

    QDomElement root = doc.documentElement();
    QDomNodeList nodeList = root.elementsByTagName("text");
    QList<QDomElement> texts;
    for (int i = 0; i < nodeList.count(); i++) {
        texts.append(nodeList.at(i).toElement());
    }

    int ix = 0;
    foreach (QDomElement text, texts) {  
        QDomElement g = text.ownerDocument().createElement("g");
        text.parentNode().insertAfter(g, text);
        g.appendChild(text);
        QMatrix m = m_textThings[ix++].flipMatrix;
        TextUtils::setSVGTransform(g, m);
    }

    return doc.toString();
}

#define MINMAX(mx, my)          \
    if (mx < minX) minX = mx;   \
    if (mx > maxX) maxX = mx;   \
    if (my < minY) minY = my;   \
    if (my > maxY) maxY = my;  


void SchematicTextLayerKinPaletteItem::positionTexts(QList<QDomElement> & texts) {
    // TODO: reuse these values unless the pin labels have changed
    //QString id = IDString.arg(0);
    //if (this->property(id.toUtf8().constData()).isValid()) {
    //    // calculated this already
    //    return;
    //}

    m_textThings.clear();

    foreach (QDomElement text, texts) {
        text.setTagName("g");
    }

    QRectF br = boundingRect();
    QImage image(qCeil(br.width()) * 2, qCeil(br.height()) * 2, QImage::Format_Mono);  // schematic text is so small it doesn't render unless bitmap is double-sized

    foreach (QDomElement text, texts) {
        TextThing textThing;
        QRectF viewBox;
        QMatrix matrix;
        renderText(image, text, textThing.minX, textThing.minY, textThing.maxX, textThing.maxY, matrix, viewBox);

        double newX = (image.width() - textThing.maxX) * viewBox.width() / image.width();
        double oldX = textThing.minX * viewBox.width() / image.width();

        QMatrix inv = matrix.inverted();   
        QMatrix t = QMatrix().translate(newX - oldX, 0);
        textThing.flipMatrix = matrix * t * inv;

        QRectF r(textThing.minX * viewBox.width() / image.width(), 
                 textThing.minY * viewBox.height() / image.height(), 
                 (textThing.maxX - textThing.minX) * viewBox.width() / image.width(),
                 (textThing.maxY - textThing.minY) * viewBox.height() / image.height());

        textThing.newRect = inv.mapRect(r);


        m_textThings.append(textThing);
    }

    foreach (QDomElement text, texts) {
        text.setTagName("text");
    }

}

void SchematicTextLayerKinPaletteItem::renderText(QImage & image, QDomElement & text, int & minX, int & minY, int & maxX, int & maxY, QMatrix & matrix, QRectF & viewBox) 
{
    QString oldid = text.attribute("id");
    text.setAttribute("id", IDString);

    // TODO: handle inherited fill/stroke values
    QString oldFill = text.attribute("fill");
    text.setAttribute("fill", "black");
    QString oldStroke = text.attribute("stroke");
    text.setAttribute("stroke", "black");
    text.setTagName("text");

    image.fill(0xffffffff);
    QByteArray byteArray = text.ownerDocument().toByteArray();
    QSvgRenderer renderer(byteArray);
	QPainter painter;
	painter.begin(&image);
	painter.setRenderHint(QPainter::Antialiasing, false);
	renderer.render(&painter  /*, sourceRes */);
	painter.end();

#ifndef QT_NO_DEBUG
    image.save(FolderUtils::getTopLevelUserDataStorePath() + "/renderText.png");
#endif

    viewBox = renderer.viewBoxF();
    double x = text.attribute("x").toDouble();
    double y = text.attribute("y").toDouble();
    QPointF xy(x, y);
    matrix = renderer.matrixForElement(IDString);
    QPointF mxy = matrix.map(xy);

    QPointF p(image.width() * mxy.x() / viewBox.width(), image.height() * mxy.y() / viewBox.height());
    QPoint iq((int) p.x(), (int) p.y());

    minX = image.width() + 1;
    maxX = -1;
    minY = image.height() + 1;
    maxY = -1;

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

    text.setTagName("g");
    if (oldid.isEmpty()) text.removeAttribute("id");
    else text.setAttribute("id", oldid);
    if (oldFill.isEmpty()) text.removeAttribute("fill");
    else text.setAttribute("fill", oldFill);
    if (oldStroke.isEmpty()) text.removeAttribute("stroke");
    else text.setAttribute("stroke", oldStroke);
}

void SchematicTextLayerKinPaletteItem::clearTextThings() {
    m_textThings.clear();
}

QString SchematicTextLayerKinPaletteItem::vflip(const QString & svg, bool isFlipped) {
    Q_UNUSED(isFlipped);

    QDomDocument doc;
    QString errorStr;
	int errorLine;
	int errorColumn;
    if (!doc.setContent(svg, &errorStr, &errorLine, &errorColumn)) {
        DebugDialog::debug(QString("unable to parse schematic text: %1 %2 %3:\n%4").arg(errorStr).arg(errorLine).arg(errorColumn).arg(QString(svg)));
		return svg;
    }

    QDomElement root = doc.documentElement();
    QDomNodeList nodeList = root.elementsByTagName("text");
    QList<QDomElement> texts;
    for (int i = 0; i < nodeList.count(); i++) {
        texts.append(nodeList.at(i).toElement());
    }

    int ix = 0;
    foreach (QDomElement text, texts) {  
        QDomElement g = text.ownerDocument().createElement("g");
        text.parentNode().insertAfter(g, text);
        g.appendChild(text);
        QRectF r = m_textThings[ix].newRect;
        ix++;
        QMatrix m;
        m.translate(r.center().x(), r.center().y());
        QMatrix inv = m.inverted();
        QMatrix matrix = inv * QMatrix().rotate(180) * m;
        TextUtils::setSVGTransform(g, matrix);
    }

    return doc.toString();
}

void SchematicTextLayerKinPaletteItem::setInitialTransform(const QTransform & matrix) {
    setTransform2(matrix);
}
