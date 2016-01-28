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

$Revision: 6904 $:
$Author: irascibl@gmail.com $:
$Date: 2013-02-26 16:26:03 +0100 (Di, 26. Feb 2013) $

********************************************************************/

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QtAlgorithms>

#include "partseditorviewswidget.h"
#include "partseditormainwindow.h"
#include "zoomcontrols.h"
#include "../utils/misc.h"
#include "../waitpushundostack.h"
#include "../debugdialog.h"

QString PartsEditorViewsWidget::EmptyBreadViewText;
QString PartsEditorViewsWidget::EmptySchemViewText;
QString PartsEditorViewsWidget::EmptyPcbViewText;

PartsEditorViewsWidget::PartsEditorViewsWidget(SketchModel *sketchModel, WaitPushUndoStack *undoStack, ConnectorsInfoWidget* info, QWidget *parent, ItemBase * fromItem) : QFrame(parent) {
	init();

	m_showTerminalPointsCheckBox = new QCheckBox(this);
	m_showTerminalPointsCheckBox->setText(tr("Show Anchor Points"));
	connect(m_showTerminalPointsCheckBox, SIGNAL(stateChanged(int)), this, SLOT(showHideTerminalPoints(int)));

	m_breadView = createViewImageWidget(sketchModel, undoStack, ViewLayer::BreadboardView, "breadboard_icon.png", EmptyBreadViewText, info, ViewLayer::Breadboard, fromItem);
	m_schemView = createViewImageWidget(sketchModel, undoStack, ViewLayer::SchematicView, "schematic_icon.png", EmptySchemViewText, info, ViewLayer::Schematic, fromItem);
	m_pcbView = createViewImageWidget(sketchModel, undoStack, ViewLayer::PCBView, "pcb_icon.png", EmptyPcbViewText, info, ViewLayer::Copper0, fromItem);

	m_breadView->setViewLayerIDs(ViewLayer::Breadboard, ViewLayer::BreadboardWire, ViewLayer::Breadboard, ViewLayer::BreadboardRuler, ViewLayer::BreadboardNote);
	m_schemView->setViewLayerIDs(ViewLayer::Schematic, ViewLayer::SchematicWire, ViewLayer::Schematic, ViewLayer::SchematicRuler, ViewLayer::SchematicNote);
	m_pcbView->setViewLayerIDs(ViewLayer::Schematic, ViewLayer::SchematicWire, ViewLayer::Schematic, ViewLayer::SchematicRuler, ViewLayer::PcbNote);

	connectPair(m_breadView,m_schemView);
	connectPair(m_schemView,m_pcbView);
	connectPair(m_pcbView,m_breadView);

	connectToThis(m_breadView);
	connectToThis(m_schemView);
	connectToThis(m_pcbView);


	m_viewsContainter = new QSplitter(this);
	m_viewsContainter->addWidget(addZoomControlsAndBrowseButton(m_breadView));
	m_viewsContainter->addWidget(addZoomControlsAndBrowseButton(m_schemView));
	m_viewsContainter->addWidget(addZoomControlsAndBrowseButton(m_pcbView));
	m_viewsContainter->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

	m_guidelines = new QLabel(tr("Please refer to the <a style='color: #52182C' href='http://fritzing.org/learning/tutorials/creating-custom-parts/'>guidelines</a> before modifying or creating parts"), this);
	m_guidelines->setOpenExternalLinks(true);
	m_guidelines->setObjectName("guidelinesLabel");

	QHBoxLayout *labelLayout = new QHBoxLayout();
	labelLayout->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::MinimumExpanding,QSizePolicy::Fixed));
	labelLayout->addWidget(m_guidelines);
	labelLayout->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::MinimumExpanding,QSizePolicy::Fixed));

	QFrame *toolsAndInfoContainer = new QFrame(this);
	QHBoxLayout *layout2 = new QHBoxLayout(toolsAndInfoContainer);
	layout2->addLayout(labelLayout);
	layout2->addWidget(m_showTerminalPointsCheckBox);
	layout2->setMargin(1);
	layout2->setSpacing(1);

	QVBoxLayout *lo = new QVBoxLayout(this);
	lo->addWidget(m_viewsContainter);
	lo->addWidget(toolsAndInfoContainer);
	lo->setMargin(3);
	lo->setSpacing(1);

	this->resize(width(),220);
	//this->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
}

PartsEditorViewsWidget::~PartsEditorViewsWidget() {
	if (m_breadView) delete m_breadView;
	if (m_schemView) delete m_schemView;
	if (m_pcbView) delete m_pcbView;
}

void PartsEditorViewsWidget::init() {
	m_connsPosChanged = false;

	if(EmptyBreadViewText.isEmpty()) {
		EmptyBreadViewText = tr("What does this\npart look like on\nthe breadboard?");
	}
	if(EmptySchemViewText.isEmpty()) {
		EmptySchemViewText = tr("What does this\npart look like in\na schematic view?");
	}
	if(EmptyPcbViewText.isEmpty()) {
		EmptyPcbViewText = tr("What does this\npart look like in\nthe PCB view?");
	}
}

PartsEditorView * PartsEditorViewsWidget::createViewImageWidget(
		SketchModel* sketchModel, WaitPushUndoStack *undoStack,
		ViewLayer::ViewIdentifier viewId, QString iconFileName, QString startText,
		ConnectorsInfoWidget* info, ViewLayer::ViewLayerID viewLayerId, ItemBase * fromItem) 
{

	PartsEditorView * viw = new PartsEditorView(viewId,tempDir(),showingTerminalPoints(),PartsEditorMainWindow::emptyViewItem(iconFileName,startText),this, 150, false, fromItem);
	viw->setSketchModel(sketchModel);
	viw->setUndoStack(undoStack);
	viw->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	viw->addViewLayer(new ViewLayer(viewLayerId, true, 2.5));

	m_views[viewId] = viw;

	connect(
		info, SIGNAL(connectorSelected(const QString&)),
		viw, SLOT(informConnectorSelection(const QString&))
	);
	connect(
		viw, SIGNAL(connectorsFoundSignal(ViewLayer::ViewIdentifier, const QList< QPointer<Connector> > &)),
		info, SLOT(syncNewConnectors(ViewLayer::ViewIdentifier, const QList< QPointer<Connector> > &))
	);
	connect(
		info, SIGNAL(existingConnectorSignal(ViewLayer::ViewIdentifier, const QString &, Connector*, Connector*)),
		viw, SLOT(checkConnectorLayers(ViewLayer::ViewIdentifier, const QString &, Connector*, Connector*))
	);

	connect(
		info, SIGNAL(setMismatching(ViewLayer::ViewIdentifier, const QString &, bool)),
		viw, SLOT(setMismatching(ViewLayer::ViewIdentifier, const QString &, bool))
	);

	return viw;
}

void PartsEditorViewsWidget::copySvgFilesToDestiny(const QString &partFileName) {
	m_breadView->copySvgFileToDestiny(partFileName);
	m_schemView->copySvgFileToDestiny(partFileName);
	m_pcbView->copySvgFileToDestiny(partFileName);
}

void PartsEditorViewsWidget::loadViewsImagesFromModel(PaletteModel *paletteModel, ModelPart *modelPart) {
	m_breadView->scene()->clear();
	m_breadView->setPaletteModel(paletteModel);
	m_breadView->loadFromModel(paletteModel, modelPart);

	m_schemView->scene()->clear();
	m_schemView->setPaletteModel(paletteModel);
	m_schemView->loadFromModel(paletteModel, modelPart);

	m_pcbView->scene()->clear();
	m_pcbView->setPaletteModel(paletteModel);
	m_pcbView->loadFromModel(paletteModel, modelPart);

	if(modelPart->connectors().size() > 0) {
		emit connectorsFoundSignal(modelPart->connectors().values());
	}
}

const QDir& PartsEditorViewsWidget::tempDir() {
	return ((PartsEditorMainWindow*)parentWidget())->tempDir();
}

void PartsEditorViewsWidget::connectPair(PartsEditorView *v1, PartsEditorView *v2) {
	connect(
		v1, SIGNAL(connectorSelected(const QString &)),
		v2, SLOT(informConnectorSelection(const QString &))
	);
	connect(
		v2, SIGNAL(connectorSelected(const QString &)),
		v1, SLOT(informConnectorSelection(const QString &))
	);
}

void PartsEditorViewsWidget::connectToThis(PartsEditorView *v) {
	connect(
		v, SIGNAL(connectorSelected(const QString &)),
		this, SLOT(informConnectorSelection(const QString &))
	);
}

void PartsEditorViewsWidget::repaint() {
	m_breadView->scene()->update();
	m_schemView->scene()->update();
	m_pcbView->scene()->update();
}

void PartsEditorViewsWidget::drawConnector(Connector* conn) {
	bool showing = showingTerminalPoints();
	m_breadView->drawConector(conn,showing);
	m_schemView->drawConector(conn,showing);
	m_pcbView->drawConector(conn,showing);
}

void PartsEditorViewsWidget::drawConnector(ViewLayer::ViewIdentifier viewId, Connector* conn) {
	bool showing = showingTerminalPoints();
	m_views[viewId]->drawConector(conn,showing);
}

void PartsEditorViewsWidget::setMismatching(ViewLayer::ViewIdentifier viewId, const QString &connId, bool mismatching) {
	m_views[viewId]->setMismatching(viewId, connId, mismatching);
	m_views[viewId]->scene()->update();
}

void PartsEditorViewsWidget::aboutToSave() {
	m_breadView->aboutToSave(false);
	m_schemView->aboutToSave(false);
	m_pcbView->aboutToSave(false);
}


void PartsEditorViewsWidget::removeConnectorFrom(const QString &connId, ViewLayer::ViewIdentifier viewId) {
	if(viewId == ViewLayer::AllViews) {
		m_breadView->removeConnector(connId);
		m_schemView->removeConnector(connId);
		m_pcbView->removeConnector(connId);
	} else {
		m_views[viewId]->removeConnector(connId);
	}
}


void PartsEditorViewsWidget::showHideTerminalPoints(int checkState) {
	bool show = checkStateToBool(checkState);

	m_breadView->showTerminalPoints(show);
	m_schemView->showTerminalPoints(show);
	m_pcbView->showTerminalPoints(show);
}

bool PartsEditorViewsWidget::showingTerminalPoints() {
	return checkStateToBool(m_showTerminalPointsCheckBox->checkState());
}

bool PartsEditorViewsWidget::checkStateToBool(int checkState) {
	if(checkState == Qt::Checked) {
		return true;
	} else if(checkState == Qt::Unchecked) {
		return false;
	}
	return false;
}

QCheckBox *PartsEditorViewsWidget::showTerminalPointsCheckBox() {
	return m_showTerminalPointsCheckBox;
}

void PartsEditorViewsWidget::informConnectorSelection(const QString &connId) {
	emit connectorSelectedInView(connId);
}

QWidget *PartsEditorViewsWidget::addZoomControlsAndBrowseButton(PartsEditorView *view) {
	QFrame *container1 = new QFrame(this);
	QVBoxLayout *lo1 = new QVBoxLayout(container1);
	lo1->setSpacing(1);
	lo1->setMargin(0);

	QLabel *button = new QLabel(QString("<a href='#'>%1</a>").arg(tr("Load image..")), this);
	button->setObjectName("browseButton");
	button->setMinimumWidth(85);
	button->setMaximumWidth(85);
	button->setFixedHeight(20);
	button->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

	connect(button, SIGNAL(linkActivated(const QString&)), view, SLOT(loadFile()));
	QHBoxLayout *lo2 = new QHBoxLayout();
	lo2->setSpacing(1);
	lo2->setMargin(0);
	lo2->addWidget(button);
	lo2->addWidget(new ZoomControls(view,container1));

	lo1->addWidget(view);
	lo1->addLayout(lo2);

	return container1;
}

PartsEditorView *PartsEditorViewsWidget::breadboardView() {
	return m_breadView;
}

PartsEditorView *PartsEditorViewsWidget::schematicView() {
	return m_schemView;
}

PartsEditorView *PartsEditorViewsWidget::pcbView() {
	return m_pcbView;
}

bool PartsEditorViewsWidget::imagesLoadedInAllViews() {
	return m_breadView->imageLoaded()
		&& m_schemView->imageLoaded()
		&& m_pcbView->imageLoaded();
}

void PartsEditorViewsWidget::connectTerminalRemoval(const ConnectorsInfoWidget* connsInfo) {
	connect(
		m_breadView, SIGNAL(removeTerminalPoint(const QString&, ViewLayer::ViewIdentifier)),
		connsInfo, SLOT(removeTerminalPoint(const QString&, ViewLayer::ViewIdentifier))
	);
	connect(
		m_schemView, SIGNAL(removeTerminalPoint(const QString&, ViewLayer::ViewIdentifier)),
		connsInfo, SLOT(removeTerminalPoint(const QString&, ViewLayer::ViewIdentifier))
	);
	connect(
		m_pcbView, SIGNAL(removeTerminalPoint(const QString&, ViewLayer::ViewIdentifier)),
		connsInfo, SLOT(removeTerminalPoint(const QString&, ViewLayer::ViewIdentifier))
	);
}

bool PartsEditorViewsWidget::connectorsPosOrSizeChanged() {
	return m_breadView->connsPosOrSizeChanged()
			|| m_schemView->connsPosOrSizeChanged()
			|| m_pcbView->connsPosOrSizeChanged();
}

void PartsEditorViewsWidget::setViewItems(ItemBase* bbItem, ItemBase* schemItem, ItemBase* pcbItem) 
{
	m_breadView->setViewItem(bbItem);
	m_schemView->setViewItem(schemItem);
	m_pcbView->setViewItem(pcbItem);
}

void PartsEditorViewsWidget::updatePinsInfo(QList< QPointer<ConnectorShared> > connsShared) {
	m_breadView->updatePinsInfo(connsShared);
	m_schemView->updatePinsInfo(connsShared);
	m_pcbView->updatePinsInfo(connsShared);
}
