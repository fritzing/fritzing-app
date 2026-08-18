#include "breadboardcsvdipphysicalresolver.h"

#include "breadboardcoordinate.h"

#include <QtGlobal>

#include <QList>
#include <QStringList>

namespace {

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

int rowPositionMil(QChar row)
{
        if (
                row >= QChar('A') &&
                row <= QChar('E')
        ) {
                return
                        (
                                row.unicode() -
                                QChar('A').unicode()
                        ) *
                        100;
        }

        if (
                row >= QChar('F') &&
                row <= QChar('J')
        ) {
                return
                        700 +
                        (
                                row.unicode() -
                                QChar('F').unicode()
                        ) *
                        100;
        }

        return -1;
}

} // namespace


bool BreadboardCsvDipPhysicalResolver::apply(
        BreadboardCsvDipFootprint &footprint,
        int spacingMil,
        QString &error
)
{
        error.clear();

        QChar physicalPin1Row;
        QChar physicalOppositeRow;

        if (spacingMil == 300) {
                /*
                 * A standard 300 mil DIP occupies E/F across the
                 * BB830 center channel.
                 */
                physicalPin1Row =
                        QChar('E');

                physicalOppositeRow =
                        QChar('F');
        }
        else if (spacingMil == 600) {
                struct RowPair
                {
                        QChar pin1Row;
                        QChar oppositeRow;
                };

                /*
                 * Standard 600 mil DIP positions that span the
                 * BB830 center channel.
                 */
                static const RowPair rowPairs[] = {
                        { QChar('B'), QChar('F') },
                        { QChar('C'), QChar('G') },
                        { QChar('D'), QChar('H') },
                        { QChar('E'), QChar('I') }
                };

                const int referenceTop =
                        rowPositionMil(
                                footprint.referencePin1Row
                        );

                const int referenceBottom =
                        rowPositionMil(
                                footprint.referenceOppositeRow
                        );

                if (
                        referenceTop < 0 ||
                        referenceBottom < 0
                ) {
                        error =
                                QString(
                                        "%1 has invalid CSV reference rows "
                                        "%2/%3."
                                )
                                        .arg(footprint.component)
                                        .arg(
                                                QString(
                                                        1,
                                                        footprint.referencePin1Row
                                                )
                                        )
                                        .arg(
                                                QString(
                                                        1,
                                                        footprint.referenceOppositeRow
                                                )
                                        );

                        return false;
                }

                int bestScore = -1;
                QList<RowPair> bestPairs;

                for (const RowPair &pair : rowPairs) {
                        const int score =
                                qAbs(
                                        referenceTop -
                                        rowPositionMil(pair.pin1Row)
                                ) +
                                qAbs(
                                        referenceBottom -
                                        rowPositionMil(pair.oppositeRow)
                                );

                        if (
                                bestScore < 0 ||
                                score < bestScore
                        ) {
                                bestScore =
                                        score;

                                bestPairs.clear();
                                bestPairs.append(
                                        pair
                                );
                        }
                        else if (score == bestScore) {
                                bestPairs.append(
                                        pair
                                );
                        }
                }

                if (bestPairs.size() != 1) {
                        QStringList descriptions;

                        for (const RowPair &pair : bestPairs) {
                                descriptions.append(
                                        QString("%1/%2")
                                                .arg(
                                                        QString(
                                                                1,
                                                                pair.pin1Row
                                                        )
                                                )
                                                .arg(
                                                        QString(
                                                                1,
                                                                pair.oppositeRow
                                                        )
                                                )
                                );
                        }

                        error =
                                QString(
                                        "%1 is known to be a 600 mil DIP, "
                                        "but CSV reference rows %2/%3 do not "
                                        "uniquely determine its physical "
                                        "BB830 row pair. Candidates: %4"
                                )
                                        .arg(footprint.component)
                                        .arg(
                                                QString(
                                                        1,
                                                        footprint.referencePin1Row
                                                )
                                        )
                                        .arg(
                                                QString(
                                                        1,
                                                        footprint.referenceOppositeRow
                                                )
                                        )
                                        .arg(
                                                descriptions.join(", ")
                                        );

                        return false;
                }

                physicalPin1Row =
                        bestPairs.first().pin1Row;

                physicalOppositeRow =
                        bestPairs.first().oppositeRow;
        }
        else {
                error =
                        QString(
                                "%1 has unsupported DIP spacing %2 mil."
                        )
                                .arg(footprint.component)
                                .arg(spacingMil);

                return false;
        }

        footprint.spacingMil =
                spacingMil;

        footprint.pin1Row =
                physicalPin1Row;

        footprint.oppositeRow =
                physicalOppositeRow;

        const BreadboardCoordinate pin1 =
                parsedTerminal(
                        footprint.board,
                        footprint.pin1Row,
                        footprint.firstColumn
                );

        const BreadboardCoordinate pinHalf =
                parsedTerminal(
                        footprint.board,
                        footprint.pin1Row,
                        footprint.lastColumn
                );

        const BreadboardCoordinate pinHalfPlus1 =
                parsedTerminal(
                        footprint.board,
                        footprint.oppositeRow,
                        footprint.lastColumn
                );

        const BreadboardCoordinate pinLast =
                parsedTerminal(
                        footprint.board,
                        footprint.oppositeRow,
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
                error =
                        QString(
                                "%1 resolved to an invalid physical "
                                "DIP corner."
                        )
                                .arg(footprint.component);

                return false;
        }

        footprint.pin1Coordinate =
                terminalCoordinate(
                        footprint.board,
                        footprint.pin1Row,
                        footprint.firstColumn
                );

        footprint.pinHalfCoordinate =
                terminalCoordinate(
                        footprint.board,
                        footprint.pin1Row,
                        footprint.lastColumn
                );

        footprint.pinHalfPlus1Coordinate =
                terminalCoordinate(
                        footprint.board,
                        footprint.oppositeRow,
                        footprint.lastColumn
                );

        footprint.pinLastCoordinate =
                terminalCoordinate(
                        footprint.board,
                        footprint.oppositeRow,
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

        return true;
}
