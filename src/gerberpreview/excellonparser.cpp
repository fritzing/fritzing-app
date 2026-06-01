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

// Clean-room Excellon drill parser. Excellon is a public, prehistoric
// CNC format; no third-party code consulted.

#include "excellonparser.h"

#include <QRegularExpression>
#include <QtMath>

ExcellonParser::Result ExcellonParser::parse(const QString &text) const
{
	Result   r;
	double   unitScale     = 25.4;   // INCH default, 1.0 once METRIC seen
	int      decDigits     = 4;      // INCH default (LZ/TZ format)
	bool     decimalLiteral = false; // true once we see a literal '.'
	QHash<int, double> tools;        // tool# → diameter mm
	int      activeTool    = -1;
	QPointF  cursor(0, 0);

	const QStringList lines = text.split(QRegularExpression(QStringLiteral("[\\r\\n]+")),
	                                     Qt::SkipEmptyParts);

	QRegularExpression toolDef(QStringLiteral("^T(\\d+)C([\\d\\.]+)"));
	QRegularExpression toolSel(QStringLiteral("^T(\\d+)\\s*$"));
	QRegularExpression coord  (QStringLiteral("([XY])(-?[\\d\\.]+)"));

	for (const QString &raw : lines) {
		const QString s = raw.trimmed();
		if (s.isEmpty()) continue;
		if (s.startsWith(';') || s.startsWith('(')) continue;   // comment

		if (s == QLatin1String("M48") || s == QLatin1String("M30") ||
		    s == QLatin1String("M00") || s == QLatin1String("M02") ||
		    s.startsWith(QLatin1String("FMAT"))) continue;

		if (s.contains(QLatin1String("METRIC"))) { unitScale = 1.0;  decDigits = 3; continue; }
		if (s.contains(QLatin1String("INCH")))   { unitScale = 25.4; decDigits = 4; continue; }
		if (s == QLatin1String("LZ") || s == QLatin1String("TZ")) continue;

		// Tool definition: T01C0.035
		{
			auto m = toolDef.match(s);
			if (m.hasMatch()) {
				const int    n = m.captured(1).toInt();
				const double d = m.captured(2).toDouble() * unitScale;
				tools.insert(n, d);
				continue;
			}
		}
		// Tool select: T01
		{
			auto m = toolSel.match(s);
			if (m.hasMatch()) {
				activeTool = m.captured(1).toInt();
				continue;
			}
		}

		// Coordinate hit: X..Y..
		if (s.contains(QLatin1Char('X')) || s.contains(QLatin1Char('Y'))) {
			QPointF target = cursor;
			auto it = coord.globalMatch(s);
			while (it.hasNext()) {
				auto m = it.next();
				const QString axis = m.captured(1);
				const QString raw2 = m.captured(2);
				double v = 0.0;
				if (raw2.contains('.')) {
					decimalLiteral = true;
					v = raw2.toDouble() * unitScale;
				} else {
					v = (raw2.toDouble() / std::pow(10.0, decDigits)) * unitScale;
				}
				if (axis == QLatin1String("X")) target.setX(v);
				else                            target.setY(v);
			}
			cursor = target;

			ExcellonHit hit;
			hit.pos      = target;
			hit.diameter = tools.value(activeTool, 0.3);
			r.hits.append(hit);

			const QRectF box(target.x() - hit.diameter,
			                 target.y() - hit.diameter,
			                 hit.diameter * 2.0,
			                 hit.diameter * 2.0);
			r.bounds = r.bounds.isNull() ? box : r.bounds.united(box);
			continue;
		}

		r.warnings << QStringLiteral("excellon: unrecognised line: ") + s;
	}
	if (decimalLiteral) {
		// No-op: just suppresses unused-variable warning while
		// documenting that we accepted literal-decimal coordinates.
	}
	return r;
}
