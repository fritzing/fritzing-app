#include "fabuploadprogress.h"
#include "fabuploadprogressprobe.h"
#include "networkhelper.h"

#include "version/version.h"

#include <QTextStream>
#include <QUrl>
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QLabel>
#include <QDesktopServices>
#include <QSettings>
#include <QMetaEnum>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "utils/fmessagebox.h"
#include "utils/uploadpair.h"
#include "debugdialog.h"

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
	mFabUploadProgressProbe = new FabUploadProgressProbe(this);
}

void FabUploadProgress::init(QNetworkAccessManager *manager, QString filename,
				 double width, double height, int boardCount, const QString & boardTitle)
{
	mManager = manager;
	mFilepath = filename;
	mWidth = width;
	mHeight = height;
	mBoardCount = boardCount;
	mBoardTitle = boardTitle;
}

void FabUploadProgress::doUpload()
{
	QSettings settings;
	mService = settings.value("service", "").toString();
	QUrl upload_url("https://service.fritzing.org/fab/upload");

	settings.beginGroup("sketches");
	QVariant settingValue = settings.value(mFilepath);
	settings.endGroup();

	if (auto opt = settingValue.value<UploadPair>(); settingValue.isValid() && !settingValue.isNull()) {
		if (!opt.link.isEmpty() && mService == opt.service) {
			// Already uploaded before, reuse the url so we get a project update
			QUrl potential_url(opt.link);
			if (potential_url.isValid()) {
				upload_url = potential_url;
				uploadMultipart(upload_url.toString(), mFilepath);
				return;
			}
			// Otherwise, use the default URL
		}
	}

	QUrlQuery query;
	QString fritzingVersion = Version::versionString();
	query.addQueryItem("fritzing_version", fritzingVersion);
	query.addQueryItem("service", mService);
	upload_url.setQuery(query);
	mRedirect_url = QString();

	QNetworkRequest request(upload_url);
	QNetworkReply *reply = mManager->get(request);
	connect(reply, SIGNAL(finished()), this, SLOT(onRequestUploadFinished()));
	// Error handling is done in onRequestUploadFinished, too
}


void FabUploadProgress::onRequestUploadFinished()
{
	auto *reply = qobject_cast<QNetworkReply*>(sender());

	//Check status code
	if (reply->error() == QNetworkReply::NoError) {
		int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
		if(statusCode == 301 || statusCode==302) {
			QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
			QNetworkRequest request(redirectUrl);
			QNetworkReply *r = mManager->get(request);
			connect(r, SIGNAL(finished()), this, SLOT(onRequestUploadFinished()));
			connect(r, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onError(QNetworkReply::NetworkError)));
		} else {
			auto d = reply->readAll();
			auto j = NetworkHelper::string_to_hash(d);
			QUrl upload_url(QUrl::fromUserInput(j["upload_url"].toString()));
			uploadMultipart(upload_url.toString(), mFilepath);
		}
	} else {
		httpError(reply);
	}
	reply->deleteLater();
}


void FabUploadProgress::uploadMultipart(const QString &urlStr, const QString &file_path)
{
	auto *httpMultiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

	std::map<QString, QString> params {
		{"fritzing_version", Version::versionString()},
		{"service", mService},
		{"width", QString::number(mWidth)},
		{"height", QString::number(mHeight)},
		{"board_count", QString::number(mBoardCount)},
		{"board_title", mBoardTitle} // Assuming mBoardTitle is a UTF-8 encoded QString
	};

	for (const auto& [key, value] : params) {
		QHttpPart textPart;
		textPart.setHeader(QNetworkRequest::ContentDispositionHeader,
						   QVariant("form-data; name=\"" + key + "\""));
		textPart.setBody(value.toUtf8());
		httpMultiPart->append(textPart);
	}

	auto *file = new QFile(file_path);
	file->open(QIODevice::ReadOnly);

	QHttpPart imagePart;
	imagePart.setHeader(QNetworkRequest::ContentDispositionHeader,
						QVariant("form-data; name=\"upload[file]\"; filename=\"" + QFileInfo(*file).fileName() + "\""));
	imagePart.setHeader(QNetworkRequest::ContentTypeHeader,
						QVariant("application/octet-stream"));
	imagePart.setBodyDevice(file);
	file->setParent(httpMultiPart); // we cannot delete the file object now, so delete it with the multiPart

	httpMultiPart->append(imagePart);

	QNetworkRequest request((QUrl(urlStr)));
	QNetworkReply *reply = mManager->post(request, httpMultiPart);

	httpMultiPart->setParent(reply); // delete the multiPart with the reply

	connect(reply, SIGNAL(finished()), this, SLOT(uploadDone()));
	connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT  (uploadProgress(qint64,qint64)));

	reply->request();
}

void FabUploadProgress::uploadProgress(qint64 bytesSent, qint64 bytesTotal) {
	if (bytesSent > 0) {
		Q_EMIT uploadProgressChanged(100 * bytesTotal / bytesSent);
	}
}

// Handle errors from NetworkManager
void FabUploadProgress::onError(QNetworkReply::NetworkError code)
{
	QMetaEnum metaEnum = QMetaEnum::fromType<QNetworkReply::NetworkError>();
	const char *errorString = metaEnum.valueToKey(code);

	auto *reply = qobject_cast<QNetworkReply*>(sender());
	FMessageBox::critical(this,
						  tr("Fritzing"),
						  tr("Could not connect to Fritzing fab.")
							  + "Error: " + reply->errorString() + " " + errorString);

	DebugDialog::debug(
		QString("Error connecting to fab %1 %2").arg(reply->errorString(), errorString));
	Q_EMIT closeUploadError();
}

// Handle http errors detected
void FabUploadProgress::httpError(QNetworkReply* reply)
{
	QString statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString();
	QByteArray responseData = reply->readAll();

	QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
	QString jsonErrors;
	if (!jsonDoc.isNull() && jsonDoc.isObject()) {
		QJsonObject jsonObj = jsonDoc.object();
		QJsonArray errorsArray = jsonObj["errors"].toArray();
		for (const QJsonValue &value : errorsArray) {
			jsonErrors += (jsonErrors.isEmpty() ? "" : ", ") + value.toString();
		}
	}

	QString errorMessage = reply->errorString() + " " + statusCode;
	if (!jsonErrors.isEmpty()) {
		errorMessage += QString(" - %1").arg(jsonErrors);
	}

	FMessageBox::critical(this, tr("Fritzing"), errorMessage);
	DebugDialog::debug(errorMessage);

	Q_EMIT closeUploadError();
}

// Handle errors reported by remote server
void FabUploadProgress::apiError(QString message)
{
	DebugDialog::debug(message);
	FMessageBox::critical(this, tr("Fritzing"), tr("Error processing the project. The factory says: %1").arg(message));
	Q_EMIT closeUploadError();
}


void FabUploadProgress::uploadDone() {
	auto *reply = qobject_cast<QNetworkReply*>(sender());
	DebugDialog::debug("Upload finished.");
	if (reply->error() == QNetworkReply::NoError) {
		auto d = reply->readAll();
		auto j = NetworkHelper::string_to_hash(d);
		QUrl callback_url(QUrl::fromUserInput(j["callback"].toString()));
		mRedirect_url = j["redirect"].toString();
		checkProcessingStatus(callback_url);
	} else {
		httpError(reply);
	}
	reply->deleteLater();
}

void FabUploadProgress::checkProcessingStatus(QUrl url)
{
	QNetworkRequest request(url);
	QNetworkReply *reply = mManager->get(request);
	connect(reply, SIGNAL(finished()), this, SLOT(updateProcessingStatus()));
//	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onError(QNetworkReply::NetworkError)));
}

void FabUploadProgress::updateProcessingStatus()
{
	auto *reply = qobject_cast<QNetworkReply*>(sender());

	if (reply->error() == QNetworkReply::NoError) {
		auto d = reply->readAll();
		auto j = NetworkHelper::string_to_hash(d);
		int progress = j["progress"].toInt();
		QString message(j["message"].toString());
		if (progress < 0) {
			apiError(message);
		} else {
			findChild<QLabel*>("message")->setText(message);
			Q_EMIT processProgressChanged(std::min(progress, 100));
			if(progress < 100) {
				QUrl url = reply->url();
				QTimer::singleShot(1000, this, [this, url](){
					checkProcessingStatus(url);
				});
			} else {
				if (j.contains("fab_project")) {
					mRedirect_url = j["fab_project"].toString();
				}
				if (mRedirect_url.isEmpty()) {
					QString error("Upload failed, no project url");
					FMessageBox::critical(this, tr("Fritzing"), error);
				} else {
					QSettings settings;
					QString service = j["service"].toString();
					if (service.isEmpty()) {
						service = settings.value("service", "").toString();
					} else {
						settings.setValue("service", service);
					}
					UploadPair data = {service, mRedirect_url};
					settings.beginGroup("sketches");
					settings.setValue(mFilepath, QVariant::fromValue(data));
					settings.endGroup();

					Q_EMIT processingDone();
				}
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
