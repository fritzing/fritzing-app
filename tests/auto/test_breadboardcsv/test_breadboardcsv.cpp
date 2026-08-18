#define BOOST_TEST_MODULE BREADBOARD_CSV Tests
#include <boost/test/included/unit_test.hpp>

#include "mainwindow/breadboardcsvdipfootprintresolver.h"
#include "mainwindow/breadboardcsvdipphysicalresolver.h"
#include "mainwindow/breadboardcsvlibrepcbprovider.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QTemporaryDir>

#include <string>

namespace {

class QtApplicationFixture
{
public:
        QtApplicationFixture() :
                application(
                        argumentCount,
                        arguments
                )
        {
        }

private:
        int argumentCount = 1;
        char applicationName[26] = "test_breadboardcsv";
        char *arguments[2] = {
                applicationName,
                nullptr
        };
        QCoreApplication application;
};

BOOST_GLOBAL_FIXTURE(QtApplicationFixture);

QString librariesRoot()
{
        return QString::fromUtf8(
                BREADBOARDCSV_TEST_LIBRARIES_ROOT
        );
}

std::string printable(const QString &text)
{
        return text.toStdString();
}

QList<BreadboardWiringCsvRow> completeDipRows(
        const QString &component,
        int pinCount,
        int board,
        QChar pin1Row,
        QChar oppositeRow,
        int firstColumn
)
{
        QList<BreadboardWiringCsvRow> rows;

        const int halfPins =
                pinCount / 2;

        for (int pin = 1; pin <= pinCount; ++pin) {
                QChar row;
                int column = 0;

                if (pin <= halfPins) {
                        row = pin1Row;
                        column = firstColumn + pin - 1;
                }
                else {
                        row = oppositeRow;
                        column = firstColumn + pinCount - pin;
                }

                BreadboardWiringCsvRow csvRow;

                csvRow.index = pin;
                csvRow.from =
                        QString("%1 pin %2")
                                .arg(component)
                                .arg(pin);
                csvRow.fromTerminal =
                        QString("B%1-%2%3")
                                .arg(board)
                                .arg(row)
                                .arg(column);

                /*
                 * The other endpoint is deliberately not a component
                 * pin and is on a rail, so it contributes no DIP evidence.
                 */
                csvRow.to =
                        QString("NET_%1")
                                .arg(pin);
                csvRow.toTerminal =
                        QString("B%1-TOP+-3")
                                .arg(board);

                rows.append(csvRow);
        }

        return rows;
}

bool copyDirectoryTree(
        const QString &sourceRoot,
        const QString &destinationRoot,
        QString &error
)
{
        error.clear();

        const QDir sourceDirectory(sourceRoot);

        if (!sourceDirectory.exists()) {
                error =
                        QString("Source directory does not exist: %1")
                                .arg(sourceRoot);

                return false;
        }

        if (!QDir().mkpath(destinationRoot)) {
                error =
                        QString("Could not create directory: %1")
                                .arg(destinationRoot);

                return false;
        }

        QDirIterator iterator(
                sourceRoot,
                QDir::AllEntries | QDir::NoDotAndDotDot,
                QDirIterator::Subdirectories
        );

        while (iterator.hasNext()) {
                const QString sourcePath =
                        iterator.next();

                const QFileInfo sourceInfo =
                        iterator.fileInfo();

                const QString relativePath =
                        sourceDirectory.relativeFilePath(
                                sourcePath
                        );

                const QString destinationPath =
                        QDir(destinationRoot).filePath(
                                relativePath
                        );

                if (sourceInfo.isDir()) {
                        if (!QDir().mkpath(destinationPath)) {
                                error =
                                        QString("Could not create directory: %1")
                                                .arg(destinationPath);

                                return false;
                        }

                        continue;
                }

                if (!QDir().mkpath(QFileInfo(destinationPath).path())) {
                        error =
                                QString("Could not create parent directory for: %1")
                                        .arg(destinationPath);

                        return false;
                }

                if (!QFile::copy(sourcePath, destinationPath)) {
                        error =
                                QString("Could not copy %1 to %2")
                                        .arg(sourcePath, destinationPath);

                        return false;
                }
        }

        return true;
}

} // namespace

BOOST_AUTO_TEST_CASE(geometry_infers_opaque_component_name)
{
        const QString component =
                QStringLiteral("My Custom CPU");

        const BreadboardCsvDipFootprintResult result =
                BreadboardCsvDipFootprintResolver::resolveAll(
                        completeDipRows(
                                component,
                                14,
                                2,
                                QChar('D'),
                                QChar('G'),
                                10
                        )
                );

        BOOST_REQUIRE_MESSAGE(
                result.ok,
                printable(result.error)
        );
        BOOST_REQUIRE_EQUAL(result.footprints.size(), 1);

        const BreadboardCsvDipFootprint &footprint =
                result.footprints.first();

        BOOST_CHECK_EQUAL(printable(footprint.component), "My Custom CPU");
        BOOST_CHECK_EQUAL(footprint.pinCount, 14);
        BOOST_CHECK_EQUAL(footprint.board, 2);
        BOOST_CHECK_EQUAL(footprint.firstColumn, 10);
        BOOST_CHECK_EQUAL(footprint.lastColumn, 16);
        BOOST_CHECK_EQUAL(footprint.observedPins, 14);
        BOOST_CHECK_EQUAL(footprint.matchedPins, 14);
        BOOST_CHECK(footprint.referencePin1Row == QChar('D'));
        BOOST_CHECK(footprint.referenceOppositeRow == QChar('G'));
        BOOST_CHECK_EQUAL(footprint.spacingMil, 0);
}

BOOST_AUTO_TEST_CASE(geometry_rejects_sparse_non_dip_evidence)
{
        QList<BreadboardWiringCsvRow> rows =
                completeDipRows(
                        QStringLiteral("Sparse Device"),
                        14,
                        1,
                        QChar('D'),
                        QChar('G'),
                        10
                );

        rows = rows.mid(0, 3);

        const BreadboardCsvDipFootprintResult result =
                BreadboardCsvDipFootprintResolver::resolveAll(rows);

        BOOST_CHECK(!result.ok);
        BOOST_CHECK(result.footprints.isEmpty());
        BOOST_CHECK(
                result.error.contains(
                        QStringLiteral("No DIP footprints")
                )
        );
}

BOOST_AUTO_TEST_CASE(physical_resolution_applies_300_mil_rows)
{
        BreadboardCsvDipFootprint footprint;

        footprint.component = QStringLiteral("Logic IC");
        footprint.pinCount = 14;
        footprint.board = 2;
        footprint.referencePin1Row = QChar('D');
        footprint.referenceOppositeRow = QChar('G');
        footprint.firstColumn = 10;
        footprint.lastColumn = 16;

        QString error;

        BOOST_REQUIRE_MESSAGE(
                BreadboardCsvDipPhysicalResolver::apply(
                        footprint,
                        300,
                        error
                ),
                printable(error)
        );

        BOOST_CHECK_EQUAL(footprint.spacingMil, 300);
        BOOST_CHECK(footprint.pin1Row == QChar('E'));
        BOOST_CHECK(footprint.oppositeRow == QChar('F'));
        BOOST_CHECK_EQUAL(printable(footprint.pin1Coordinate), "B2-E10");
        BOOST_CHECK_EQUAL(printable(footprint.pinHalfCoordinate), "B2-E16");
        BOOST_CHECK_EQUAL(printable(footprint.pinHalfPlus1Coordinate), "B2-F16");
        BOOST_CHECK_EQUAL(printable(footprint.pinLastCoordinate), "B2-F10");
        BOOST_CHECK_EQUAL(printable(footprint.pin1ConnectorId), "pin10E");
        BOOST_CHECK_EQUAL(printable(footprint.pinLastConnectorId), "pin10F");
}

BOOST_AUTO_TEST_CASE(physical_resolution_applies_unique_600_mil_rows)
{
        BreadboardCsvDipFootprint footprint;

        footprint.component = QStringLiteral("Wide DIP");
        footprint.pinCount = 40;
        footprint.board = 1;
        footprint.referencePin1Row = QChar('C');
        footprint.referenceOppositeRow = QChar('G');
        footprint.firstColumn = 1;
        footprint.lastColumn = 20;

        QString error;

        BOOST_REQUIRE_MESSAGE(
                BreadboardCsvDipPhysicalResolver::apply(
                        footprint,
                        600,
                        error
                ),
                printable(error)
        );

        BOOST_CHECK_EQUAL(footprint.spacingMil, 600);
        BOOST_CHECK(footprint.pin1Row == QChar('C'));
        BOOST_CHECK(footprint.oppositeRow == QChar('G'));
        BOOST_CHECK_EQUAL(printable(footprint.pin1Coordinate), "B1-C1");
        BOOST_CHECK_EQUAL(printable(footprint.pinHalfCoordinate), "B1-C20");
        BOOST_CHECK_EQUAL(printable(footprint.pinHalfPlus1Coordinate), "B1-G20");
        BOOST_CHECK_EQUAL(printable(footprint.pinLastCoordinate), "B1-G1");
}

BOOST_AUTO_TEST_CASE(physical_resolution_rejects_ambiguous_600_mil_rows)
{
        BreadboardCsvDipFootprint footprint;

        footprint.component = QStringLiteral("Ambiguous Wide DIP");
        footprint.pinCount = 40;
        footprint.board = 1;
        footprint.referencePin1Row = QChar('D');
        footprint.referenceOppositeRow = QChar('G');
        footprint.firstColumn = 1;
        footprint.lastColumn = 20;

        QString error;

        BOOST_CHECK(
                !BreadboardCsvDipPhysicalResolver::apply(
                        footprint,
                        600,
                        error
                )
        );
        BOOST_CHECK(
                error.contains(
                        QStringLiteral("do not uniquely determine")
                )
        );
}

BOOST_AUTO_TEST_CASE(physical_resolution_rejects_unsupported_width)
{
        BreadboardCsvDipFootprint footprint;

        footprint.component = QStringLiteral("Unsupported DIP");

        QString error;

        BOOST_CHECK(
                !BreadboardCsvDipPhysicalResolver::apply(
                        footprint,
                        400,
                        error
                )
        );
        BOOST_CHECK(
                error.contains(
                        QStringLiteral("unsupported DIP spacing 400 mil")
                )
        );
}

BOOST_AUTO_TEST_CASE(librepcb_candidate_discovery_is_literal_and_case_insensitive)
{
        QString error;

        const QList<BreadboardCsvLibrePcbCandidate> candidates =
                BreadboardCsvLibrePcbProvider::findMpnCandidates(
                        librariesRoot(),
                        QStringLiteral("w65c02s6tpg"),
                        error,
                        10
                );

        BOOST_REQUIRE_MESSAGE(error.isEmpty(), printable(error));
        BOOST_REQUIRE_EQUAL(candidates.size(), 1);
        BOOST_CHECK_EQUAL(printable(candidates.first().mpn), "W65C02S6TPG-14");
        BOOST_CHECK_EQUAL(
                printable(candidates.first().manufacturer),
                "The Western Design Center, Inc."
        );
        BOOST_CHECK_EQUAL(
                printable(candidates.first().deviceUuid),
                "413e7c69-1e79-44fe-90ca-28692049e422"
        );

        error.clear();

        const QList<BreadboardCsvLibrePcbCandidate> wildcardCandidates =
                BreadboardCsvLibrePcbProvider::findMpnCandidates(
                        librariesRoot(),
                        QStringLiteral("%"),
                        error,
                        10
                );

        BOOST_CHECK(error.isEmpty());
        BOOST_CHECK(wildcardCandidates.isEmpty());
}

BOOST_AUTO_TEST_CASE(librepcb_exact_resolution_validates_geometry_and_signals)
{
        const BreadboardCsvLibrePcbResolution result =
                BreadboardCsvLibrePcbProvider::resolveExactMpn(
                        librariesRoot(),
                        QStringLiteral("w65c02s6tpg-14"),
                        40
                );

        BOOST_REQUIRE_MESSAGE(result.error.isEmpty(), printable(result.error));
        BOOST_REQUIRE(result.matched);
        BOOST_CHECK_EQUAL(printable(result.mpn), "W65C02S6TPG-14");
        BOOST_CHECK_EQUAL(
                printable(result.manufacturer),
                "The Western Design Center, Inc."
        );
        BOOST_CHECK_EQUAL(result.pinCount, 40);
        BOOST_CHECK_EQUAL(result.pitchMil, 100);
        BOOST_CHECK_EQUAL(result.spacingMil, 600);
        BOOST_CHECK_EQUAL(
                printable(result.packageName),
                "DIP1524W55P254L5232H533Q40"
        );
        BOOST_REQUIRE_EQUAL(result.pinSignalNames.size(), 39);
        BOOST_CHECK_EQUAL(printable(result.pinSignalNames.value(1)), "VPB");
        BOOST_CHECK_EQUAL(printable(result.pinSignalNames.value(34)), "RWB");
        BOOST_CHECK(!result.pinSignalNames.contains(35));
        BOOST_CHECK_EQUAL(printable(result.pinSignalNames.value(36)), "BE");
        BOOST_CHECK_EQUAL(printable(result.pinSignalNames.value(40)), "RESB");
}

BOOST_AUTO_TEST_CASE(librepcb_exact_resolution_rejects_pin_count_mismatch)
{
        const BreadboardCsvLibrePcbResolution result =
                BreadboardCsvLibrePcbProvider::resolveExactMpn(
                        librariesRoot(),
                        QStringLiteral("W65C02S6TPG-14"),
                        28
                );

        BOOST_CHECK(!result.matched);
        BOOST_CHECK(
                result.error.contains(
                        QStringLiteral("CSV geometry says 28 pins")
                )
        );
}

BOOST_AUTO_TEST_CASE(librepcb_unknown_mpn_is_normal_provider_miss)
{
        const BreadboardCsvLibrePcbResolution result =
                BreadboardCsvLibrePcbProvider::resolveExactMpn(
                        librariesRoot(),
                        QStringLiteral("THIS-MPN-DOES-NOT-EXIST"),
                        40
                );

        BOOST_CHECK(!result.matched);
        BOOST_CHECK(result.error.isEmpty());
}

BOOST_AUTO_TEST_CASE(librepcb_rejects_device_mapping_unknown_package_pad)
{
        QTemporaryDir temporaryDirectory;

        BOOST_REQUIRE(temporaryDirectory.isValid());

        QString copyError;

        BOOST_REQUIRE_MESSAGE(
                copyDirectoryTree(
                        librariesRoot(),
                        temporaryDirectory.path(),
                        copyError
                ),
                printable(copyError)
        );

        const QString devicePath =
                QDir(temporaryDirectory.path()).filePath(
                        QStringLiteral(
                                "remote/6d6ab4d4-2f58-4c99-b389-f8407d51c753.lplib/"
                                "dev/413e7c69-1e79-44fe-90ca-28692049e422/device.lp"
                        )
                );

        QFile deviceFile(devicePath);

        BOOST_REQUIRE(deviceFile.open(QIODevice::ReadOnly));

        QByteArray deviceData =
                deviceFile.readAll();

        deviceFile.close();

        const QByteArray insertionMarker =
                " (part \"W65C02S6TPG-14\"";

        const qsizetype insertionPosition =
                deviceData.indexOf(insertionMarker);

        BOOST_REQUIRE(insertionPosition >= 0);

        deviceData.insert(
                insertionPosition,
                " (pad 11111111-2222-4333-8444-555555555555 "
                "(signal 334e5fd5-f063-55da-bb65-8fd1300ef4dd))\n"
        );

        BOOST_REQUIRE(
                deviceFile.open(
                        QIODevice::WriteOnly |
                        QIODevice::Truncate
                )
        );
        BOOST_REQUIRE_EQUAL(
                deviceFile.write(deviceData),
                deviceData.size()
        );
        deviceFile.close();

        const BreadboardCsvLibrePcbResolution result =
                BreadboardCsvLibrePcbProvider::resolveExactMpn(
                        temporaryDirectory.path(),
                        QStringLiteral("W65C02S6TPG-14"),
                        40
                );

        BOOST_CHECK(!result.matched);
        BOOST_CHECK(
                result.error.contains(
                        QStringLiteral("unknown Package pad UUID")
                )
        );
}
