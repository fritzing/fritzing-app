#include "fabuploaddialog.h"
#include "networkhelper.h"
#include "qstyle.h"
#include "qurlquery.h"
#include "referencemodel/sqlitereferencemodel.h"
#include "ui_fabuploaddialog.h"

#include <QFile>
#include <QSettings>


FabUploadDialog::FabUploadDialog(QNetworkAccessManager *manager,
								 QString filename,
								 QWidget *parent) :
	QDialog(parent),
	ui(new Ui::FabUploadDialog),
	m_manager(manager)
{

	ui->setupUi(this);	
	setWindowFlags(Qt::Dialog | windowFlags());
	ui->stackedWidget->setCurrentIndex(0);
	ui->upload->init(manager, filename);
	ui->uploadButton_2->setEnabled(false);

	requestFabInfo();
}


FabUploadDialog::~FabUploadDialog()
{
	delete ui;
}

void FabUploadDialog::setFabMessage(const QString& text)
{
	ui->fab_message->setText(text);
}

void FabUploadDialog::setFabIcon(const QPixmap& pixmap)
{
	int bigIconSize = QApplication::style()->pixelMetric(QStyle::PM_MessageBoxIconSize);
	QPixmap resizedPixmap = pixmap.scaled(bigIconSize, bigIconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	SqliteReferenceModel().insertIcon(m_fabName, resizedPixmap);
	ui->fab_icon->setPixmap(resizedPixmap);
}

void FabUploadDialog::setUploadButtonEnabled(bool enabled)
{
	ui->uploadButton->setEnabled(enabled);
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

void FabUploadDialog::requestFabInfo()
{
	QSettings settings;
	QString fabName = settings.value("service", "").toString();

	QUrl get_fab("https://service.fritzing.org/fab/show");

	QUrlQuery query;
	if (!fabName.isEmpty()) {
		query.addQueryItem("name", fabName);
		get_fab.setQuery(query);
	}
	QNetworkRequest request(get_fab);
	QNetworkReply *reply = m_manager->get(request);
	connect(reply, SIGNAL(finished()), this, SLOT(onRequestFabInfoFinished()));
}

void FabUploadDialog::onRequestFabInfoFinished()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
	if (!reply)
		return; // Safety check

	if (reply->error() != QNetworkReply::NoError) {
		handleError(reply, "Checking for fab... " + reply->errorString());
		return;
	}

	QByteArray responseData = reply->readAll();
	QJsonObject jsonObject = NetworkHelper::string_to_hash(QString::fromUtf8(responseData));

	if (jsonObject.isEmpty()) {
		handleError(reply, "Invalid JSON response.");
		return;
	}

	QString name = jsonObject.value("name").toString();
	QString uploadMessage = jsonObject.value("upload_message").toString();
	QString iconUrl_s = jsonObject.value("icon_url").toString();

	// Verify all the necessary fields are present and not empty
	if (name.isEmpty() || uploadMessage.isEmpty() || iconUrl_s.isEmpty()) {
		handleError(reply, "Incomplete JSON response. Required fields are missing.");
		return;
	}

	setFabName(name);
	setUploadButtonEnabled(true);
	setFabMessage(uploadMessage);

	QUrl iconUrl(iconUrl_s);
	QNetworkRequest iconRequest(iconUrl);
	QNetworkReply *iconReply = m_manager->get(iconRequest);
	connect(iconReply, &QNetworkReply::finished, this, [this, iconReply]() {
		QByteArray iconData = iconReply->readAll();
		QPixmap pixmap;
		pixmap.loadFromData(iconData);
		setFabIcon(pixmap);
		iconReply->deleteLater();
	});

	reply->deleteLater();
}

void FabUploadDialog::handleError(QNetworkReply *reply, const QString &message)
{
	qDebug() << "Error: " << message;
	setFabMessage(message);
	setUploadButtonEnabled(false);
	if (reply)
		reply->deleteLater();
}

void FabUploadDialog::setFabName(QString fabName)
{
	QSettings settings;
	settings.setValue("service", fabName);
	m_fabName = fabName;
}

