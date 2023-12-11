#include "networkhelper.h"

#include <QJsonDocument>
#include <QTextStream>

#include "debugdialog.h"

NetworkHelper::NetworkHelper()
{}

const QString NetworkHelper::debugRequest(QNetworkRequest request)
{
	QString ret(request.url().toString()); //output the url
	const QList<QByteArray>& rawHeaderList(request.rawHeaderList());
	Q_FOREACH (QByteArray rawHeader, rawHeaderList) { //traverse and output the header
		ret += rawHeader + ":" + request.rawHeader(rawHeader);
	}
	return ret;
}

QJsonObject NetworkHelper::string_to_hash(QString str)
{
	QJsonObject obj;
	QJsonDocument doc = QJsonDocument::fromJson(str.toUtf8());
	// check validity of the document
	if(!doc.isNull()) {
		if(doc.isObject()) {
			obj = doc.object();
		} else {
			DebugDialog::debug("Document is not an object");
		}
	} else {
			DebugDialog::debug("Invalid JSON.");
	}

	return obj;
}
