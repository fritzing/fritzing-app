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
#include <QList>

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
	int clearPreviousAutorouteWires();
	int autoplacePartsOnBreadboard();
	int routeCollectedNets(QUndoCommand * parentCommand);
	int routeRatsnestDemands(QUndoCommand * parentCommand);
	QList< QList<ConnectorItem *> > collectCandidateGroups(const QList<ConnectorItem *> & candidates) const;
	bool isBreadboardItem(ItemBase * itemBase) const;
	bool isBreadboardDecorationItem(ItemBase * itemBase) const;
	bool isMovableBreadboardPart(ItemBase * itemBase) const;
	bool isPlaceablePin(ConnectorItem * connectorItem) const;
	bool isTargetBreadboardHole(ConnectorItem * connectorItem) const;
	bool connectorsShareBreadboardBus(ConnectorItem * first, ConnectorItem * second) const;
	QString connectorSummary(ConnectorItem * connectorItem) const;
	QString itemSummary(ItemBase * itemBase) const;
	QString logFilePath() const;
	void logAutoroute(const QString & message) const;
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

private:
	BreadboardSketchWidget * m_sketchWidget = nullptr;
	QList< QList<ConnectorItem *> * > m_allPartConnectorItems;
	QString m_lastPlacementReport;
	BreadboardRoutingScore m_lastRoutingScore;
	double m_componentLeadLength = 0.0;
};

#endif
