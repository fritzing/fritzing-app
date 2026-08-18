#ifndef BREADBOARDCSVDIPFOOTPRINTRESOLVER_H
#define BREADBOARDCSVDIPFOOTPRINTRESOLVER_H

#include "breadboardwiringcsvparser.h"

#include <QChar>
#include <QHash>
#include <QList>
#include <QString>

struct BreadboardCsvDipFootprint
{
        bool ok = false;
        QString error;

        QString component;
        int pinCount = 0;

        /*
         * Component/package resolution.
         *
         * CSV geometry does not populate these fields.
         */
        bool nativePart = false;
        QString moduleId;
        QString resolutionSource;
        QHash<int, QString> pinConnectorIds;

        /*
         * Physical DIP package width.
         *
         * Zero means package metadata has not resolved it yet.
         */
        int spacingMil = 0;

        int board = 0;

        /*
         * Rows inferred from CSV electrical-hole observations.
         * These are evidence about orientation/location, not
         * authoritative physical package-leg rows.
         */
        QChar referencePin1Row;
        QChar referenceOppositeRow;

        /*
         * Actual physical package-leg rows.
         * These are populated only after package resolution.
         */
        QChar pin1Row;
        QChar oppositeRow;

        int firstColumn = 0;
        int lastColumn = 0;

        int observedPins = 0;
        int matchedPins = 0;

        QString pin1Coordinate;
        QString pinHalfCoordinate;
        QString pinHalfPlus1Coordinate;
        QString pinLastCoordinate;

        QString pin1ConnectorId;
        QString pinHalfConnectorId;
        QString pinHalfPlus1ConnectorId;
        QString pinLastConnectorId;
};

struct BreadboardCsvDipFootprintResult
{
        bool ok = false;
        QString error;
        QList<BreadboardCsvDipFootprint> footprints;
};

class BreadboardCsvDipFootprintResolver
{
public:
        static BreadboardCsvDipFootprintResult resolveAll(
                const QList<BreadboardWiringCsvRow> &rows
        );
};

#endif
