#include "fabuploadprogress.h"
#include "networkhelper.h"

#include <QTextStream>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QLabel>
#include <QDesktopServices>
#include <QSettings>

#include <QDebug>

#include <src/utils/fmessagebox.h>

//TODO: Fix dialog layout, add image
//TODO: Request upload url, logo + text at fritzing.org/fab/new
//TODO: If fab not reachable: Show hint to Aisler and gerber export
//TODO: remove qDebug

// Happy flow
// 0. Save file
// 1. Request upload url
// 2. Check upload url response
// 3. start upload (call uploadMultipart)
// 4. show upload progress
// 5. after upload, start checking processing on fab
// 6. show processing progress
// 7. processing finished, ask "open in browser"

// Error handling
// 0.  Show information, abort
// > 1. File not saved -> Show error, abort
//


FabUploadProgress::FabUploadProgress(QWidget *parent) : QWidget(parent)
{
}

void FabUploadProgress::init(QNetworkAccessManager *manager, QString filename)
{
	mManager = manager;
	mFilepath = filename;
	mActivity = 0;
}

void FabUploadProgress::doUpload()
{
	QSettings settings;
	QString upload_url_str = settings.value("aisler/" + mFilepath, "").toString();
	if (!upload_url_str.isEmpty()) {
		uploadMultipart(QUrl(upload_url_str), mFilepath);
		return;
	}
	QUrl new_url("https://fritzing.org/fab/upload");
	QNetworkRequest request(new_url);
	QNetworkReply *reply = mManager->get(request);
	connect(reply, SIGNAL(finished()), this, SLOT(onRequestUploadFinished()));
	// Note: When an error occures, finished() is signaled, and we can handle the
	// error better in onRequestUploadFinished
	//connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onError(QNetworkReply::NetworkError)));
}


void FabUploadProgress::onRequestUploadFinished()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

	//Check status code
	if (reply->error() == QNetworkReply::NoError) {
		int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
		if(statusCode == 301 || statusCode==302) {
			QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
//			qDebug() << redirectUrl.toString();
			QNetworkRequest request(redirectUrl);
			QNetworkReply *r = mManager->get(request);
			connect(r, SIGNAL(finished()), this, SLOT(onRequestUploadFinished()));
			connect(r, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onError(QNetworkReply::NetworkError)));
		} else {
//			qDebug() << statusCode << Qt::endl;
			auto d = reply->readAll();
//			qDebug() << d << Qt::endl << Qt::flush;
			auto j = NetworkHelper::string_to_hash(d);
//			qDebug() << j["upload_url"].toString() << Qt::endl << Qt::flush;
			QUrl upload_url(QUrl::fromUserInput(j["upload_url"].toString()));
			QUrl project_url(j["project_url"].toString());
			uploadMultipart(upload_url, mFilepath);
			QSettings settings;
			settings.setValue("aisler/" + mFilepath, j["upload_url"].toString());
		}
	} else {
		httpError(reply);
	}
	reply->deleteLater();
}


void FabUploadProgress::uploadMultipart(const QUrl &url, const QString &file_path)
{
//	qDebug() << url.toString() << Qt::endl << Qt::flush;

	QHttpMultiPart *httpMultiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
	QFile *file = new QFile(file_path);
	QHttpPart imagePart;
	imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"upload[file]\"; filename=\"" + QFileInfo(*file).fileName() + "\""));
	imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream"));
//	qDebug() << httpMultiPart->boundary();

	file->open(QIODevice::ReadOnly);
	imagePart.setBodyDevice(file);
	file->setParent(httpMultiPart); // we cannot delete the file object now, so delete it with the multiPart

	httpMultiPart->append(imagePart);

	QNetworkRequest request(url);
	QNetworkReply *reply = mManager->post(request, httpMultiPart);

	httpMultiPart->setParent(reply); // delete the multiPart with the reply

	connect(reply, SIGNAL(finished()), this, SLOT(uploadDone()));
	connect(reply, SIGNAL(uploadProgress(qint64, qint64)), this, SLOT  (uploadProgress(qint64, qint64)));

	auto r = reply->request();
//	qDebug() << NetworkHelper::debugRequest(r);
}

void FabUploadProgress::uploadProgress(qint64 bytesSent, qint64 bytesTotal) {
	qDebug() << "---------Uploaded--------------" << bytesSent<< "of" <<bytesTotal;
	if (bytesSent > 0) {
		emit uploadProgressChanged(100 * bytesTotal / bytesSent);
	}
}

//// Handle errors from NetworkManager
void FabUploadProgress::onError(QNetworkReply::NetworkError code)
{
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
	qDebug() << "onError" << code << Qt::endl << Qt::flush;
	FMessageBox::critical(this, tr("Fritzing"), tr("Could not connect to Fritzing fab.") + "Error: " + reply->errorString());
	emit closeUploadError();
}

// Handle http errors detected
void FabUploadProgress::httpError(QNetworkReply* reply)
{
	QString error(reply->errorString() + reply->attribute( QNetworkRequest::HttpStatusCodeAttribute).toString());
	qDebug() << error;
	FMessageBox::critical(this, tr("Fritzing"), error);
	emit closeUploadError();
}

// Handle errors reported by remote server
void FabUploadProgress::apiError(QString message)
{
	qDebug() << message;
	FMessageBox::critical(this, tr("Fritzing"), tr("Error processing the project. The factory says: %1").arg(message));
	emit closeUploadError();
}


void FabUploadProgress::uploadDone() {
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
	qDebug() << "----------Finished--------------" << Qt::endl;
	if (reply->error() == QNetworkReply::NoError) {
		auto d = reply->readAll();
//		qDebug() << d << Qt::endl << Qt::flush;
		auto j = NetworkHelper::string_to_hash(d);
//		qDebug() << j["upload_url"].toString() << Qt::endl << Qt::flush;
		QUrl callback_url(QUrl::fromUserInput(j["callback"].toString()));
		mRedirect_url = j["redirect"].toString();
		mActivity = 0;
		checkProcessingStatus(callback_url);
	} else {
		httpError(reply);
	}
	reply->deleteLater();
}

void FabUploadProgress::checkProcessingStatus(QUrl url)
{
//	qDebug() << url.toString() << Qt::endl << Qt::flush;
	QNetworkRequest request(url);
	QNetworkReply *reply = mManager->get(request);
	connect(reply, SIGNAL(finished()), this, SLOT(updateProcessingStatus()));
//	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onError(QNetworkReply::NetworkError)));
}

void FabUploadProgress::updateProcessingStatus()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

	if (reply->error() == QNetworkReply::NoError) {
		auto d = reply->readAll();
//		qDebug() << d << Qt::endl << Qt::flush;
		auto j = NetworkHelper::string_to_hash(d);
		// Produce a funny number which is growing most of the time,
		// and somewhat related to what is really happening. It should
		// change on every signal, so if it stops, there is a problem.
		int progress = j["progress"].toInt();
//		qDebug() << progress;
		QString message(j["message"].toString());
		if (progress < 0) {
			apiError(message);
		} else {
			if (progress + mActivity > 90) {
				mActivity = 0;
				progress = j["progress"].toInt();
			}
			mActivity += 1;
			findChild<QLabel*>("message")->setText(message);
			emit processProgressChanged(std::min(progress + mActivity, 100));
			if(progress < 100) {
				QUrl url = reply->url();
				QTimer::singleShot(1000, this, [this, url](){
					checkProcessingStatus(url);
				});
			} else {
				emit processingDone();
			}
		}
	} else {
		httpError(reply);
	}
	reply->deleteLater();
}

void FabUploadProgress::openInBrowser()
{
	QDesktopServices::openUrl(mRedirect_url);
}
