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


#include <QKeyEvent>
#include <QProgressDialog>
#include <QApplication>
#include <QScrollBar>
#include <QMessageBox>

#include "connectorsinfowidget.h"
#include "addremoveconnectorbutton.h"
#include "partseditorviewswidget.h"
#include "../debugdialog.h"
#include "../connectors/connectorshared.h"
#include "../processeventblocker.h"

ConnectorsInfoWidget::ConnectorsInfoWidget(WaitPushUndoStack *undoStack, QWidget *parent)
	: QFrame(parent)
{
	m_objToDelete = NULL;
	m_selected = NULL;
	m_undoStack = undoStack;
	m_views = NULL;

	createScrollArea();
	createToolsArea();

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(m_title,0,0);
	layout->addWidget(m_scrollArea,1,0);
	layout->addWidget(m_toolsContainter,2,0);
	layout->setContentsMargins(3, 10, 3, 10);

	setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
	setFocusPolicy(Qt::StrongFocus);

	installEventFilter(this);

	m_connRemoved = false;
	m_connAdded = false;
}

void ConnectorsInfoWidget::emitPaintNeeded() {
	emit repaintNeeded();
}

void ConnectorsInfoWidget::createToolsArea() {
	m_toolsContainter = new QFrame(this);
	QHBoxLayout *lo = new QHBoxLayout(m_toolsContainter);

	QPushButton *addBtn = new QPushButton(QObject::tr("Add connector"),this);
	connect(addBtn, SIGNAL(clicked()), this, SLOT(addConnector()));

	lo->setMargin(2);
	lo->setSpacing(2);
	lo->addWidget(addBtn);
	lo->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::Expanding));
}

void ConnectorsInfoWidget::createScrollArea() {
	m_scrollContent = new QFrame();
	m_scrollContent->setObjectName("connInfoContent");
	m_scrollContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	m_mismatchersFrameParent = new QFrame(this);
	m_mismatchersFrameParent->setObjectName("mismatchConns");
	m_mismatchersFrameParent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

	QVBoxLayout *parentLo = new QVBoxLayout(m_mismatchersFrameParent);
	parentLo->setMargin(1);

	m_mismatchersFrame = new QFrame(m_mismatchersFrameParent);
	//m_mismatchersFrame->resize(this->width(),m_mismatchersFrame->height());
	QVBoxLayout *mismatchLayout = new QVBoxLayout(m_mismatchersFrame);
	mismatchLayout->setMargin(0);
	mismatchLayout->setSpacing(0);
	m_mismatchersFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

	QLabel *mismatchConnHeader = new QLabel(tr("Mismatching Connector IDs"));
	mismatchConnHeader->setObjectName("mismatchConnsHeader");
	parentLo->addWidget(mismatchConnHeader);
	parentLo->addWidget(m_mismatchersFrame);

	/*QLabel *mismatchConnFooter = new QLabel(tr("These problems need to be fixed in the svg-files directly"));
	mismatchConnFooter->setObjectName("mismatchConnsFooter");
	parentLo->addWidget(mismatchConnFooter);*/

	m_mismatchersFrameParent->hide();


	QVBoxLayout *scrollLayout = new QVBoxLayout(m_scrollContent);
	scrollLayout->setMargin(0);
	scrollLayout->setSpacing(0);
	scrollContentLayout()->addWidget(m_mismatchersFrameParent);
	scrollContentLayout()->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::Expanding,QSizePolicy::MinimumExpanding));


	m_scrollArea = new QScrollArea(this);
	m_scrollArea->setWidget(m_scrollContent);
	m_scrollArea->setWidgetResizable(true);

	m_title = new QLabel("  "+tr("List of Connectors"));
	m_title->setObjectName("title");
}

void ConnectorsInfoWidget::selectionChanged(AbstractConnectorInfoWidget* selected) {
	if(m_selected) {
		m_selected->setSelected(false);
	}
	m_selected = selected;

	QTimer::singleShot(20, this, SLOT(emitPaintNeeded()));
}

void ConnectorsInfoWidget::setSelected(AbstractConnectorInfoWidget * newSelected) {
	if(newSelected != m_selected) {
		newSelected->setSelected(true,false);
		selectionChanged(newSelected);
	}
}

bool ConnectorsInfoWidget::eventFilter(QObject *obj, QEvent *event) {
	if(obj == this) {
		if(event->type() == QEvent::KeyPress || event->type() == QEvent::ShortcutOverride) {
			QKeyEvent *keyEvent = (QKeyEvent*)event;
			if(keyEvent->key() == Qt::Key_Up) {
				selectPrev();
				return true;
			} else if(keyEvent->key() == Qt::Key_Down) {
				selectNext();
				return true;
			}
		}
	}
	return false;
}

/*void ConnectorsInfoWidget::ensureSelection(int i) {
	// TODO Mariano: Strange bug (if this is set, each up or dow key pressed event triggers twice)
	if(i==1) { // 1 == the fixed idx of the tab
		bool found = false;
		if(!m_selected) {
			if(m_connsInfo.size() > 0) {
				setSelected(m_connsInfo[0]);
				found = true;
			} else if(m_mismatchConnsInfo.size() > 0) {
				setSelected(m_mismatchConnsInfo[0]);
				found = true;
			}
			if(found) {
				this->setFocus();
			}
		}
	}
}*/

void ConnectorsInfoWidget::selectNext() {
	if(m_selected) {
		int selIdx = m_connsInfo.indexOf((SingleConnectorInfoWidget*)m_selected);
		if(selIdx > -1 && selIdx < m_connsInfo.size()-1) { // It's a single connector
			setSelected(m_connsInfo[selIdx+1]);
		} else {
			selIdx = m_mismatchConnsInfo.indexOf((MismatchingConnectorWidget*)m_selected);
			if(selIdx > -1) {
				if(selIdx < m_mismatchConnsInfo.size()-1) {
					setSelected(m_mismatchConnsInfo[selIdx+1]);
				} else if(m_connsInfo.size() > 0) {
					setSelected(m_connsInfo[0]);
				}
			}
		}
	}
}

void ConnectorsInfoWidget::selectPrev() {
	if(m_selected) {
		int selIdx = m_connsInfo.indexOf((SingleConnectorInfoWidget*)m_selected);
		if(selIdx > -1 ) { // It's a single connector and not the first
			if(selIdx > 0) {
				setSelected(m_connsInfo[selIdx-1]);
			} else if(m_mismatchConnsInfo.size()>0) {
				setSelected(m_mismatchConnsInfo[m_mismatchConnsInfo.size()-1]);
			}
		} else {
			selIdx = m_mismatchConnsInfo.indexOf((MismatchingConnectorWidget*)m_selected);
			if(selIdx > 0) { // it's a mismatch
				setSelected(m_mismatchConnsInfo[selIdx-1]);
			}
		}
	}
}

void ConnectorsInfoWidget::updateLayout() {
	m_mismatchersFrame->updateGeometry();
	m_mismatchersFrame->adjustSize();
	m_mismatchersFrameParent->updateGeometry();
	m_mismatchersFrameParent->adjustSize();
	m_scrollContent->adjustSize();
}

void ConnectorsInfoWidget::addConnectorInfo(MismatchingConnectorWidget* mcw) {
	if(mcw->prevConn()) {
		addConnectorInfo(mcw->prevConn());
	} else {
		addConnectorInfo(mcw->connId());
	}
}

Connector* ConnectorsInfoWidget::addConnectorInfo(QString id) {
	ConnectorShared *connShared = new ConnectorShared();
	connShared->setId(id);

	connShared->addPin(
		ViewLayer::BreadboardView,
		m_views->breadboardView()->svgIdForConnector(id),
		m_views->breadboardView()->connectorsLayerId(),
		m_views->breadboardView()->terminalIdForConnector(id),
        "", false
	);
	connShared->addPin(
		ViewLayer::SchematicView,
		m_views->schematicView()->svgIdForConnector(id),
		m_views->schematicView()->connectorsLayerId(),
		m_views->schematicView()->terminalIdForConnector(id),
        "", false
	);
	connShared->addPin(
		ViewLayer::PCBView,
		m_views->pcbView()->svgIdForConnector(id),
		m_views->pcbView()->connectorsLayerId(),
		m_views->pcbView()->terminalIdForConnector(id),
        "", false
	);

	Connector *conn = new Connector(connShared,0); // modelPart =? null
	addConnectorInfo(conn);
	return conn;
}

void ConnectorsInfoWidget::addConnectorInfo(Connector *conn) {
	QString connId = conn->connectorSharedID();
	m_connIds << connId;

	int connCount = m_connsInfo.size();
	SingleConnectorInfoWidget *sci = new SingleConnectorInfoWidget(this, m_undoStack, conn, m_scrollContent);
	scrollContentLayout()->insertWidget(connCount+1,sci);
	m_connsInfo << sci;
	m_allConnsInfo[connId] = sci;
	connect(sci,SIGNAL(editionStarted()),this,SLOT(updateLayout()));
	connect(sci,SIGNAL(editionFinished()),this,SLOT(updateLayout()));
	connect(sci,SIGNAL(tellSistersImNewSelected(AbstractConnectorInfoWidget*)),this,SLOT(selectionChanged(AbstractConnectorInfoWidget*)));
	connect(sci,SIGNAL(tellViewsMyConnectorIsNewSelected(const QString&)),this,SLOT(informConnectorSelection(const QString &)));
	connect(this,SIGNAL(editionCompleted()),sci,SLOT(editionCompleted()));
	m_scrollContent->updateGeometry();
}

void ConnectorsInfoWidget::addMismatchingConnectorInfo(ViewLayer::ViewIdentifier viewId, QString connId) {
	m_connIds << connId;
	MismatchingConnectorWidget *mcw = new MismatchingConnectorWidget(this,viewId,connId,m_mismatchersFrame);
	addMismatchingConnectorInfo(mcw);
	connect(
		mcw, SIGNAL(completeConn(MismatchingConnectorWidget*)),
		this, SLOT(completeConn(MismatchingConnectorWidget*))
	);
}

void ConnectorsInfoWidget::addMismatchingConnectorInfo(MismatchingConnectorWidget *mcw) {
	int connCount = m_mismatchConnsInfo.size();

	((QVBoxLayout*)m_mismatchersFrame->layout())->insertWidget(connCount,mcw);
	m_mismatchConnsInfo << mcw;
	m_allConnsInfo[mcw->connId()] = mcw;
	connect(mcw,SIGNAL(tellSistersImNewSelected(AbstractConnectorInfoWidget*)),this,SLOT(selectionChanged(AbstractConnectorInfoWidget*)));
	connect(mcw,SIGNAL(tellViewsMyConnectorIsNewSelected(const QString &)),this,SLOT(informConnectorSelection(const QString &)));
	if(m_mismatchConnsInfo.size()==1) {
		m_mismatchersFrameParent->show();
	}

	/*if(!m_selected && connCount == 0) {
		setSelected(mcw);
	}*/

	foreach(ViewLayer::ViewIdentifier viewId, mcw->views()) {
		emit setMismatching(viewId, mcw->connId(), true);
	}
}

QVBoxLayout *ConnectorsInfoWidget::scrollContentLayout() {
	return (QVBoxLayout*)m_scrollContent->layout();
}

void ConnectorsInfoWidget::connectorsFound(QList< QPointer<Connector> > conns) {
	qSort(conns);

	QProgressDialog progress(tr("Loading connectors..."), 0, 0, conns.size(), this);
	progress.show();
	for(int i=0; i < conns.size(); i++) {
		progress.setValue(i);
		ProcessEventBlocker::processEvents(); // to keep the app away from freezing
		addConnectorInfo(conns[i]);
	}
	progress.setValue(conns.size());

	updateLayout();
}

void ConnectorsInfoWidget::informConnectorSelection(const QString &connId) {
	emit connectorSelected(connId);
}

void ConnectorsInfoWidget::informEditionCompleted() {
	emit editionCompleted();
}

const QList< QPointer<ConnectorShared> > ConnectorsInfoWidget::connectorsShared() {
	QList< QPointer<ConnectorShared> > connectorsShared;
	for(int i=0; i<m_connsInfo.size(); i++) {
		SingleConnectorInfoWidget *sci = m_connsInfo[i];
		QString id = sci->id();
		Connector *conn = sci->connector();
		//foreach (ConnectorItem * connectorItem, conn->viewItems()) {
			//connectorItem->debugInfo("what is this connector");
		//}
		ConnectorShared* cs = conn->connectorShared();
		//foreach (SvgIdLayer * sil, cs->pins()) {
			//DebugDialog::debug(QString("what is this pin %1").arg(sil->m_svgViewLayerID));
		//}
		cs->setId(id);
		cs->setSharedName(sci->name());
		cs->setDescription(sci->description());
		cs->setConnectorType(sci->type());

		connectorsShared << cs;
	}
	return connectorsShared;
}

void ConnectorsInfoWidget::removeTerminalPoint(const QString &connId, ViewLayer::ViewIdentifier vid) {
	for(int i=0; i<m_connsInfo.size(); i++) {
		SingleConnectorInfoWidget *sci = m_connsInfo[i];
		Connector *conn = sci->connector();
		foreach(SvgIdLayer *sil, conn->connectorShared()->pins().values(vid)) {
			if(conn->connectorSharedID() == connId) {
				sil->m_terminalId = ___emptyString___;
			}
		}
	}
}

// If we're reloading an image, clear mismatching connectors related exclusively to that view
void ConnectorsInfoWidget::clearMismatchingForView(ViewLayer::ViewIdentifier viewId) {
	foreach(MismatchingConnectorWidget* mcw, m_mismatchConnsInfo) {
		if(mcw->views().size()==1 &&  mcw->views()[0] == viewId) {
			removeMismatchingConnectorInfo(mcw, false);
		}
	}
}

// Updates previous connector to mismatching if they are not in the list
void ConnectorsInfoWidget::singleToMismatchingNotInView(ViewLayer::ViewIdentifier viewId, const QStringList &connIds) {
	foreach(SingleConnectorInfoWidget* sci, m_connsInfo) {
		if(connIds.indexOf(sci->id()) == -1) {
			MismatchingConnectorWidget *mcw = sci->toMismatching(viewId);
			connect(
				mcw, SIGNAL(completeConn(MismatchingConnectorWidget*)),
				this, SLOT(completeConn(MismatchingConnectorWidget*))
			);
			removeConnectorInfo(sci,false);
			addMismatchingConnectorInfo(mcw);
		}
	}
	m_mismatchersFrame->adjustSize();
	scrollContentLayout()->update();
	m_mismatchersFrameParent->adjustSize();
	m_mismatchersFrameParent->layout()->update();
	updateLayout();
}

void ConnectorsInfoWidget::syncNewConnectors(ViewLayer::ViewIdentifier viewId, const QList< QPointer<Connector> > &conns) {
	clearMismatchingForView(viewId);

	// clean the old pins for this view
	/*foreach(QString oldId, m_connectorsPins.keys()) {
		m_connectorsPins[oldId].remove(viewId);
	}*/

	QStringList connIds;
	foreach(Connector *conn, conns) {
		QString connId = conn->connectorSharedID();
		connIds << connId;

		if(existingConnId(connId)) {
			SingleConnectorInfoWidget * sci = findSCI(connId);
			resetType(viewId, sci, conn);
			resetName(viewId, sci, conn);
			emit existingConnectorSignal(viewId, connId, sci ? sci->connector() : NULL, conn);
		} else {
			MismatchingConnectorWidget *mcw = NULL;
			if(( mcw = existingMismatchingConnector(connId) )) {
				if(mcw->onlyMissingThisView(viewId)) {
					Connector * prevConn = mcw->prevConn();
					removeMismatchingConnectorInfo(mcw, false);
					// mcw was just deleted in removeMismatchingConnectorInfo so we can't use it directly
					if(prevConn) {
						addConnectorInfo(prevConn);
					} else {
						addConnectorInfo(connId);
					}
					
					SingleConnectorInfoWidget * sci = findSCI(connId);
					resetType(viewId, sci, conn);
					resetName(viewId, sci, conn);
					emit existingConnectorSignal(viewId, connId, sci ? sci->connector() : NULL, conn);
				} else {
					mcw->addViewPresence(viewId);
					emit setMismatching(viewId, mcw->connId(), true);
				}
			} else {
				addMismatchingConnectorInfo(viewId, connId);
			}
		}
	}

	//clearMismatchingForView(viewId);
	singleToMismatchingNotInView(viewId,connIds);
}

bool ConnectorsInfoWidget::existingConnId(const QString &id) {
	return findConnector(id) != NULL;
}

MismatchingConnectorWidget* ConnectorsInfoWidget::existingMismatchingConnector(const QString &id) {
	foreach(MismatchingConnectorWidget *mci, m_mismatchConnsInfo) {
		if(mci->connId() == id) {
			return mci;
		}
	}
	return NULL;
}

void ConnectorsInfoWidget::removeMismatchingConnectorInfo(MismatchingConnectorWidget* mcw, bool singleShot, bool alsoDeleteFromView) {
	m_mismatchersFrame->layout()->removeWidget(mcw);
	m_mismatchConnsInfo.removeOne(mcw);
	m_allConnsInfo.remove(mcw->connId());
	if(m_mismatchConnsInfo.size()==0) {
		m_mismatchersFrameParent->hide();
		updateLayout();
	}

	if(alsoDeleteFromView) {
		emit removeConnectorFrom(mcw->connId(),ViewLayer::AllViews);
	} else {
		foreach(ViewLayer::ViewIdentifier viewId, mcw->views()) {
			emit setMismatching(viewId, mcw->connId(), false);
		}
	}

	if(m_selected == mcw) {
		m_selected = NULL;
	}

	m_objToDelete = mcw;
	if(singleShot) {
		QTimer::singleShot(100, this, SLOT(deleteAux()));
	} else {
		deleteAux();
	}
}

void ConnectorsInfoWidget::removeConnectorInfo(SingleConnectorInfoWidget *sci, bool singleShot, bool alsoDeleteFromView) {
	m_connRemoved = true;

	scrollContentLayout()->removeWidget(sci);
	m_connsInfo.removeOne(sci);
	m_allConnsInfo.remove(sci->id());

	if(m_selected == sci) {
		m_selected = NULL;
	}

	if(alsoDeleteFromView) {
		emit removeConnectorFrom(sci->id(), ViewLayer::AllViews);
	}

	m_objToDelete = sci;
	if(singleShot) {
		QTimer::singleShot(100, this, SLOT(deleteAux()));
	} else {
		deleteAux();
	}
}

Connector* ConnectorsInfoWidget::findConnector(const QString &id) {
	foreach(SingleConnectorInfoWidget *sci, m_connsInfo) {
		if(sci->id() == id) {
			return sci->connector();
		}
	}
	return NULL;
}

SingleConnectorInfoWidget* ConnectorsInfoWidget::findSCI(const QString &id) {
	foreach(SingleConnectorInfoWidget *sci, m_connsInfo) {
		if(sci->id() == id) {
			return sci;
		}
	}
	return NULL;
}

void ConnectorsInfoWidget::addConnector() {
	m_connAdded = true;

	if (m_views->breadboardView()->myItem() == NULL ||
		m_views->schematicView()->myItem() == NULL ||
		m_views->pcbView()->myItem() == NULL) 
	{
		QMessageBox::warning(
			parentWidget(),
			tr("Couldn't add connector"),
			tr("Please, first load an image in each view,\nbefore adding any connectors")
		);
		
		return;
	}

	QString connId = QString("connector%1").arg(nextConnId());
	emit drawConnector(addConnectorInfo(connId));

	if(m_allConnsInfo[connId]) {
		setSelected(m_allConnsInfo[connId]);
	}
}

void ConnectorsInfoWidget::removeSelectedConnector() {
	if(!m_selected) return;
	removeConnector(m_selected);
}

void ConnectorsInfoWidget::removeConnector(AbstractConnectorInfoWidget* connInfo) {
	MismatchingConnectorWidget* mismatch = dynamic_cast<MismatchingConnectorWidget*>(connInfo);
	if(mismatch) {
		removeMismatchingConnectorInfo(mismatch, true, true);
	} else {
		SingleConnectorInfoWidget *single = dynamic_cast<SingleConnectorInfoWidget*>(connInfo);
		if(single) {
			removeConnectorInfo(single, true, true);
		}
	}
}

int ConnectorsInfoWidget::nextConnId() {
	int max = 0;
	foreach(QString connId, m_connIds) {
		QString currId = connId;

		if(currId.startsWith("connector")) {
			QString helpStr = currId.remove("connector");
			if(!helpStr.isEmpty()) {
				bool isInt;
				int helpInt = helpStr.toInt(&isInt);
				if(isInt && max <= helpInt) {
					max = ++helpInt;
				}
			}
		}
	}
	return max;
}

void ConnectorsInfoWidget::deleteAux() {
	if(m_objToDelete) {
		if(m_selected == m_objToDelete) {
			m_selected = NULL;
		}
		delete m_objToDelete;
		m_objToDelete = NULL;

	}
}

int ConnectorsInfoWidget::scrollBarWidth() {
	return m_scrollArea->verticalScrollBar()->width();
}


void ConnectorsInfoWidget::connectorSelectedInView(const QString &connId) {
	setSelected(m_allConnsInfo[connId]);
}

void ConnectorsInfoWidget::setViews(PartsEditorViewsWidget* connsView) {
	m_views = connsView;
}


void ConnectorsInfoWidget::completeConn(MismatchingConnectorWidget* mcw) {
	if(m_views->imagesLoadedInAllViews()) {
		QList<ViewLayer::ViewIdentifier> missingViews = mcw->missingViews();
		QList<ViewLayer::ViewIdentifier> availViews = mcw->views();
		QString connId = mcw->connId();
		removeMismatchingConnectorInfo(mcw);

		if(mcw->prevConn()) {
			addConnectorInfo(mcw->prevConn());
		} else {
			addConnectorInfo(connId);
		}

		Connector *connector = findConnector(connId);
		foreach(ViewLayer::ViewIdentifier viewId, missingViews) {
			emit drawConnector(viewId, connector);
		}
		foreach(ViewLayer::ViewIdentifier viewId, availViews) {
			emit setMismatching(viewId,connId,false);
		}
	} else {
		QMessageBox::information(
			parentWidget(),
			tr("Couldn't fix connector"),
			tr("Please, first load an image in each view,\nin order to fix this connector")
		);
	}
}

bool ConnectorsInfoWidget::connectorsRemoved() {
	return m_connRemoved;
}

bool ConnectorsInfoWidget::connectorAdded() {
	return m_connAdded;
}

bool ConnectorsInfoWidget::connectorsCountChanged() {
	return connectorsRemoved() || connectorAdded();
}

bool ConnectorsInfoWidget::hasMismatchingConnectors() {
	return m_mismatchConnsInfo.size() > 0;
}

void ConnectorsInfoWidget::resetType(ViewLayer::ViewIdentifier viewId, SingleConnectorInfoWidget * sci, Connector * conn) {
	if (viewId != ViewLayer::PCBView) return;

	if (conn->connectorType() == sci->connectorType()) return;

	if (conn->connectorType() == Connector::Pad) {
		// was male or female
		sci->setConnectorType(conn->connectorType());
		return;
	}

	if (sci->connectorType() != Connector::Pad) return;

	// was pad, should it be male or female?

	// TODO: figure out which gender or ask the user to fix it.
	sci->setConnectorType(Connector::Male);

}


void ConnectorsInfoWidget::resetName(ViewLayer::ViewIdentifier viewId, SingleConnectorInfoWidget * sci, Connector * conn) 
{
	Q_UNUSED(viewId);

	if (!sci->name().startsWith("pin", Qt::CaseInsensitive)) return;  // probably already changed by the user

	QString name = conn->connectorSharedName();
	if (name.isEmpty()) return;

	if (name.startsWith("pin", Qt::CaseInsensitive)) return;

	bool ok;
	int num = name.toInt(&ok);
	if (ok) return;					// just a number, don't bother

	Q_UNUSED(num);
	sci->setName(name);
}
