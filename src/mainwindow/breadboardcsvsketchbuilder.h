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

struct BreadboardCsvCpuProbeResult
{
	bool ok = false;
	bool aligned = false;
	QString error;
	long cpuId = -1;

	double pin1Error = 0.0;
	double pin20Error = 0.0;
	double pin21Error = 0.0;
	double pin40Error = 0.0;
};

class SketchWidget;

class BreadboardCsvSketchBuilder
{
public:
	static BreadboardCsvPlacementResult placeThreeBreadboards(
		SketchWidget *breadboardView
	);

	static BreadboardCsvCpuProbeResult placeCpuAlignmentProbe(
		SketchWidget *breadboardView,
		long boardId,
		const QString &pin1ConnectorId,
		const QString &pin20ConnectorId,
		const QString &pin21ConnectorId,
		const QString &pin40ConnectorId
	);
};

#endif
