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

// Clean-room implementation of Ucamco's RS-274X spec
// (https://www.ucamco.com/en/gerber/downloads). The grammar and
// statement set implemented here is documented in homerun-gui.md
// §12.1. No GPL-2 code from gerbv is consulted.

#include "gerberparser.h"

#include <QFile>
#include <QIODevice>
#include <QRegularExpression>
#include <QStringList>
#include <QtMath>

namespace {

/// Internal mutable state carried across statements. Lives only in
/// this translation unit so it cannot collide with anything else.
struct ParserState {
	int     xIntDigits     = 4;      ///< From %FSLAX46…
	int     xDecDigits     = 6;
	int     yIntDigits     = 4;
	int     yDecDigits     = 6;
	double  unitScale      = 25.4;   ///< 25.4 IN (default), 1.0 MM
	bool    inRegion       = false;
	int     activeAperture = -1;
	QPointF cursor{0.0, 0.0};
	QPainterPath regionPath;
};

/// Spec: leading zeros may be omitted (LA), decimal point implicit,
/// value scaled by 10^-decDigits then by unit scale.
double decodeCoord(const QString &raw, int decDigits, double unitScale)
{
	if (raw.isEmpty()) return 0.0;
	const qlonglong i = raw.toLongLong();
	return (i / std::pow(10.0, decDigits)) * unitScale;
}

void parseAperture(const QString &body, GerberDocument &doc, ParserState &state)
{
	// body looks like: "D10C,0.0100"  or  "11R,0.060X0.040X0.0157"
	QRegularExpression head(QStringLiteral("^D?(\\d+)([CROP]),(.*)$"));
	auto m = head.match(body);
	if (!m.hasMatch()) {
		doc.warnings << QStringLiteral("malformed AD: ") + body;
		return;
	}
	const int     code = m.captured(1).toInt();
	const QString kind = m.captured(2);
	const QString args = m.captured(3);

	GerberAperture ap;
	const QStringList parts = args.split(QLatin1Char('X'), Qt::SkipEmptyParts);
	auto asMM = [&](const QString &t) {
		return t.toDouble() * state.unitScale;
	};

	if (kind == QLatin1String("C")) {
		ap.kind = GerberAperture::Circle;
		if (parts.size() >= 1) ap.w = asMM(parts[0]);
		if (parts.size() >= 2) ap.holeDiameter = asMM(parts[1]);
	} else if (kind == QLatin1String("R")) {
		ap.kind = GerberAperture::Rectangle;
		if (parts.size() >= 1) ap.w = asMM(parts[0]);
		if (parts.size() >= 2) ap.h = asMM(parts[1]);
		else                   ap.h = ap.w;
		if (parts.size() >= 3) ap.holeDiameter = asMM(parts[2]);
	} else if (kind == QLatin1String("O")) {
		ap.kind = GerberAperture::Obround;
		if (parts.size() >= 1) ap.w = asMM(parts[0]);
		if (parts.size() >= 2) ap.h = asMM(parts[1]);
		else                   ap.h = ap.w;
		if (parts.size() >= 3) ap.holeDiameter = asMM(parts[2]);
	} else if (kind == QLatin1String("P")) {
		ap.kind = GerberAperture::Polygon;
		if (parts.size() >= 1) ap.w           = asMM(parts[0]);
		if (parts.size() >= 2) ap.vertices    = parts[1].toInt();
		if (parts.size() >= 3) ap.rotationDeg = parts[2].toDouble();
		if (parts.size() >= 4) ap.holeDiameter = asMM(parts[3]);
	}
	doc.apertures.insert(code, ap);
}

void parseExtended(const QString &s, GerberDocument &doc, ParserState &state)
{
	if (s.startsWith(QLatin1String("FS"))) {
		QRegularExpression re(QStringLiteral("X(\\d)(\\d)Y(\\d)(\\d)"));
		auto m = re.match(s);
		if (m.hasMatch()) {
			state.xIntDigits = m.captured(1).toInt();
			state.xDecDigits = m.captured(2).toInt();
			state.yIntDigits = m.captured(3).toInt();
			state.yDecDigits = m.captured(4).toInt();
		} else {
			doc.warnings << QStringLiteral("FS: could not parse format spec: ") + s;
		}
		return;
	}
	if (s.startsWith(QLatin1String("MO"))) {
		if      (s.contains(QLatin1String("IN"))) state.unitScale = 25.4;
		else if (s.contains(QLatin1String("MM"))) state.unitScale = 1.0;
		else doc.warnings << QStringLiteral("MO: unrecognised units: ") + s;
		return;
	}
	if (s.startsWith(QLatin1String("AD"))) {
		parseAperture(s.mid(2), doc, state);
		return;
	}
	if (s.startsWith(QLatin1String("AM"))) {
		doc.warnings << QStringLiteral("aperture macro (AM) not supported; "
			"will be substituted with a 0.1 mm circle");
		return;
	}
	if (s.startsWith(QLatin1String("SR"))) {
		doc.warnings << QStringLiteral("step-and-repeat (SR) not supported");
		return;
	}
	// Polarity / X3 attributes — recognised, not used by the renderer yet.
	static const QStringList silentPrefixes = {
		QStringLiteral("LP"), QStringLiteral("IP"), QStringLiteral("OF"),
		QStringLiteral("IR"), QStringLiteral("AS"), QStringLiteral("MI"),
		QStringLiteral("SF"), QStringLiteral("TF"), QStringLiteral("TA"),
		QStringLiteral("TO"), QStringLiteral("TD")
	};
	for (const QString &p : silentPrefixes) {
		if (s.startsWith(p)) return;
	}
	doc.warnings << QStringLiteral("unhandled extended cmd: ") + s.left(40);
}

void parseCoordinate(const QString &s, GerberDocument &doc, ParserState &state)
{
	QPointF target = state.cursor;   // modal: omitted axis reuses prior

	QRegularExpression coord(QStringLiteral("([XYIJ])(-?\\d+)"));
	auto it = coord.globalMatch(s);
	while (it.hasNext()) {
		auto m = it.next();
		const QString axis = m.captured(1);
		const QString raw  = m.captured(2);
		if (axis == QLatin1String("X")) {
			target.setX(decodeCoord(raw, state.xDecDigits, state.unitScale));
		} else if (axis == QLatin1String("Y")) {
			target.setY(decodeCoord(raw, state.yDecDigits, state.unitScale));
		}
		// I/J ignored — see G02/G03 warning above.
	}

	int op = 1;    // default D01 if implicit
	QRegularExpression dop(QStringLiteral("D0?(\\d)$"));
	auto m = dop.match(s);
	if (m.hasMatch()) op = m.captured(1).toInt();

	switch (op) {
		case 1: {  // stroke / region edge
			if (state.inRegion) {
				if (state.regionPath.isEmpty())
					state.regionPath.moveTo(state.cursor);
				state.regionPath.lineTo(target);
			} else {
				GerberCommand cmd;
				cmd.kind         = GerberCommand::Stroke;
				cmd.apertureCode = state.activeAperture;
				cmd.a            = state.cursor;
				cmd.b            = target;
				doc.commands.append(cmd);
			}
			doc.extend(target);
			doc.extend(state.cursor);
			break;
		}
		case 2: {  // move only
			if (state.inRegion) state.regionPath.moveTo(target);
			doc.extend(target);
			break;
		}
		case 3: {  // flash
			GerberCommand cmd;
			cmd.kind         = GerberCommand::Flash;
			cmd.apertureCode = state.activeAperture;
			cmd.b            = target;
			doc.commands.append(cmd);
			doc.extend(target, 1.0);  // 1 mm slack per flash
			break;
		}
		default:
			doc.warnings << QStringLiteral("unknown D-op in: ") + s;
	}
	state.cursor = target;
}

void handleStatement(const QString &s, GerberDocument &doc, ParserState &state)
{
	if (s.isEmpty()) return;

	if (s == QLatin1String("G36")) { state.inRegion = true;  state.regionPath = QPainterPath(); return; }
	if (s == QLatin1String("G37")) {
		state.inRegion = false;
		if (!state.regionPath.isEmpty()) {
			GerberCommand cmd;
			cmd.kind   = GerberCommand::Region;
			cmd.region = state.regionPath;
			doc.commands.append(cmd);
		}
		state.regionPath = QPainterPath();
		return;
	}
	if (s == QLatin1String("G01") || s == QLatin1String("G1"))  return;
	if (s == QLatin1String("G02") || s == QLatin1String("G2") ||
	    s == QLatin1String("G03") || s == QLatin1String("G3"))  {
		doc.warnings << QStringLiteral("arc interpolation (G02/G03) "
			"approximated as straight stroke");
		return;
	}
	if (s == QLatin1String("G70")) { state.unitScale = 25.4; return; }
	if (s == QLatin1String("G71")) { state.unitScale = 1.0;  return; }
	if (s == QLatin1String("G75") || s == QLatin1String("G74")) return;
	if (s == QLatin1String("M02") || s == QLatin1String("M0") ||
	    s == QLatin1String("M00")) return;
	if (s.startsWith(QLatin1String("G04")) ||
	    s.startsWith(QLatin1String("G4 "))) return;

	// Pure D-code: aperture select.
	{
		QRegularExpression re(QStringLiteral("^G?54?D(\\d+)$"));
		auto mm = re.match(s);
		if (mm.hasMatch()) {
			state.activeAperture = mm.captured(1).toInt();
			return;
		}
	}

	if (s.contains(QLatin1Char('X')) || s.contains(QLatin1Char('Y')) ||
	    s.endsWith(QLatin1String("D01")) ||
	    s.endsWith(QLatin1String("D02")) ||
	    s.endsWith(QLatin1String("D03"))) {
		parseCoordinate(s, doc, state);
		return;
	}

	doc.warnings << QStringLiteral("unrecognised statement: ") + s;
}

} // namespace


GerberDocument GerberParser::parse(QIODevice *device) const
{
	GerberDocument doc;
	if (!device) {
		doc.warnings << QStringLiteral("null device");
		return doc;
	}
	if (!device->isOpen() && !device->open(QIODevice::ReadOnly | QIODevice::Text)) {
		doc.warnings << QStringLiteral("could not open device");
		return doc;
	}
	const QByteArray bytes = device->readAll();
	return parse(QString::fromUtf8(bytes));
}

GerberDocument GerberParser::parse(const QString &text) const
{
	GerberDocument doc;
	ParserState    state;

	// Two-stage tokenisation: extended commands (%...%) wrap one or
	// more inner statements that we still split on '*'.
	int        i = 0;
	const int  n = text.size();
	bool       inExtended = false;
	QString    extendedBuf;

	auto flushExtended = [&]() {
		if (extendedBuf.isEmpty()) return;
		const QStringList parts = extendedBuf.split('*', Qt::SkipEmptyParts);
		for (const QString &part : parts) {
			const QString trimmed = part.trimmed();
			if (trimmed.isEmpty()) continue;
			parseExtended(trimmed, doc, state);
		}
		extendedBuf.clear();
	};

	QString stmt;
	while (i < n) {
		const QChar c = text.at(i);
		if (inExtended) {
			if (c == QLatin1Char('%')) {
				flushExtended();
				inExtended = false;
			} else {
				extendedBuf.append(c);
			}
			++i;
			continue;
		}
		if (c == QLatin1Char('%')) {
			if (!stmt.trimmed().isEmpty()) {
				handleStatement(stmt.trimmed(), doc, state);
			}
			stmt.clear();
			inExtended = true;
			++i;
			continue;
		}
		if (c == QLatin1Char('*')) {
			handleStatement(stmt.trimmed(), doc, state);
			stmt.clear();
			++i;
			continue;
		}
		if (c == QLatin1Char('\r') || c == QLatin1Char('\n')) {
			++i;
			continue;
		}
		stmt.append(c);
		++i;
	}
	if (!stmt.trimmed().isEmpty()) {
		handleStatement(stmt.trimmed(), doc, state);
	}
	if (inExtended) {
		doc.warnings << QStringLiteral("unterminated %% extended block");
	}
	return doc;
}
