/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2014 Fachhochschule Potsdam - http://fh-potsdam.de

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

$Revision: 6912 $:
$Author: irascibl@gmail.com $:
$Date: 2013-03-09 08:18:59 +0100 (Sa, 09. Mrz 2013) $

********************************************************************/

#include "partlabel.h"
#include "../items/itembase.h"
#include "../viewgeometry.h"
#include "../debugdialog.h"
#include "../sketch/infographicsview.h"
#include "../model/modelpart.h"
#include "../utils/graphicsutils.h"
#include "../utils/textutils.h"
#include "../installedfonts.h"
#include "../fsvgrenderer.h"

#include <QGraphicsScene>
#include <QMenu>
#include <QApplication>
#include <QInputDialog>
#include <QStringList>
#include <QFont>
#include <QSvgRenderer>

// TODO:
//		** selection: coordinate with part selection: it's a layerkin
//		** select a part, highlight its label; click a label, highlight its part
//		** viewinfo update when selected
//		** viewinfo for wires
//		** graphics (esp. drag area vs. edit area) 
//		** html info box needs to update when view switches
//		** undo delete text
//		** undo change text
//		** undo move
//		** layers and z order
//		** hide and show layer
//		** sync hide/show checkbox with visibility state
//		** save and load
//		** text color needs to be separate in separate views
//		** hide silkscreen should hide silkscreen label
//		** delete owner: delete label
//		** make label single-line (ignore enter key)

//		** rotate/flip 
//		** undo rotate/flip
//		format: bold, italic, size (small normal large huge), color?, 
//		undo format
//		heads-up controls

//		rotate/flip may need to be relative to part?
//		copy/paste?
//		z-order manipulation?
//		hover?
//		show = autoselect?
//		undo delete show?
//		close focus on enter/return?

//		-- multiple selection?
//		-- undo select
//		-- export to svg for export diy (silkscreen layer is not exported)


enum PartLabelAction {
	PartLabelRotate45CW = 1,
	PartLabelRotate90CW,
	PartLabelRotate135CW,
	PartLabelRotate180,
	PartLabelRotate45CCW,
	PartLabelRotate90CCW,
	PartLabelRotate135CCW,
	PartLabelFlipHorizontal,
	PartLabelFlipVertical,
	PartLabelEdit,
    PartLabelFontSizeTiny,
	PartLabelFontSizeSmall,
	PartLabelFontSizeMedium,
	PartLabelFontSizeLarge,
	PartLabelDisplayLabelText,
	PartLabelHide
};

/////////////////////////////////////////////

static QMultiHash<long, PartLabel *> AllPartLabels;
static const QString LabelTextKey = "";
static const double InactiveOpacity = 0.4;

///////////////////////////////////////////

PartLabel::PartLabel(ItemBase * owner, QGraphicsItem * parent)
	: QGraphicsSvgItem(parent)
{
	m_renderer = NULL;
	m_owner = owner;
	m_spaceBarWasPressed = false;
	m_text = m_displayText = "";

	m_inactive = m_hidden = m_initialized = false;
	m_displayKeys.append(LabelTextKey);
	if (m_owner->hasPartNumberProperty() && m_owner->viewID() != ViewLayer::PCBView) {
		m_displayKeys.append(ModelPartShared::PartNumberPropertyName);
	}

	setFlag(QGraphicsItem::ItemIsSelectable, false);
	setFlag(QGraphicsItem::ItemIsMovable, false);					// don't move this in the standard QGraphicsItem way
	setVisible(false);
	m_viewLayerID = ViewLayer::UnknownLayer;
	setAcceptHoverEvents(true);
	AllPartLabels.insert(m_owner->id(), this);
}

PartLabel::~PartLabel() 
{
	AllPartLabels.remove(m_owner->id(), this);
	if (m_owner) {
		m_owner->clearPartLabel();
	}
}

void PartLabel::showLabel(bool showIt, ViewLayer * viewLayer) {
	if (showIt == this->isVisible()) return;

	if (showIt && !m_initialized) {
		if (m_owner == NULL) return;
		if (m_owner->scene() == NULL) return;

		bool flipped = (viewLayer->viewLayerID() == ViewLayer::Silkscreen0Label);

		if (m_owner->viewID() != ViewLayer::PCBView) {
			foreach (QString dk, m_owner->modelPart()->displayKeys()) {
				if (!m_displayKeys.contains(dk)) {
					m_displayKeys.append(dk);
				}
			}
		}

		this->setZValue(viewLayer->nextZ());
		m_viewLayerID = viewLayer->viewLayerID();
		m_owner->scene()->addItem(this);
		setUpText();
		setPlainText(m_owner->instanceTitle());
		m_initialized = true;

		QRectF obr = m_owner->boundingRect();
		QRectF tbr = QGraphicsSvgItem::boundingRect();
		QPointF initial = (flipped) 
			? m_owner->pos() + QPointF(-tbr.width(), -tbr.height())
			: m_owner->pos() + QPointF(obr.width(), -tbr.height());
		this->setPos(initial);
		m_offset = initial - m_owner->pos();
		if (flipped) {
			transformLabel(QTransform().scale(-1,1));
		}

	}

	setVisible(showIt);
}

QPainterPath PartLabel::shape() const
{
	QRectF t = boundingRect();
	QPainterPath path;
	path.addRect(t);
    return path;
}

void PartLabel::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (!m_owner->isSelected()) {
		event->ignore();
		return;
	}

	InfoGraphicsView *infographics = InfoGraphicsView::getInfoGraphicsView(this);
	if (infographics != NULL && infographics->spaceBarIsPressed()) {
		m_spaceBarWasPressed = true;
		event->ignore();
		return;
	}

	m_spaceBarWasPressed = false;

	if (!this->isSelected()) {
		this->setSelected(true);
	}

	m_doDrag = true;
	m_initialPosition = pos();
	m_initialOffset = m_offset;
}

void PartLabel::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if (!m_owner->isSelected()) {
		event->ignore();
		return;
	}

	if (m_spaceBarWasPressed) {
		event->ignore();
		return;
	}

	if (m_doDrag) {
		QPointF currentParentPos = mapToParent(mapFromScene(event->scenePos()));
		QPointF buttonDownParentPos = mapToParent(mapFromScene(event->buttonDownScenePos(Qt::LeftButton)));
		setPos(m_initialPosition + currentParentPos - buttonDownParentPos);
		m_offset = this->pos() - m_owner->pos();
		return;
	}

	QGraphicsSvgItem::mouseMoveEvent(event);
}

void PartLabel::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (!m_owner->isSelected()) {
		event->ignore();
		return;
	}

	if (m_spaceBarWasPressed) {
		event->ignore();
		return;
	}


	if (m_doDrag) {
		m_owner->partLabelMoved(m_initialPosition, m_initialOffset, pos(), m_offset);
	}

	QGraphicsSvgItem::mouseReleaseEvent(event);
}


void PartLabel::setPlainText(const QString & text) 
{
	m_text = text;
	displayTexts();
}

void PartLabel::displayTextsIf() {
	if (!m_initialized) return;
	if (m_hidden) return;
	if (!isVisible()) return;

	displayTexts();
}

void PartLabel::displayTexts() {
	QStringList texts;

	foreach (QString key, m_displayKeys) {
		QString t;
		if (key.compare(LabelTextKey) == 0) {
			t = m_text;
		}
		else {
			t = m_owner->getProperty(key);
		}
		if (!t.isEmpty()) texts.append(t);
	}

	QString text = texts.join("\n");

	if (text.length() == 0) {
		text = "?";					// make sure there's something visible
	}

	m_displayText = text;
	resetSvg();
}

bool PartLabel::initialized() {
	return m_initialized;
}

void PartLabel::ownerMoved(QPointF newPos) {
	this->setPos(m_offset + newPos);
}

void PartLabel::setHidden(bool hide) {
	if (!m_initialized) return;

	m_hidden = hide;
	setHiddenOrInactive();
}

void PartLabel::setInactive(bool inactivate) {
	if (!m_initialized) return;

	m_inactive = inactivate;
	setHiddenOrInactive();
}

void PartLabel::setHiddenOrInactive() {
	bool hide = m_hidden || m_inactive || !m_owner->isSelected();

	setAcceptedMouseButtons(hide ? Qt::NoButton : ALLMOUSEBUTTONS);
	setAcceptHoverEvents(!hide);
	update();
}


ViewLayer::ViewLayerID PartLabel::viewLayerID() {
	return m_viewLayerID;
}

bool PartLabel::hidden() {
	return m_hidden;
}

bool PartLabel::inactive() {
	return m_inactive;
}

void PartLabel::saveInstance(QXmlStreamWriter & streamWriter) {
	if (!m_initialized) return;

	streamWriter.writeStartElement("titleGeometry");
	streamWriter.writeAttribute("visible", isVisible() ? "true" : "false");
	streamWriter.writeAttribute("x", QString::number(pos().x()));
	streamWriter.writeAttribute("y", QString::number(pos().y()));
	streamWriter.writeAttribute("z", QString::number(zValue()));
	streamWriter.writeAttribute("xOffset", QString::number(m_offset.x()));
	streamWriter.writeAttribute("yOffset", QString::number(m_offset.y()));
	streamWriter.writeAttribute("textColor", m_color.name());
	streamWriter.writeAttribute("fontSize", QString::number(m_font.pointSizeF()));
	GraphicsUtils::saveTransform(streamWriter, transform());
	foreach (QString key, m_displayKeys) {
		streamWriter.writeStartElement("displayKey");
		streamWriter.writeAttribute("key", key);
		streamWriter.writeEndElement();
	}
	streamWriter.writeEndElement();
}

void PartLabel::restoreLabel(QDomElement & labelGeometry, ViewLayer::ViewLayerID viewLayerID) 
{
	m_viewLayerID = viewLayerID;
	m_initialized = true;
	m_owner->scene()->addItem(this);
	setVisible(labelGeometry.attribute("visible").compare("true") == 0);
	QPointF p = pos();
	bool ok = false;
	double x = labelGeometry.attribute("x").toDouble(&ok);
	if (ok) p.setX(x);
	double y = labelGeometry.attribute("y").toDouble(&ok);
	if (ok) p.setY(y);
	setPos(p);
	x = labelGeometry.attribute("xOffset").toDouble(&ok);
	if (ok) m_offset.setX(x);
	y = labelGeometry.attribute("yOffset").toDouble(&ok);
	if (ok) m_offset.setY(y);
	double z = labelGeometry.attribute("z").toDouble(&ok);
	if (ok) this->setZValue(z);

	//ignore the textColor attribute so the labels are always set from standard colors
	//QColor c;
	//c.setNamedColor(labelGeometry.attribute("textColor"));
	//setBrush(QBrush(c));

	setUpText();
	m_initialized = true;
	double fs = labelGeometry.attribute("fontSize").toDouble(&ok);
	if (!ok) {
		InfoGraphicsView *infographics = InfoGraphicsView::getInfoGraphicsView(this);
		if (infographics != NULL) {
            fs = infographics->getLabelFontSizeMedium();
			ok = true;
		}
	}
	if (ok) {
		m_font.setPointSizeF(fs);
	}

	m_displayKeys.clear();
	QDomElement displayKey = labelGeometry.firstChildElement("displayKey");
	while (!displayKey.isNull()) {
		m_displayKeys.append(displayKey.attribute("key"));
		displayKey = displayKey.nextSiblingElement("displayKey");
	}

	if (m_displayKeys.length() == 0) {
		m_displayKeys.append(LabelTextKey);
		if (m_owner->hasPartNumberProperty()) m_displayKeys.append(ModelPartShared::PartNumberPropertyName);
	}

	displayTexts();

	QTransform t;
	if (GraphicsUtils::loadTransform(labelGeometry.firstChildElement("transform"), t)) {
		setTransform(t);
	}
}

void PartLabel::moveLabel(QPointF newPos, QPointF newOffset) 
{
	this->setPos(newPos);
	m_offset = newOffset;
}

ItemBase * PartLabel::owner() {
	return m_owner;
}

void PartLabel::initMenu() 
{
	// todo: make this a static var?

    QAction *editAct = m_menu.addAction(tr("Edit"));
	editAct->setData(QVariant(PartLabelEdit));
	editAct->setStatusTip(tr("Edit label text"));

    QAction *hideAct = m_menu.addAction(tr("Hide"));
	hideAct->setData(QVariant(PartLabelHide));
	hideAct->setStatusTip(tr("Hide part label"));

	m_menu.addSeparator();

	QMenu * dvmenu = m_menu.addMenu(tr("Display Values"));
	QMenu * rlmenu = m_menu.addMenu(tr("Flip/Rotate"));
	QMenu * fsmenu = m_menu.addMenu(tr("Font Size"));

	bool include45 = (m_owner != NULL) && (m_owner->viewID() == ViewLayer::PCBView);

	if (include45) {
		QAction *rotate45cwAct = rlmenu->addAction(tr("Rotate 45\u00B0 Clockwise"));
		rotate45cwAct->setData(QVariant(PartLabelRotate45CW));
		rotate45cwAct->setStatusTip(tr("Rotate the label by 45 degrees clockwise"));
	}

    QAction *rotate90cwAct = rlmenu->addAction(tr("Rotate 90\u00B0 Clockwise"));
	rotate90cwAct->setData(QVariant(PartLabelRotate90CW));
	rotate90cwAct->setStatusTip(tr("Rotate the label by 90 degrees clockwise"));

	if (include45) {
		QAction *rotate135cwAct = rlmenu->addAction(tr("Rotate 135\u00B0 Clockwise"));
		rotate135cwAct->setData(QVariant(PartLabelRotate135CW));
		rotate135cwAct->setStatusTip(tr("Rotate the label by 135 degrees clockwise"));
	}

 	QAction *rotate180Act = rlmenu->addAction(tr("Rotate 180\u00B0"));
	rotate180Act->setData(QVariant(PartLabelRotate180));
	rotate180Act->setStatusTip(tr("Rotate the label by 180 degrees"));
  
	if (include45) {
		QAction *rotate135ccwAct = rlmenu->addAction(tr("Rotate 135\u00B0 Counter Clockwise"));
		rotate135ccwAct->setData(QVariant(PartLabelRotate135CCW));
		rotate135ccwAct->setStatusTip(tr("Rotate the label by 135 degrees counter clockwise"));
	}

	QAction *rotate90ccwAct = rlmenu->addAction(tr("Rotate 90\u00B0 Counter Clockwise"));
	rotate90ccwAct->setData(QVariant(PartLabelRotate90CCW));
	rotate90ccwAct->setStatusTip(tr("Rotate current selection 90 degrees counter clockwise"));
		
	if (include45) {
		QAction *rotate45ccwAct = rlmenu->addAction(tr("Rotate 45\u00B0 Counter Clockwise"));
		rotate45ccwAct->setData(QVariant(PartLabelRotate45CCW));
		rotate45ccwAct->setStatusTip(tr("Rotate the label by 45 degrees counter clockwise"));
	}

	QAction *flipHorizontalAct = rlmenu->addAction(tr("Flip Horizontal"));
	flipHorizontalAct->setData(QVariant(PartLabelFlipHorizontal));
	flipHorizontalAct->setStatusTip(tr("Flip label horizontally"));

	QAction *flipVerticalAct = rlmenu->addAction(tr("Flip Vertical"));
	flipVerticalAct->setData(QVariant(PartLabelFlipVertical));
	flipVerticalAct->setStatusTip(tr("Flip label vertically"));

    m_tinyAct = fsmenu->addAction(tr("Tiny"));
        m_tinyAct->setData(QVariant(PartLabelFontSizeTiny));
        m_tinyAct->setStatusTip(tr("Set font size to tiny"));
        m_tinyAct->setCheckable(true);
        m_tinyAct->setChecked(false);

    m_smallAct = fsmenu->addAction(tr("Small"));
	m_smallAct->setData(QVariant(PartLabelFontSizeSmall));
	m_smallAct->setStatusTip(tr("Set font size to small"));
	m_smallAct->setCheckable(true);
	m_smallAct->setChecked(false);

    m_mediumAct = fsmenu->addAction(tr("Medium"));
	m_mediumAct->setData(QVariant(PartLabelFontSizeMedium));
	m_mediumAct->setStatusTip(tr("Set font size to medium"));
	m_mediumAct->setCheckable(true);
	m_mediumAct->setChecked(false);

    m_largeAct = fsmenu->addAction(tr("Large"));
	m_largeAct->setData(QVariant(PartLabelFontSizeLarge));
	m_largeAct->setStatusTip(tr("Set font size to large"));
	m_largeAct->setCheckable(true);
	m_largeAct->setChecked(false);

    m_labelAct = dvmenu->addAction(tr("Label text"));
	m_labelAct->setData(QVariant(PartLabelDisplayLabelText));
	m_labelAct->setCheckable(true);
	m_labelAct->setChecked(true);
	m_labelAct->setStatusTip(tr("Display the text of the label"));

	dvmenu->addSeparator();

	QHash<QString,QString> properties = m_owner->modelPart()->properties();
	foreach (QString key, properties.keys()) {
		QString translatedName = ItemBase::translatePropertyName(key);
		QAction * action = dvmenu->addAction(translatedName);
		action->setData(QVariant(key));
		action->setCheckable(true);
		action->setChecked(false);
		action->setStatusTip(tr("Display the value of property %1").arg(translatedName));
		m_displayActs.append(action);
	}
}

void PartLabel::rotateFlipLabel(double degrees, Qt::Orientations orientation) {
	if (degrees != 0) {
		transformLabel(QTransform().rotate(degrees));
	}
	else {
		int xScale, yScale;
		if (orientation == Qt::Vertical) {
			xScale = 1;
			yScale = -1;
		} 
		else if(orientation == Qt::Horizontal) {
			xScale = -1;
			yScale = 1;
		}
		else return;

		transformLabel(QTransform().scale(xScale,yScale));
	}
}

void PartLabel::transformLabel(QTransform currTransf) 
{
	QRectF rect = this->boundingRect();
	double x = rect.width() / 2.0;
	double y = rect.height() / 2.0;
	QTransform transf = transform() * QTransform().translate(-x, -y) * currTransf * QTransform().translate(x, y);
	setTransform(transf);
}

void PartLabel::setUpText() {
	InfoGraphicsView *infographics = InfoGraphicsView::getInfoGraphicsView(this);
	if (infographics != NULL) {
		infographics->getLabelFont(m_font, m_color, m_owner);
	}
}

QVariant PartLabel::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant & value)
{
	switch (change) {
		case QGraphicsItem::ItemSceneHasChanged:
			if (this->scene()) {
			}
			break;
		default:
			break;
	}

	return QGraphicsSvgItem::itemChange(change, value);
}

void PartLabel::ownerSelected(bool selected) 
{
	bool hide = !selected;
	if (m_hidden || m_inactive) {
		hide = true;
	}
	setAcceptedMouseButtons(hide ? Qt::NoButton : ALLMOUSEBUTTONS);
	setAcceptHoverEvents(!hide);
}

void PartLabel::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
	if (m_hidden || m_inactive || !m_owner->isSelected()) {
		event->ignore();
		return;
	}

	if (m_menu.isEmpty()) {
		initMenu();
	}

	m_labelAct->setChecked(m_displayKeys.contains(LabelTextKey));
	foreach (QAction * displayAct, m_displayActs) {
		QString data = displayAct->data().toString();
		displayAct->setChecked(m_displayKeys.contains(data));
	}

    m_tinyAct->setChecked(false);
	m_smallAct->setChecked(false);
	m_mediumAct->setChecked(false);
	m_largeAct->setChecked(false);
	InfoGraphicsView *infographics = InfoGraphicsView::getInfoGraphicsView(this);
	if (infographics != NULL) {
		int fs = m_font.pointSize();
		if (fs == infographics->getLabelFontSizeTiny()) {
			m_tinyAct->setChecked(true);
		}
		else if (fs == infographics->getLabelFontSizeSmall()) {
            m_smallAct->setChecked(true);
        }
		else if (fs == infographics->getLabelFontSizeMedium()) {
			m_mediumAct->setChecked(true);
		}
		else if (fs == infographics->getLabelFontSizeLarge()) {
			m_largeAct->setChecked(true);
		}
	}
    
	QAction *selectedAction = m_menu.exec(event->screenPos());
	if (selectedAction == NULL) return;

	PartLabelAction action = (PartLabelAction) selectedAction->data().toInt();
	switch (action) {
		case PartLabelRotate45CW:
		case PartLabelRotate45CCW:
		case PartLabelRotate90CW:
		case PartLabelRotate90CCW:
		case PartLabelRotate135CW:
		case PartLabelRotate135CCW:
		case PartLabelRotate180:
		case PartLabelFlipHorizontal:
		case PartLabelFlipVertical:
			rotateFlip(action);
			break;
		case PartLabelEdit:
			partLabelEdit();
			break;
		case PartLabelHide:
			partLabelHide();
			break;
                case PartLabelFontSizeTiny:
		case PartLabelFontSizeSmall:
		case PartLabelFontSizeMedium:
		case PartLabelFontSizeLarge:
			setFontSize(action);
			resetSvg();
			break;
		case PartLabelDisplayLabelText:
			setLabelDisplay(LabelTextKey);
			break;
		default:
			setLabelDisplay(selectedAction->data().toString());
			break;
	}
}

void PartLabel::rotateFlip(int action) {
	double degrees = 0;
	Qt::Orientations orientation = 0;
	switch (action) {
		case PartLabelRotate45CW:
			degrees = 45;
			break;
		case PartLabelRotate90CW:
			degrees = 90;
			break;
		case PartLabelRotate135CW:
			degrees = 135;
			break;
		case PartLabelRotate90CCW:
			degrees = 270;
			break;
		case PartLabelRotate135CCW:
			degrees = 225;
			break;
		case PartLabelRotate180:
			degrees = 180;
			break;
		case PartLabelRotate45CCW:
			degrees = 315;
			break;
		case PartLabelFlipHorizontal:
			orientation = Qt::Horizontal;
			break;
		case PartLabelFlipVertical:
			orientation = Qt::Vertical;
			break;
		default:
			break;
	}

	m_owner->rotateFlipPartLabel(degrees, orientation);
}


void PartLabel::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
	if (!m_owner->isSelected()) {
		event->ignore();
		return;
	}

	m_doDrag = false;
	partLabelEdit();
}

void PartLabel::partLabelHide() 
{
	m_owner->hidePartLabel();
}

void PartLabel::partLabelEdit() 
{
	bool ok;
	QString oldText = m_text;
    QString text = QInputDialog::getText((QGraphicsView *) this->scene()->parent(), tr("Set label for %1").arg(m_owner->title()),
                                          tr("Label text:"), QLineEdit::Normal, oldText, &ok);
	if (ok && (oldText.compare(text) != 0)) {
		if (m_owner) {
			m_owner->partLabelChanged(text);
			foreach (PartLabel * p, AllPartLabels.values(m_owner->id())) {
				p->setPlainText(text);
			}
		}	
	}
}

void PartLabel::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if (m_hidden) return;

	if (m_inactive) {
		painter->save();
		painter->setOpacity(InactiveOpacity);
	}

	if (m_owner->isSelected()) {
		GraphicsUtils::qt_graphicsItem_highlightSelected(painter, option, boundingRect(), shape());
    }

    QGraphicsSvgItem::paint(painter, option, widget);

	if (m_inactive) {
		painter->restore();
	}
}

void PartLabel::setFontSize(int action) {
	InfoGraphicsView *infographics = InfoGraphicsView::getInfoGraphicsView(this);
	if (infographics == NULL) return;

	double fs = 0;
	switch (action) {
        case PartLabelFontSizeTiny:
            fs = infographics->getLabelFontSizeTiny();
            break;
		case PartLabelFontSizeSmall:
			fs = infographics->getLabelFontSizeSmall();
			break;
		case PartLabelFontSizeMedium:
			fs = infographics->getLabelFontSizeMedium();
			break;
		case PartLabelFontSizeLarge:
			fs = infographics->getLabelFontSizeLarge();
			break;
		default:
			return;
	}
	m_font.setPointSize(fs);
}

void PartLabel::setFontPointSize(double pointSize) {
    m_font.setPointSize(pointSize);
    resetSvg();
}

void PartLabel::setLabelDisplay(const QString & key) {
	if (m_displayKeys.contains(key)) {
		m_displayKeys.removeOne(key);
	}
	else {
		m_displayKeys.append(key);
	}
	displayTexts();
}

QString mapToSVGWeight(int w) {
	int percent = 50;
	switch (w) {
		case QFont::Light: 
			percent = 25; 
			break;
		case QFont::Normal:	
			return "normal";    // 50
		case QFont::DemiBold: 
			percent = 63;
			break;
		case QFont::Bold: 
			return "bold";		// 75
		case QFont::Black: 
			percent = 87; 
			break;
		default:
			return "normal";
	}

	return QString::number((qRound(8 * percent / 100.0) * 100) + 100);   // 	normal | bold | bolder | lighter | 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900 | inherit
}

QString mapToSVGStyle(QFont::Style style) {
	switch (style) {
		case QFont::StyleNormal: return "normal";
		case QFont::StyleOblique: return "oblique";
		case QFont::StyleItalic: return "italic";
		default: return "normal";
	}
}

QString PartLabel::makeSvg(bool blackOnly, double dpi, double printerScale, bool includeTransform) {
	double w, h;
	QString svg = makeSvgAux(blackOnly, dpi, printerScale, w, h);
	if (includeTransform) {
		QTransform t = transform();
		if (!t.isIdentity()) {
			svg = TextUtils::svgTransform(svg, t, false, QString());
		}
	}
	return svg;
}


QString PartLabel::makeSvgAux(bool blackOnly, double dpi, double printerScale, double & w, double & h) 
{
	if (m_displayText.isEmpty()) return "";

	double pixels = m_font.pointSizeF() * printerScale / 72;
	double y = pixels * 0.75;
	//DebugDialog::debug(QString("initial y:%1").arg(y));
	
	QString svg = QString("<g font-size='%1' font-style='%2' font-weight='%3' fill='%4' font-family=\"'%5'\" id='%6' fill-opacity='1' stroke='none' >")
		.arg(m_font.pointSizeF() * dpi / 72)
		.arg(mapToSVGStyle(m_font.style()))
		.arg(mapToSVGWeight(m_font.weight()))
		.arg(blackOnly ? "#000000" : m_color.name())
		.arg(InstalledFonts::InstalledFontsNameMapper.value(m_font.family()))
		.arg(ViewLayer::viewLayerXmlNameFromID(m_viewLayerID)
		);

	w = 0;
	QStringList texts = m_displayText.split("\n");
	foreach (QString t, texts) {
		QString t1 = TextUtils::convertExtendedChars(TextUtils::escapeAnd(t));
		svg += QString("<text x='0' y='%1'>%2</text>")
			.arg(y * dpi / printerScale)
			.arg(t1);
		y += pixels;
		w = qMax(w, t.length() * pixels * 0.75);
		//DebugDialog::debug(QString("\t%1, %2").arg(w).arg(y));
	}

	svg += "</g>";

	h = y - (pixels / 2);

	//QFontInfo fontInfo(m_font);
	//DebugDialog::debug(QString("%1 match:%2 ps:%3 sty:%4 w:%5")
	//	.arg(fontInfo.family())
	//	.arg(fontInfo.exactMatch())
	//	.arg(fontInfo.pointSizeF())
	//	.arg(fontInfo.style())
	//	.arg(fontInfo.weight())
	//	);

	return svg;

	//DebugDialog::debug(QString("final:%1 %2 %3").arg(w).arg(h).arg(m_font.toString()));
    
}

void PartLabel::resetSvg()
{

	double w, h;
	QString innerSvg = makeSvgAux(false, GraphicsUtils::StandardFritzingDPI, GraphicsUtils::SVGDPI, w, h);
	if (innerSvg.isEmpty()) return;

	QString svg = TextUtils::makeSVGHeader(GraphicsUtils::SVGDPI, GraphicsUtils::StandardFritzingDPI, w, h) + innerSvg + "\n</svg>";

	if (m_renderer == NULL) {
		m_renderer = new QSvgRenderer(this);
	}

	// using renderer()->load() doesn't seem to work, so keep a separate shared renderer as a workaround
	bool loaded = m_renderer->load(svg.toUtf8());
	if (loaded) {
		setSharedRenderer(m_renderer);
	}
}
