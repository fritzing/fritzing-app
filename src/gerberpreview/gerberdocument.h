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

#ifndef GERBERDOCUMENT_H
#define GERBERDOCUMENT_H

#include "gerberaperture.h"

#include <QHash>
#include <QList>
#include <QPainterPath>
#include <QPointF>
#include <QRectF>

/**
 * @brief A single drawing operation produced by the parser. The
 *        renderer is a switch on `kind` — nothing fancier. Keeping
 *        commands as a flat POD list (instead of a tree) makes
 *        layer compositing trivial and is more than fast enough for
 *        Fritzing-scale output (<= 10k commands per layer).
 */
struct GerberCommand {
	enum Kind {
		Stroke,   ///< D01 — draw aperture-stroked segment from `a` to `b`
		Flash,    ///< D03 — drop one copy of aperture at `b`
		Region    ///< G36..G37 fill — `region` holds the closed path
	};

	Kind          kind = Stroke;
	int           apertureCode = -1;  ///< Index into GerberDocument::apertures
	QPointF       a;                  ///< Stroke start (Flash/Region: unused)
	QPointF       b;                  ///< Stroke end / Flash centre
	QPainterPath  region;             ///< Region kind only
};

/**
 * @brief One parsed Gerber file. Owns the aperture dictionary and
 *        an ordered list of commands. Coordinates are in
 *        millimetres after the parser applies the format spec
 *        (`%FSLAX46Y46*%`) and unit directive (`%MOMM*%` / `%MOIN*%`).
 */
class GerberDocument {
public:
	GerberDocument() = default;

	/// Aperture dictionary, keyed by D-code (>= 10).
	QHash<int, GerberAperture> apertures;

	/// Ordered drawing operations.
	QList<GerberCommand> commands;

	/// Warnings collected during parse (one-line each, human readable).
	QStringList warnings;

	/// Tight bounding box in mm. Updated by the parser as it walks.
	QRectF bounds;

	/// Convenience: extend `bounds` to include p with `slack` margin.
	void extend(const QPointF &p, double slack = 0.0);
};

#endif // GERBERDOCUMENT_H
