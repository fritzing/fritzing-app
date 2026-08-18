#ifndef BREADBOARDCSVIMPORTDIALOG_H
#define BREADBOARDCSVIMPORTDIALOG_H

#include "breadboardwiringcsvparser.h"

#include <QDialog>

#include <array>

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QLabel;
class QPushButton;
class QTableWidget;

class BreadboardCsvImportDialog : public QDialog
{
	Q_OBJECT

public:
	BreadboardCsvImportDialog(
		const QString &fileName,
		const BreadboardWiringCsvSource &source,
		QWidget *parent = nullptr
	);

	BreadboardWiringCsvResult mappedResult() const;
	BreadboardWiringCsvColumnMapping selectedMapping() const;

private:
	void rebuildForHeaderMode();
	void applySuggestedMapping();
	void restoreMapping();
	void saveMapping() const;
	void refreshSourcePreview();
	void refreshMappedPreview();
	void refreshValidation();

	QString mappingSettingsKey() const;
	QString sourceHeaderForField(
		BreadboardWiringCsvField field
	) const;

	int dataRecordStart() const;
	int selectedSourceColumn(
		BreadboardWiringCsvField field
	) const;

	QString m_fileName;
	BreadboardWiringCsvSource m_source;
	BreadboardWiringCsvResult m_mappedResult;

	QLabel *m_sourceSummary = nullptr;
	QCheckBox *m_firstRowHeader = nullptr;
	QCheckBox *m_rememberMapping = nullptr;
	QTableWidget *m_sourcePreview = nullptr;
	QTableWidget *m_mappedPreview = nullptr;
	QLabel *m_validation = nullptr;
	QDialogButtonBox *m_buttonBox = nullptr;
	QPushButton *m_continueButton = nullptr;

	std::array<QComboBox *, BreadboardWiringCsvFieldCount> m_mappingControls{};
};

#endif
