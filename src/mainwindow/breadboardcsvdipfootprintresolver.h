#ifndef BREADBOARDCSVDIPFOOTPRINTRESOLVER_H
#define BREADBOARDCSVDIPFOOTPRINTRESOLVER_H

#include "breadboardwiringcsvparser.h"

#include <QChar>
#include <QList>
#include <QString>

struct BreadboardCsvDipFootprint
{
	bool ok = false;
	QString error;

	QString component;
	int pinCount = 0;
	int spacingMil = 0;

	int board = 0;
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
