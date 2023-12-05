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

#ifndef FABUPLOADPROGRESSPROBE_H
#define FABUPLOADPROGRESSPROBE_H

#include "testing/FProbe.h"
#include "dialogs/fabuploadprogress.h"

#include <QObject>

class FabUploadProgressProbe : public QObject, public FProbe
{
	Q_OBJECT
public:
	FabUploadProgressProbe(FabUploadProgress *fabUploadProgress);
	~FabUploadProgressProbe() {}
	QVariant read() override;
	void write(QVariant) override {}

private:
	FabUploadProgress *m_fabUploadProgress;
	QString m_reason;
};

#endif // FABUPLOADPROGRESSPROBE_H
