/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2026 Fritzing contributors

Fritzing is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

********************************************************************/

#ifndef BREADBOARDPLACEMENTKERNEL_H
#define BREADBOARDPLACEMENTKERNEL_H

#include <QHash>
#include <QPointF>
#include <QVector>

namespace BreadboardPlacementKernel
{

// Uniform-grid index over immutable breadboard-hole positions. Queries return
// original indices in ascending order, so replacing a brute-force scan cannot
// change candidate tie-breaking or make results depend on hash iteration.
class HoleSpatialIndex
{
public:
	HoleSpatialIndex() = default;
	HoleSpatialIndex(const QVector<QPointF> &positions, const QVector<int> &boardIds, double cellSize);

	QVector<int> withinRadius(const QPointF &center, double radius, int boardId = -1) const;
	bool isEmpty() const;

private:
	static quint64 cellKey(int x, int y);
	int cellCoordinate(double coordinate) const;

	QVector<QPointF> m_positions;
	QVector<int> m_boardIds;
	QHash<quint64, QVector<int>> m_cells;
	double m_cellSize = 1.0;
};

}

#endif
