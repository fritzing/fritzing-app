#include "breadboardcsvdipfootprintresolver.h"

#include "breadboardcoordinate.h"

#include <QHash>
#include <QSet>

namespace {

struct DipDefinition
{
	QString component;
	int pinCount;
	int spacingMil;
};

using PinCoordinates = QHash<int, QSet<QString>>;

const QList<DipDefinition> &dipCatalog()
{
	static const QList<DipDefinition> definitions = {
		{ "W65C02", 40, 600 },
		{ "HCT245", 20, 300 },
		{ "HCT14", 14, 300 },
		{ "HCT157", 16, 300 },
		{ "HCT00", 14, 300 },

		{ "SRAM", 28, 600 },
		{ "EEPROM", 28, 600 },
		{ "HCT138", 16, 300 },
		{ "HCT08", 14, 300 },
		{ "HCT573", 20, 300 },

		{ "VIA1", 40, 600 },
		{ "VIA2", 40, 600 },
		{ "ACIA", 28, 600 },
		{ "LM386", 8, 300 }
	};

	return definitions;
}

int pinNumberForEndpoint(
	const QString &endpoint,
	const DipDefinition &definition
)
{
	const QString prefix =
		definition.component + " pin ";

	const QString text = endpoint.trimmed();

	if (!text.startsWith(prefix)) {
		return -1;
	}

	qsizetype end = prefix.size();

	while (
		end < text.size() &&
		text.at(end).isDigit()
	) {
		++end;
	}

	if (end == prefix.size()) {
		return -1;
	}

	bool ok = false;

	const int pin =
		text.mid(
			prefix.size(),
			end - prefix.size()
		).toInt(&ok);

	if (
		!ok ||
		pin < 1 ||
		pin > definition.pinCount
	) {
		return -1;
	}

	return pin;
}

QString terminalCoordinate(
	int board,
	QChar row,
	int column
)
{
	return QString("B%1-%2%3")
		.arg(board)
		.arg(QString(1, row))
		.arg(column);
}

BreadboardCoordinate parsedTerminal(
	int board,
	QChar row,
	int column
)
{
	return BreadboardCoordinateParser::parse(
		terminalCoordinate(
			board,
			row,
			column
		)
	);
}

} // namespace

BreadboardCsvDipFootprintResult
BreadboardCsvDipFootprintResolver::resolveAll(
	const QList<BreadboardWiringCsvRow> &rows
)
{
	BreadboardCsvDipFootprintResult result;

	QHash<QString, PinCoordinates> observations;

	auto capture =
		[&](
			const QString &endpoint,
			const QString &terminal
		) {
			const QString coordinateText =
				terminal.trimmed();

			const BreadboardCoordinate coordinate =
				BreadboardCoordinateParser::parse(
					coordinateText
				);

			if (
				coordinate.kind !=
				BreadboardCoordinate::Kind::TerminalStrip
			) {
				return;
			}

			for (
				const DipDefinition &definition :
				dipCatalog()
			) {
				const int pin =
					pinNumberForEndpoint(
						endpoint,
						definition
					);

				if (pin < 1) {
					continue;
				}

				observations[definition.component][pin]
					.insert(coordinateText);

				return;
			}
		};

	for (const BreadboardWiringCsvRow &row : rows) {
		capture(
			row.from,
			row.fromTerminal
		);

		capture(
			row.to,
			row.toTerminal
		);
	}

	for (
		const DipDefinition &definition :
		dipCatalog()
	) {
		BreadboardCsvDipFootprint footprint;

		footprint.component =
			definition.component;

		footprint.pinCount =
			definition.pinCount;

		footprint.spacingMil =
			definition.spacingMil;

		const PinCoordinates pinCoordinates =
			observations.value(
				definition.component
			);

		footprint.observedPins =
			pinCoordinates.size();

		if (footprint.observedPins == 0) {
			result.error =
				QString(
					"%1 has no terminal-strip pin "
					"references in the CSV."
				)
					.arg(definition.component);

			return result;
		}

		const int halfPins =
			definition.pinCount / 2;

		int bestScore = -1;
		int bestCandidateCount = 0;

		int bestBoard = 0;
		QChar bestPin1Row;
		QChar bestOppositeRow;
		int bestFirstColumn = 0;

		const int maximumFirstColumn =
			63 - halfPins + 1;

		for (int board = 1; board <= 3; ++board) {
			for (
				char topLetter = 'A';
				topLetter <= 'E';
				++topLetter
			) {
				const QChar pin1Row =
					QChar::fromLatin1(topLetter);

				for (
					char bottomLetter = 'F';
					bottomLetter <= 'J';
					++bottomLetter
				) {
					const QChar oppositeRow =
						QChar::fromLatin1(
							bottomLetter
						);

					for (
						int firstColumn = 1;
						firstColumn <=
							maximumFirstColumn;
						++firstColumn
					) {
						int score = 0;

						for (
							auto pinIt =
								pinCoordinates.constBegin();
							pinIt !=
								pinCoordinates.constEnd();
							++pinIt
						) {
							const int pin =
								pinIt.key();

							QChar expectedRow;
							int expectedColumn = 0;

							if (pin <= halfPins) {
								expectedRow =
									pin1Row;

								expectedColumn =
									firstColumn +
									pin - 1;
							}
							else {
								expectedRow =
									oppositeRow;

								expectedColumn =
									firstColumn +
									definition.pinCount -
									pin;
							}

							const QString expected =
								terminalCoordinate(
									board,
									expectedRow,
									expectedColumn
								);

							if (
								pinIt.value().contains(
									expected
								)
							) {
								++score;
							}
						}

						if (score > bestScore) {
							bestScore = score;
							bestCandidateCount = 1;

							bestBoard = board;
							bestPin1Row =
								pin1Row;

							bestOppositeRow =
								oppositeRow;

							bestFirstColumn =
								firstColumn;
						}
						else if (
							score == bestScore
						) {
							++bestCandidateCount;
						}
					}
				}
			}
		}

		footprint.matchedPins =
			bestScore;

		if (
			bestScore != footprint.observedPins
		) {
			result.error =
				QString(
					"%1 DIP footprint matched only "
					"%2 of %3 observed pins."
				)
					.arg(definition.component)
					.arg(bestScore)
					.arg(footprint.observedPins);

			return result;
		}

		if (bestCandidateCount != 1) {
			result.error =
				QString(
					"%1 DIP footprint is ambiguous: "
					"%2 candidates matched all "
					"%3 observed pins."
				)
					.arg(definition.component)
					.arg(bestCandidateCount)
					.arg(footprint.observedPins);

			return result;
		}

		/*
		 * CSV component endpoints identify electrically equivalent
		 * breadboard-strip holes, not necessarily the physical hole
		 * occupied by the DIP leg.
		 *
		 * On the BB830, the resolved 300 mil devices consistently
		 * reference D/G strip holes. A physical 300 mil DIP spans
		 * the center trench on E/F. Preserve the CSV-derived board
		 * and columns, but normalize the actual package leg rows.
		 *
		 * 600 mil footprints retain their CSV-derived rows; their
		 * B/F geometry has already been validated experimentally.
		 */
		if (definition.spacingMil == 300) {
			bestPin1Row = QChar('E');
			bestOppositeRow = QChar('F');
		}

		footprint.board =
			bestBoard;

		footprint.pin1Row =
			bestPin1Row;

		footprint.oppositeRow =
			bestOppositeRow;

		footprint.firstColumn =
			bestFirstColumn;

		footprint.lastColumn =
			bestFirstColumn +
			halfPins - 1;

		const BreadboardCoordinate pin1 =
			parsedTerminal(
				bestBoard,
				bestPin1Row,
				footprint.firstColumn
			);

		const BreadboardCoordinate pinHalf =
			parsedTerminal(
				bestBoard,
				bestPin1Row,
				footprint.lastColumn
			);

		const BreadboardCoordinate pinHalfPlus1 =
			parsedTerminal(
				bestBoard,
				bestOppositeRow,
				footprint.lastColumn
			);

		const BreadboardCoordinate pinLast =
			parsedTerminal(
				bestBoard,
				bestOppositeRow,
				footprint.firstColumn
			);

		if (
			pin1.kind !=
				BreadboardCoordinate::Kind::TerminalStrip ||
			pinHalf.kind !=
				BreadboardCoordinate::Kind::TerminalStrip ||
			pinHalfPlus1.kind !=
				BreadboardCoordinate::Kind::TerminalStrip ||
			pinLast.kind !=
				BreadboardCoordinate::Kind::TerminalStrip ||
			pin1.connectorId.isEmpty() ||
			pinHalf.connectorId.isEmpty() ||
			pinHalfPlus1.connectorId.isEmpty() ||
			pinLast.connectorId.isEmpty()
		) {
			result.error =
				QString(
					"%1 resolved to an invalid "
					"terminal-strip footprint."
				)
					.arg(definition.component);

			return result;
		}

		footprint.pin1Coordinate =
			terminalCoordinate(
				bestBoard,
				bestPin1Row,
				footprint.firstColumn
			);

		footprint.pinHalfCoordinate =
			terminalCoordinate(
				bestBoard,
				bestPin1Row,
				footprint.lastColumn
			);

		footprint.pinHalfPlus1Coordinate =
			terminalCoordinate(
				bestBoard,
				bestOppositeRow,
				footprint.lastColumn
			);

		footprint.pinLastCoordinate =
			terminalCoordinate(
				bestBoard,
				bestOppositeRow,
				footprint.firstColumn
			);

		footprint.pin1ConnectorId =
			pin1.connectorId;

		footprint.pinHalfConnectorId =
			pinHalf.connectorId;

		footprint.pinHalfPlus1ConnectorId =
			pinHalfPlus1.connectorId;

		footprint.pinLastConnectorId =
			pinLast.connectorId;

		footprint.ok = true;

		result.footprints.append(
			footprint
		);
	}

	result.ok = true;

	return result;
}
