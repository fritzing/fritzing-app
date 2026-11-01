/*******************************************************************

Part of the Fritzing project - https://fritzing.org
Copyright (c) 2026 Fritzing GmbH

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

#ifndef PROXYURL_H
#define PROXYURL_H

#include <QUrl>

#ifdef Q_OS_WASM
#include <emscripten/val.h>
#include <string>
#endif

// Returns a URL the application can fetch. That is the URL itself, except in the
// WebAssembly build: the browser blocks requests to other sites, so the web server
// serving the app forwards https://<host>/<path> from /proxy/<host>/<path>
// (see docker/wasm/templates/default.conf.template for the allowed hosts).
inline QUrl browserReachableUrl(const QUrl & url)
{
#ifdef Q_OS_WASM
	QString origin = QString::fromStdString(emscripten::val::global("location")["origin"].as<std::string>());
	QUrl proxied(origin + "/proxy/" + url.host() + url.path(QUrl::FullyEncoded));
	if (url.hasQuery()) {
		proxied.setQuery(url.query(QUrl::FullyEncoded));
	}
	return proxied;
#else
	return url;
#endif
}

// The reverse of browserReachableUrl(): the URL on the original site, for example to
// resolve links in a page fetched through the proxy.
inline QUrl originalUrl(const QUrl & url)
{
#ifdef Q_OS_WASM
	static const QString prefix("/proxy/");
	QString path = url.path(QUrl::FullyEncoded);
	if (path.startsWith(prefix)) {
		int hostEnd = path.indexOf('/', prefix.length());
		QString host = path.mid(prefix.length(), hostEnd < 0 ? -1 : hostEnd - prefix.length());
		QUrl original("https://" + host + (hostEnd < 0 ? QString("/") : path.mid(hostEnd)));
		if (url.hasQuery()) {
			original.setQuery(url.query(QUrl::FullyEncoded));
		}
		return original;
	}
#endif
	return url;
}

#endif
