#include "breadboardwiringcsvparser.h"

#include <QFile>
#include <QTextStream>

QStringList BreadboardWiringCsvParser::parseCsvLine(const QString &line, bool &ok)
{
	QStringList fields;
	QString field;
	bool inQuotes = false;

	for (qsizetype i = 0; i < line.size(); ++i) {
		const QChar ch = line.at(i);

		if (inQuotes) {
			if (ch == '"') {
				if ((i + 1) < line.size() && line.at(i + 1) == '"') {
					field += '"';
					++i;
				}
				else {
					inQuotes = false;
				}
			}
			else {
				field += ch;
			}
		}
		else {
			if (ch == ',') {
				fields.append(field);
				field.clear();
			}
			else if (ch == '"') {
				if (!field.isEmpty()) {
					ok = false;
					return {};
				}

				inQuotes = true;
			}
			else {
				field += ch;
			}
		}
	}

	if (inQuotes) {
		ok = false;
		return {};
	}

	fields.append(field);
	ok = true;
	return fields;
}

BreadboardWiringCsvResult BreadboardWiringCsvParser::parseFile(const QString &fileName)
{
	BreadboardWiringCsvResult result;

	QFile file(fileName);

	if (!file.exists()) {
		result.error = QString("File does not exist: %1").arg(fileName);
		return result;
	}

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		result.error = QString("Cannot open %1: %2")
			.arg(fileName, file.errorString());
		return result;
	}

	QTextStream stream(&file);

	if (stream.atEnd()) {
		result.error = QString("CSV file is empty.");
		return result;
	}

	bool csvOk = false;
	const QString headerLine = stream.readLine();
	QStringList headers = parseCsvLine(headerLine, csvOk);

	if (!csvOk) {
		result.error = QString("Invalid CSV syntax in header row.");
		return result;
	}

	const QStringList expectedHeaders = {
		"Wire ID",
		"Signal",
		"Color",
		"From",
		"From hole/terminal",
		"To",
		"To hole/terminal",
		"Note"
	};

	if (!headers.isEmpty() && headers.first().startsWith(QChar(0xFEFF))) {
		headers[0].remove(0, 1);
	}

	if (headers != expectedHeaders) {
		result.error =
			QString("Unexpected CSV header.\n\nExpected:\n%1\n\nFound:\n%2")
				.arg(expectedHeaders.join(","))
				.arg(headers.join(","));
		return result;
	}

	int physicalLineNumber = 1;

	while (!stream.atEnd()) {
		const QString line = stream.readLine();
		++physicalLineNumber;

		if (line.trimmed().isEmpty()) {
			continue;
		}

		QStringList fields = parseCsvLine(line, csvOk);

		if (!csvOk) {
			result.error =
				QString("Invalid CSV quoting on line %1.")
					.arg(physicalLineNumber);
			return result;
		}

		if (fields.size() != expectedHeaders.size()) {
			result.error =
				QString(
					"Line %1 has %2 fields; expected %3."
				)
					.arg(physicalLineNumber)
					.arg(fields.size())
					.arg(expectedHeaders.size());
			return result;
		}

		BreadboardWiringCsvRow row;
		row.index = result.rows.size();
		row.wireId = fields.at(0);
		row.signal = fields.at(1);
		row.color = fields.at(2);
		row.from = fields.at(3);
		row.fromTerminal = fields.at(4);
		row.to = fields.at(5);
		row.toTerminal = fields.at(6);
		row.note = fields.at(7);

		result.rows.append(row);
	}

	if (result.rows.isEmpty()) {
		result.error = QString("CSV contains a header but no records.");
		return result;
	}

	result.ok = true;
	return result;
}
