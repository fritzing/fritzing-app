#ifndef BREADBOARDWIRINGCSVPARSER_H
#define BREADBOARDWIRINGCSVPARSER_H

#include <QList>
#include <QString>
#include <QStringList>

struct BreadboardWiringCsvRow
{
	int index = -1;
	QString wireId;
	QString signal;
	QString color;
	QString from;
	QString fromTerminal;
	QString to;
	QString toTerminal;
	QString note;
};

struct BreadboardWiringCsvResult
{
	bool ok = false;
	QString error;
	QList<BreadboardWiringCsvRow> rows;
};

class BreadboardWiringCsvParser
{
public:
	static BreadboardWiringCsvResult parseFile(const QString &fileName);

private:
	static QStringList parseCsvLine(const QString &line, bool &ok);
};

#endif
