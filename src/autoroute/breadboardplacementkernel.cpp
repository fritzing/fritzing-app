/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2026 Fritzing contributors

Fritzing is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

********************************************************************/

#include "breadboardplacementkernel.h"

#include <QtMath>
#include <algorithm>

namespace BreadboardPlacementKernel
{

HoleSpatialIndex::HoleSpatialIndex(const QVector<QPointF> &positions, const QVector<int> &boardIds, double cellSize)
	: m_positions(positions),
	  m_boardIds(boardIds),
	  m_cellSize(qMax(1.0, cellSize))
{
	if (m_boardIds.count() != m_positions.count())
		m_boardIds.fill(-1, m_positions.count());

	for (int index = 0; index < m_positions.count(); index++)
	{
		const QPointF &position = m_positions.at(index);
		m_cells[cellKey(cellCoordinate(position.x()), cellCoordinate(position.y()))].append(index);
	}
}

QVector<int> HoleSpatialIndex::withinRadius(const QPointF &center, double radius, int boardId) const
{
	QVector<int> result;
	if (radius < 0.0 || m_positions.isEmpty())
		return result;

	const int minX = cellCoordinate(center.x() - radius);
	const int maxX = cellCoordinate(center.x() + radius);
	const int minY = cellCoordinate(center.y() - radius);
	const int maxY = cellCoordinate(center.y() + radius);
	const double radiusSquared = radius * radius;

	for (int cellY = minY; cellY <= maxY; cellY++)
	{
		for (int cellX = minX; cellX <= maxX; cellX++)
		{
			const auto found = m_cells.constFind(cellKey(cellX, cellY));
			if (found == m_cells.constEnd())
				continue;
			for (int index : found.value())
			{
				if (boardId >= 0 && m_boardIds.at(index) != boardId)
					continue;
				const QPointF delta = m_positions.at(index) - center;
				if (delta.x() * delta.x() + delta.y() * delta.y() <= radiusSquared)
					result.append(index);
			}
		}
	}

	std::sort(result.begin(), result.end());
	return result;
}

bool HoleSpatialIndex::isEmpty() const
{
	return m_positions.isEmpty();
}

quint64 HoleSpatialIndex::cellKey(int x, int y)
{
	return (quint64(quint32(x)) << 32) | quint32(y);
}

int HoleSpatialIndex::cellCoordinate(double coordinate) const
{
	return qFloor(coordinate / m_cellSize);
}

}
