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
#include "../sketch/sketchwidget.h"
#include "../utils/textutils.h"

class DebugConnectors : public QObject {
	Q_OBJECT
public:
	DebugConnectors(SketchWidget *breadboardGraphicsView, SketchWidget *schematicGraphicsView, SketchWidget *pcbGraphicsView);

public slots:
	void routingCheckSlot();

private slots:
	void doRoutingCheck();

private:
	SketchWidget *m_breadboardGraphicsView;
	SketchWidget *m_schematicGraphicsView;
	SketchWidget *m_pcbGraphicsView;

	QSet<QString> getItemConnectorSet(ConnectorItem *connectorItem);
	QList<ItemBase *> toSortedItembases(const QList<QGraphicsItem *> &graphicsItems);
	void collectPartsForCheck(QList<ItemBase *> &partList, QGraphicsScene *scene);

	QTimer *timer;
	QElapsedTimer lastExecution;
	bool firstCall;
	static constexpr qint64 minimumInterval = 300;
};

#endif // DEBUGCONNECTORS_H
