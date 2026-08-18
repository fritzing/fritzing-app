#include "breadboardcsvimportdialog.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QGridLayout>

#include <algorithm>

namespace {

constexpr int previewRowLimit = 50;

BreadboardWiringCsvField fieldAt(int index)
{
	return static_cast<BreadboardWiringCsvField>(index);
}

QString fieldLabel(BreadboardWiringCsvField field)
{
	switch (field) {
	case BreadboardWiringCsvField::WireId:
		return QObject::tr("Wire ID");

	case BreadboardWiringCsvField::Signal:
		return QObject::tr("Signal");

	case BreadboardWiringCsvField::Color:
		return QObject::tr("Color");

	case BreadboardWiringCsvField::From:
		return QObject::tr("From");

	case BreadboardWiringCsvField::FromTerminal:
		return QObject::tr("From hole/terminal");

	case BreadboardWiringCsvField::To:
		return QObject::tr("To");

	case BreadboardWiringCsvField::ToTerminal:
		return QObject::tr("To hole/terminal");

	case BreadboardWiringCsvField::Note:
		return QObject::tr("Note");
	}

	return QString();
}

bool requiredField(BreadboardWiringCsvField field)
{
	return
		field == BreadboardWiringCsvField::From ||
		field == BreadboardWiringCsvField::FromTerminal ||
		field == BreadboardWiringCsvField::To ||
		field == BreadboardWiringCsvField::ToTerminal;
}

QString recordValue(const QStringList &record, int column)
{
	if (column < 0 || column >= record.size()) {
		return QString();
	}

	return record.at(column).trimmed();
}

} // namespace

BreadboardCsvImportDialog::BreadboardCsvImportDialog(
	const QString &fileName,
	const BreadboardWiringCsvSource &source,
	QWidget *parent
) :
	QDialog(parent),
	m_fileName(fileName),
	m_source(source)
{
	setWindowTitle(tr("Preview and Map Breadboard Wiring CSV"));
	setModal(true);
	resize(1120, 780);

	/*
	 * Fritzing's application stylesheet can combine with the native
	 * macOS palette to produce light control text and dark table text
	 * on similarly colored backgrounds. Keep this data-review surface
	 * explicitly high contrast without changing the rest of the app.
	 */
	setStyleSheet(
		QStringLiteral(
			"QGroupBox, QCheckBox { color: #202124; }"
			"QComboBox {"
			"  background-color: #ffffff;"
			"  color: #202124;"
			"  selection-background-color: #1769aa;"
			"  selection-color: #ffffff;"
			"}"
			"QComboBox QAbstractItemView {"
			"  background-color: #ffffff;"
			"  color: #202124;"
			"  selection-background-color: #1769aa;"
			"  selection-color: #ffffff;"
			"}"
			"QTableWidget {"
			"  background-color: #ffffff;"
			"  alternate-background-color: #f1f3f4;"
			"  color: #202124;"
			"  gridline-color: #bdc1c6;"
			"  selection-background-color: #1769aa;"
			"  selection-color: #ffffff;"
			"}"
			"QTableWidget::item { color: #202124; }"
			"QTableWidget::item:selected { color: #ffffff; }"
			"QHeaderView::section, QTableCornerButton::section {"
			"  background-color: #e8eaed;"
			"  color: #202124;"
			"  border: 1px solid #bdc1c6;"
			"  padding: 4px;"
			"}"
		)
	);

	QVBoxLayout *mainLayout =
		new QVBoxLayout(this);

	QLabel *intro =
		new QLabel(
			tr(
				"Preview the source data and map its columns to Fritzing's "
				"breadboard wiring fields. No sketch changes are made until "
				"the later placement confirmation."
			),
			this
		);

	intro->setWordWrap(true);
	mainLayout->addWidget(intro);

	m_sourceSummary =
		new QLabel(this);

	m_sourceSummary->setTextInteractionFlags(
		Qt::TextSelectableByMouse
	);

	mainLayout->addWidget(m_sourceSummary);

	m_firstRowHeader =
		new QCheckBox(
			tr("First row contains column names"),
			this
		);

	m_firstRowHeader->setChecked(
		m_source.firstRowLikelyHeader
	);

	mainLayout->addWidget(m_firstRowHeader);

	QGroupBox *mappingGroup =
		new QGroupBox(
			tr("Column mapping"),
			this
		);

	QGridLayout *mappingLayout =
		new QGridLayout(mappingGroup);

	mappingLayout->addWidget(
		new QLabel(tr("Fritzing field"), mappingGroup),
		0,
		0
	);

	mappingLayout->addWidget(
		new QLabel(tr("Source column"), mappingGroup),
		0,
		1
	);

	mappingLayout->addWidget(
		new QLabel(tr("Requirement"), mappingGroup),
		0,
		2
	);

	for (int fieldIndex = 0; fieldIndex < BreadboardWiringCsvFieldCount; ++fieldIndex) {
		const BreadboardWiringCsvField field =
			fieldAt(fieldIndex);

		QLabel *label =
			new QLabel(
				fieldLabel(field),
				mappingGroup
			);

		QComboBox *control =
			new QComboBox(mappingGroup);

		m_mappingControls.at(fieldIndex) =
			control;

		mappingLayout->addWidget(
			label,
			fieldIndex + 1,
			0
		);

		mappingLayout->addWidget(
			control,
			fieldIndex + 1,
			1
		);

		mappingLayout->addWidget(
			new QLabel(
				requiredField(field)
					? tr("Required")
					: tr("Optional"),
				mappingGroup
			),
			fieldIndex + 1,
			2
		);

		connect(
			control,
			static_cast<void (QComboBox::*)(int)>(
				&QComboBox::currentIndexChanged
			),
			this,
			[this](int) {
				refreshMappedPreview();
				refreshValidation();
			}
		);
	}

	QPushButton *suggestButton =
		new QPushButton(
			tr("Reset to suggested mapping"),
			mappingGroup
		);

	mappingLayout->addWidget(
		suggestButton,
		BreadboardWiringCsvFieldCount + 1,
		1,
		1,
		2,
		Qt::AlignRight
	);

	mainLayout->addWidget(mappingGroup);

	QTabWidget *previews =
		new QTabWidget(this);

	m_sourcePreview =
		new QTableWidget(previews);

	m_mappedPreview =
		new QTableWidget(previews);

	for (
		QTableWidget *table :
		{ m_sourcePreview, m_mappedPreview }
	) {
		table->setEditTriggers(
			QAbstractItemView::NoEditTriggers
		);

		table->setSelectionBehavior(
			QAbstractItemView::SelectRows
		);

		table->setAlternatingRowColors(true);
		table->horizontalHeader()->setStretchLastSection(true);
	}

	previews->addTab(
		m_sourcePreview,
		tr("Source preview")
	);

	previews->addTab(
		m_mappedPreview,
		tr("Mapped preview")
	);

	mainLayout->addWidget(previews, 1);

	m_validation =
		new QLabel(this);

	m_validation->setWordWrap(true);
	mainLayout->addWidget(m_validation);

	m_rememberMapping =
		new QCheckBox(
			tr("Remember this mapping for files with these columns"),
			this
		);

	m_rememberMapping->setChecked(true);
	mainLayout->addWidget(m_rememberMapping);

	m_buttonBox =
		new QDialogButtonBox(
			QDialogButtonBox::Ok |
				QDialogButtonBox::Cancel,
			this
		);

	m_continueButton =
		m_buttonBox->button(
			QDialogButtonBox::Ok
		);

	m_continueButton->setText(
		tr("Continue")
	);

	mainLayout->addWidget(m_buttonBox);

	connect(
		m_firstRowHeader,
		&QCheckBox::toggled,
		this,
		[this](bool) {
			rebuildForHeaderMode();
		}
	);

	connect(
		suggestButton,
		&QPushButton::clicked,
		this,
		[this]() {
			applySuggestedMapping();
			refreshMappedPreview();
			refreshValidation();
		}
	);

	connect(
		m_buttonBox,
		&QDialogButtonBox::rejected,
		this,
		&QDialog::reject
	);

	connect(
		m_buttonBox,
		&QDialogButtonBox::accepted,
		this,
		[this]() {
			refreshValidation();

			if (!m_mappedResult.ok) {
				return;
			}

			if (m_rememberMapping->isChecked()) {
				saveMapping();
			}

			accept();
		}
	);

	rebuildForHeaderMode();
}

BreadboardWiringCsvResult BreadboardCsvImportDialog::mappedResult() const
{
	return m_mappedResult;
}

BreadboardWiringCsvColumnMapping BreadboardCsvImportDialog::selectedMapping() const
{
	BreadboardWiringCsvColumnMapping mapping;
	mapping.firstRowIsHeader =
		m_firstRowHeader->isChecked();

	for (int fieldIndex = 0; fieldIndex < BreadboardWiringCsvFieldCount; ++fieldIndex) {
		mapping.setColumn(
			fieldAt(fieldIndex),
			selectedSourceColumn(
				fieldAt(fieldIndex)
			)
		);
	}

	return mapping;
}

int BreadboardCsvImportDialog::selectedSourceColumn(
	BreadboardWiringCsvField field
) const
{
	const QComboBox *control =
		m_mappingControls.at(
			static_cast<int>(field)
		);

	return control == nullptr
		? -1
		: control->currentData().toInt();
}

int BreadboardCsvImportDialog::dataRecordStart() const
{
	return m_firstRowHeader->isChecked()
		? 1
		: 0;
}

void BreadboardCsvImportDialog::rebuildForHeaderMode()
{
	const QStringList headers =
		BreadboardWiringCsvParser::displayHeaders(
			m_source,
			m_firstRowHeader->isChecked()
		);

	for (QComboBox *control : m_mappingControls) {
		control->blockSignals(true);
		control->clear();
		control->addItem(
			tr("Not mapped"),
			-1
		);

		for (int column = 0; column < headers.size(); ++column) {
			control->addItem(
				tr("Column %1 — %2")
					.arg(column + 1)
					.arg(headers.at(column)),
				column
			);
		}

		control->blockSignals(false);
	}

	applySuggestedMapping();
	restoreMapping();
	refreshSourcePreview();
	refreshMappedPreview();
	refreshValidation();
}

void BreadboardCsvImportDialog::applySuggestedMapping()
{
	const BreadboardWiringCsvColumnMapping suggested =
		BreadboardWiringCsvParser::suggestMapping(
			m_source,
			m_firstRowHeader->isChecked()
		);

	for (int fieldIndex = 0; fieldIndex < BreadboardWiringCsvFieldCount; ++fieldIndex) {
		QComboBox *control =
			m_mappingControls.at(fieldIndex);

		const int sourceColumn =
			suggested.column(
				fieldAt(fieldIndex)
			);

		const int controlIndex =
			control->findData(sourceColumn);

		control->setCurrentIndex(
			controlIndex >= 0
				? controlIndex
				: 0
		);
	}
}

QString BreadboardCsvImportDialog::mappingSettingsKey() const
{
	const QStringList headers =
		BreadboardWiringCsvParser::displayHeaders(
			m_source,
			m_firstRowHeader->isChecked()
		);

	QStringList normalizedHeaders;

	for (const QString &header : headers) {
		normalizedHeaders.append(
			BreadboardWiringCsvParser::normalizedHeader(
				header
			)
		);
	}

	const QByteArray identity =
		QString("%1|%2")
			.arg(
				m_firstRowHeader->isChecked()
					? 1
					: 0
			)
			.arg(normalizedHeaders.join(QChar(0x001F)))
			.toUtf8();

	const QString digest =
		QString::fromLatin1(
			QCryptographicHash::hash(
				identity,
				QCryptographicHash::Sha256
			).toHex()
		);

	return QStringLiteral(
		"breadboardCsvImport/columnMappings/v1/"
	) + digest;
}

QString BreadboardCsvImportDialog::sourceHeaderForField(
	BreadboardWiringCsvField field
) const
{
	const int sourceColumn =
		selectedSourceColumn(field);

	const QStringList headers =
		BreadboardWiringCsvParser::displayHeaders(
			m_source,
			m_firstRowHeader->isChecked()
		);

	if (
		sourceColumn < 0 ||
		sourceColumn >= headers.size()
	) {
		return QString();
	}

	return headers.at(sourceColumn);
}

void BreadboardCsvImportDialog::restoreMapping()
{
	QSettings settings;
	settings.beginGroup(mappingSettingsKey());

	if (!settings.value(QStringLiteral("saved"), false).toBool()) {
		settings.endGroup();
		return;
	}

	const QStringList headers =
		BreadboardWiringCsvParser::displayHeaders(
			m_source,
			m_firstRowHeader->isChecked()
		);

	for (int fieldIndex = 0; fieldIndex < BreadboardWiringCsvFieldCount; ++fieldIndex) {
		const BreadboardWiringCsvField field =
			fieldAt(fieldIndex);

		const QString fieldKey =
			BreadboardWiringCsvParser::fieldKey(field);

		const int storedColumn =
			settings.value(
				fieldKey + QStringLiteral("Column"),
				-1
			).toInt();

		const QString storedHeader =
			settings.value(
				fieldKey + QStringLiteral("Header")
			).toString();

		int restoredColumn = -1;

		if (
			storedColumn >= 0 &&
			storedColumn < headers.size() &&
			BreadboardWiringCsvParser::normalizedHeader(
				headers.at(storedColumn)
			) == storedHeader
		) {
			restoredColumn = storedColumn;
		}
		else if (!storedHeader.isEmpty()) {
			for (int column = 0; column < headers.size(); ++column) {
				if (
					BreadboardWiringCsvParser::normalizedHeader(
						headers.at(column)
					) == storedHeader
				) {
					restoredColumn = column;
					break;
				}
			}
		}

		QComboBox *control =
			m_mappingControls.at(fieldIndex);

		const int controlIndex =
			control->findData(restoredColumn);

		control->setCurrentIndex(
			controlIndex >= 0
				? controlIndex
				: 0
		);
	}

	settings.endGroup();
}

void BreadboardCsvImportDialog::saveMapping() const
{
	QSettings settings;
	settings.beginGroup(mappingSettingsKey());
	settings.remove(QString());
	settings.setValue(
		QStringLiteral("saved"),
		true
	);

	for (int fieldIndex = 0; fieldIndex < BreadboardWiringCsvFieldCount; ++fieldIndex) {
		const BreadboardWiringCsvField field =
			fieldAt(fieldIndex);

		const QString fieldKey =
			BreadboardWiringCsvParser::fieldKey(field);

		settings.setValue(
			fieldKey + QStringLiteral("Column"),
			selectedSourceColumn(field)
		);

		settings.setValue(
			fieldKey + QStringLiteral("Header"),
			BreadboardWiringCsvParser::normalizedHeader(
				sourceHeaderForField(field)
			)
		);
	}

	settings.endGroup();
}

void BreadboardCsvImportDialog::refreshSourcePreview()
{
	const QStringList headers =
		BreadboardWiringCsvParser::displayHeaders(
			m_source,
			m_firstRowHeader->isChecked()
		);

	const int firstRecord =
		dataRecordStart();

	const int availableRecords =
		std::max(
			0,
			static_cast<int>(m_source.records.size()) - firstRecord
		);

	const int previewRecords =
		std::min(
			previewRowLimit,
			availableRecords
		);

	m_sourceSummary->setText(
		tr(
			"File: %1\nEncoding: %2    Delimiter: %3    "
			"Columns: %4    Data records: %5"
		)
			.arg(QFileInfo(m_fileName).fileName())
			.arg(m_source.encoding)
			.arg(
				BreadboardWiringCsvParser::delimiterName(
					m_source.delimiter
				)
			)
			.arg(m_source.columnCount)
			.arg(availableRecords)
	);

	m_sourcePreview->clear();
	m_sourcePreview->setColumnCount(m_source.columnCount);
	m_sourcePreview->setRowCount(previewRecords);
	m_sourcePreview->setHorizontalHeaderLabels(headers);

	for (int previewRow = 0; previewRow < previewRecords; ++previewRow) {
		const QStringList &record =
			m_source.records.at(
				firstRecord + previewRow
			);

		for (int column = 0; column < m_source.columnCount; ++column) {
			m_sourcePreview->setItem(
				previewRow,
				column,
				new QTableWidgetItem(
					recordValue(record, column)
				)
			);
		}
	}

	m_sourcePreview->resizeColumnsToContents();
}

void BreadboardCsvImportDialog::refreshMappedPreview()
{
	const int firstRecord =
		dataRecordStart();

	const int availableRecords =
		std::max(
			0,
			static_cast<int>(m_source.records.size()) - firstRecord
		);

	const int previewRecords =
		std::min(
			previewRowLimit,
			availableRecords
		);

	QStringList mappedHeaders;

	for (int fieldIndex = 0; fieldIndex < BreadboardWiringCsvFieldCount; ++fieldIndex) {
		mappedHeaders.append(
			fieldLabel(
				fieldAt(fieldIndex)
			)
		);
	}

	m_mappedPreview->clear();
	m_mappedPreview->setColumnCount(BreadboardWiringCsvFieldCount);
	m_mappedPreview->setRowCount(previewRecords);
	m_mappedPreview->setHorizontalHeaderLabels(mappedHeaders);

	for (int previewRow = 0; previewRow < previewRecords; ++previewRow) {
		const QStringList &record =
			m_source.records.at(
				firstRecord + previewRow
			);

		for (int fieldIndex = 0; fieldIndex < BreadboardWiringCsvFieldCount; ++fieldIndex) {
			const BreadboardWiringCsvField field =
				fieldAt(fieldIndex);

			QString value =
				recordValue(
					record,
					selectedSourceColumn(field)
				);

			if (
				field == BreadboardWiringCsvField::WireId &&
				value.isEmpty()
			) {
				const int recordIndex =
					firstRecord + previewRow;

				const int physicalLine =
					recordIndex < m_source.physicalLineNumbers.size()
						? m_source.physicalLineNumbers.at(recordIndex)
						: recordIndex + 1;

				value =
					tr("ROW-%1")
						.arg(physicalLine);
			}

			QTableWidgetItem *item =
				new QTableWidgetItem(value);

			if (
				requiredField(field) &&
				selectedSourceColumn(field) < 0
			) {
				item->setBackground(
					QColor(255, 224, 224)
				);
			}

			m_mappedPreview->setItem(
				previewRow,
				fieldIndex,
				item
			);
		}
	}

	m_mappedPreview->resizeColumnsToContents();
}

void BreadboardCsvImportDialog::refreshValidation()
{
	m_mappedResult =
		BreadboardWiringCsvParser::applyMapping(
			m_source,
			selectedMapping()
		);

	if (m_mappedResult.ok) {
		m_validation->setText(
			tr(
				"Mapping is valid. %1 records are ready for the existing "
				"coordinate, component and placement validation."
			)
				.arg(m_mappedResult.rows.size())
		);

		m_validation->setStyleSheet(
			QStringLiteral("color: #207020;")
		);

		m_continueButton->setEnabled(true);
		return;
	}

	m_validation->setText(
		tr("Mapping is not ready: %1")
			.arg(m_mappedResult.error)
	);

	m_validation->setStyleSheet(
		QStringLiteral("color: #a02020;")
	);

	m_continueButton->setEnabled(false);
}
