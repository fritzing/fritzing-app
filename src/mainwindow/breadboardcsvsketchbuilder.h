#ifndef BREADBOARDCSVSKETCHBUILDER_H
#define BREADBOARDCSVSKETCHBUILDER_H

#include "breadboardcsvdipfootprintresolver.h"

#include <QList>
#include <QString>

struct BreadboardCsvPlacementResult
{
	bool ok = false;
	QString error;
	QList<long> boardIds;
	int boardsAdded = 0;
	bool reusedExistingBoard = false;
};

struct BreadboardCsvDipPlacementResult
{
	bool ok = false;
	bool aligned = false;
	QString error;

	QList<long> itemIds;
	int partsPlaced = 0;

	double maxCornerError = 0.0;
	QString worstComponent;
};

class SketchWidget;

class BreadboardCsvSketchBuilder
{
public:
	static BreadboardCsvPlacementResult placeThreeBreadboards(
		SketchWidget *breadboardView
	);

	static BreadboardCsvDipPlacementResult placeDipFootprints(
		SketchWidget *breadboardView,
		const QList<long> &boardIds,
		const QList<BreadboardCsvDipFootprint> &footprints
	);
};

#endif
