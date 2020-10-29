/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2020 Fritzing

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

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "../mainwindow/mainwindow.h"
#include "../items/itembase.h"

class Simulator : public QObject
{
	Q_OBJECT

public:
	Simulator(class MainWindow *mainWindow);
	~Simulator();

public slots:
	void simulate();

protected:
	void drawSmoke(ItemBase* part);
	void removeSimItems();

	MainWindow *m_mainWindow;
	QPointer<class BreadboardSketchWidget> m_breadboardGraphicsView;
	QPointer<class SchematicSketchWidget> m_schematicGraphicsView;

	bool m_showingSimulation = false;

	QHash<ItemBase *, ItemBase *> m_sch2bbItemHash;
	QHash<ConnectorItem *, int> m_connector2netHash;

	QList<QGraphicsSvgItem *> m_bbSimItems;
	QList<QGraphicsSvgItem *> m_schSimItems;
};

#endif // SIMULATOR_H
