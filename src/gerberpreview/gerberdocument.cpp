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

#include "gerberdocument.h"

void GerberDocument::extend(const QPointF &p, double slack)
{
	// NOTE: QRectF::isNull() is true for any zero-size rect, and
	// QRectF::united() drops a null operand — so a zero-slack call
	// must be widened into a real (non-zero) rect or we silently
	// lose the contribution of strokes and moves.
	const double s = qMax(slack, 1e-9);
	const QRectF box(p.x() - s, p.y() - s, s * 2.0, s * 2.0);
	if (!bounds.isValid()) bounds = box;
	else                   bounds = bounds.united(box);
}
