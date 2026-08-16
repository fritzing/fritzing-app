#include "breadboardcoordinate.h"

#include <QRegularExpression>
#include <QSet>

bool BreadboardCoordinateParser::isValidRailPosition(int position)
{
	static const QSet<int> validPositions = {
		3, 4, 5, 6, 7,
		9, 10, 11, 12, 13,
		15, 16, 17, 18, 19,
		21, 22, 23, 24, 25,
		27, 28, 29, 30, 31,
		33, 34, 35, 36, 37,
		39, 40, 41, 42, 43,
		45, 46, 47, 48, 49,
		51, 52, 53, 54, 55,
		57, 58, 59, 60, 61
	};

	return validPositions.contains(position);
}

BreadboardCoordinate BreadboardCoordinateParser::parse(const QString &text)
{
	BreadboardCoordinate result;

	static const QRegularExpression terminalPattern(
		"^B([1-3])-([A-J])([1-9][0-9]?)$"
	);

	static const QRegularExpression railPattern(
		"^B([1-3])-(TOP|BOT)([+-])-([1-9][0-9]?)$"
	);

	QRegularExpressionMatch match = terminalPattern.match(text);

	if (match.hasMatch()) {
		result.board = match.captured(1).toInt();
		result.row = match.captured(2).at(0);
		result.column = match.captured(3).toInt();

		if (result.column < 1 || result.column > 63) {
			result.error =
				QString("Terminal-strip column out of range: %1")
					.arg(result.column);
			return result;
		}

		result.kind = BreadboardCoordinate::Kind::TerminalStrip;
		result.connectorId =
			QString("pin%1%2")
				.arg(result.column)
				.arg(result.row);

		return result;
	}

	match = railPattern.match(text);

	if (match.hasMatch()) {
		result.board = match.captured(1).toInt();
		result.rail = match.captured(2);
		result.polarity = match.captured(3).at(0);
		result.column = match.captured(4).toInt();

		if (!isValidRailPosition(result.column)) {
			result.error =
				QString("No physical power-rail hole exists at position %1")
					.arg(result.column);
			return result;
		}

		QChar railLetter;

		if (result.rail == "TOP") {
			railLetter =
				result.polarity == '+' ? QChar('Y') : QChar('Z');
		}
		else {
			railLetter =
				result.polarity == '+' ? QChar('W') : QChar('X');
		}

		result.kind = BreadboardCoordinate::Kind::PowerRail;
		result.connectorId =
			QString("pin%1%2")
				.arg(result.column)
				.arg(railLetter);

		return result;
	}

	result.error =
		QString("Unrecognized breadboard coordinate: %1").arg(text);

	return result;
}
