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

#ifndef EXCELLONPARSER_H
#define EXCELLONPARSER_H

#include <QHash>
#include <QList>
#include <QPointF>
#include <QString>
#include <QStringList>
#include <QRectF>

/**
 * @brief A single hit from a `.drl` (Excellon) drill file.
 *
 * Excellon is a much smaller spec than RS-274X; we only need the
 * tool table (`T01C0.035`) and the hit list (`X10000Y20000`).
 * Units are mm after parser conversion.
 */
struct ExcellonHit {
	QPointF pos;             ///< Hit centre in mm
	double  diameter = 0.3;  ///< Tool diameter in mm
};

/**
 * @brief Parses a Fritzing-emitted `.drl` (combined PTH + NPTH).
 *
 * Implemented subset:
 *   - Headers:  M48, FMAT,2, METRIC/INCH, optional LZ/TZ
 *   - Format:   INCH,LZ or METRIC,TZ etc.
 *   - Tool def: T01C0.035  (diameter in current unit)
 *   - Tool sel: T01
 *   - Hits:     X10000Y20000  (modal X/Y like RS-274X)
 *   - End:      M30
 *
 * Anything else is silently ignored (warning recorded).
 */
class ExcellonParser {
public:
	struct Result {
		QList<ExcellonHit> hits;
		QStringList        warnings;
		QRectF             bounds;  ///< Tight in mm
	};

	Result parse(const QString &text) const;
};

#endif // EXCELLONPARSER_H
