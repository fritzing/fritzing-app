#include "exportparametersdialog.h"
#include "ui_exportparametersdialog.h"

ExportParametersDialog::ExportParametersDialog(int dpi, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExportParametersDialog),
    m_dpi(dpi)
{
    ui->setupUi(this);
    ui->dpiSpinBox->setRange(minDPI, maxDPI);
    ui->dpiSpinBox->setSingleStep(stepDPI);
    ui->dpiSpinBox->setValue(m_dpi);
    this->setWindowTitle(tr("Export parameters"));
}

ExportParametersDialog::~ExportParametersDialog()
{
    delete ui;
}

int ExportParametersDialog::getDpi()
{
    return m_dpi;
}

void ExportParametersDialog::setValue(int value)
{
    ui->dpiSpinBox->setValue(value);
}
