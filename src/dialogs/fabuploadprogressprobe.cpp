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

#include "fabuploadprogressprobe.h"
#include "dialogs/fabuploadprogress.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QEventLoop>
#include <QTimer>

FabUploadProgressProbe::FabUploadProgressProbe(FabUploadProgress *fabUploadProgress) : FProbe("FabUploadProgressProbe"), m_fabUploadProgress(fabUploadProgress) {}

QVariant FabUploadProgressProbe::read() {
	QEventLoop loop;
	bool timeout = false;
	bool error = false;
	QObject::connect(m_fabUploadProgress, &FabUploadProgress::processingDone,
					&loop, &QEventLoop::quit);
	QObject::connect(m_fabUploadProgress, &FabUploadProgress::closeUploadError,
					&loop, [&loop, &error](){
		error = true;
		loop.quit();
	});

	constexpr int timeoutSeconds = 300;
	QTimer::singleShot(timeoutSeconds * 1000, &loop, [&loop, &timeout](){
		timeout = true;
		loop.quit();
	});
	loop.exec();
	if (error) return QVariant("error");
	return QVariant(timeout ? "timeout" : "success");
}
