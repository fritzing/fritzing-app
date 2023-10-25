#ifndef NETWORKHELPER_H
#define NETWORKHELPER_H

#include <QNetworkRequest>
#include <QJsonObject>

class NetworkHelper
{
public:
	NetworkHelper();

	static const QString debugRequest(QNetworkRequest request);
	static QJsonObject string_to_hash(QString str);
};

#endif // NETWORKHELPER_H
