#ifndef BREADBOARDCSVSKETCHBUILDER_H
#define BREADBOARDCSVSKETCHBUILDER_H

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

class SketchWidget;

class BreadboardCsvSketchBuilder
{
public:
	static BreadboardCsvPlacementResult placeThreeBreadboards(
		SketchWidget *breadboardView
	);
};

#endif
