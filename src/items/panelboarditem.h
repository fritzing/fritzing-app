/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2026 Fritzing

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

#ifndef PANELBOARDITEM_H
#define PANELBOARDITEM_H

#include "../items/resizableboard.h"

class QPainter;
class QRectF;
class QStyleOptionGraphicsItem;
class ViewGeometry;

/**
 * @brief A Board subclass that represents the synthetic panel outline.
 *
 * Placed in the PCB sketch's QGraphicsScene to represent the full
 * panelized output. Draws the panel outer outline plus an overlay
 * SVG (V-cut / mouse-bite separators, fiducials, tooling holes)
 * supplied via setSeparationSvg().
 *
 * Inherits from Board (resizableboard.h) for save/load and render
 * plumbing, but identifies itself as a synthetic container via
 * isPanelBoard() so MainWindow can skip DRC, connector-graph, and
 * parts-bin operations on it.
 */
class PanelBoardItem : public Board
{
	Q_OBJECT

public:
	/**
	 * @brief Constructs a PanelBoardItem with the given panel geometry.
	 * @param viewGeometry ViewGeometry with location and size for the panel
	 *                     (units: inches * GraphicsUtils::StandardFritzingDPI).
	 * @param referenceModel ReferenceModel used to resolve the underlying
	 *                       ModelPart; must not be null (Board ctor would crash).
	 * @param parent Parent QGraphicsItem (if any).
	 */
	PanelBoardItem(const ViewGeometry &viewGeometry,
	               class ReferenceModel *referenceModel,
	               QGraphicsItem *parent = nullptr);

	~PanelBoardItem() override;

	// QGraphicsItem interface:
	QRectF boundingRect() const override;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
	           QWidget *widget) override;

	/**
	 * @brief Sets the separation SVG overlay (V-cut / mouse-bite / fiducial geometry).
	 * @param svg The SVG string from PanelizerSeparators::vcutSvg() etc.
	 */
	void setSeparationSvg(const QString &svg);

	/**
	 * @brief Identifies this item as the synthetic panel container.
	 * @return Always true for PanelBoardItem; default false in ItemBase.
	 *
	 * MainWindow uses this to skip DRC, connector-graph rebuilds, and
	 * parts-bin operations - the panel is a production artifact, not
	 * an electrical board.
	 */
	bool isPanelBoard() const { return true; }

protected:
	double m_panelWidth;
	double m_panelHeight;
	QString m_separationSvg;

signals:
	/**
	 * @brief Emitted when the panel outline changes (e.g., after re-layout).
	 */
	void panelChanged();
};

#endif
