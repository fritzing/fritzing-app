#include "modfiledialog.h"
#include "ui_modfiledialog.h"

#include <QPushButton>

ModFileDialog::ModFileDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ModFileDialog)
{
    ui->setupUi(this);
    ui->decision->setText(tr("Fritzing can proceed with the update, "
                             "but the set of files listed below must first be cleaned (removed or reset). "
                             "It may take a few minutes. "
                             "<p>Do you want to proceed with cleaning these files?</p>"));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Clean files"));
}

ModFileDialog::~ModFileDialog()
{
    delete ui;
}

void ModFileDialog::setText(const QString & text) {
    ui->label->setText(text);
}

void ModFileDialog::addList(const QString &header, const QStringList &list) {
    if (list.isEmpty()) return;

    if (ui->listWidget->count() > 0) {
        // add a separator
        ui->listWidget->addItem(" ");
    }
    ui->listWidget->addItem(header);
    foreach (QString string, list) {
        ui->listWidget->addItem("  " + string);
    }
}

void ModFileDialog::accept() {
    ui->buttonBox->setDisabled(true);
    ui->label->setText(tr("Now cleaning files. Please don't interrupt the process."));
    QApplication::processEvents();
    emit cleanRepo(this);

}
