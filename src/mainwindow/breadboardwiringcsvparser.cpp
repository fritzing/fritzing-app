#include "breadboardwiringcsvparser.h"

#include <QByteArray>
#include <QFile>
#include <QSet>
#include <QStringConverter>
#include <QStringDecoder>
#include <QVector>

#include <algorithm>

namespace {

constexpr qint64 maximumCsvFileBytes =
	32LL * 1024LL * 1024LL;

const QStringList canonicalHeaders = {
	QStringLiteral("Wire ID"),
	QStringLiteral("Signal"),
	QStringLiteral("Color"),
	QStringLiteral("From"),
	QStringLiteral("From hole/terminal"),
	QStringLiteral("To"),
	QStringLiteral("To hole/terminal"),
	QStringLiteral("Note")
};

bool recordIsEmpty(const QStringList &record)
{
	for (const QString &field : record) {
		if (!field.trimmed().isEmpty()) {
			return false;
		}
	}

	return true;
}

QString decodeBytes(
	const QByteArray &bytes,
	QString &encoding,
	QString &error
)
{
	encoding.clear();
	error.clear();

	if (bytes.startsWith("\xEF\xBB\xBF")) {
		encoding = QStringLiteral("UTF-8 with BOM");

		QStringDecoder decoder(QStringConverter::Utf8);
		const QString text = decoder.decode(bytes.mid(3));

		if (decoder.hasError()) {
			error = QStringLiteral("CSV contains invalid UTF-8 data.");
			return QString();
		}

		return text;
	}

	const bool utf16LittleEndian =
		bytes.size() >= 2 &&
		static_cast<unsigned char>(bytes.at(0)) == 0xFF &&
		static_cast<unsigned char>(bytes.at(1)) == 0xFE;

	const bool utf16BigEndian =
		bytes.size() >= 2 &&
		static_cast<unsigned char>(bytes.at(0)) == 0xFE &&
		static_cast<unsigned char>(bytes.at(1)) == 0xFF;

	if (utf16LittleEndian || utf16BigEndian) {
		if (((bytes.size() - 2) % 2) != 0) {
			error = QStringLiteral("CSV contains truncated UTF-16 data.");
			return QString();
		}

		QVector<char16_t> units;
		units.reserve((bytes.size() - 2) / 2);

		for (qsizetype index = 2; index < bytes.size(); index += 2) {
			const unsigned char first =
				static_cast<unsigned char>(bytes.at(index));

			const unsigned char second =
				static_cast<unsigned char>(bytes.at(index + 1));

			const char16_t unit = utf16LittleEndian
				? static_cast<char16_t>(
					first |
					(static_cast<unsigned int>(second) << 8)
				)
				: static_cast<char16_t>(
					(static_cast<unsigned int>(first) << 8) |
					second
				);

			units.append(unit);
		}

		encoding = utf16LittleEndian
			? QStringLiteral("UTF-16 LE")
			: QStringLiteral("UTF-16 BE");

		return QString::fromUtf16(
			units.constData(),
			units.size()
		);
	}

	QStringDecoder decoder(QStringConverter::Utf8);
	const QString text = decoder.decode(bytes);

	if (decoder.hasError()) {
		error =
			QStringLiteral(
				"CSV is not valid UTF-8 and has no supported UTF-16 byte-order mark."
			);

		return QString();
	}

	encoding = QStringLiteral("UTF-8");
	return text;
}

QString valueAt(const QStringList &record, int column)
{
	if (column < 0 || column >= record.size()) {
		return QString();
	}

	return record.at(column).trimmed();
}

bool requiredField(BreadboardWiringCsvField field)
{
	return
		field == BreadboardWiringCsvField::From ||
		field == BreadboardWiringCsvField::FromTerminal ||
		field == BreadboardWiringCsvField::To ||
		field == BreadboardWiringCsvField::ToTerminal;
}

QString fieldLabel(BreadboardWiringCsvField field)
{
	switch (field) {
	case BreadboardWiringCsvField::WireId:
		return QStringLiteral("Wire ID");

	case BreadboardWiringCsvField::Signal:
		return QStringLiteral("Signal");

	case BreadboardWiringCsvField::Color:
		return QStringLiteral("Color");

	case BreadboardWiringCsvField::From:
		return QStringLiteral("From");

	case BreadboardWiringCsvField::FromTerminal:
		return QStringLiteral("From hole/terminal");

	case BreadboardWiringCsvField::To:
		return QStringLiteral("To");

	case BreadboardWiringCsvField::ToTerminal:
		return QStringLiteral("To hole/terminal");

	case BreadboardWiringCsvField::Note:
		return QStringLiteral("Note");
	}

	return QString();
}

QStringList aliases(BreadboardWiringCsvField field)
{
	switch (field) {
	case BreadboardWiringCsvField::WireId:
		return {
			QStringLiteral("wireid"),
			QStringLiteral("wire"),
			QStringLiteral("connectionid"),
			QStringLiteral("jumperid"),
			QStringLiteral("id")
		};

	case BreadboardWiringCsvField::Signal:
		return {
			QStringLiteral("signal"),
			QStringLiteral("signalname"),
			QStringLiteral("net"),
			QStringLiteral("netname")
		};

	case BreadboardWiringCsvField::Color:
		return {
			QStringLiteral("color"),
			QStringLiteral("colour"),
			QStringLiteral("wirecolor"),
			QStringLiteral("wirecolour")
		};

	case BreadboardWiringCsvField::From:
		return {
			QStringLiteral("from"),
			QStringLiteral("source"),
			QStringLiteral("fromcomponent"),
			QStringLiteral("sourcecomponent"),
			QStringLiteral("component1"),
			QStringLiteral("endpoint1"),
			QStringLiteral("start")
		};

	case BreadboardWiringCsvField::FromTerminal:
		return {
			QStringLiteral("fromholeterminal"),
			QStringLiteral("fromterminal"),
			QStringLiteral("fromhole"),
			QStringLiteral("frompin"),
			QStringLiteral("sourcehole"),
			QStringLiteral("sourcepin"),
			QStringLiteral("terminal1"),
			QStringLiteral("hole1"),
			QStringLiteral("pin1")
		};

	case BreadboardWiringCsvField::To:
		return {
			QStringLiteral("to"),
			QStringLiteral("destination"),
			QStringLiteral("tocomponent"),
			QStringLiteral("destinationcomponent"),
			QStringLiteral("component2"),
			QStringLiteral("endpoint2"),
			QStringLiteral("end")
		};

	case BreadboardWiringCsvField::ToTerminal:
		return {
			QStringLiteral("toholeterminal"),
			QStringLiteral("toterminal"),
			QStringLiteral("tohole"),
			QStringLiteral("topin"),
			QStringLiteral("destinationhole"),
			QStringLiteral("destinationpin"),
			QStringLiteral("terminal2"),
			QStringLiteral("hole2"),
			QStringLiteral("pin2")
		};

	case BreadboardWiringCsvField::Note:
		return {
			QStringLiteral("note"),
			QStringLiteral("notes"),
			QStringLiteral("comment"),
			QStringLiteral("comments"),
			QStringLiteral("description")
		};
	}

	return {};
}

int recognizedHeaderCount(const QStringList &record)
{
	int recognized = 0;
	QSet<int> recognizedFields;

	for (const QString &header : record) {
		const QString normalized =
			BreadboardWiringCsvParser::normalizedHeader(
				header
			);

		for (int fieldIndex = 0; fieldIndex < BreadboardWiringCsvFieldCount; ++fieldIndex) {
			const BreadboardWiringCsvField field =
				static_cast<BreadboardWiringCsvField>(fieldIndex);

			if (
				!recognizedFields.contains(fieldIndex) &&
				aliases(field).contains(normalized)
			) {
				recognizedFields.insert(fieldIndex);
				++recognized;
				break;
			}
		}
	}

	return recognized;
}

bool likelyHeaderRow(const QStringList &record)
{
	if (recognizedHeaderCount(record) >= 2) {
		return true;
	}

	int breadboardCoordinates = 0;

	for (const QString &field : record) {
		const QString value =
			field.trimmed().toCaseFolded();

		if (
			value.startsWith(QStringLiteral("b1-")) ||
			value.startsWith(QStringLiteral("b2-")) ||
			value.startsWith(QStringLiteral("b3-"))
		) {
			++breadboardCoordinates;
		}
	}

	return breadboardCoordinates < 2;
}

} // namespace

int BreadboardWiringCsvColumnMapping::column(
	BreadboardWiringCsvField field
) const
{
	switch (field) {
	case BreadboardWiringCsvField::WireId:
		return wireIdColumn;

	case BreadboardWiringCsvField::Signal:
		return signalColumn;

	case BreadboardWiringCsvField::Color:
		return colorColumn;

	case BreadboardWiringCsvField::From:
		return fromColumn;

	case BreadboardWiringCsvField::FromTerminal:
		return fromTerminalColumn;

	case BreadboardWiringCsvField::To:
		return toColumn;

	case BreadboardWiringCsvField::ToTerminal:
		return toTerminalColumn;

	case BreadboardWiringCsvField::Note:
		return noteColumn;
	}

	return -1;
}

void BreadboardWiringCsvColumnMapping::setColumn(
	BreadboardWiringCsvField field,
	int sourceColumn
)
{
	switch (field) {
	case BreadboardWiringCsvField::WireId:
		wireIdColumn = sourceColumn;
		break;

	case BreadboardWiringCsvField::Signal:
		signalColumn = sourceColumn;
		break;

	case BreadboardWiringCsvField::Color:
		colorColumn = sourceColumn;
		break;

	case BreadboardWiringCsvField::From:
		fromColumn = sourceColumn;
		break;

	case BreadboardWiringCsvField::FromTerminal:
		fromTerminalColumn = sourceColumn;
		break;

	case BreadboardWiringCsvField::To:
		toColumn = sourceColumn;
		break;

	case BreadboardWiringCsvField::ToTerminal:
		toTerminalColumn = sourceColumn;
		break;

	case BreadboardWiringCsvField::Note:
		noteColumn = sourceColumn;
		break;
	}
}

BreadboardWiringCsvSource BreadboardWiringCsvParser::parseText(
	const QString &text,
	QChar delimiter,
	const QString &encoding
)
{
	BreadboardWiringCsvSource source;
	source.delimiter = delimiter;
	source.encoding = encoding;

	QStringList record;
	QString field;

	bool inQuotes = false;
	bool afterQuote = false;

	int physicalLine = 1;
	int recordLine = 1;

	const auto appendRecord = [&]() {
		record.append(field);
		field.clear();

		if (!recordIsEmpty(record)) {
			source.columnCount =
				std::max(
					source.columnCount,
					static_cast<int>(record.size())
				);

			source.records.append(record);
			source.physicalLineNumbers.append(recordLine);
		}

		record.clear();
		afterQuote = false;
	};

	for (qsizetype index = 0; index < text.size(); ++index) {
		const QChar character = text.at(index);

		if (inQuotes) {
			if (character == QChar('"')) {
				if (
					(index + 1) < text.size() &&
					text.at(index + 1) == QChar('"')
				) {
					field += QChar('"');
					++index;
				}
				else {
					inQuotes = false;
					afterQuote = true;
				}
			}
			else {
				field += character;

				if (character == QChar('\n')) {
					++physicalLine;
				}
			}

			continue;
		}

		if (afterQuote) {
			if (character == delimiter) {
				record.append(field);
				field.clear();
				afterQuote = false;
				continue;
			}

			if (
				character == QChar('\n') ||
				character == QChar('\r')
			) {
				if (
					character == QChar('\r') &&
					(index + 1) < text.size() &&
					text.at(index + 1) == QChar('\n')
				) {
					++index;
				}

				appendRecord();
				++physicalLine;
				recordLine = physicalLine;
				continue;
			}

			if (character.isSpace()) {
				continue;
			}

			source.error =
				QString(
					"Unexpected character after a quoted field on line %1."
				)
					.arg(physicalLine);

			return source;
		}

		if (character == delimiter) {
			record.append(field);
			field.clear();
			continue;
		}

		if (
			character == QChar('\n') ||
			character == QChar('\r')
		) {
			if (
				character == QChar('\r') &&
				(index + 1) < text.size() &&
				text.at(index + 1) == QChar('\n')
			) {
				++index;
			}

			appendRecord();
			++physicalLine;
			recordLine = physicalLine;
			continue;
		}

		if (character == QChar('"')) {
			if (!field.isEmpty()) {
				source.error =
					QString(
						"Unexpected quote in an unquoted field on line %1."
					)
						.arg(physicalLine);

				return source;
			}

			inQuotes = true;
			continue;
		}

		field += character;
	}

	if (inQuotes) {
		source.error =
			QString(
				"Quoted field beginning on or before line %1 is not closed."
			)
				.arg(physicalLine);

		return source;
	}

	if (!field.isEmpty() || !record.isEmpty() || afterQuote) {
		appendRecord();
	}

	if (source.records.isEmpty()) {
		source.error = QStringLiteral("CSV file is empty.");
		return source;
	}

	source.firstRowLikelyHeader =
		likelyHeaderRow(
			source.records.first()
		);

	source.ok = true;
	return source;
}

BreadboardWiringCsvSource BreadboardWiringCsvParser::parseSource(
	const QString &fileName,
	QChar delimiter
)
{
	BreadboardWiringCsvSource source;

	QFile file(fileName);

	if (!file.exists()) {
		source.error =
			QString("File does not exist: %1")
				.arg(fileName);

		return source;
	}

	if (!file.open(QIODevice::ReadOnly)) {
		source.error =
			QString("Cannot open %1: %2")
				.arg(fileName, file.errorString());

		return source;
	}

	if (file.size() > maximumCsvFileBytes) {
		source.error =
			QString(
				"CSV is larger than the supported 32 MiB import limit."
			);

		return source;
	}

	const QByteArray bytes = file.readAll();

	if (bytes.isEmpty()) {
		source.error = QStringLiteral("CSV file is empty.");
		return source;
	}

	QString encoding;
	QString decodeError;

	const QString text =
		decodeBytes(
			bytes,
			encoding,
			decodeError
		);

	if (!decodeError.isEmpty()) {
		source.error = decodeError;
		return source;
	}

	if (text.contains(QChar::Null)) {
		source.error =
			QStringLiteral(
				"CSV contains embedded null characters."
			);

		return source;
	}

	if (!delimiter.isNull()) {
		return parseText(
			text,
			delimiter,
			encoding
		);
	}

	const QList<QChar> candidates = {
		QChar(','),
		QChar(';'),
		QChar('\t'),
		QChar('|')
	};

	int bestScore = -1;
	BreadboardWiringCsvSource bestSource;

	for (const QChar candidate : candidates) {
		BreadboardWiringCsvSource parsed =
			parseText(
				text,
				candidate,
				encoding
			);

		if (
			!parsed.ok ||
			parsed.records.isEmpty() ||
			parsed.columnCount < 2
		) {
			continue;
		}

		const int expectedColumns =
			parsed.records.first().size();

		int consistentRows = 0;
		int inspectedRows = 0;

		for (const QStringList &record : parsed.records) {
			if (inspectedRows >= 50) {
				break;
			}

			++inspectedRows;

			if (record.size() == expectedColumns) {
				++consistentRows;
			}
		}

		const int score =
			(consistentRows * 1000) +
			(recognizedHeaderCount(parsed.records.first()) * 5000) +
			(expectedColumns * 10) -
			(parsed.columnCount - expectedColumns);

		if (score > bestScore) {
			bestScore = score;
			bestSource = parsed;
		}
	}

	if (bestScore < 0) {
		source.error =
			QStringLiteral(
				"Could not detect a supported CSV delimiter. "
				"Supported delimiters are comma, semicolon, tab and pipe."
			);

		return source;
	}

	return bestSource;
}

QStringList BreadboardWiringCsvParser::displayHeaders(
	const BreadboardWiringCsvSource &source,
	bool firstRowIsHeader
)
{
	QStringList headers;

	if (
		firstRowIsHeader &&
		!source.records.isEmpty()
	) {
		headers = source.records.first();
	}

	while (headers.size() < source.columnCount) {
		headers.append(QString());
	}

	for (int column = 0; column < source.columnCount; ++column) {
		if (headers.at(column).trimmed().isEmpty()) {
			headers[column] =
				QString("Column %1")
					.arg(column + 1);
		}
	}

	return headers;
}

QString BreadboardWiringCsvParser::normalizedHeader(
	const QString &header
)
{
	QString normalized;

	for (
		const QChar character :
		header.trimmed().toCaseFolded().normalized(
			QString::NormalizationForm_D
		)
	) {
		if (character.isLetterOrNumber()) {
			normalized += character;
		}
	}

	return normalized;
}

BreadboardWiringCsvColumnMapping BreadboardWiringCsvParser::suggestMapping(
	const BreadboardWiringCsvSource &source,
	bool firstRowIsHeader
)
{
	BreadboardWiringCsvColumnMapping mapping;
	mapping.firstRowIsHeader = firstRowIsHeader;

	if (!source.ok || !firstRowIsHeader) {
		return mapping;
	}

	const QStringList headers =
		displayHeaders(
			source,
			true
		);

	QSet<int> usedColumns;

	for (int fieldIndex = 0; fieldIndex < BreadboardWiringCsvFieldCount; ++fieldIndex) {
		const BreadboardWiringCsvField field =
			static_cast<BreadboardWiringCsvField>(fieldIndex);

		const QStringList fieldAliases =
			aliases(field);

		for (int column = 0; column < headers.size(); ++column) {
			if (usedColumns.contains(column)) {
				continue;
			}

			if (
				fieldAliases.contains(
					normalizedHeader(headers.at(column))
				)
			) {
				mapping.setColumn(field, column);
				usedColumns.insert(column);
				break;
			}
		}
	}

	return mapping;
}

BreadboardWiringCsvResult BreadboardWiringCsvParser::applyMapping(
	const BreadboardWiringCsvSource &source,
	const BreadboardWiringCsvColumnMapping &mapping
)
{
	BreadboardWiringCsvResult result;

	if (!source.ok) {
		result.error = source.error.isEmpty()
			? QStringLiteral("CSV source is not valid.")
			: source.error;

		return result;
	}

	const int firstDataRecord =
		mapping.firstRowIsHeader
			? 1
			: 0;

	if (source.records.size() <= firstDataRecord) {
		result.error = mapping.firstRowIsHeader
			? QStringLiteral("CSV contains a header but no records.")
			: QStringLiteral("CSV contains no records.");

		return result;
	}

	QSet<int> usedColumns;

	for (int fieldIndex = 0; fieldIndex < BreadboardWiringCsvFieldCount; ++fieldIndex) {
		const BreadboardWiringCsvField field =
			static_cast<BreadboardWiringCsvField>(fieldIndex);

		const int sourceColumn =
			mapping.column(field);

		if (requiredField(field) && sourceColumn < 0) {
			result.error =
				QString("Required field is not mapped: %1")
					.arg(fieldLabel(field));

			return result;
		}

		if (sourceColumn < 0) {
			continue;
		}

		if (sourceColumn >= source.columnCount) {
			result.error =
				QString(
					"%1 is mapped to unavailable source column %2."
				)
					.arg(fieldLabel(field))
					.arg(sourceColumn + 1);

			return result;
		}

		if (usedColumns.contains(sourceColumn)) {
			result.error =
				QString(
					"Source column %1 is mapped to more than one import field."
				)
					.arg(sourceColumn + 1);

			return result;
		}

		usedColumns.insert(sourceColumn);
	}

	for (
		int recordIndex = firstDataRecord;
		recordIndex < source.records.size();
		++recordIndex
	) {
		const QStringList &record =
			source.records.at(recordIndex);

		const int physicalLine =
			recordIndex < source.physicalLineNumbers.size()
				? source.physicalLineNumbers.at(recordIndex)
				: recordIndex + 1;

		BreadboardWiringCsvRow row;
		row.index = result.rows.size();

		row.wireId =
			valueAt(
				record,
				mapping.wireIdColumn
			);

		if (row.wireId.isEmpty()) {
			row.wireId =
				QString("ROW-%1")
					.arg(physicalLine);
		}

		row.signal =
			valueAt(
				record,
				mapping.signalColumn
			);

		row.color =
			valueAt(
				record,
				mapping.colorColumn
			);

		row.from =
			valueAt(
				record,
				mapping.fromColumn
			);

		row.fromTerminal =
			valueAt(
				record,
				mapping.fromTerminalColumn
			);

		row.to =
			valueAt(
				record,
				mapping.toColumn
			);

		row.toTerminal =
			valueAt(
				record,
				mapping.toTerminalColumn
			);

		row.note =
			valueAt(
				record,
				mapping.noteColumn
			);

		result.rows.append(row);
	}

	result.ok = true;
	return result;
}

BreadboardWiringCsvResult BreadboardWiringCsvParser::parseFile(
	const QString &fileName
)
{
	const BreadboardWiringCsvSource source =
		parseSource(
			fileName,
			QChar(',')
		);

	if (!source.ok) {
		BreadboardWiringCsvResult result;
		result.error = source.error;
		return result;
	}

	const QStringList headers =
		displayHeaders(
			source,
			true
		);

	if (headers != canonicalHeaders) {
		BreadboardWiringCsvResult result;
		result.error =
			QString("Unexpected CSV header.\n\nExpected:\n%1\n\nFound:\n%2")
				.arg(canonicalHeaders.join(','))
				.arg(headers.join(','));

		return result;
	}

	for (int recordIndex = 1; recordIndex < source.records.size(); ++recordIndex) {
		const QStringList &record =
			source.records.at(recordIndex);

		if (record.size() == canonicalHeaders.size()) {
			continue;
		}

		const int physicalLine =
			recordIndex < source.physicalLineNumbers.size()
				? source.physicalLineNumbers.at(recordIndex)
				: recordIndex + 1;

		BreadboardWiringCsvResult result;
		result.error =
			QString(
				"Line %1 has %2 fields; expected %3."
			)
				.arg(physicalLine)
				.arg(record.size())
				.arg(canonicalHeaders.size());

		return result;
	}

	BreadboardWiringCsvColumnMapping mapping;
	mapping.firstRowIsHeader = true;
	mapping.wireIdColumn = 0;
	mapping.signalColumn = 1;
	mapping.colorColumn = 2;
	mapping.fromColumn = 3;
	mapping.fromTerminalColumn = 4;
	mapping.toColumn = 5;
	mapping.toTerminalColumn = 6;
	mapping.noteColumn = 7;

	return applyMapping(
		source,
		mapping
	);
}

QString BreadboardWiringCsvParser::delimiterName(QChar delimiter)
{
	if (delimiter == QChar(',')) {
		return QStringLiteral("Comma");
	}

	if (delimiter == QChar(';')) {
		return QStringLiteral("Semicolon");
	}

	if (delimiter == QChar('\t')) {
		return QStringLiteral("Tab");
	}

	if (delimiter == QChar('|')) {
		return QStringLiteral("Pipe");
	}

	return QStringLiteral("Unknown");
}

QString BreadboardWiringCsvParser::fieldKey(
	BreadboardWiringCsvField field
)
{
	switch (field) {
	case BreadboardWiringCsvField::WireId:
		return QStringLiteral("wireId");

	case BreadboardWiringCsvField::Signal:
		return QStringLiteral("signal");

	case BreadboardWiringCsvField::Color:
		return QStringLiteral("color");

	case BreadboardWiringCsvField::From:
		return QStringLiteral("from");

	case BreadboardWiringCsvField::FromTerminal:
		return QStringLiteral("fromTerminal");

	case BreadboardWiringCsvField::To:
		return QStringLiteral("to");

	case BreadboardWiringCsvField::ToTerminal:
		return QStringLiteral("toTerminal");

	case BreadboardWiringCsvField::Note:
		return QStringLiteral("note");
	}

	return QString();
}
