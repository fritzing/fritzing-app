/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2019 Fritzing

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

********************************************************************/

#ifndef AUTOROUTER_H
#define AUTOROUTER_H

#include <QAction>
#include <QHash>
#include <QVector>
#include <QList>
#include <QPointF>
#include <QGraphicsItem>
#include <QLine>
#include <QProgressDialog>
#include <QUndoCommand>

#include "../viewgeometry.h"
#include "../viewlayer.h"
#include "../connectors/connectoritem.h"
#include "../commands.h"

class PCBSketchWidget;
class SymbolPaletteItem;
class JumperItem;
class Via;
class Autorouter : public QObject
{
	Q_OBJECT

public:
	Autorouter(PCBSketchWidget *);
	virtual ~Autorouter() = default;

	virtual void start() = 0;

public:
	static const QString MaxCyclesName;


protected:
	virtual void cleanUpNets();
	virtual void updateRoutingStatus();
	virtual class TraceWire * drawOneTrace(QPointF fromPos, QPointF toPos, double width, ViewLayer::ViewLayerPlacement);
	void initUndo(QUndoCommand * parentCommand);
	void addUndoConnection(bool connect, SymbolPaletteItem *, QUndoCommand * parentCommand);
	void addUndoConnection(bool connect, JumperItem *, QUndoCommand * parentCommand);
	void addUndoConnection(bool connect, Via *, QUndoCommand * parentCommand);
	void addUndoConnection(bool connect, TraceWire *, QUndoCommand * parentCommand);
	void addUndoConnection(bool connect, ConnectorItem *, BaseCommand::CrossViewType, QUndoCommand * parentCommand);
	void restoreOriginalState(QUndoCommand * parentCommand);
	void doCancel(QUndoCommand * parentCommand);
	void clearTracesAndJumpers();
	void addToUndo(QUndoCommand * parentCommand);
	void addWireToUndo(Wire * wire, QUndoCommand * parentCommand);

public slots:
	virtual void cancel();
	virtual void cancelTrace();
	virtual void stopTracing();
	virtual void useBest();
	virtual void setMaxCycles(int);

signals:
	void setMaximumProgress(int);
	void setProgressValue(int);
	void wantTopVisible();
	void wantBottomVisible();
	void wantBothVisible();
	void setProgressMessage(const QString &);
	void setProgressMessage2(const QString &);
	void setCycleMessage(const QString &);
	void setCycleCount(int);
	void disableButtons();

protected:
	PCBSketchWidget * m_sketchWidget = nullptr;
	QList< QList<ConnectorItem*>* > m_allPartConnectorItems;
	bool m_cancelled = false;
	bool m_cancelTrace = false;
	bool m_stopTracing = false;
	bool m_useBest = false;
	bool m_bothSidesNow = false;
	int m_maximumProgressPart = 0;
	int m_currentProgressPart = 0;
	QGraphicsItem * m_board = nullptr;
	int m_maxCycles = 0;
	double m_keepoutPixels = 0.0;
	QRectF m_maxRect;
	bool m_pcbType = false;
};

#endif
