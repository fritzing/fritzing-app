#ifndef FABUPLOADDIALOG_H
#define FABUPLOADDIALOG_H

#include <QDialog>
#include <QNetworkAccessManager>

namespace Ui {
class FabUploadDialog;
}

class FabUploadDialog : public QDialog
{
	Q_OBJECT

public:
	explicit FabUploadDialog(QNetworkAccessManager* manager,
							 QString filename,
//							 double area,
//							 double boardCount,
							 QWidget *parent = nullptr);
	~FabUploadDialog();

private slots:
	void onUploadStarted();
	void onUploadReady();

private:
	Ui::FabUploadDialog *ui;

};

#endif // FABUPLOADDIALOG_H
