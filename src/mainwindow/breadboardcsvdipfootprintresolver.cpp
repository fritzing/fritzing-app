#include "breadboardcsvdipfootprintresolver.h"

#include "breadboardcoordinate.h"

#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>

#include <algorithm>

namespace {

using PinCoordinates = QHash<int, QSet<QString>>;

struct ComponentObservation
{
        QString component;
        PinCoordinates pins;
};

struct EndpointPinReference
{
        bool ok = false;
        QString component;
        int pin = 0;
};

struct DipCandidate
{
        int pinCount = 0;
        int board = 0;
        QChar pin1Row;
        QChar oppositeRow;
        int firstColumn = 0;
        int score = 0;
};

EndpointPinReference parseEndpointPin(const QString &endpoint)
{
        EndpointPinReference result;

        /*
         * Component names are deliberately opaque here.
         *
         * Examples:
         *
         *   CPU_A pin 12
         *   U1 pin 12
         *   My Custom CPU pin 12
         *
         * No component names or semiconductor families belong in
         * this parser.
         */
        static const QRegularExpression pattern(
                R"(^\s*(.+?)\s+pin\s+([1-9][0-9]*)\b)",
                QRegularExpression::CaseInsensitiveOption
        );

        const QRegularExpressionMatch match =
                pattern.match(endpoint);

        if (!match.hasMatch()) {
                return result;
        }

        const QString component =
                match.captured(1).trimmed();

        bool pinOk = false;

        const int pin =
                match.captured(2).toInt(&pinOk);

        if (
                component.isEmpty() ||
                !pinOk ||
                pin < 1
        ) {
                return result;
        }

        result.ok = true;
        result.component = component;
        result.pin = pin;

        return result;
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


bool hasBothDipSides(const PinCoordinates &pinCoordinates)
{
        bool top = false;
        bool bottom = false;

        for (auto pinIt = pinCoordinates.constBegin();
             pinIt != pinCoordinates.constEnd();
             ++pinIt) {

                for (const QString &coordinateText : pinIt.value()) {
                        const BreadboardCoordinate coordinate =
                                BreadboardCoordinateParser::parse(
                                        coordinateText
                                );

                        if (
                                coordinate.kind !=
                                BreadboardCoordinate::Kind::TerminalStrip
                        ) {
                                continue;
                        }

                        if (
                                coordinate.row >= QChar('A') &&
                                coordinate.row <= QChar('E')
                        ) {
                                top = true;
                        }

                        if (
                                coordinate.row >= QChar('F') &&
                                coordinate.row <= QChar('J')
                        ) {
                                bottom = true;
                        }
                }
        }

        return top && bottom;
}

QList<int> observedBoards(
        const PinCoordinates &pinCoordinates
)
{
        QSet<int> boardSet;

        for (auto pinIt = pinCoordinates.constBegin();
             pinIt != pinCoordinates.constEnd();
             ++pinIt) {

                for (const QString &coordinateText : pinIt.value()) {
                        const BreadboardCoordinate coordinate =
                                BreadboardCoordinateParser::parse(
                                        coordinateText
                                );

                        if (
                                coordinate.kind ==
                                BreadboardCoordinate::Kind::TerminalStrip
                        ) {
                                boardSet.insert(coordinate.board);
                        }
                }
        }

        QList<int> boards =
                boardSet.values();

        std::sort(
                boards.begin(),
                boards.end()
        );

        return boards;
}

int scoreCandidate(
        const PinCoordinates &pinCoordinates,
        const DipCandidate &candidate
)
{
        const int halfPins =
                candidate.pinCount / 2;

        int score = 0;

        for (auto pinIt = pinCoordinates.constBegin();
             pinIt != pinCoordinates.constEnd();
             ++pinIt) {

                const int pin =
                        pinIt.key();

                if (
                        pin < 1 ||
                        pin > candidate.pinCount
                ) {
                        continue;
                }

                QChar expectedRow;
                int expectedColumn = 0;

                if (pin <= halfPins) {
                        expectedRow =
                                candidate.pin1Row;

                        expectedColumn =
                                candidate.firstColumn +
                                pin - 1;
                }
                else {
                        expectedRow =
                                candidate.oppositeRow;

                        expectedColumn =
                                candidate.firstColumn +
                                candidate.pinCount -
                                pin;
                }

                const QString expected =
                        terminalCoordinate(
                                candidate.board,
                                expectedRow,
                                expectedColumn
                        );

                if (pinIt.value().contains(expected)) {
                        ++score;
                }
        }

        return score;
}

QString candidateDescription(
        const DipCandidate &candidate
)
{
        return QString(
                "%1-pin B%2 %3/%4 cols %5-%6"
        )
                .arg(candidate.pinCount)
                .arg(candidate.board)
                .arg(candidate.pin1Row)
                .arg(candidate.oppositeRow)
                .arg(candidate.firstColumn)
                .arg(
                        candidate.firstColumn +
                        candidate.pinCount / 2 -
                        1
                );
}

/*
 * Translate the hole rows found in the wiring CSV into the physical
 * DIP leg rows used by Fritzing.
 *
 * A CSV endpoint identifies an electrically equivalent hole on a
 * BB830 strip. It is not guaranteed to be the exact hole occupied
 * by the component leg.
 *
 * Therefore this is BB830 geometry knowledge, not semiconductor
 * knowledge.
 */

} // namespace

BreadboardCsvDipFootprintResult
BreadboardCsvDipFootprintResolver::resolveAll(
        const QList<BreadboardWiringCsvRow> &rows
)
{
        BreadboardCsvDipFootprintResult result;

        /*
         * Keyed case-insensitively so cosmetic capitalization
         * differences do not create separate physical components.
         * The first spelling seen is preserved for display.
         */
        QHash<QString, ComponentObservation> observations;

        auto capture =
                [&](
                        const QString &endpoint,
                        const QString &terminal
                ) {
                        const BreadboardCoordinate coordinate =
                                BreadboardCoordinateParser::parse(
                                        terminal.trimmed()
                                );

                        if (
                                coordinate.kind !=
                                BreadboardCoordinate::Kind::TerminalStrip
                        ) {
                                return;
                        }

                        const EndpointPinReference endpointPin =
                                parseEndpointPin(endpoint);

                        if (!endpointPin.ok) {
                                return;
                        }

                        const QString key =
                                endpointPin.component.toCaseFolded();

                        ComponentObservation &observation =
                                observations[key];

                        if (observation.component.isEmpty()) {
                                observation.component =
                                        endpointPin.component;
                        }

                        /*
                         * Normalize the coordinate string before
                         * storing it so comparison does not depend on
                         * how the CSV happened to format the text.
                         */
                        observation.pins[endpointPin.pin].insert(
                                terminalCoordinate(
                                        coordinate.board,
                                        coordinate.row,
                                        coordinate.column
                                )
                        );
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

        if (observations.isEmpty()) {
                result.error =
                        "No component pin references were found in the CSV.";

                return result;
        }

        QStringList componentKeys =
                observations.keys();

        std::sort(
                componentKeys.begin(),
                componentKeys.end(),
                [](const QString &left, const QString &right) {
                        return left.compare(
                                right,
                                Qt::CaseInsensitive
                        ) < 0;
                }
        );

        for (const QString &key : componentKeys) {
                const ComponentObservation observation =
                        observations.value(key);

                const PinCoordinates &pinCoordinates =
                        observation.pins;

                /*
                 * Geometry-only inference needs evidence from both
                 * sides of a DIP and several independent pins.
                 *
                 * Sparse or non-DIP parts are not force-fit here.
                 * They will be handled by the native/custom part
                 * resolver layer.
                 */
                if (
                        pinCoordinates.size() < 4 ||
                        !hasBothDipSides(pinCoordinates)
                ) {
                        continue;
                }

                int maximumObservedPin = 0;

                for (auto pinIt = pinCoordinates.constBegin();
                     pinIt != pinCoordinates.constEnd();
                     ++pinIt) {
                        maximumObservedPin =
                                std::max(
                                        maximumObservedPin,
                                        pinIt.key()
                                );
                }

                int firstPinCount =
                        std::max(
                                4,
                                maximumObservedPin
                        );

                if ((firstPinCount % 2) != 0) {
                        ++firstPinCount;
                }

                /*
                 * A BB830 has 63 columns. A DIP has half its pins on
                 * each side, so 126 is the largest package that can
                 * possibly fit this board model.
                 *
                 * This is a board-geometry limit, not a component
                 * catalog.
                 */
                constexpr int maximumPinCount = 126;

                const QList<int> boards =
                        observedBoards(pinCoordinates);

                int bestScore = -1;
                QList<DipCandidate> bestCandidates;

                for (
                        int pinCount = firstPinCount;
                        pinCount <= maximumPinCount;
                        pinCount += 2
                ) {
                        const int halfPins =
                                pinCount / 2;

                        const int maximumFirstColumn =
                                63 - halfPins + 1;

                        if (maximumFirstColumn < 1) {
                                continue;
                        }

                        for (int board : boards) {
                                for (
                                        char topLetter = 'A';
                                        topLetter <= 'E';
                                        ++topLetter
                                ) {
                                        const QChar pin1Row =
                                                QChar::fromLatin1(
                                                        topLetter
                                                );

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
                                                        DipCandidate candidate;

                                                        candidate.pinCount =
                                                                pinCount;

                                                        candidate.board =
                                                                board;

                                                        candidate.pin1Row =
                                                                pin1Row;

                                                        candidate.oppositeRow =
                                                                oppositeRow;

                                                        candidate.firstColumn =
                                                                firstColumn;

                                                        candidate.score =
                                                                scoreCandidate(
                                                                        pinCoordinates,
                                                                        candidate
                                                                );

                                                        if (
                                                                candidate.score >
                                                                bestScore
                                                        ) {
                                                                bestScore =
                                                                        candidate.score;

                                                                bestCandidates.clear();
                                                                bestCandidates.append(
                                                                        candidate
                                                                );
                                                        }
                                                        else if (
                                                                candidate.score ==
                                                                bestScore
                                                        ) {
                                                                bestCandidates.append(
                                                                        candidate
                                                                );
                                                        }
                                                }
                                        }
                                }
                        }
                }

                const int observedPins =
                        pinCoordinates.size();

                if (
                        bestScore != observedPins ||
                        bestCandidates.isEmpty()
                ) {
                        QStringList candidates;

                        const int count =
                                std::min(
                                        6,
                                        static_cast<int>(
                                                bestCandidates.size()
                                        )
                                );

                        for (int i = 0; i < count; ++i) {
                                candidates.append(
                                        candidateDescription(
                                                bestCandidates.at(i)
                                        )
                                );
                        }

                        result.error =
                                QString(
                                        "%1 could not be resolved as a DIP "
                                        "without component-specific rules.\n\n"
                                        "Observed pins: %2\n"
                                        "Best geometric score: %3/%2"
                                )
                                        .arg(observation.component)
                                        .arg(observedPins)
                                        .arg(bestScore);

                        if (!candidates.isEmpty()) {
                                result.error +=
                                        "\nBest candidates:\n" +
                                        candidates.join("\n");
                        }

                        return result;
                }

                if (bestCandidates.size() != 1) {
                        QStringList candidates;

                        const int count =
                                std::min(
                                        8,
                                        static_cast<int>(
                                                bestCandidates.size()
                                        )
                                );

                        for (int i = 0; i < count; ++i) {
                                candidates.append(
                                        candidateDescription(
                                                bestCandidates.at(i)
                                        )
                                );
                        }

                        result.error =
                                QString(
                                        "%1 has %2 equally valid DIP "
                                        "footprints. The importer will not "
                                        "guess.\n\n%3"
                                )
                                        .arg(observation.component)
                                        .arg(bestCandidates.size())
                                        .arg(candidates.join("\n"));

                        return result;
                }

                const DipCandidate best =
                        bestCandidates.first();

                BreadboardCsvDipFootprint footprint;

                footprint.component =
                        observation.component;

                footprint.pinCount =
                        best.pinCount;

                footprint.board =
                        best.board;

                footprint.firstColumn =
                        best.firstColumn;

                footprint.lastColumn =
                        best.firstColumn +
                        best.pinCount / 2 -
                        1;

                footprint.observedPins =
                        observedPins;

                footprint.matchedPins =
                        best.score;

                /*
                 * Stop here at geometry.
                 *
                 * The CSV tells us which electrical strips best fit
                 * the observed pin numbering. It does NOT establish
                 * the physical DIP package width.
                 */
                footprint.referencePin1Row =
                        best.pin1Row;

                footprint.referenceOppositeRow =
                        best.oppositeRow;

                footprint.ok = true;

                result.footprints.append(
                        footprint
                );
        }

        if (result.footprints.isEmpty()) {
                result.error =
                        "No DIP footprints could be inferred from the CSV.";

                return result;
        }

        /*
         * QHash discovery order is intentionally irrelevant.
         * Placement order must instead be deterministic and physical.
         */
        std::sort(
                result.footprints.begin(),
                result.footprints.end(),
                [](
                        const BreadboardCsvDipFootprint &left,
                        const BreadboardCsvDipFootprint &right
                ) {
                        if (left.board != right.board) {
                                return left.board < right.board;
                        }

                        if (
                                left.firstColumn !=
                                right.firstColumn
                        ) {
                                return
                                        left.firstColumn <
                                        right.firstColumn;
                        }

                        return left.component.compare(
                                right.component,
                                Qt::CaseInsensitive
                        ) < 0;
                }
        );

        result.ok = true;

        return result;
}
