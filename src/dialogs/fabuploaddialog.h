#ifndef FABUPLOADDIALOG_H
#define FABUPLOADDIALOG_H

#include <QDialog>
#include <QNetworkAccessManager>
#include <QPixmap>

namespace Ui {
class FabUploadDialog;
}

class FabUploadDialog : public QDialog
{
	Q_OBJECT

public:
	explicit FabUploadDialog(QNetworkAccessManager* manager,
							 QString filename,
							 double width,
							 double height,
							 int boardCount,
							 QWidget *parent = nullptr);
	~FabUploadDialog();

	void setUploadButtonEnabled(bool enabled);
public Q_SLOTS:

	void setFabMessage(const QString& text);
	void setFabIcon(const QPixmap& pixmap);

	void onRequestFabInfoFinished();
private Q_SLOTS:
	void onUploadStarted();
	void onUploadReady();

private:
	void requestFabInfo();
	Ui::FabUploadDialog *ui;
	QNetworkAccessManager *m_manager;

	QString m_fabName;
	void handleError(QNetworkReply *reply, const QString &message);
	void setFabName(QString);
};

#endif // FABUPLOADDIALOG_H
