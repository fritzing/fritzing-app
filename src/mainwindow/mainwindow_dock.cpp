/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2013 Fachhochschule Potsdam - http://fh-potsdam.de

Fritzing is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or

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

#include <QSizeGrip>
#include <QStatusBar>
#include <QtDebug>
#include <QApplication>

#include "mainwindow.h"
#include "../dock/triplenavigator.h"
#include "../utils/fsizegrip.h"
#include "../dock/viewswitcherdockwidget.h"
#include "../utils/misc.h"
#include "../partsbinpalette/binmanager/binmanager.h"
#include "../infoview/htmlinfoview.h"
#include "../dock/layerpalette.h"
#include "../utils/fileprogressdialog.h"
#include "../debugdialog.h"

/////////////////////////////////////

static const int PartsBinMinHeight = 100;
static const int NavigatorDefaultHeight = 60;
static const int NavigatorMinHeight = 40;
static const int UndoHistoryDefaultHeight = 70;
static const int UndoHistoryMinHeight = UndoHistoryDefaultHeight;
const int MainWindow::DockMinWidth = 130;
const int MainWindow::DockMinHeight = 30;

FDockWidget * makeViewSwitcherDock(const QString & title, QWidget * parent) {
	return new ViewSwitcherDockWidget(title, parent);
}

///////////////////////////////////////

void MainWindow::dockChangeActivation(bool activate, QWidget * originator) {
	Q_UNUSED(activate);
	Q_UNUSED(originator);
	if (!m_closing) {
		m_sizeGrip->rearrange();
	}

}

void MainWindow::createDockWindows()
{
	QWidget * widget = new QWidget();
	widget->setMinimumHeight(0);
	widget->setMaximumHeight(0);
	FDockWidget * dock = makeDock(tr("View Switcher"), widget, 0,  0, Qt::RightDockWidgetArea, makeViewSwitcherDock);
	m_viewSwitcherDock = qobject_cast<ViewSwitcherDockWidget *>(dock);
	connect(this, SIGNAL(mainWindowMoved(QWidget *)), m_viewSwitcherDock, SLOT(windowMoved(QWidget *)));

#ifndef QT_NO_DEBUG
	//dock->setStyleSheet("background-color: red;");
	//m_viewSwitcher->setStyleSheet("background-color: blue;");
#endif

	makeDock(BinManager::Title, m_binManager, PartsBinMinHeight, PartsBinHeightDefault/*, Qt::LeftDockWidgetArea*/);

    makeDock(tr("Inspector"), m_infoView, InfoViewMinHeight, InfoViewHeightDefault);

	makeDock(tr("Undo History"), m_undoView, UndoHistoryMinHeight, UndoHistoryDefaultHeight)->hide();
    m_undoView->setMinimumSize(DockMinWidth, UndoHistoryMinHeight);

	m_tripleNavigator = new TripleNavigator(this);
    this ->setObjectName("tripleNav");
    m_navigators << (m_miniViewContainerBreadboard = new MiniViewContainer(m_tripleNavigator));
	m_miniViewContainerBreadboard->filterMousePress();
	connect(m_miniViewContainerBreadboard, SIGNAL(navigatorMousePressedSignal(MiniViewContainer *)),
								this, SLOT(currentNavigatorChanged(MiniViewContainer *)));

    m_navigators << (m_miniViewContainerSchematic = new MiniViewContainer(m_tripleNavigator));
	m_miniViewContainerSchematic->filterMousePress();
	connect(m_miniViewContainerSchematic, SIGNAL(navigatorMousePressedSignal(MiniViewContainer *)),
								this, SLOT(currentNavigatorChanged(MiniViewContainer *)));

    m_navigators << (m_miniViewContainerPCB = new MiniViewContainer(m_tripleNavigator));
	m_miniViewContainerPCB->filterMousePress();
	connect(m_miniViewContainerPCB, SIGNAL(navigatorMousePressedSignal(MiniViewContainer *)),
								this, SLOT(currentNavigatorChanged(MiniViewContainer *)));


	m_tripleNavigator->addView(m_miniViewContainerBreadboard, tr("Breadboard"));
	m_tripleNavigator->addView(m_miniViewContainerSchematic, tr("Schematic"));
	m_tripleNavigator->addView(m_miniViewContainerPCB, tr("PCB"));
	m_navigatorDock = makeDock(tr("Navigator"), m_tripleNavigator, NavigatorMinHeight, NavigatorDefaultHeight);
	m_navigatorDock->hide();

    makeDock(tr("Layers"), m_layerPalette, DockMinWidth, DockMinHeight)->hide();
    m_layerPalette->setMinimumSize(DockMinWidth, DockMinHeight);
	m_layerPalette->setShowAllLayersAction(m_showAllLayersAct);
	m_layerPalette->setHideAllLayersAction(m_hideAllLayersAct);

    if (m_programView == NULL) {
        m_windowMenu->addSeparator();
        m_windowMenu->addAction(m_openProgramWindowAct);
    }

#ifndef QT_NO_DEBUG
    m_windowMenu->addSeparator();
    m_windowMenu->addAction(m_toggleDebuggerOutputAct);
#endif

    m_windowMenuSeparator = m_windowMenu->addSeparator();
}

FDockWidget * MainWindow::makeDock(const QString & title, QWidget * widget, int dockMinHeight, int dockDefaultHeight, Qt::DockWidgetArea area, DockFactory dockFactory) {
	FDockWidget * dock = ((dockFactory) ? dockFactory(title, this) : new FDockWidget(title, this));
    dock->setObjectName(title);
    dock->setWidget(widget);
    widget->setParent(dock);
    widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(dock, SIGNAL(positionChanged()), this, SLOT(keepMargins()));
    connect(dock, SIGNAL(topLevelChanged(bool)), this, SLOT(keepMargins()));
    connect(dock, SIGNAL(visibilityChanged(bool)), this, SLOT(keepMargins()));

	return dockIt(dock, dockMinHeight, dockDefaultHeight, area);
}

FDockWidget *MainWindow::dockIt(FDockWidget* dock, int dockMinHeight, int dockDefaultHeight, Qt::DockWidgetArea area) {
	dock->setAllowedAreas(area);
	addDockWidget(area, dock);
    if (m_windowMenu) {
        m_windowMenu->addAction(dock->toggleViewAction());
    }

    dock->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	dock->setMinimumSize(DockMinWidth, dockMinHeight);
	dock->resize(DockWidthDefault, dockDefaultHeight);
	connect(dock, SIGNAL(dockChangeActivationSignal(bool, QWidget *)), this, SLOT(dockChangeActivation(bool, QWidget *)));
	connect(dock, SIGNAL(destroyed(QObject *)), qApp, SLOT(topLevelWidgetDestroyed(QObject *)));
    connect(dock, SIGNAL(dockChangeActivationSignal(bool, QWidget *)), qApp, SLOT(changeActivation(bool, QWidget *)), Qt::DirectConnection);

    m_docks << dock;

    return dock;
}

FDockWidget *MainWindow::newTopWidget() {
	int topMostY = 10000;
	FDockWidget *topWidget = NULL;
	foreach(FDockWidget* dock, m_docks) {
		if(/*!dock->isFloating() && dock->isVisible() &&*/
			dockWidgetArea(dock) == Qt::RightDockWidgetArea
			&& dock->pos().y() < topMostY) {
			topMostY = dock->pos().y();
			topWidget = dock;
		}
	}
	return topWidget;
}

FDockWidget *MainWindow::newBottomWidget() {
	int bottomMostY = -1;
	FDockWidget *bottomWidget = NULL;
	foreach(FDockWidget* dock, m_docks) {
		if(!dock->isFloating() && dock->isVisible() &&
			dockWidgetArea(dock) == Qt::RightDockWidgetArea
			&& dock->pos().y() > bottomMostY) {
			bottomMostY = dock->pos().y();
			bottomWidget = dock;
		}
	}
	return bottomWidget;
}

void MainWindow::keepMargins() {
	if (m_dontKeepMargins) return;

	/*FDockWidget* newTopWidget = this->newTopWidget();
	if(m_topDock != newTopWidget) {
		removeMargin(m_topDock);
		m_topDock = newTopWidget;
		if(m_topDock) m_oldTopDockStyle = m_topDock->styleSheet();
		addTopMargin(m_topDock);
	}*/

	FDockWidget* newBottomWidget = this->newBottomWidget();
	if(m_bottomDock != newBottomWidget) {
		removeMargin(m_bottomDock);
		m_bottomDock = newBottomWidget;
		if(m_bottomDock) m_oldBottomDockStyle = m_bottomDock->styleSheet();
		addBottomMargin(m_bottomDock);
		m_sizeGrip->raise();
	}
}


void MainWindow::removeMargin(FDockWidget* dock) {
	if(dock) {
		TripleNavigator *tn = qobject_cast<TripleNavigator*>(dock->widget());
		if(tn) {
			tn->showBottomMargin(false);
		} else {
			dockMarginAux(dock, "", m_oldBottomDockStyle);
		}
	}
}

void MainWindow::addTopMargin(FDockWidget* dock) {
	if(dock) dockMarginAux(dock, "topMostDock", dock->widget()->styleSheet());
}

void MainWindow::addBottomMargin(FDockWidget* dock) {
	if(dock) {
		TripleNavigator *tn = qobject_cast<TripleNavigator*>(dock->widget());
		if(tn) {
			tn->showBottomMargin(true);
		} else if(qobject_cast<BinManager*>(dock->widget())) {
			// already has enought space
		} else {
			dockMarginAux(dock, "bottomMostDock", dock->widget()->styleSheet());
		}
	}
}


void MainWindow::dockMarginAux(FDockWidget* dock, const QString &name, const QString &style) {
	if(dock) {
		dock->widget()->setObjectName(name);
		dock->widget()->setStyleSheet(style);
		dock->setStyleSheet(dock->styleSheet());
	} else {
		qWarning() << tr("Couldn't get the dock widget");
	}

}

void MainWindow::dontKeepMargins() {
	m_dontKeepMargins = true;
}

void MainWindow::initDock() {
	m_layerPalette = new LayerPalette(this);

	m_infoView = new HtmlInfoView();
    m_infoView->init(false);
    connect(m_infoView, SIGNAL(clickObsoleteSignal()), this, SLOT(selectAllObsolete()));
	//DebugDialog::debug("after html view");

	m_binManager = new BinManager(m_referenceModel, m_infoView, m_undoStack, this);
    m_binManager->initStandardBins();

	//DebugDialog::debug("after creating bins");
	if (m_fileProgressDialog) {
		m_fileProgressDialog->setValue(89);
	}
}

void MainWindow::moreInitDock() {
	//DebugDialog::debug("create view switcher");

	m_viewSwitcher = new ViewSwitcher(m_programView != NULL);
	connect(m_viewSwitcher, SIGNAL(viewSwitched(int)), this, SLOT(viewSwitchedTo(int)));
	connect(this, SIGNAL(viewSwitched(int)), m_viewSwitcher, SLOT(viewSwitchedTo(int)));
	m_viewSwitcher->viewSwitchedTo(0);

    createDockWindows();

	if (m_fileProgressDialog) {
		m_fileProgressDialog->setValue(93);
	}
}


void MainWindow::saveDocks()
{
	for (int i = 0; i < children().count(); i++) {
		FDockWidget * dock = qobject_cast<FDockWidget *>(children()[i]);
		if (dock == NULL) continue;

		//DebugDialog::debug(QString("saving dock %1").arg(dock->windowTitle()));
		dock->saveState();

		if (dock->isFloating() && dock->isVisible()) {
			//DebugDialog::debug(QString("hiding dock %1").arg(dock->windowTitle()));
			dock->hide();
		}
	}
}

void MainWindow::restoreDocks() {
	for (int i = 0; i < children().count(); i++) {
		FDockWidget * dock = qobject_cast<FDockWidget *>(children()[i]);
		if (dock == NULL) continue;

		// DebugDialog::debug(QString("restoring dock %1").arg(dock->windowTitle()));
		dock->restoreState();
	}
}

