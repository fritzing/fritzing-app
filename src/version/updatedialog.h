/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2016 Fritzing

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
#include <QXmlStreamReader>
#include <QNetworkReply>
#include <QDialogButtonBox>
#include <QProgressBar>
#include <QListWidget>

#include "partschecker.h"

class UpdateDialog : public QDialog, public PartsCheckerUpdateInterface {
	Q_OBJECT

public:
	UpdateDialog(QWidget *parent = 0);
	~UpdateDialog();

	void setVersionChecker(class VersionChecker *);
	void setAtUserRequest(bool);
    void setRepoPath(const QString & repoPath, const QString & shaFromDataBase);
    void updateProgress(double progress);
    void installFinished(const QString & error);

signals:
    void enableAgainSignal(bool enable);
    void installNewParts();

protected slots:
	void releasesAvailableSlot();
	void xmlErrorSlot(QXmlStreamReader::Error errorCode);
    void httpErrorSlot(QNetworkReply::NetworkError);
    void jsonPartsErrorSlot(QString error);
    void httpPartsErrorSlot(QString error);
	void stopClose();
    void updateParts();
    void onCleanRepo(class ModFileDialog *modFileDialog);

protected:
    bool setAvailableReleases(const QList<struct AvailableRelease *> & availableReleases);
    void handleError();
    void handlePartsError(const QString & error);
    QString genTable(const QString & title, struct AvailableRelease *);
    void closeEvent(QCloseEvent *);

protected:
	class VersionChecker * m_versionChecker;
	bool m_atUserRequest;
    QString m_repoPath;
    QString m_shaFromDataBase;
    QString m_remoteSha;
    QLabel * m_feedbackLabel;
    QDialogButtonBox * m_buttonBox;
    QProgressBar * m_progressBar;
    PartsCheckerResult m_partsCheckerResult;
    bool m_doQuit;
    bool m_doClose;
};


#endif
