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

#ifndef BREADBOARDAUTOROUTER_H
#define BREADBOARDAUTOROUTER_H

#include <QObject>
#include <QElapsedTimer>
#include <QHash>
#include <QList>
#include <QPair>
#include <QStringList>

#include "../viewgeometry.h"
#include "../connectors/connectoritem.h"
#include "breadboardroutingscore.h"

class BreadboardSketchWidget;
class ItemBase;
class QUndoCommand;

class BreadboardAutorouter : public QObject
{
	Q_OBJECT

public:
	explicit BreadboardAutorouter(BreadboardSketchWidget * sketchWidget);
	~BreadboardAutorouter() override;

	void start();

Q_SIGNALS:
	void setMaximumProgress(int);
	void setProgressValue(int);
	void setProgressMessage(const QString &);
	void setProgressMessage2(const QString &);

private:
	// Defined in the .cpp: carries the state shared across one net-routing
	// pass so the per-net phases can live in named functions instead of one
	// monolithic loop body. Nested so it can use the router's private
	// helpers without widening the public surface.
	struct NetRoutingPass;
	// Same pattern for part placement: board topology, hole caches, planning
	// state and rejection counters shared by the placement phases (movable
	// part collection, rigid/bendable candidate search, commit, verify).
	struct PlacementPass;

	int clearPreviousAutorouteWires();
	int autoplacePartsOnBreadboard();
	bool verifyPlacedConnections(const QHash<ConnectorItem *, ConnectorItem *> &placedTargets, QStringList &failures) const;
	int routeCollectedNets(QUndoCommand * parentCommand);
	int routeRatsnestDemands(QUndoCommand * parentCommand);
	QList< QList<ConnectorItem *> > collectCandidateGroups(const QList<ConnectorItem *> & candidates) const;
	bool isBreadboardItem(ItemBase * itemBase) const;
	bool isBreadboardDecorationItem(ItemBase * itemBase) const;
	bool isMovableBreadboardPart(ItemBase * itemBase) const;
	bool isPlaceablePin(ConnectorItem * connectorItem) const;
	bool isTargetBreadboardHole(ConnectorItem * connectorItem) const;
	bool connectorsShareBreadboardBus(ConnectorItem * first, ConnectorItem * second) const;
	int busGroupFor(ConnectorItem * connectorItem) const;
	const QList<QPair<ConnectorItem *, ConnectorItem *> > & normalBreadboardWireEnds() const;
	void invalidateRoutingCaches();

	// Bus ownership: the mechanism behind the prime invariant. Every bus is
	// free (-1) or owned by exactly one key; keys are net indices (>= 0) or
	// unique negative sentinels claimed by no-net pins (e.g. unused DIP
	// outputs, which are still real outputs and must never join a net).
	int busOwner(int busGroup) const;
	bool busAvailableFor(int busGroup, int ownerKey) const;
	void claimBus(int busGroup, int ownerKey);
	int makeNoNetOwnerKey();
	int ownerKeyForPin(ConnectorItem * pin) const;
	void seedBusOwnership();
	bool verifySchematicConformance(QStringList & violations, bool recordBaseline = false);
	QString connectorSummary(ConnectorItem * connectorItem) const;
	QString itemSummary(ItemBase * itemBase) const;
	QString logFilePath() const;
	void logAutoroute(const QString & message) const;
	void flushAutorouteLog() const;
	ConnectorItem * connectedPartConnector(ConnectorItem * wireConnector) const;
	ConnectorItem * connectedBreadboardHoleFor(ConnectorItem * partConnector) const;
	ConnectorItem * breadboardHoleFor(ConnectorItem * partConnector) const;
	ConnectorItem * nearestFreeBusHole(ConnectorItem * breadboardHole) const;
	ConnectorItem * routingConnectorFor(ConnectorItem * wireConnector) const;
	QList< QList<ConnectorItem *> > collectRoutableSubnets(QList<ConnectorItem *> * net) const;
	QList<ConnectorItem *> routingCandidatesForSubnet(const QList<ConnectorItem *> & subnet) const;
	ConnectorItem * chooseRepresentative(const QList<ConnectorItem *> & subnet) const;
	double partConnectivityScore(ItemBase * part, const QHash<ConnectorItem *, int> & netForConnector, const QHash<int, QList<ConnectorItem *> > & connectorsForNet) const;
	double routeScore(ConnectorItem * from, ConnectorItem * to) const;
	int countUnresolvedNets() const;
	void clearCollectedNets();
	void sortCollectedNets();
	void reportProgress(int percent, const QString & detail);
	void loadTuning();

	// Wall-clock per pipeline phase, reset each start(); logged as one
	// greppable phase-summary line so critical paths are comparable per
	// sketch and across builds.
	struct PhaseStats
	{
		qint64 clearMs = 0;
		qint64 collectMs = 0;
		qint64 placeSearchMs = 0;
		qint64 placeExecMs = 0;
		qint64 routeSearchMs = 0;
		qint64 routeExecMs = 0;
		qint64 completionMs = 0;
		qint64 cleanupMs = 0;
		QString toString() const;
	};

private:
	BreadboardSketchWidget * m_sketchWidget = nullptr;
	QList< QList<ConnectorItem *> * > m_allPartConnectorItems;
	QString m_lastPlacementReport;
	BreadboardRoutingScore m_lastRoutingScore;
	double m_componentLeadLength = 0.0;

	// Placement tuning weights, read from QSettings at every start() so the
	// toolbar sliders take effect without restarting. Defaults live here.
	PhaseStats m_phaseStats;

	// Per-run memoization: bus membership never changes during an autoroute,
	// and BreadboardTopology::connectorsShareBus rebuilds bus lists per call
	// (dominant cost of route search pre-Stage-1). Cleared in start() and
	// whenever pushed commands change scene wires.
	mutable QHash<ConnectorItem *, int> m_busGroupForConnector;
	mutable int m_busGroupCount = 0;
	QHash<int, int> m_busOwnerForGroup;      // busGroup -> ownerKey, absent = free
	QHash<ConnectorItem *, int> m_netForConnector;   // schematic net index per part pin
	int m_nextNoNetOwnerKey = -2;
	QSet<QPair<int, int> > m_preExistingNetContacts; // net pairs already touching before we run
	// Log lines are buffered and flushed at phase boundaries: opening and
	// closing the file per line dominated route-search time on large boards.
	mutable QStringList m_logBuffer;
	QElapsedTimer m_progressPumpTimer;
	mutable bool m_wireEndsCacheValid = false;
	mutable QList<QPair<ConnectorItem *, ConnectorItem *> > m_normalBreadboardWireEnds;

	double m_maxLegLength = 120.0;
	double m_leadLengthWeight = 1.0;
	double m_jumperPenalty = 100000.0;
	double m_leadAngleWeight = 4.0;
	double m_foldbackWeight = 6.0;
};

#endif
