#include "exportparametersdialog.h"
#include "ui_exportparametersdialog.h"

ExportParametersDialog::ExportParametersDialog(int dpi, QWidget *parent) :
    m_dpi(dpi),
    QDialog(parent),
    ui(new Ui::ExportParametersDialog)
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

void ExportParametersDialog::on_dpiSpinBox_valueChanged(int arg1)
{
    m_dpi = arg1;
}
