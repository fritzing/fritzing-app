#include "fabuploaddialog.h"
#include "ui_fabuploaddialog.h"

#include <QFile>

FabUploadDialog::FabUploadDialog(QNetworkAccessManager *manager,
								 QString filename,
								 QWidget *parent) :
	QDialog(parent),
	ui(new Ui::FabUploadDialog)
{
	ui->setupUi(this);	
	setWindowFlags(Qt::Dialog | windowFlags());
	ui->stackedWidget->setCurrentIndex(0);
	ui->upload->init(manager, filename);
	ui->uploadButton_2->setEnabled(false);
}


FabUploadDialog::~FabUploadDialog()
{
	delete ui;
}

void FabUploadDialog::onUploadStarted()
{
	ui->stackedWidget->setCurrentIndex(1);
}

void FabUploadDialog::onUploadReady()
{
	ui->uploadButton_2->setEnabled(true);
	ui->uploadButton_2->setText(tr("Open in browser"));
}
