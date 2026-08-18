#include "breadboardcsvlibrepcbprovider.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QList>
#include <QMap>
#include <QPointF>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QUuid>

#include <algorithm>
#include <cmath>

namespace {

struct IndexedElement
{
        QString uuid;
        QString relativePath;
};

struct IndexedDevice
{
        QString mpn;
        QString manufacturer;

        QString uuid;
        QString relativePath;

        QString componentUuid;
        QString packageUuid;
};

struct PackageGeometry
{
        bool ok = false;

        QString error;
        QString packageName;

        int pinCount = 0;
        int pitchMil = 0;
        int spacingMil = 0;

        QHash<QString, int> padUuidToPin;
};

QString decodeLibrePcbString(QString value)
{
        value.replace(QStringLiteral("\\n"), QStringLiteral("\n"));
        value.replace(QStringLiteral("\\r"), QStringLiteral("\r"));
        value.replace(QStringLiteral("\\t"), QStringLiteral("\t"));
        value.replace(QStringLiteral("\\\""), QStringLiteral("\""));
        value.replace(QStringLiteral("\\\\"), QStringLiteral("\\"));

        return value;
}

int parenthesisDelta(const QString &line)
{
        int delta = 0;
        bool quoted = false;
        bool escaped = false;

        for (const QChar ch : line) {
                if (escaped) {
                        escaped = false;
                        continue;
                }

                if (quoted && ch == QChar('\\')) {
                        escaped = true;
                        continue;
                }

                if (ch == QChar('"')) {
                        quoted = !quoted;
                        continue;
                }

                if (quoted) {
                        continue;
                }

                if (ch == QChar('(')) {
                        ++delta;
                }
                else if (ch == QChar(')')) {
                        --delta;
                }
        }

        return delta;
}

bool readLines(
        const QString &filePath,
        QStringList &lines,
        QString &error
)
{
        lines.clear();
        error.clear();

        QFile file(filePath);

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                error =
                        QStringLiteral("Could not open LibrePCB file: %1")
                                .arg(filePath);

                return false;
        }

        while (!file.atEnd()) {
                lines.append(
                        QString::fromUtf8(
                                file.readLine()
                        )
                                .remove(
                                        QRegularExpression(
                                                QStringLiteral("[\\r\\n]+$")
                                        )
                                )
                );
        }

        return true;
}

QString resolveIndexedFile(
        const QString &librariesRoot,
        const QString &relativeDirectory,
        const QString &fileName,
        QString &error
)
{
        error.clear();

        const QString cleanRelative =
                QDir::cleanPath(
                        relativeDirectory
                );

        if (
                cleanRelative.isEmpty() ||
                QDir::isAbsolutePath(cleanRelative) ||
                cleanRelative == QStringLiteral("..") ||
                cleanRelative.startsWith(QStringLiteral("../"))
        ) {
                error =
                        QStringLiteral(
                                "LibrePCB cache returned an unsafe relative path: %1"
                        )
                                .arg(relativeDirectory);

                return QString();
        }

        const QDir rootDir(librariesRoot);

        const QString candidate =
                QFileInfo(
                        rootDir.absoluteFilePath(
                                cleanRelative +
                                QStringLiteral("/") +
                                fileName
                        )
                ).canonicalFilePath();

        const QString canonicalRoot =
                QFileInfo(
                        librariesRoot
                ).canonicalFilePath();

        if (
                candidate.isEmpty() ||
                canonicalRoot.isEmpty() ||
                !candidate.startsWith(
                        canonicalRoot + QChar('/')
                )
        ) {
                error =
                        QStringLiteral(
                                "LibrePCB indexed file could not be resolved safely: %1/%2"
                        )
                                .arg(
                                        relativeDirectory,
                                        fileName
                                );

                return QString();
        }

        return candidate;
}

bool readDeviceFile(
        const QString &filePath,
        const QString &expectedDeviceUuid,
        QString &componentUuid,
        QString &packageUuid,
        QHash<QString, QString> &padToSignal,
        QString &error
)
{
        componentUuid.clear();
        packageUuid.clear();
        padToSignal.clear();
        error.clear();

        QStringList lines;

        if (!readLines(filePath, lines, error)) {
                return false;
        }

        static const QRegularExpression rootPattern(
                QStringLiteral(
                        "^\\s*\\(librepcb_device\\s+([0-9a-fA-F-]+)"
                )
        );

        static const QRegularExpression componentPattern(
                QStringLiteral(
                        "^\\s*\\(component\\s+([0-9a-fA-F-]+)\\)\\s*$"
                )
        );

        static const QRegularExpression packagePattern(
                QStringLiteral(
                        "^\\s*\\(package\\s+([0-9a-fA-F-]+)\\)\\s*$"
                )
        );

        static const QRegularExpression padPattern(
                QStringLiteral(
                        "^\\s*\\(pad\\s+([0-9a-fA-F-]+)\\s+"
                        "\\(signal\\s+([0-9a-fA-F-]+)\\)\\)\\s*$"
                )
        );

        bool rootSeen = false;

        for (const QString &line : lines) {
                QRegularExpressionMatch match =
                        rootPattern.match(line);

                if (match.hasMatch()) {
                        rootSeen = true;

                        if (
                                match.captured(1).compare(
                                        expectedDeviceUuid,
                                        Qt::CaseInsensitive
                                ) != 0
                        ) {
                                error =
                                        QStringLiteral(
                                                "LibrePCB Device UUID mismatch in %1."
                                        )
                                                .arg(filePath);

                                return false;
                        }

                        continue;
                }

                match =
                        componentPattern.match(line);

                if (match.hasMatch()) {
                        componentUuid =
                                match.captured(1);

                        continue;
                }

                match =
                        packagePattern.match(line);

                if (match.hasMatch()) {
                        packageUuid =
                                match.captured(1);

                        continue;
                }

                match =
                        padPattern.match(line);

                if (match.hasMatch()) {
                        const QString padUuid =
                                match.captured(1);

                        const QString signalUuid =
                                match.captured(2);

                        if (padToSignal.contains(padUuid)) {
                                error =
                                        QStringLiteral(
                                                "LibrePCB Device contains duplicate pad UUID %1."
                                        )
                                                .arg(padUuid);

                                return false;
                        }

                        padToSignal.insert(
                                padUuid,
                                signalUuid
                        );
                }
        }

        if (
                !rootSeen ||
                componentUuid.isEmpty() ||
                packageUuid.isEmpty()
        ) {
                error =
                        QStringLiteral(
                                "LibrePCB Device is missing required identity data: %1"
                        )
                                .arg(filePath);

                return false;
        }

        return true;
}

bool readComponentSignals(
        const QString &filePath,
        const QString &expectedComponentUuid,
        QHash<QString, QString> &signalNames,
        QString &error
)
{
        signalNames.clear();
        error.clear();

        QStringList lines;

        if (!readLines(filePath, lines, error)) {
                return false;
        }

        static const QRegularExpression rootPattern(
                QStringLiteral(
                        "^\\s*\\(librepcb_component\\s+([0-9a-fA-F-]+)"
                )
        );

        static const QRegularExpression signalPattern(
                QStringLiteral(
                        "^\\s*\\(signal\\s+([0-9a-fA-F-]+)\\s+"
                        "\\(name\\s+\"((?:[^\"\\\\]|\\\\.)*)\"\\)"
                )
        );

        bool rootSeen = false;

        for (const QString &line : lines) {
                QRegularExpressionMatch match =
                        rootPattern.match(line);

                if (match.hasMatch()) {
                        rootSeen = true;

                        if (
                                match.captured(1).compare(
                                        expectedComponentUuid,
                                        Qt::CaseInsensitive
                                ) != 0
                        ) {
                                error =
                                        QStringLiteral(
                                                "LibrePCB Component UUID mismatch in %1."
                                        )
                                                .arg(filePath);

                                return false;
                        }

                        continue;
                }

                match =
                        signalPattern.match(line);

                if (!match.hasMatch()) {
                        continue;
                }

                const QString signalUuid =
                        match.captured(1);

                const QString signalName =
                        decodeLibrePcbString(
                                match.captured(2)
                        );

                if (signalNames.contains(signalUuid)) {
                        error =
                                QStringLiteral(
                                        "LibrePCB Component contains duplicate signal UUID %1."
                                )
                                        .arg(signalUuid);

                        return false;
                }

                signalNames.insert(
                        signalUuid,
                        signalName
                );
        }

        if (!rootSeen) {
                error =
                        QStringLiteral(
                                "LibrePCB Component identity was not found: %1"
                        )
                                .arg(filePath);

                return false;
        }

        return true;
}

QList<double> uniqueCoordinates(
        QList<double> values,
        double tolerance
)
{
        std::sort(
                values.begin(),
                values.end()
        );

        QList<double> unique;

        for (const double value : values) {
                if (
                        unique.isEmpty() ||
                        std::abs(
                                value - unique.last()
                        ) > tolerance
                ) {
                        unique.append(value);
                }
        }

        return unique;
}

bool equalStep(
        const QList<double> &values,
        double tolerance,
        double &step
)
{
        step = 0.0;

        if (values.size() < 2) {
                return false;
        }

        const double first =
                values.at(1) - values.at(0);

        if (first <= 0.0) {
                return false;
        }

        for (
                int index = 2;
                index < values.size();
                ++index
        ) {
                const double current =
                        values.at(index) -
                        values.at(index - 1);

                if (
                        std::abs(
                                current - first
                        ) > tolerance
                ) {
                        return false;
                }
        }

        step = first;

        return true;
}

int millimetersToMil(double millimeters)
{
        return qRound(
                millimeters *
                1000.0 /
                25.4
        );
}

bool deriveDipGeometry(
        const QHash<int, QPointF> &positions,
        int pinCount,
        int &pitchMil,
        int &spacingMil
)
{
        pitchMil = 0;
        spacingMil = 0;

        if (
                pinCount < 2 ||
                (pinCount % 2) != 0 ||
                positions.size() != pinCount
        ) {
                return false;
        }

        QList<double> xValues;
        QList<double> yValues;

        for (
                int pin = 1;
                pin <= pinCount;
                ++pin
        ) {
                if (!positions.contains(pin)) {
                        return false;
                }

                xValues.append(
                        positions.value(pin).x()
                );

                yValues.append(
                        positions.value(pin).y()
                );
        }

        constexpr double coordinateToleranceMm =
                0.01;

        const QList<double> uniqueX =
                uniqueCoordinates(
                        xValues,
                        coordinateToleranceMm
                );

        const QList<double> uniqueY =
                uniqueCoordinates(
                        yValues,
                        coordinateToleranceMm
                );

        double pitchMm = 0.0;
        double spacingMm = 0.0;

        if (
                uniqueX.size() == 2 &&
                uniqueY.size() == pinCount / 2
        ) {
                if (
                        !equalStep(
                                uniqueY,
                                coordinateToleranceMm,
                                pitchMm
                        )
                ) {
                        return false;
                }

                spacingMm =
                        std::abs(
                                uniqueX.at(1) -
                                uniqueX.at(0)
                        );
        }
        else if (
                uniqueY.size() == 2 &&
                uniqueX.size() == pinCount / 2
        ) {
                if (
                        !equalStep(
                                uniqueX,
                                coordinateToleranceMm,
                                pitchMm
                        )
                ) {
                        return false;
                }

                spacingMm =
                        std::abs(
                                uniqueY.at(1) -
                                uniqueY.at(0)
                        );
        }
        else {
                return false;
        }

        pitchMil =
                millimetersToMil(
                        pitchMm
                );

        spacingMil =
                millimetersToMil(
                        spacingMm
                );

        return true;
}

PackageGeometry readPackageGeometry(
        const QString &filePath,
        const QString &expectedPackageUuid
)
{
        PackageGeometry result;

        QStringList lines;

        if (
                !readLines(
                        filePath,
                        lines,
                        result.error
                )
        ) {
                return result;
        }

        static const QRegularExpression rootPattern(
                QStringLiteral(
                        "^\\s*\\(librepcb_package\\s+([0-9a-fA-F-]+)"
                )
        );

        static const QRegularExpression namePattern(
                QStringLiteral(
                        "^\\s*\\(name\\s+\"((?:[^\"\\\\]|\\\\.)*)\"\\)\\s*$"
                )
        );

        static const QRegularExpression packagePadPattern(
                QStringLiteral(
                        "^\\s*\\(pad\\s+([0-9a-fA-F-]+)\\s+"
                        "\\(name\\s+\"([1-9][0-9]*)\"\\)\\)\\s*$"
                )
        );

        static const QRegularExpression footprintPattern(
                QStringLiteral(
                        "^\\s*\\(footprint\\s+([0-9a-fA-F-]+)\\s*$"
                )
        );

        static const QRegularExpression footprintPadPattern(
                QStringLiteral(
                        "^\\s*\\(pad\\s+([0-9a-fA-F-]+)\\s+\\(side\\s+"
                )
        );

        static const QRegularExpression positionPattern(
                QStringLiteral(
                        "^\\s*\\(position\\s+"
                        "(-?[0-9]+(?:\\.[0-9]+)?)\\s+"
                        "(-?[0-9]+(?:\\.[0-9]+)?)\\)"
                )
        );

        bool rootSeen = false;
        bool packageNameSeen = false;

        int depth = 0;

        bool insideFootprint = false;
        QString currentPadUuid;

        QHash<int, QPointF> currentPositions;
        QList<QHash<int, QPointF>> completeFootprints;

        for (const QString &line : lines) {
                const int beforeDepth =
                        depth;

                QRegularExpressionMatch match =
                        rootPattern.match(line);

                if (match.hasMatch()) {
                        rootSeen = true;

                        if (
                                match.captured(1).compare(
                                        expectedPackageUuid,
                                        Qt::CaseInsensitive
                                ) != 0
                        ) {
                                result.error =
                                        QStringLiteral(
                                                "LibrePCB Package UUID mismatch in %1."
                                        )
                                                .arg(filePath);

                                return result;
                        }
                }

                if (
                        beforeDepth == 1 &&
                        !packageNameSeen
                ) {
                        match =
                                namePattern.match(line);

                        if (match.hasMatch()) {
                                result.packageName =
                                        decodeLibrePcbString(
                                                match.captured(1)
                                        );

                                packageNameSeen = true;
                        }
                }

                if (
                        beforeDepth == 1 &&
                        !insideFootprint
                ) {
                        match =
                                packagePadPattern.match(line);

                        if (match.hasMatch()) {
                                bool pinOk = false;

                                const int pin =
                                        match.captured(2).toInt(
                                                &pinOk
                                        );

                                if (
                                        !pinOk ||
                                        pin < 1
                                ) {
                                        result.error =
                                                QStringLiteral(
                                                        "LibrePCB Package has an invalid numeric pin name."
                                                );

                                        return result;
                                }

                                const QString padUuid =
                                        match.captured(1);

                                if (
                                        result.padUuidToPin.contains(
                                                padUuid
                                        )
                                ) {
                                        result.error =
                                                QStringLiteral(
                                                        "LibrePCB Package contains duplicate package pad UUID %1."
                                                )
                                                        .arg(padUuid);

                                        return result;
                                }

                                result.padUuidToPin.insert(
                                        padUuid,
                                        pin
                                );
                        }
                }

                if (
                        beforeDepth == 1 &&
                        footprintPattern.match(line).hasMatch()
                ) {
                        insideFootprint = true;
                        currentPadUuid.clear();
                        currentPositions.clear();
                }

                if (
                        insideFootprint &&
                        beforeDepth == 2
                ) {
                        match =
                                footprintPadPattern.match(line);

                        if (match.hasMatch()) {
                                currentPadUuid =
                                        match.captured(1);
                        }
                }

                if (
                        insideFootprint &&
                        !currentPadUuid.isEmpty() &&
                        beforeDepth == 3
                ) {
                        match =
                                positionPattern.match(line);

                        if (match.hasMatch()) {
                                bool xOk = false;
                                bool yOk = false;

                                const double x =
                                        match.captured(1).toDouble(
                                                &xOk
                                        );

                                const double y =
                                        match.captured(2).toDouble(
                                                &yOk
                                        );

                                if (
                                        xOk &&
                                        yOk &&
                                        result.padUuidToPin.contains(
                                                currentPadUuid
                                        )
                                ) {
                                        currentPositions.insert(
                                                result.padUuidToPin.value(
                                                        currentPadUuid
                                                ),
                                                QPointF(
                                                        x,
                                                        y
                                                )
                                        );
                                }
                        }
                }

                depth +=
                        parenthesisDelta(line);

                if (
                        insideFootprint &&
                        !currentPadUuid.isEmpty() &&
                        depth <= 2
                ) {
                        currentPadUuid.clear();
                }

                if (
                        insideFootprint &&
                        depth <= 1
                ) {
                        if (
                                !result.padUuidToPin.isEmpty() &&
                                currentPositions.size() ==
                                        result.padUuidToPin.size()
                        ) {
                                completeFootprints.append(
                                        currentPositions
                                );
                        }

                        insideFootprint = false;
                        currentPadUuid.clear();
                        currentPositions.clear();
                }
        }

        if (!rootSeen) {
                result.error =
                        QStringLiteral(
                                "LibrePCB Package identity was not found: %1"
                        )
                                .arg(filePath);

                return result;
        }

        result.pinCount =
                result.padUuidToPin.size();

        if (
                result.pinCount < 2 ||
                (result.pinCount % 2) != 0
        ) {
                result.error =
                        QStringLiteral(
                                "LibrePCB Package does not contain a supported even DIP pin count."
                        );

                return result;
        }

        for (
                int pin = 1;
                pin <= result.pinCount;
                ++pin
        ) {
                if (
                        !result.padUuidToPin.values().contains(
                                pin
                        )
                ) {
                        result.error =
                                QStringLiteral(
                                        "LibrePCB Package pin numbering is not a complete 1..%1 sequence."
                                )
                                        .arg(
                                                result.pinCount
                                        );

                        return result;
                }
        }

        if (completeFootprints.isEmpty()) {
                result.error =
                        QStringLiteral(
                                "LibrePCB Package has no complete footprint geometry."
                        );

                return result;
        }

        bool geometrySeen = false;

        for (
                const QHash<int, QPointF> &positions :
                completeFootprints
        ) {
                int candidatePitchMil = 0;
                int candidateSpacingMil = 0;

                if (
                        !deriveDipGeometry(
                                positions,
                                result.pinCount,
                                candidatePitchMil,
                                candidateSpacingMil
                        )
                ) {
                        continue;
                }

                if (!geometrySeen) {
                        result.pitchMil =
                                candidatePitchMil;

                        result.spacingMil =
                                candidateSpacingMil;

                        geometrySeen = true;

                        continue;
                }

                if (
                        result.pitchMil != candidatePitchMil ||
                        result.spacingMil != candidateSpacingMil
                ) {
                        result.error =
                                QStringLiteral(
                                        "LibrePCB Package footprints disagree about DIP geometry."
                                );

                        return result;
                }
        }

        if (!geometrySeen) {
                result.error =
                        QStringLiteral(
                                "LibrePCB Package geometry is not a two-row DIP."
                        );

                return result;
        }

        if (result.pitchMil != 100) {
                result.error =
                        QStringLiteral(
                                "LibrePCB Package uses unsupported DIP pitch %1 mil."
                        )
                                .arg(
                                        result.pitchMil
                                );

                return result;
        }

        if (
                result.spacingMil != 300 &&
                result.spacingMil != 600
        ) {
                result.error =
                        QStringLiteral(
                                "LibrePCB Package uses unsupported DIP row spacing %1 mil."
                        )
                                .arg(
                                        result.spacingMil
                                );

                return result;
        }

        result.ok = true;

        return result;
}

bool queryUniqueElement(
        QSqlDatabase &database,
        const QString &table,
        const QString &uuid,
        IndexedElement &element,
        QString &error
)
{
        element = IndexedElement();
        error.clear();

        if (
                table != QStringLiteral("components") &&
                table != QStringLiteral("packages")
        ) {
                error =
                        QStringLiteral(
                                "Internal LibrePCB provider error: unsupported cache table."
                        );

                return false;
        }

        QSqlQuery query(database);

        const QString sql =
                QStringLiteral(
                        "SELECT uuid, filepath "
                        "FROM %1 "
                        "WHERE uuid = ? AND deprecated = 0"
                )
                        .arg(table);

        if (!query.prepare(sql)) {
                error =
                        query.lastError().text();

                return false;
        }

        query.addBindValue(uuid);

        if (!query.exec()) {
                error =
                        query.lastError().text();

                return false;
        }

        QList<IndexedElement> matches;

        while (query.next()) {
                IndexedElement candidate;

                candidate.uuid =
                        query.value(0).toString();

                candidate.relativePath =
                        query.value(1).toString();

                matches.append(candidate);
        }

        if (matches.size() != 1) {
                error =
                        QStringLiteral(
                                "LibrePCB cache contains %1 active %2 entries for UUID %3; expected exactly one."
                        )
                                .arg(matches.size())
                                .arg(table)
                                .arg(uuid);

                return false;
        }

        element =
                matches.first();

        return true;
}

} // namespace

QList<BreadboardCsvLibrePcbCandidate>
BreadboardCsvLibrePcbProvider::findMpnCandidates(
        const QString &librariesRoot,
        const QString &queryText,
        QString &error,
        int maxResults
)
{
        QList<BreadboardCsvLibrePcbCandidate> result;

        error.clear();

        const QString queryTextTrimmed =
                queryText.trimmed();

        if (queryTextTrimmed.isEmpty()) {
                error =
                        QStringLiteral(
                                "LibrePCB candidate search text is empty."
                        );

                return result;
        }

        if (maxResults < 1) {
                error =
                        QStringLiteral(
                                "LibrePCB candidate search result limit must be positive."
                        );

                return result;
        }

        const QFileInfo rootInfo(
                librariesRoot
        );

        if (!rootInfo.isDir()) {
                error =
                        QStringLiteral(
                                "LibrePCB libraries directory does not exist: %1"
                        )
                                .arg(
                                        librariesRoot
                                );

                return result;
        }

        const QString canonicalRoot =
                rootInfo.canonicalFilePath();

        if (canonicalRoot.isEmpty()) {
                error =
                        QStringLiteral(
                                "LibrePCB libraries directory could not be canonicalized: %1"
                        )
                                .arg(
                                        librariesRoot
                                );

                return result;
        }

        QDir rootDir(
                canonicalRoot
        );

        const QStringList cacheFiles =
                rootDir.entryList(
                        QStringList()
                                << QStringLiteral(
                                           "cache_v*.sqlite"
                                   ),
                        QDir::Files,
                        QDir::Name
                );

        if (cacheFiles.isEmpty()) {
                error =
                        QStringLiteral(
                                "LibrePCB library cache was not found in %1."
                        )
                                .arg(
                                        rootDir.absolutePath()
                                );

                return result;
        }

        if (cacheFiles.size() != 1) {
                error =
                        QStringLiteral(
                                "LibrePCB libraries directory contains multiple cache "
                                "databases; candidate discovery will not guess which "
                                "schema to use:\n%1"
                        )
                                .arg(
                                        cacheFiles.join(
                                                QStringLiteral("\n")
                                        )
                                );

                return result;
        }

        const QString cachePath =
                rootDir.absoluteFilePath(
                        cacheFiles.first()
                );

        const QString connectionName =
                QStringLiteral(
                        "BreadboardCsvLibrePcbCandidateSearch_%1"
                )
                        .arg(
                                QUuid::createUuid()
                                        .toString(
                                                QUuid::WithoutBraces
                                        )
                        );

        {
                QSqlDatabase database =
                        QSqlDatabase::addDatabase(
                                QStringLiteral("QSQLITE"),
                                connectionName
                        );

                database.setDatabaseName(
                        cachePath
                );

                database.setConnectOptions(
                        QStringLiteral(
                                "QSQLITE_OPEN_READONLY"
                        )
                );

                if (!database.open()) {
                        error =
                                QStringLiteral(
                                        "Could not open LibrePCB cache read-only: %1"
                                )
                                        .arg(
                                                database.lastError().text()
                                        );
                }
                else {
                        QSqlQuery sqlQuery(
                                database
                        );

                        if (
                                !sqlQuery.prepare(
                                        QStringLiteral(
                                                "SELECT DISTINCT "
                                                "p.mpn, "
                                                "p.manufacturer, "
                                                "d.uuid "
                                                "FROM parts AS p "
                                                "JOIN devices AS d "
                                                "ON d.id = p.device_id "
                                                "WHERE d.deprecated = 0 "
                                                "AND instr(lower(p.mpn), lower(?)) > 0 "
                                                "ORDER BY "
                                                "length(p.mpn), "
                                                "lower(p.mpn), "
                                                "lower(p.manufacturer), "
                                                "d.uuid "
                                                "LIMIT ?"
                                        )
                                )
                        ) {
                                error =
                                        sqlQuery.lastError().text();
                        }
                        else {
                                sqlQuery.addBindValue(
                                        queryTextTrimmed
                                );

                                sqlQuery.addBindValue(
                                        maxResults
                                );

                                if (!sqlQuery.exec()) {
                                        error =
                                                sqlQuery.lastError().text();
                                }
                                else {
                                        while (sqlQuery.next()) {
                                                BreadboardCsvLibrePcbCandidate candidate;

                                                candidate.mpn =
                                                        sqlQuery.value(
                                                                0
                                                        ).toString();

                                                candidate.manufacturer =
                                                        sqlQuery.value(
                                                                1
                                                        ).toString();

                                                candidate.deviceUuid =
                                                        sqlQuery.value(
                                                                2
                                                        ).toString();

                                                if (
                                                        candidate.mpn.trimmed().isEmpty() ||
                                                        candidate.deviceUuid.trimmed().isEmpty()
                                                ) {
                                                        error =
                                                                QStringLiteral(
                                                                        "LibrePCB cache returned an invalid "
                                                                        "candidate record."
                                                                );

                                                        result.clear();
                                                        break;
                                                }

                                                result.append(
                                                        candidate
                                                );
                                        }
                                }
                        }
                }

                database.close();
        }

        QSqlDatabase::removeDatabase(
                connectionName
        );

        return result;
}


BreadboardCsvLibrePcbResolution
BreadboardCsvLibrePcbProvider::resolveExactMpn(
        const QString &librariesRoot,
        const QString &identifier,
        int expectedPinCount
)
{
        BreadboardCsvLibrePcbResolution result;

        result.identifier =
                identifier.trimmed();

        if (result.identifier.isEmpty()) {
                result.error =
                        QStringLiteral(
                                "LibrePCB component identifier is empty."
                        );

                return result;
        }

        const QFileInfo rootInfo(
                librariesRoot
        );

        if (!rootInfo.isDir()) {
                result.error =
                        QStringLiteral(
                                "LibrePCB libraries directory does not exist: %1"
                        )
                                .arg(
                                        librariesRoot
                                );

                return result;
        }

        QDir rootDir(
                rootInfo.canonicalFilePath()
        );

        const QStringList cacheFiles =
                rootDir.entryList(
                        QStringList()
                                << QStringLiteral(
                                           "cache_v*.sqlite"
                                   ),
                        QDir::Files,
                        QDir::Name
                );

        if (cacheFiles.isEmpty()) {
                result.error =
                        QStringLiteral(
                                "LibrePCB library cache was not found in %1."
                        )
                                .arg(
                                        rootDir.absolutePath()
                                );

                return result;
        }

        /*
         * Lexical order is deterministic. Multiple cache versions are
         * rejected instead of silently choosing one, because their
         * schemas are external to Fritzing.
         */
        if (cacheFiles.size() != 1) {
                result.error =
                        QStringLiteral(
                                "LibrePCB libraries directory contains multiple cache databases; "
                                "the importer will not guess which schema to use:\n%1"
                        )
                                .arg(
                                        cacheFiles.join(
                                                QStringLiteral("\n")
                                        )
                                );

                return result;
        }

        const QString cachePath =
                rootDir.absoluteFilePath(
                        cacheFiles.first()
                );

        const QString connectionName =
                QStringLiteral(
                        "BreadboardCsvLibrePcbProvider_%1"
                )
                        .arg(
                                QUuid::createUuid()
                                        .toString(
                                                QUuid::WithoutBraces
                                        )
                        );

        IndexedDevice indexedDevice;
        IndexedElement indexedComponent;
        IndexedElement indexedPackage;

        QString databaseError;

        {
                QSqlDatabase database =
                        QSqlDatabase::addDatabase(
                                QStringLiteral("QSQLITE"),
                                connectionName
                        );

                database.setDatabaseName(
                        cachePath
                );

                database.setConnectOptions(
                        QStringLiteral(
                                "QSQLITE_OPEN_READONLY"
                        )
                );

                if (!database.open()) {
                        databaseError =
                                QStringLiteral(
                                        "Could not open LibrePCB cache read-only: %1"
                                )
                                        .arg(
                                                database.lastError().text()
                                        );
                }
                else {
                        QSqlQuery query(database);

                        if (
                                !query.prepare(
                                        QStringLiteral(
                                                "SELECT "
                                                "p.mpn, "
                                                "p.manufacturer, "
                                                "d.uuid, "
                                                "d.filepath, "
                                                "d.component_uuid, "
                                                "d.package_uuid "
                                                "FROM parts AS p "
                                                "JOIN devices AS d "
                                                "ON d.id = p.device_id "
                                                "WHERE lower(p.mpn) = lower(?) "
                                                "AND d.deprecated = 0"
                                        )
                                )
                        ) {
                                databaseError =
                                        query.lastError().text();
                        }
                        else {
                                query.addBindValue(
                                        result.identifier
                                );

                                if (!query.exec()) {
                                        databaseError =
                                                query.lastError().text();
                                }
                                else {
                                        QList<IndexedDevice> matches;

                                        while (query.next()) {
                                                IndexedDevice candidate;

                                                candidate.mpn =
                                                        query.value(0).toString();

                                                candidate.manufacturer =
                                                        query.value(1).toString();

                                                candidate.uuid =
                                                        query.value(2).toString();

                                                candidate.relativePath =
                                                        query.value(3).toString();

                                                candidate.componentUuid =
                                                        query.value(4).toString();

                                                candidate.packageUuid =
                                                        query.value(5).toString();

                                                matches.append(
                                                        candidate
                                                );
                                        }

                                        if (matches.size() > 1) {
                                                databaseError =
                                                        QStringLiteral(
                                                                "%1 matched multiple active LibrePCB Devices. "
                                                                "The importer will not guess."
                                                        )
                                                                .arg(
                                                                        result.identifier
                                                                );
                                        }
                                        else if (matches.size() == 1) {
                                                indexedDevice =
                                                        matches.first();

                                                if (
                                                        !queryUniqueElement(
                                                                database,
                                                                QStringLiteral(
                                                                        "components"
                                                                ),
                                                                indexedDevice.componentUuid,
                                                                indexedComponent,
                                                                databaseError
                                                        )
                                                ) {
                                                        // databaseError already set.
                                                }
                                                else {
                                                        queryUniqueElement(
                                                                database,
                                                                QStringLiteral(
                                                                        "packages"
                                                                ),
                                                                indexedDevice.packageUuid,
                                                                indexedPackage,
                                                                databaseError
                                                        );
                                                }
                                        }
                                }
                        }
                }

                database.close();
        }

        QSqlDatabase::removeDatabase(
                connectionName
        );

        if (!databaseError.isEmpty()) {
                result.error =
                        databaseError;

                return result;
        }

        /*
         * No exact MPN is a normal provider miss. Do not manufacture
         * an error; the caller may continue to another provider.
         */
        if (indexedDevice.uuid.isEmpty()) {
                return result;
        }

        QString pathError;

        result.deviceFilePath =
                resolveIndexedFile(
                        rootDir.absolutePath(),
                        indexedDevice.relativePath,
                        QStringLiteral("device.lp"),
                        pathError
                );

        if (result.deviceFilePath.isEmpty()) {
                result.error =
                        pathError;

                return result;
        }

        result.componentFilePath =
                resolveIndexedFile(
                        rootDir.absolutePath(),
                        indexedComponent.relativePath,
                        QStringLiteral("component.lp"),
                        pathError
                );

        if (result.componentFilePath.isEmpty()) {
                result.error =
                        pathError;

                return result;
        }

        result.packageFilePath =
                resolveIndexedFile(
                        rootDir.absolutePath(),
                        indexedPackage.relativePath,
                        QStringLiteral("package.lp"),
                        pathError
                );

        if (result.packageFilePath.isEmpty()) {
                result.error =
                        pathError;

                return result;
        }

        QString fileComponentUuid;
        QString filePackageUuid;
        QHash<QString, QString> padToSignal;

        if (
                !readDeviceFile(
                        result.deviceFilePath,
                        indexedDevice.uuid,
                        fileComponentUuid,
                        filePackageUuid,
                        padToSignal,
                        result.error
                )
        ) {
                return result;
        }

        if (
                fileComponentUuid.compare(
                        indexedDevice.componentUuid,
                        Qt::CaseInsensitive
                ) != 0 ||
                filePackageUuid.compare(
                        indexedDevice.packageUuid,
                        Qt::CaseInsensitive
                ) != 0
        ) {
                result.error =
                        QStringLiteral(
                                "LibrePCB Device file disagrees with the cache index."
                        );

                return result;
        }

        QHash<QString, QString> signalNames;

        if (
                !readComponentSignals(
                        result.componentFilePath,
                        indexedDevice.componentUuid,
                        signalNames,
                        result.error
                )
        ) {
                return result;
        }

        const PackageGeometry geometry =
                readPackageGeometry(
                        result.packageFilePath,
                        indexedDevice.packageUuid
                );

        if (!geometry.ok) {
                result.error =
                        geometry.error;

                return result;
        }

        if (
                expectedPinCount > 0 &&
                geometry.pinCount != expectedPinCount
        ) {
                result.error =
                        QStringLiteral(
                                "%1 resolves to a %2-pin LibrePCB package, "
                                "but CSV geometry says %3 pins."
                        )
                                .arg(
                                        result.identifier
                                )
                                .arg(
                                        geometry.pinCount
                                )
                                .arg(
                                        expectedPinCount
                                );

                return result;
        }

        /*
         * Every Device pad mapping must reference a pad declared by
         * the resolved Package. Do not silently ignore stale or
         * corrupted Device mappings.
         */
        for (
                auto iterator =
                        padToSignal.constBegin();
                iterator !=
                        padToSignal.constEnd();
                ++iterator
        ) {
                if (
                        !geometry.padUuidToPin.contains(
                                iterator.key()
                        )
                ) {
                        result.error =
                                QStringLiteral(
                                        "LibrePCB Device maps unknown Package "
                                        "pad UUID %1."
                                )
                                        .arg(
                                                iterator.key()
                                        );

                        return result;
                }
        }

        QHash<int, QString> pinSignalNames;

        for (
                auto iterator =
                        geometry.padUuidToPin.constBegin();
                iterator !=
                        geometry.padUuidToPin.constEnd();
                ++iterator
        ) {
                const QString &padUuid =
                        iterator.key();

                const int pin =
                        iterator.value();

                const QString signalUuid =
                        padToSignal.value(
                                padUuid
                        );

                if (signalUuid.isEmpty()) {
                        /*
                         * A package pin may legitimately be unused by a
                         * particular Device. Placement still has valid
                         * package geometry, so leave its signal empty.
                         */
                        continue;
                }

                if (!signalNames.contains(signalUuid)) {
                        result.error =
                                QStringLiteral(
                                        "LibrePCB Device maps package pin %1 to unknown "
                                        "Component signal UUID %2."
                                )
                                        .arg(pin)
                                        .arg(signalUuid);

                        return result;
                }

                pinSignalNames.insert(
                        pin,
                        signalNames.value(
                                signalUuid
                        )
                );
        }

        result.matched = true;

        result.mpn =
                indexedDevice.mpn;

        result.manufacturer =
                indexedDevice.manufacturer;

        result.deviceUuid =
                indexedDevice.uuid;

        result.componentUuid =
                indexedDevice.componentUuid;

        result.packageUuid =
                indexedDevice.packageUuid;

        result.packageName =
                geometry.packageName;

        result.pinCount =
                geometry.pinCount;

        result.pitchMil =
                geometry.pitchMil;

        result.spacingMil =
                geometry.spacingMil;

        result.pinSignalNames =
                pinSignalNames;

        return result;
}
