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

#include <QBuffer>
#include <QHBoxLayout>
#include <QSettings>
#include <QPalette>
#include <QFontMetricsF>
#include <qmath.h>

#include "htmlinfoview.h"
#include "../sketch/infographicsview.h"
#include "../debugdialog.h"
#include "../connectors/connectorshared.h"
#include "../connectors/connector.h"
#include "../fsvgrenderer.h"
#include "../utils/flineedit.h"
#include "../items/moduleidnames.h"
#include "../items/paletteitem.h"
#include "../utils/clickablelabel.h"
#include "../utils/textutils.h"


#define HTML_EOF "</body>\n</html>"

QPixmap * NoIcon = NULL;

const int HtmlInfoView::STANDARD_ICON_IMG_WIDTH = 32;
const int HtmlInfoView::STANDARD_ICON_IMG_HEIGHT = 32;
const int IconSpace = 0;

static const int MaxSpinBoxWidth = 60;
static const int AfterSpinBoxWidth = 5;

/////////////////////////////////////

QLabel * addLabel(QHBoxLayout * hboxLayout, QPixmap * pixmap) {
	QLabel * label = new QLabel();
	label->setObjectName("iconLabel");
	label->setAutoFillBackground(true);
	label->setPixmap(*pixmap);
	label->setFixedSize(pixmap->size());
	hboxLayout->addWidget(label);
	hboxLayout->addSpacing(IconSpace);

	return label;
}

QString format3(double d) {
    return QString("%1").arg(d, 0, 'f', 3);
}

//////////////////////////////////////


TagLabel::TagLabel(QWidget * parent) : QLabel(parent) 
{
}

QSize TagLabel::sizeHint() const
{
	QSize hint = QLabel::sizeHint();
	QString t = text();
	if (t.isEmpty()) return hint;

	QFontMetricsF fm(this->font());
	double textWidth = fm.width(t);

	QWidget * w = this->window();
	QPoint pos(0,0);
	pos = this->mapTo(w, pos);

	int myWidth = w->width() - pos.x() - 10;
	if (textWidth < myWidth) {
		return hint;
	}

	int lines = qCeil(textWidth / myWidth);
	int h = lines * fm.height();

	return QSize(myWidth, h);
}

//////////////////////////////////////

HtmlInfoView::HtmlInfoView(QWidget * parent) : QScrollArea(parent) 
{
    this->setWidgetResizable(true);

	this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	m_lastTitleItemBase = NULL;
	m_lastTagsModelPart = NULL;
	m_lastConnectorItem = NULL;
	m_lastIconItemBase = NULL;
	m_lastPropsModelPart = NULL;
	m_lastPropsItemBase = NULL;

    m_tinyMode = false;

	m_partTitle = NULL;
	m_partUrl = NULL;
	m_partVersion = NULL;
	m_lockCheckbox = NULL;
	m_stickyCheckbox = NULL;
	m_connDescr = NULL;
	m_tagsTextLabel = NULL;
	m_lastSwappingEnabled = false;
	m_lastItemBase = NULL;
	m_setContentTimer.setSingleShot(true);
	m_setContentTimer.setInterval(10);
	connect(&m_setContentTimer, SIGNAL(timeout()), this, SLOT(setContent()));

	m_currentItem = NULL;
	m_currentSwappingEnabled = false;

    m_layerWidget = NULL;

}


HtmlInfoView::~HtmlInfoView() {
	foreach (PropThing * propThing, m_propThings) {
		delete propThing;
	}
	m_propThings.clear();
}

void HtmlInfoView::init(bool tinyMode) {
    m_tinyMode = tinyMode;

    QFrame * mainFrame = new QFrame(this);
	mainFrame->setObjectName("infoViewMainFrame");

    QVBoxLayout *vlo = new QVBoxLayout(mainFrame);
	vlo->setSpacing(0);
	vlo->setContentsMargins(0, 0, 0, 0);
	vlo->setSizeConstraint( QLayout::SetMinAndMaxSize );

        /* Part Title */

	m_titleEdit = new FLineEdit(mainFrame);
	m_titleEdit->setObjectName("instanceTitleEditor");
	m_titleEdit->setToolTip(tr("Change the part label here"));
	m_titleEdit->setAlignment(Qt::AlignLeft);

	connect(m_titleEdit, SIGNAL(editingFinished()), this, SLOT(setInstanceTitle()), Qt::QueuedConnection);
	connect(m_titleEdit, SIGNAL(mouseEnter()), this, SLOT(instanceTitleEnter()));
	connect(m_titleEdit, SIGNAL(mouseLeave()), this, SLOT(instanceTitleLeave()));
	connect(m_titleEdit, SIGNAL(editable(bool)), this, SLOT(instanceTitleEditable(bool)));

	setInstanceTitleColors(m_titleEdit, QColor(0xaf, 0xaf, 0xb4), QColor(0x00, 0x00, 0x00)); //b3b3b3, 575757
	m_titleEdit->setAutoFillBackground(true);
	
    vlo->addWidget(m_titleEdit);
    if (tinyMode) m_titleEdit->setVisible(false);

    /* Part Icons */

	if (NoIcon == NULL) {
		NoIcon = new QPixmap(":/resources/images/icons/noicon.png");
	}

	QFrame * iconFrame = new QFrame(mainFrame);
	iconFrame->setObjectName("iconFrame");

	QHBoxLayout * hboxLayout = new QHBoxLayout();
	hboxLayout->setContentsMargins(0, 0, 0, 0);
	hboxLayout->addSpacing(IconSpace);
	m_icon1 = addLabel(hboxLayout, NoIcon);
	m_icon1->setToolTip(tr("Part breadboard view image"));
	m_icon2 = addLabel(hboxLayout, NoIcon);
	m_icon2->setToolTip(tr("Part schematic view image"));
	m_icon3 = addLabel(hboxLayout, NoIcon);
	m_icon3->setToolTip(tr("Part pcb view image"));

	QVBoxLayout * versionLayout = new QVBoxLayout();

	QHBoxLayout * subVersionLayout = new QHBoxLayout();
	m_partVersion = new QLabel();
	m_partVersion->setObjectName("infoViewPartVersion");
	m_partVersion->setToolTip(tr("Part version number"));
    m_partVersion->setOpenExternalLinks(false);
    m_partVersion->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    connect(m_partVersion, SIGNAL(linkActivated(const QString &)), this, SLOT(clickObsolete(const QString &)));
	subVersionLayout->addWidget(m_partVersion, 0, Qt::AlignLeft);
	subVersionLayout->addStretch(1);
	versionLayout->addLayout(subVersionLayout);

	hboxLayout->addLayout(versionLayout);
	
	hboxLayout->addSpacerItem(new QSpacerItem(IconSpace, 1, QSizePolicy::Expanding));
	iconFrame->setLayout(hboxLayout);
	vlo->addWidget(iconFrame);

	m_partUrl = new TagLabel(this);
	m_partUrl->setWordWrap(false);
	m_partUrl->setObjectName("infoViewPartUrl");
	m_partUrl->setOpenExternalLinks(true);
	vlo->addWidget(m_partUrl);
    if (tinyMode) m_partUrl->setVisible(false);

	m_partTitle = new TagLabel(this);
	m_partTitle->setWordWrap(true);
	m_partTitle->setObjectName("infoViewPartTitle");
	m_partTitle->setOpenExternalLinks(true);
	vlo->addWidget(m_partTitle);
    if (tinyMode) m_partTitle->setVisible(false);

    m_placementLabel = new QLabel(tr("Placement"), NULL);
	m_placementLabel->setObjectName("expandableViewLabel");
	vlo->addWidget(m_placementLabel);

    m_placementFrame = new QFrame(this);
	m_placementFrame->setObjectName("infoViewPropertyFrame");
	m_placementLayout = new QGridLayout(m_placementFrame);
	m_placementLayout->setSpacing(0);
	m_placementLayout->setContentsMargins(0, 0, 0, 0);

	m_layerLabel = new QLabel(tr("pcb layer"), this);
	m_layerLabel->setObjectName("propNameLabel");
	m_layerLabel->setWordWrap(true);
	m_placementLayout->addWidget(m_layerLabel, 0, 0);

	m_layerFrame = new QFrame(this);
	m_layerFrame->setObjectName("propValueFrame");
	m_layerLayout = new QVBoxLayout(m_layerFrame);
	m_layerLayout->setSpacing(0);
	m_layerLayout->setContentsMargins(0, 0, 0, 0);
    m_layerFrame->setLayout(m_layerLayout);

    m_placementLayout->addWidget(m_layerFrame, 0, 1);

    makeLocationFrame();
    m_placementLayout->addWidget(m_locationLabel, 1, 0);
    m_placementLayout->addWidget(m_locationFrame, 1, 1);

    makeRotationFrame();
    m_placementLayout->addWidget(m_rotationLabel, 2, 0);
    m_placementLayout->addWidget(m_rotationFrame, 2, 1);

    makeLockFrame();
    m_placementLayout->addWidget(m_lockLabel, 3, 0);
    m_placementLayout->addWidget(m_lockFrame, 3, 1);


	m_placementFrame->setLayout(m_placementLayout);
	vlo->addWidget(m_placementFrame);

	m_propLabel = new QLabel(tr("Properties"), NULL);
	m_propLabel->setObjectName("expandableViewLabel");
	vlo->addWidget(m_propLabel);

	m_propFrame = new QFrame(this);
	m_propFrame->setObjectName("infoViewPropertyFrame");
	m_propLayout = new QGridLayout(m_propFrame);
	m_propLayout->setSpacing(0);
	m_propLayout->setContentsMargins(0, 0, 0, 0);
	m_propFrame->setLayout(m_propLayout);
	vlo->addWidget(m_propFrame);

	m_tagLabel = new QLabel(tr("Tags"), NULL);
	m_tagLabel->setObjectName("expandableViewLabel");
	vlo->addWidget(m_tagLabel);
    if (tinyMode) m_tagLabel->setVisible(false);

	m_tagsTextLabel = new TagLabel(this);
	m_tagsTextLabel->setWordWrap(true);
	m_tagsTextLabel->setObjectName("tagsValue");
	vlo->addWidget(m_tagsTextLabel);
    if (tinyMode) m_tagsTextLabel->setVisible(false);

	m_connLabel = new QLabel(tr("Connections"), NULL);
	m_connLabel->setObjectName("expandableViewLabel");
	vlo->addWidget(m_connLabel);
    if (tinyMode) m_connLabel->setVisible(false);

	m_connFrame = new QFrame(this);
	m_connFrame->setObjectName("connectionsFrame");

    QGridLayout * connLayout = new QGridLayout(m_connFrame);
	connLayout->setSpacing(0);
	connLayout->setContentsMargins(0, 0, 0, 0);
	m_connFrame->setLayout(connLayout);

	QLabel * descrLabel = new QLabel(tr("conn."), this);
	descrLabel->setObjectName("connectionsLabel");
	m_connDescr = new QLabel(this);
	m_connDescr->setObjectName("connectionsValue");
    connLayout->addWidget(descrLabel, 0, 0);
    connLayout->addWidget(m_connDescr, 0, 1);

	QLabel * nameLabel = new QLabel(tr("name"), this);
	nameLabel->setObjectName("connectionsLabel");
	m_connName = new QLabel(this);
	m_connName->setObjectName("connectionsValue");
    connLayout->addWidget(nameLabel, 1, 0);
    connLayout->addWidget(m_connName, 1, 1);

	QLabel * typeLabel = new QLabel(tr("type"), this);
	typeLabel->setObjectName("connectionsLabel");
	m_connType = new QLabel(this);
	m_connType->setObjectName("connectionsValue");
	connLayout->addWidget(typeLabel, 2, 0);
    connLayout->addWidget(m_connType, 2, 1);

	vlo->addWidget(m_connFrame);

    if (m_tinyMode) m_connFrame->setVisible(false);

	vlo->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));

	mainFrame->setLayout(vlo);

	this->setWidget(mainFrame);
}


void HtmlInfoView::cleanup() {
	if (NoIcon) {
		delete NoIcon;
		NoIcon = NULL;
	}
}

void HtmlInfoView::viewItemInfo(InfoGraphicsView *, ItemBase* item, bool swappingEnabled) 
{
	m_setContentTimer.stop();
	m_lastItemBase = m_pendingItemBase = item;
	m_lastSwappingEnabled = m_pendingSwappingEnabled = swappingEnabled;
	m_setContentTimer.start();
}

void HtmlInfoView::hoverEnterItem(InfoGraphicsView *, QGraphicsSceneHoverEvent *, ItemBase * item, bool swappingEnabled) {
	m_setContentTimer.stop();
	m_pendingItemBase = item;
	m_pendingSwappingEnabled = swappingEnabled;
	m_setContentTimer.start();
}

void HtmlInfoView::hoverLeaveItem(InfoGraphicsView *, QGraphicsSceneHoverEvent *, ItemBase * itemBase){
	Q_UNUSED(itemBase);
	//DebugDialog::debug(QString("hoverLeaveItem itembase %1").arg(itemBase ? itemBase->instanceTitle() : "NULL"));
	m_setContentTimer.stop();
	m_pendingItemBase = m_lastItemBase;
	m_pendingSwappingEnabled = m_lastSwappingEnabled;
	m_setContentTimer.start();
}

void HtmlInfoView::viewConnectorItemInfo(QGraphicsSceneHoverEvent *, ConnectorItem * connectorItem) {

	int count = connectorItem ? connectorItem->connectionsCount() : 0;
	if (m_lastConnectorItem == connectorItem && m_lastConnectorItemCount == count) return;

	m_lastConnectorItem = connectorItem;
	m_lastConnectorItemCount = count;

	Connector * connector = NULL;
	if (connectorItem) {
		if (connectorItem->attachedTo() != m_lastItemBase) {
			return;
		}

		connector = connectorItem->connector();
	}

	if (m_connDescr) {
		m_connDescr->setText(connectorItem ? tr("connected to %n item(s)", "", connectorItem->connectionsCount()) : "");
		m_connName->setText(connector ? connector->connectorSharedName() : "");
		m_connType->setText(connector ? Connector::connectorNameFromType(connector->connectorType()) : "");
	}

}

void HtmlInfoView::hoverEnterConnectorItem(InfoGraphicsView *igv, QGraphicsSceneHoverEvent *event, ConnectorItem * item, bool swappingEnabled) {
	Q_UNUSED(event)
	Q_UNUSED(swappingEnabled)
	Q_UNUSED(igv)
	viewConnectorItemInfo(event, item);
}

void HtmlInfoView::hoverLeaveConnectorItem(InfoGraphicsView *igv, QGraphicsSceneHoverEvent *event, ConnectorItem *connItem) {
	Q_UNUSED(event);
	Q_UNUSED(connItem);
	Q_UNUSED(igv);
	viewConnectorItemInfo(event, NULL);
}

void HtmlInfoView::appendStuff(ItemBase* item, bool swappingEnabled) {
	Wire *wire = qobject_cast<Wire*>(item);
	if (wire) {
		appendWireStuff(wire, swappingEnabled);
	} else {
		appendItemStuff(item, swappingEnabled);
	}
}

void HtmlInfoView::appendWireStuff(Wire* wire, bool swappingEnabled) {
	if (wire == NULL) return;

	ModelPart *modelPart = wire->modelPart();
	if (modelPart == NULL) return;
	if (modelPart->modelPartShared() == NULL) return;

	QString autoroutable = wire->getAutoroutable() ? tr("(autoroutable)") : "";
	QString nameString = tr("Wire");
	if (swappingEnabled) {
		if (wire->getRatsnest()) {
			nameString = tr("Ratsnest wire");
		} 
		else if(wire->getTrace()) {
			nameString = tr("Trace wire %1").arg(autoroutable);
		} 
	}
	else {
		 nameString = modelPart->description();
	}
	partTitle(nameString, modelPart->version(), modelPart->url(), modelPart->isObsolete());
    m_lockFrame->setVisible(false);
    m_lockLabel->setVisible(false);
    m_locationFrame->setVisible(false);
    m_locationLabel->setVisible(false);
    m_rotationFrame->setVisible(false);
    m_rotationLabel->setVisible(false);

	setUpTitle(wire);
	setUpIcons(wire, swappingEnabled);

	displayProps(modelPart, wire, swappingEnabled);

    bool hasLayer = (wire->viewID() == ViewLayer::PCBView) && wire->getViewGeometry().getPCBTrace();
    m_placementFrame->setVisible(hasLayer);
	m_placementLabel->setVisible(hasLayer);

	addTags(modelPart);
}

void HtmlInfoView::appendItemStuff(ItemBase* base, bool swappingEnabled) {
	if (base == NULL) return;

	appendItemStuff(base, base->modelPart(), swappingEnabled);
}

void HtmlInfoView::appendItemStuff(ItemBase * itemBase, ModelPart * modelPart, bool swappingEnabled) {

	if (modelPart == NULL) return;
	if (modelPart->modelPartShared() == NULL) return;

	setUpTitle(itemBase);
	setUpIcons(itemBase, swappingEnabled);

	QString nameString;
	if (swappingEnabled) {
		nameString = (itemBase) ? itemBase->getInspectorTitle() : modelPart->title();
	}
	else {
		nameString = modelPart->description();
	}
	partTitle(nameString, modelPart->version(), modelPart->url(), modelPart->isObsolete());
    m_lockFrame->setVisible(swappingEnabled);
    m_lockLabel->setVisible(swappingEnabled);
	m_lockCheckbox->setChecked(itemBase->moveLock());

    m_locationFrame->setVisible(swappingEnabled);
    m_locationLabel->setVisible(swappingEnabled);
    m_locationFrame->setDisabled(itemBase->moveLock());

    m_rotationFrame->setVisible(swappingEnabled);
    m_rotationLabel->setVisible(swappingEnabled);
    m_rotationFrame->setEnabled(itemBase->rotationAllowed());

    setLocation(itemBase);
    setRotation(itemBase);

    if (itemBase->isBaseSticky()) {
	    m_stickyCheckbox->setVisible(true);
        m_stickyCheckbox->setChecked(itemBase->isSticky());
    }
    else {
	    m_stickyCheckbox->setVisible(false);
    }

	displayProps(modelPart, itemBase, swappingEnabled);
	addTags(modelPart);

    m_placementLabel->setVisible(swappingEnabled);
    m_placementFrame->setVisible(swappingEnabled);

}

void HtmlInfoView::setContent()
{
	m_setContentTimer.stop();
	//DebugDialog::debug(QString("start updating %1").arg(QTime::currentTime().toString("HH:mm:ss.zzz")));
	if (m_pendingItemBase == NULL) {
		setNullContent();
		m_setContentTimer.stop();
		return;
	}

	//DebugDialog::debug(QString("pending %1").arg(m_pendingItemBase->title()));
	m_currentSwappingEnabled = m_pendingSwappingEnabled;

	appendStuff(m_pendingItemBase, m_pendingSwappingEnabled);
	setCurrentItem(m_pendingItemBase);

    if (!m_tinyMode) {
	    m_connFrame->setVisible(m_pendingSwappingEnabled);
	    m_tagLabel->setVisible(true);
	    m_connLabel->setVisible(m_pendingSwappingEnabled);
    }
	m_propLabel->setVisible(true);
	m_propFrame->setVisible(true);

	m_setContentTimer.stop();
	//DebugDialog::debug(QString("end   updating %1").arg(QTime::currentTime().toString("HH:mm:ss.zzz")));

}

QSize HtmlInfoView::sizeHint() const {
	return QSize(DockWidthDefault, InfoViewHeightDefault);
}

void HtmlInfoView::setCurrentItem(ItemBase * item) {
	m_currentItem = item;
}

void HtmlInfoView::unregisterCurrentItem() {
        m_setContentTimer.stop();
	setCurrentItem(NULL);
        m_pendingItemBase = NULL;
        m_setContentTimer.start();
}

void HtmlInfoView::unregisterCurrentItemIf(long id) {
	if (m_currentItem == NULL) {
		return;
	}
	if (m_currentItem->id() == id) {
		unregisterCurrentItem();
	}
}

ItemBase *HtmlInfoView::currentItem() {
	return m_currentItem;
}

void HtmlInfoView::reloadContent(InfoGraphicsView * infoGraphicsView) {
	if(m_currentItem) {
		viewItemInfo(infoGraphicsView, m_currentItem, m_currentSwappingEnabled);
	}
}

void HtmlInfoView::setNullContent()
{
	setUpTitle(NULL);
	partTitle("", "", "", false);
	setUpIcons(NULL, false);
	displayProps(NULL, NULL, false);
	addTags(NULL);
	viewConnectorItemInfo(NULL, NULL);
	m_connFrame->setVisible(false);
	m_propFrame->setVisible(false);
	m_propLabel->setVisible(false);
	m_placementFrame->setVisible(false);
	m_placementLabel->setVisible(false);
	m_tagLabel->setVisible(false);
	m_connLabel->setVisible(false);
}

void HtmlInfoView::setInstanceTitle() {
	FLineEdit * edit = qobject_cast<FLineEdit *>(sender());
	if (edit == NULL) return;
	if (!edit->isEnabled()) return;
	if (m_currentItem == NULL) return;

    m_currentItem->setInspectorTitle(m_partTitle->text(), edit->text());
}

void HtmlInfoView::instanceTitleEnter() {
	FLineEdit * edit = qobject_cast<FLineEdit *>(sender());
	if (edit->isEnabled()) {
		setInstanceTitleColors(edit, QColor(0xeb, 0xeb, 0xee), QColor(0x00, 0x00, 0x00)); //c8c8c8, 575757
	}
}

void HtmlInfoView::instanceTitleLeave() {
	FLineEdit * edit = qobject_cast<FLineEdit *>(sender());
	if (edit->isEnabled()) {
		setInstanceTitleColors(edit, QColor(0xd2, 0xd2, 0xd7), QColor(0x00, 0x00, 0x00)); //b3b3b3, 575757
	}
}

void HtmlInfoView::instanceTitleEditable(bool editable) {
	FLineEdit * edit = qobject_cast<FLineEdit *>(sender());
	if (editable) {
		setInstanceTitleColors(edit, QColor(0xff, 0xff, 0xff), QColor(0x00, 0x00, 0x00)); //fcfcfc, 000000
	}
	else {
		setInstanceTitleColors(edit, QColor(0xd2, 0xd2, 0xd7), QColor(0x00, 0x00, 0x00)); //b3b3b3, 57575
	}
}

void HtmlInfoView::setInstanceTitleColors(FLineEdit * edit, const QColor & base, const QColor & text) {
	edit->setStyleSheet(QString("background: rgb(%1,%2,%3); color: rgb(%4,%5,%6);")
		.arg(base.red()).arg(base.green()).arg(base.blue())
		.arg(text.red()).arg(text.green()).arg(text.blue()) );
}

void HtmlInfoView::setUpTitle(ItemBase * itemBase) 
{
	if (itemBase == m_lastTitleItemBase) {
        if (itemBase == NULL) return;
        if (itemBase->instanceTitle().compare(m_titleEdit->text()) == 0) return;
    }

	m_lastTitleItemBase = itemBase;
    bool titleEnabled = true;
	if (itemBase != NULL) {
		QString title = itemBase->getInspectorTitle();
		if (title.isEmpty()) {
			// assumes a part with an empty title only comes from the parts bin palette
			titleEnabled = false;
			title = itemBase->title();
		}
        if (itemBase->viewID() == ViewLayer::IconView) titleEnabled = false;
		m_titleEdit->setText(title);
	}
	else {
		titleEnabled = false;
		m_titleEdit->setText("");
	}
    m_titleEdit->setEnabled(titleEnabled);
	// helps keep it left aligned?
	m_titleEdit->setCursorPosition(0);

}

void HtmlInfoView::setUpIcons(ItemBase * itemBase, bool swappingEnabled) {
	if (m_lastIconItemBase == itemBase) return;
	
	m_lastIconItemBase = itemBase;

	QPixmap *pixmap1 = NULL;
	QPixmap *pixmap2 = NULL;
	QPixmap *pixmap3 = NULL;

    QSize size = NoIcon->size();

    if (itemBase) {
        itemBase->getPixmaps(pixmap1, pixmap2, pixmap3, swappingEnabled, size);
    }

	QPixmap* use1 = pixmap1;
	QPixmap* use2 = pixmap2;
	QPixmap* use3 = pixmap3;

	if (use1 == NULL) use1 = NoIcon;
	if (use2 == NULL) use2 = NoIcon;
	if (use3 == NULL) use3 = NoIcon;

	m_icon1->setPixmap(*use1);
	m_icon2->setPixmap(*use2);
	m_icon3->setPixmap(*use3);

    if (pixmap1 != NULL) delete pixmap1;
    if (pixmap2 != NULL) delete pixmap2;
    if (pixmap3 != NULL) delete pixmap3;
}

void HtmlInfoView::addTags(ModelPart * modelPart) {
	if (m_tagsTextLabel == NULL) return;

	if (m_lastTagsModelPart == modelPart) return;

	m_lastTagsModelPart = modelPart;

	if (modelPart == NULL || modelPart->tags().isEmpty()) {
		m_tagsTextLabel->setText("");
		return;
	}

	m_tagsTextLabel->setText(modelPart->tags().join(", "));
}

void HtmlInfoView::partTitle(const QString & title, const QString & version, const QString & url, bool isObsolete) {
	if (m_partTitle == NULL) return;

	if (m_lastPartTitle == title && m_lastPartVersion == version) return;

	m_lastPartTitle = title;
	m_lastPartVersion = version;

    if (!m_tinyMode) {
	    if (url.isEmpty()) {
		    m_partUrl->setVisible(false);	
		    m_partUrl->setText("");
	    }
	    else {
		    m_partUrl->setText(QString("<a href=\"%1\">%1</a>").arg(url));
		    m_partUrl->setVisible(true);
	    }
    }

	m_partTitle->setText(title);
	if (!version.isEmpty()) {
		m_partVersion->setText(tr("v. %1 %2").arg(version).arg(isObsolete ? QString("<a href='x'>%1</a>").arg(tr("obsolete")) : ""));
	}
	else m_partVersion->setText("");
}

void HtmlInfoView::displayProps(ModelPart * modelPart, ItemBase * itemBase, bool swappingEnabled) 
{
	bool repeatPossible = (modelPart == m_lastPropsModelPart && itemBase == m_lastPropsItemBase && swappingEnabled == m_lastPropsSwappingEnabled);
	if (repeatPossible && modelPart == NULL && itemBase == NULL) {
		//DebugDialog::debug("display props bail");
		return;
	}

	m_propLayout->setEnabled(false);

	if (repeatPossible) {
		DebugDialog::debug(QString("repeat possible %1").arg(repeatPossible));
	}

	bool wantDebug = false;
#ifndef QT_NO_DEBUG
	wantDebug = true;
#endif

	QStringList keys;
	QHash<QString, QString> properties = getPartProperties(modelPart, itemBase, wantDebug, keys);
	QString family = properties.value("family", "").toLower();

    bool sl = false;
    if (keys.contains("layer")) {
        keys.removeOne("layer");
        sl = (itemBase->viewID() == ViewLayer::PCBView);
    }
    
    showLayers(sl, itemBase, family, properties.value("layer", ""), swappingEnabled);

	int ix = 0;
	foreach(QString key, keys) {
		if (ix >= m_propThings.count()) {
			//DebugDialog::debug(QString("new prop thing %1").arg(ix));
			PropThing * propThing = new PropThing;
			propThing->m_plugin = NULL;
			m_propThings.append(propThing);

			QLabel * propNameLabel = new QLabel(this);
			propNameLabel->setObjectName("propNameLabel");
			propNameLabel->setWordWrap(true);
			propThing->m_name = propNameLabel;
			m_propLayout->addWidget(propNameLabel, ix, 0);

			QFrame * valueFrame = new QFrame(this);
			valueFrame->setObjectName("propValueFrame");
			QVBoxLayout * vlayout = new QVBoxLayout(valueFrame);
			vlayout->setSpacing(0);
			vlayout->setContentsMargins(0, 0, 0, 0);
			propThing->m_layout = vlayout;
			propThing->m_frame = valueFrame;

			QLabel * propValueLabel = new QLabel(valueFrame);
			propValueLabel->setObjectName("propValueLabel");
			propValueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
			vlayout->addWidget(propValueLabel);
			propThing->m_value = propValueLabel;
			m_propLayout->addWidget(valueFrame, ix, 1);
		}

		PropThing * propThing = m_propThings.at(ix++);

		QWidget * oldPlugin = propThing->m_plugin;
		propThing->m_plugin = NULL;

		QString value = properties.value(key,"");
		QString translatedName = ItemBase::translatePropertyName(key);
		QString resultKey, resultValue;
		QWidget * resultWidget = oldPlugin;
		bool result = false;
        bool hide = false;
		if (itemBase != NULL) {
			result = itemBase->collectExtraInfo(propThing->m_name->parentWidget(), family, key, value, swappingEnabled, resultKey, resultValue, resultWidget, hide);
		}

		QString newName;
		QString newValue;
		QWidget * newWidget = NULL;
		if (result && !hide) {
			newName = resultKey;
			if (resultWidget) {
				newWidget = resultWidget;
				if (resultWidget != oldPlugin) {
					propThing->m_layout->addWidget(resultWidget);
				}
				else {
					oldPlugin = NULL;
				}
                //DebugDialog::debug(QString("adding %1 %2").arg(newName).arg((long) resultWidget, 0, 16));
				propThing->m_plugin = resultWidget;
			}
			else {
				newValue = resultValue;
			}
		}
		else {
			newName = translatedName;
			newValue = value;
		}
		
		if (oldPlugin) {
			clearPropThingPlugin(propThing, oldPlugin);
		}

		if (propThing->m_name->text().compare(newName) != 0) {
			propThing->m_name->setText(newName);
		}

		propThing->m_name->setVisible(!hide);
		propThing->m_frame->setVisible(!hide);

		if (newWidget == NULL && propThing->m_value->text().compare(newValue) != 0) {
			propThing->m_value->setText(newValue);
		}
		propThing->m_value->setVisible(newWidget == NULL && !hide);
	}

	for (int jx = ix; jx < m_propThings.count(); jx++) {
		PropThing * propThing = m_propThings.at(jx);
		propThing->m_name->setVisible(false);
		propThing->m_value->setVisible(false);
		propThing->m_frame->setVisible(false);
		if (propThing->m_plugin) {
			propThing->m_plugin->setVisible(false);
		}
	}

	m_propLayout->setEnabled(true);


	/*
	foreach (PropThing * propThing, m_propThings) {
		if (propThing->m_layout->count() > 1) {
			DebugDialog::debug(QString("too many %1").arg(propThing->m_layout->count()));
		}
	}
	*/
}

void HtmlInfoView::clearPropThingPlugin(PropThing * propThing) 
{

	if (propThing->m_plugin) {
		clearPropThingPlugin(propThing, propThing->m_plugin);
		propThing->m_plugin = NULL;		
	}
}

void HtmlInfoView::clearPropThingPlugin(PropThing * propThing, QWidget * plugin) 
{
    //DebugDialog::debug(QString("clearing %1").arg((long) plugin, 0, 16));

	propThing->m_layout->removeWidget(plugin);
    plugin->blockSignals(true);     
	plugin->setVisible(false);          // seems to trigger an unwanted focus out signal
	plugin->deleteLater();
}


QHash<QString, QString> HtmlInfoView::getPartProperties(ModelPart * modelPart, ItemBase * itemBase, bool wantDebug, QStringList & keys) 
{
	QHash<QString, QString> properties;
	QString family;
	QString partNumber;
	if (modelPart != NULL && itemBase != NULL) {
        properties = itemBase->prepareProps(modelPart, wantDebug, keys);
	}

	return properties;
}

void HtmlInfoView::changeLock(bool lockState)
{
	if (m_currentItem == NULL) return;
	if (m_currentItem->itemType() == ModelPart::Wire) return;

	m_currentItem->setMoveLock(lockState);
    m_locationFrame->setDisabled(lockState);
    m_rotationFrame->setDisabled(lockState);
}


void HtmlInfoView::changeSticky(bool lockState)
{
	if (m_currentItem == NULL) return;
	if (m_currentItem->itemType() == ModelPart::Wire) return;

	m_currentItem->setLocalSticky(lockState);
}

void HtmlInfoView::clickObsolete(const QString &) {
    emit clickObsoleteSignal();
}

void HtmlInfoView::showLayers(bool show, ItemBase * itemBase, const QString & family, const QString & value, bool swappingEnabled) {

    if (m_layerWidget) {
	    m_layerLayout->removeWidget(m_layerWidget);
        m_layerWidget->blockSignals(true);     
	    m_layerWidget->setVisible(false);          // seems to trigger an unwanted focus out signal
	    m_layerWidget->deleteLater();
        m_layerWidget = NULL;
    }

    if (itemBase == NULL) show = false;
    m_layerFrame->setVisible(show);
    m_layerLabel->setVisible(show);
    if (!show) return;

    QString resultKey, resultValue;
    bool hide;
    bool result = itemBase->collectExtraInfo(m_layerLabel->parentWidget(), family, "layer", value, swappingEnabled, resultKey, resultValue, m_layerWidget, hide);
    if (result && m_layerWidget != NULL) {
        m_layerLayout->addWidget(m_layerWidget);
    }
}

void HtmlInfoView::makeLockFrame() {
	m_lockLabel = new QLabel("    ", this);
	m_lockLabel->setObjectName("propNameLabel");
	m_lockLabel->setWordWrap(true);

    m_lockFrame = new QFrame(this);
    QHBoxLayout * lockLayout = new QHBoxLayout();
	lockLayout->setSpacing(0);
	lockLayout->setContentsMargins(0, 0, 0, 0);
    m_lockFrame->setObjectName("propValueFrame");

	m_lockCheckbox = new QCheckBox(tr("Locked"));
	m_lockCheckbox->setObjectName("infoViewLockCheckbox");
	m_lockCheckbox->setToolTip(tr("Change the locked state of the part in this view. A locked part can't be moved."));
	connect(m_lockCheckbox, SIGNAL(clicked(bool)), this, SLOT(changeLock(bool)));
	lockLayout->addWidget(m_lockCheckbox);

    lockLayout->addSpacing(10);

	m_stickyCheckbox = new QCheckBox(tr("Sticky"));
	m_stickyCheckbox->setObjectName("infoViewLockCheckbox");
	m_stickyCheckbox->setToolTip(tr("Change the \"sticky\" state of the part in this view. When a sticky part is moved, objects on top of it also move."));
	connect(m_stickyCheckbox, SIGNAL(clicked(bool)), this, SLOT(changeSticky(bool)));
	lockLayout->addWidget(m_stickyCheckbox);

    lockLayout->addSpacerItem(new QSpacerItem(1,1, QSizePolicy::Expanding));

    m_lockFrame->setLayout(lockLayout);
}

void HtmlInfoView::makeLocationFrame() {
	m_locationLabel = new QLabel(tr("location"), this);
	m_locationLabel->setObjectName("propNameLabel");
	m_locationLabel->setWordWrap(true);

    m_locationFrame = new QFrame(this);
    QHBoxLayout * locationLayout = new QHBoxLayout();
	locationLayout->setSpacing(0);
	locationLayout->setContentsMargins(0, 0, 0, 0);
    m_locationFrame->setObjectName("propValueFrame");

	m_xEdit = new QDoubleSpinBox;
    m_xEdit->setDecimals(3);
    m_xEdit->setRange(-99999.999, 99999.999);
    m_xEdit->setKeyboardTracking(false);
    m_xEdit->setObjectName("infoViewDoubleSpinBox");
    m_xEdit->setMaximumWidth(MaxSpinBoxWidth);
    m_xEdit->setMinimumWidth(MaxSpinBoxWidth);
    m_xEdit->setLocale(QLocale::C);
    locationLayout->addWidget(m_xEdit);

    locationLayout->addSpacing(AfterSpinBoxWidth);

	m_yEdit = new QDoubleSpinBox;
    m_yEdit->setDecimals(3);
    m_yEdit->setRange(-99999.999, 99999.999);
    m_yEdit->setKeyboardTracking(false);
    m_yEdit->setObjectName("infoViewDoubleSpinBox");
    m_yEdit->setMaximumWidth(MaxSpinBoxWidth);
    m_yEdit->setMinimumWidth(MaxSpinBoxWidth);
    m_yEdit->setLocale(QLocale::C);
    locationLayout->addWidget(m_yEdit);

    locationLayout->addSpacing(3);

	m_unitsLabel = new ClickableLabel("px", this);
    m_unitsLabel->setObjectName("infoViewSpinBoxLabel");
    m_unitsLabel->setCursor(Qt::PointingHandCursor);
    locationLayout->addWidget(m_unitsLabel);

    locationLayout->addSpacerItem(new QSpacerItem(1,1, QSizePolicy::Expanding));

    m_locationFrame->setLayout(locationLayout);

    connect(m_xEdit, SIGNAL(valueChanged(double)), this, SLOT(xyEntry()));
    connect(m_yEdit, SIGNAL(valueChanged(double)), this, SLOT(xyEntry()));
	connect(m_unitsLabel, SIGNAL(clicked()), this, SLOT(unitsClicked()));
    unitsClicked();   // increments from px so that default is inches
}

void HtmlInfoView::makeRotationFrame() {
	m_rotationLabel = new QLabel(tr("rotation"), this);
	m_rotationLabel->setObjectName("propNameLabel");
	m_rotationLabel->setWordWrap(true);

    m_rotationFrame = new QFrame(this);
    QHBoxLayout * rotationLayout = new QHBoxLayout();
	rotationLayout->setSpacing(0);
	rotationLayout->setContentsMargins(0, 0, 0, 0);
    m_rotationFrame->setObjectName("propValueFrame");

	m_rotEdit = new QDoubleSpinBox;
    m_rotEdit->setDecimals(1);
    m_rotEdit->setRange(-360, 360);
    m_rotEdit->setKeyboardTracking(false);
    m_rotEdit->setObjectName("infoViewDoubleSpinBox");
    m_rotEdit->setMaximumWidth(MaxSpinBoxWidth);
    m_rotEdit->setMinimumWidth(MaxSpinBoxWidth);
    m_rotEdit->setLocale(QLocale::C);
    rotationLayout->addWidget(m_rotEdit);

    rotationLayout->addSpacing(AfterSpinBoxWidth);

	QLabel * label = new QLabel(tr("degrees"), this);
    label->setObjectName("infoViewSpinBoxLabel");
    rotationLayout->addWidget(label);

    rotationLayout->addSpacerItem(new QSpacerItem(1,1, QSizePolicy::Expanding));

    m_rotationFrame->setLayout(rotationLayout);

    connect(m_rotEdit, SIGNAL(valueChanged(double)), this, SLOT(rotEntry()));
}

void HtmlInfoView::unitsClicked() {
    QString units = m_unitsLabel->text();

    QString xs = m_xEdit->text();
    QString ys = m_yEdit->text();
    double x = TextUtils::convertToInches(xs + units);
    double y = TextUtils::convertToInches(ys + units);

    double dpi = 1;
    double step = 1;
	if (units.compare("mm") == 0) {
		units = "px";
        dpi = 90;
        step = 1;
	}
	else if (units.compare("px") == 0) {
		units = "in";
        dpi = 1;
        step = 0.1;
	}
	else if (units.compare("in") == 0) {
		units = "mm";
        dpi = 25.4;
        step = 1;
	}
	else {
		units = "in";
        dpi = 1;
        step = 0.1;
	}
    m_unitsLabel->setText(units);

    m_xEdit->blockSignals(true);
    m_yEdit->blockSignals(true);
    m_xEdit->setSingleStep(step);
    m_yEdit->setSingleStep(step);
    if (!xs.isEmpty()) m_xEdit->setValue(x * dpi);
    if (!ys.isEmpty()) m_yEdit->setValue(y * dpi);
    m_xEdit->blockSignals(false);
    m_yEdit->blockSignals(false);
}

void HtmlInfoView::setLocation(ItemBase * itemBase) {
    if (itemBase == NULL) {
        m_xEdit->blockSignals(true);
        m_yEdit->blockSignals(true);
        m_xEdit->setEnabled(false);
        m_yEdit->setEnabled(false);
        m_xEdit->setValue(0);
        m_yEdit->setValue(0);
        m_xEdit->blockSignals(false);
        m_yEdit->blockSignals(false);
        return;
    }

    m_xEdit->setEnabled(!itemBase->moveLock());
    m_yEdit->setEnabled(!itemBase->moveLock());

    QString units = m_unitsLabel->text();
    QPointF loc = itemBase->pos();
    if (units == "px") {
    }
    else if (units == "in") {
        loc /= 90;
    }
    else if (units == "mm") {
        loc /= (90 / 25.4);
    }

    m_xEdit->blockSignals(true);
    m_yEdit->blockSignals(true);
    m_xEdit->setValue(loc.x());
    m_yEdit->setValue(loc.y());
    m_xEdit->blockSignals(false);
    m_yEdit->blockSignals(false);
}

void HtmlInfoView::setRotation(ItemBase * itemBase) {
    if (itemBase == NULL) {
        m_rotEdit->blockSignals(true);
        m_rotEdit->setEnabled(false);
        m_rotEdit->setValue(0);
        m_rotEdit->blockSignals(false);
        return;
    }

    double step = 90;
    bool enabled = true;
    if (itemBase->moveLock()) enabled = false;
    else if (!itemBase->rotationAllowed()) enabled = false;
    else {
        if (itemBase->rotation45Allowed()) step = 45;
        if (itemBase->freeRotationAllowed()) step = 1;
    }

    m_rotEdit->setEnabled(enabled);
    m_rotEdit->setSingleStep(step);

    QTransform transform = itemBase->transform();
    double angle = atan2(transform.m12(), transform.m11()) * 180 / M_PI;

    m_rotEdit->blockSignals(true);
    m_rotEdit->setValue(angle);
    m_rotEdit->blockSignals(false);
}

void HtmlInfoView::updateLocation(ItemBase * itemBase) {
    if (itemBase == NULL) return;
    if (itemBase != m_lastItemBase) return;

    setLocation(itemBase);
}

void HtmlInfoView::updateRotation(ItemBase * itemBase) {
    if (itemBase == NULL) return;
    if (itemBase != m_lastItemBase) return;

    setRotation(itemBase);
}

void HtmlInfoView::xyEntry() {
    InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(m_lastItemBase);
    if (infoGraphicsView == NULL) return;

    DebugDialog::debug(QString("xedit %1 %2 %3").arg(m_xEdit->text()).arg(m_yEdit->text()).arg(sender() == m_xEdit));
    double x = TextUtils::convertToInches(m_xEdit->text() + m_unitsLabel->text());
    double y = TextUtils::convertToInches(m_yEdit->text() + m_unitsLabel->text());
    if (infoGraphicsView != NULL && m_lastItemBase != NULL) {
        infoGraphicsView->moveItem(m_lastItemBase, x * 90, y * 90);
    }
}

void HtmlInfoView::rotEntry() {
    if (m_lastItemBase != NULL) {
        InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(m_lastItemBase);
        if (infoGraphicsView == NULL) return;

        double newAngle = m_rotEdit->value();

        if (m_rotEdit->singleStep() == 1) {
        }
        else {
            newAngle = qFloor(newAngle / m_rotEdit->singleStep()) * m_rotEdit->singleStep();
        }

        QTransform transform = m_lastItemBase->transform();
        double angle = atan2(transform.m12(), transform.m11()) * 180 / M_PI;
        infoGraphicsView->rotateX(newAngle - angle, false, m_lastItemBase);
    }
}

