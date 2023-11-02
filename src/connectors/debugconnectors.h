/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2023 Fritzing GmbH

Fritzing is free software: you can redistribute it and/or modify\
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

#ifndef DEBUGCONNECTORS_H
#define DEBUGCONNECTORS_H

#include <QSet>
#include <QString>
#include <QColor>
#include "../sketch/sketchwidget.h"

class DebugConnectors : public QObject {
	Q_OBJECT
public:
	DebugConnectors(SketchWidget *breadboardGraphicsView, SketchWidget *schematicGraphicsView, SketchWidget *pcbGraphicsView);


public slots:
	void monitorConnections(bool enabled);
	void onChangeConnection();

	void onSelectErrors();
	void onRepairErrors();
private:
	friend class DebugConnectorsProbe;
	QList<ItemBase *> doRoutingCheck();

	SketchWidget *m_breadboardGraphicsView;
	SketchWidget *m_schematicGraphicsView;
	SketchWidget *m_pcbGraphicsView;

	QSet<QString> getItemConnectorSet(ConnectorItem *connectorItem);
	QList<ItemBase *> toSortedItembases(const QList<QGraphicsItem *> &graphicsItems);
	void collectPartsForCheck(QList<ItemBase *> &partList, QGraphicsScene *scene);
	void fixColor();

	QTimer *timer;
	QElapsedTimer lastExecution;
	bool firstCall;
	bool colorChanged;
	static constexpr qint64 minimumInterval = 300;
	QColor breadboardBackgroundColor;
	QColor schematicBackgroundColor;
	QColor pcbBackgroundColor;

	bool m_monitorEnabled;
};

#endif // DEBUGCONNECTORS_H
