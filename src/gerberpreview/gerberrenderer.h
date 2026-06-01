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

#ifndef GERBERRENDERER_H
#define GERBERRENDERER_H

#include "excellonparser.h"
#include "gerberdocument.h"

#include <QColor>

class QPainter;

/**
 * @brief Stateless: paints a GerberDocument (or Excellon hit list)
 *        into a QPainter using the painter's current world transform.
 *
 * The caller is responsible for the world-to-pixel transform — the
 * renderer paints in millimetres. This keeps zoom/pan logic in the
 * widget and the renderer trivially unit-testable.
 */
class GerberRenderer {
public:
	/**
	 * @brief Render every command in @p doc into @p painter using
	 *        @p color as the fill/stroke colour.
	 * @note Painter state (pen, brush, transform) is saved on entry
	 *       and restored on exit, so the caller can chain calls
	 *       without bookkeeping.
	 */
	void render(QPainter &painter,
	            const GerberDocument &doc,
	            const QColor &color) const;

	/**
	 * @brief Render drill hits as filled rings — outer ring in
	 *        @p ringColor, hole in @p holeColor. Hole defaults to
	 *        transparent so the background colour shows through.
	 */
	void renderDrills(QPainter &painter,
	                  const ExcellonParser::Result &drills,
	                  const QColor &ringColor,
	                  const QColor &holeColor) const;
};

#endif // GERBERRENDERER_H
