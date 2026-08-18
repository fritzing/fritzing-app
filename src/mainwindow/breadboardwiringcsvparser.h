#ifndef BREADBOARDWIRINGCSVPARSER_H
#define BREADBOARDWIRINGCSVPARSER_H

#include <QList>
#include <QString>
#include <QStringList>

enum class BreadboardWiringCsvField
{
	WireId = 0,
	Signal,
	Color,
	From,
	FromTerminal,
	To,
	ToTerminal,
	Note
};

constexpr int BreadboardWiringCsvFieldCount = 8;

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

struct BreadboardWiringCsvSource
{
	bool ok = false;
	QString error;

	QChar delimiter;
	QString encoding;

	QList<QStringList> records;
	QList<int> physicalLineNumbers;
	int columnCount = 0;
	bool firstRowLikelyHeader = true;
};

struct BreadboardWiringCsvColumnMapping
{
	bool firstRowIsHeader = true;

	int wireIdColumn = -1;
	int signalColumn = -1;
	int colorColumn = -1;
	int fromColumn = -1;
	int fromTerminalColumn = -1;
	int toColumn = -1;
	int toTerminalColumn = -1;
	int noteColumn = -1;

	int column(BreadboardWiringCsvField field) const;
	void setColumn(BreadboardWiringCsvField field, int sourceColumn);
};

class BreadboardWiringCsvParser
{
public:
	/*
	 * Legacy strict parser retained for callers which explicitly require
	 * the original eight-column export contract.
	 */
	static BreadboardWiringCsvResult parseFile(const QString &fileName);

	/*
	 * Read CSV syntax without assigning application meaning to columns.
	 * A null delimiter requests automatic delimiter detection.
	 */
	static BreadboardWiringCsvSource parseSource(
		const QString &fileName,
		QChar delimiter = QChar()
	);

	static BreadboardWiringCsvColumnMapping suggestMapping(
		const BreadboardWiringCsvSource &source,
		bool firstRowIsHeader = true
	);

	static BreadboardWiringCsvResult applyMapping(
		const BreadboardWiringCsvSource &source,
		const BreadboardWiringCsvColumnMapping &mapping
	);

	static QStringList displayHeaders(
		const BreadboardWiringCsvSource &source,
		bool firstRowIsHeader
	);

	static QString normalizedHeader(const QString &header);
	static QString delimiterName(QChar delimiter);
	static QString fieldKey(BreadboardWiringCsvField field);

private:
	static BreadboardWiringCsvSource parseText(
		const QString &text,
		QChar delimiter,
		const QString &encoding
	);
};

#endif
