#ifndef EXPORTPARAMETERSDIALOG_H
#define EXPORTPARAMETERSDIALOG_H

#include <QDialog>

namespace Ui {
class ExportParametersDialog;
}

class ExportParametersDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExportParametersDialog(int dpi, QWidget *parent = nullptr);
    ~ExportParametersDialog();

    int getDpi();
    void setValue(int value);

private slots:
    void on_dpiSpinBox_valueChanged(int arg1);

private:
    Ui::ExportParametersDialog *ui;
    int m_dpi;
    const int stepDPI = 10;
    const int minDPI = 10;
    const int maxDPI = 1500;
};

#endif // EXPORTPARAMETERSDIALOG_H
