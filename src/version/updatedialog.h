/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2012 Fachhochschule Potsdam - http://fh-potsdam.de

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

********************************************************************

$Revision: 6904 $:
$Author: irascibl@gmail.com $:
$Date: 2013-02-26 16:26:03 +0100 (Di, 26. Feb 2013) $

********************************************************************/

#ifndef UPDATEDIALOG_H
#define UPDATEDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QHttp>
#include <QXmlStreamReader>

class UpdateDialog : public QDialog {
	Q_OBJECT

public:
	UpdateDialog(QWidget *parent = 0);
	~UpdateDialog();

	void setVersionChecker(class VersionChecker *);
	void setAtUserRequest(bool);

signals:
	void enableAgainSignal(bool enable);

protected slots:
	void releasesAvailableSlot();
	void xmlErrorSlot(QXmlStreamReader::Error errorCode);
	void httpErrorSlot(QHttp::Error statusCode);
	void stopClose();

protected:
	void setAvailableReleases(const QList<struct AvailableRelease *> & availableReleases); 
	void handleError();
	QString genTable(const QString & title, struct AvailableRelease *);

protected:
	class VersionChecker * m_versionChecker;
	bool m_atUserRequest;
	QLabel * m_feedbackLabel;

};

#endif
