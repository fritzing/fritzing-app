/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2023 Fritzing GmbH

Fritzing is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Fritzing is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************/

#ifndef FABUPLOADPROGRESS_H
#define FABUPLOADPROGRESS_H

#include <QWidget>
#include <QNetworkReply>

class FabUploadProgressProbe;

class FabUploadProgress : public QWidget
{
	Q_OBJECT
public:
	explicit FabUploadProgress(QWidget *parent = nullptr);
	~FabUploadProgress(){};
	void init(QNetworkAccessManager *manager, QString filename, double width, double height, int boardCount);

public Q_SLOTS:
	void doUpload();
	void openInBrowser();

private Q_SLOTS:
	void onRequestUploadFinished();
	void onError(QNetworkReply::NetworkError code);
	void uploadDone();
	void uploadProgress(qint64 bytesSent, qint64 bytesTotal);
	void updateProcessingStatus();

Q_SIGNALS:
	void uploadProgressChanged(int);
	void processProgressChanged(int);
	void processingDone();
	void closeUploadError();

private:
	friend class FabUploadProgressProbe;
	FabUploadProgressProbe *mFabUploadProgressProbe;
	QNetworkAccessManager *mManager;
	QString mFilepath;
	QString mRedirect_url;
	double mWidth;
	double mHeight;
	int mBoardCount;

	void uploadMultipart(const QUrl &url, const QString &file_path);
	void checkProcessingStatus(QUrl url);
	void httpError(QNetworkReply *reply);
	void apiError(QString message);
};


#endif // FABUPLOADPROGRESS_H
