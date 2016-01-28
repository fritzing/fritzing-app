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

$Revision: 6934 $:
$Author: irascibl@gmail.com $:
$Date: 2013-03-22 08:35:24 +0100 (Fr, 22. Mrz 2013) $

********************************************************************/

#include <QPixmap>
#include <QPainter>

#include "svgiconwidget.h"
#include "../sketch/infographicsview.h"
#include "../debugdialog.h"
#include "../utils/misc.h"
#include "../fsvgrenderer.h"
#include "../items/moduleidnames.h"
#include "../layerattributes.h"

#include "partsbinview.h"

#define SELECTED_STYLE "background-color: white;"
#define NON_SELECTED_STYLE "background-color: #C2C2C2;"

const QColor SectionHeaderColor(80, 80, 80);

#define SELECTION_THICKNESS 2
#define HALF_SELECTION_THICKNESS 1
#define ICON_SIZE 32
#define SINGULAR_OFFSET 3
#define PLURAL_OFFSET 2

static QPixmap * PluralImage = NULL;
static QPixmap * SingularImage = NULL;

////////////////////////////////////////////////////////////

SvgIconPixmapItem::SvgIconPixmapItem(const QPixmap & pixmap, QGraphicsItem * parent) : QGraphicsPixmapItem(pixmap, parent) 
{
}

void  SvgIconPixmapItem::setPlural(bool plural) {
	m_plural = plural;
}

void SvgIconPixmapItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) 
{
	QGraphicsPixmapItem::paint(painter, option, widget);

	if (this->parentItem()->isSelected()) {
		painter->save();
		QPen pen = painter->pen();
		pen.setColor(QColor(122, 15, 49));
		pen.setWidth(SELECTION_THICKNESS);
		painter->setPen(pen);
		painter->drawRect(m_plural ? HALF_SELECTION_THICKNESS : HALF_SELECTION_THICKNESS + 1, 
						  m_plural ? HALF_SELECTION_THICKNESS : HALF_SELECTION_THICKNESS + 1, 
						  ICON_SIZE + SELECTION_THICKNESS, ICON_SIZE + SELECTION_THICKNESS);
		painter->restore();
	} 

}

////////////////////////////////////////////////////////////

SvgIconWidget::SvgIconWidget(ModelPart * modelPart, ViewLayer::ViewID viewID, ItemBase * itemBase, bool plural)
	: QGraphicsWidget() 
{
	m_moduleId = modelPart->moduleID();
	m_itemBase = itemBase;

	if (modelPart->itemType() == ModelPart::Space) {
		m_moduleId = ModuleIDNames::SpacerModuleIDName;
		QString text = PartsBinView::TranslatedCategoryNames.value(modelPart->instanceText(), modelPart->instanceText());
		this->setData(Qt::UserRole, text);
		if (text.isEmpty()) {
			this->setMaximumSize(PluralImage->size().width(), 1);
		}
		else {
			this->setMaximumSize(PluralImage->size().width(), 8);
		}
		setAcceptHoverEvents(false);
		setFlags(0);
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
	}
	else {
		this->setMaximumSize(PluralImage->size());
		setAcceptHoverEvents(true);
		setFlags(QGraphicsItem::ItemIsSelectable);
		setupImage(plural, viewID);
	}
}

void SvgIconWidget::initNames() {
	if (PluralImage == NULL) {
		PluralImage = new QPixmap(":/resources/images/icons/parts_plural_v3_plur.png");
	}
	if (SingularImage == NULL) {
		SingularImage = new QPixmap(":/resources/images/icons/parts_plural_v3_sing.png");
	}
}

SvgIconWidget::~SvgIconWidget() {
	//delete m_itemBase;  // these are now shared
}

void SvgIconWidget::cleanup() {
	if (PluralImage) {
		delete PluralImage;
		PluralImage = NULL;
	}
	if (SingularImage) {
		delete SingularImage;
		SingularImage = NULL;
	}
}

ItemBase *SvgIconWidget::itemBase() const {
	return m_itemBase;
}

ModelPart *SvgIconWidget::modelPart() const {
	return m_itemBase->modelPart();
}

const QString &SvgIconWidget::moduleID() const {
	return m_moduleId;
}

void SvgIconWidget::hoverEnterEvent ( QGraphicsSceneHoverEvent * event ){
	QGraphicsWidget::hoverEnterEvent(event);
	InfoGraphicsView * igv = InfoGraphicsView::getInfoGraphicsView(this);
	if (igv) {
		igv->hoverEnterItem(event, m_itemBase);
	}
}

void SvgIconWidget::hoverLeaveEvent ( QGraphicsSceneHoverEvent * event ) {
	QGraphicsWidget::hoverLeaveEvent(event);
	InfoGraphicsView * igv = InfoGraphicsView::getInfoGraphicsView(this);
	if (igv) {
		igv->hoverLeaveItem(event, m_itemBase);
	}
}

void SvgIconWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) 
{
	if (m_moduleId.compare(ModuleIDNames::SpacerModuleIDName) == 0) {
		QString text = data(Qt::UserRole).toString();
		if (!text.isEmpty()) {
			painter->save();
			QRectF r = this->boundingRect();
			QPen pen = painter->pen();
			pen.setColor(SectionHeaderColor);
			pen.setWidth(1);
			painter->setPen(pen);
			QFont font = painter->font();
			font.setPointSize(8);
			painter->setFont(font);
			painter->drawText(r.left(), r.bottom(), text);
			//QFontMetrics fm(font);
			//r.setLeft(r.left() + fm.width(text));
			//r.setBottom(r.bottom() + 2);
			//painter->drawLine(r.left(), r.bottom(), scene()->width(), r.bottom());
			painter->restore();	
		}		
		return;
	}

	QGraphicsWidget::paint(painter, option, widget);
}

void SvgIconWidget::setItemBase(ItemBase * itemBase, bool plural)
{
	m_itemBase = itemBase;
	setupImage(plural, itemBase->viewID());
}

void SvgIconWidget::setupImage(bool plural, ViewLayer::ViewID viewID)
{
	LayerAttributes layerAttributes;
    m_itemBase->initLayerAttributes(layerAttributes, viewID, ViewLayer::Icon, ViewLayer::NewTop, false, false);
	ModelPart * modelPart = m_itemBase->modelPart();
	FSvgRenderer * renderer = m_itemBase->setUpImage(modelPart, layerAttributes);
    if (renderer == NULL) {
        DebugDialog::debug(QString("missing renderer for icon %1").arg(modelPart->moduleID()));
    }
	if (renderer && m_itemBase) {
		m_itemBase->setFilename(renderer->filename());
	}

	QPixmap pixmap(plural ? *PluralImage : *SingularImage);
	QPixmap * icon = (renderer == NULL) ? NULL : FSvgRenderer::getPixmap(renderer, QSize(ICON_SIZE, ICON_SIZE));
	if (icon) {
		QPainter painter;
		painter.begin(&pixmap);
		if (plural) {
			painter.drawPixmap(PLURAL_OFFSET, PLURAL_OFFSET, *icon);
		}
		else {
			painter.drawPixmap(SINGULAR_OFFSET, SINGULAR_OFFSET, *icon);
		}
		painter.end();
		delete icon;
	}

	m_pixmapItem = new SvgIconPixmapItem(pixmap, this);
	m_pixmapItem->setPlural(plural);

	m_pixmapItem->setFlags(0);
	m_pixmapItem->setPos(0, 0);

	if (m_itemBase) {
		m_itemBase->setTooltip();
		setToolTip(m_itemBase->toolTip());
	}

    if (renderer) {
        m_itemBase->setSharedRendererEx(renderer);
    }
}
