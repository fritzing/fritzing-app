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
																		
$Revision: 6112 $:
$Author: cohen@irascible.com $:
$Date: 2012-06-28 00:18:10 +0200 (Do, 28. Jun 2012) $
		
********************************************************************/

// much code borrowed from Qt's rsslisting example


#include "updatedialog.h"	
#include "version.h"
#include "versionchecker.h"
#include "modfiledialog.h"
#include "../debugdialog.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QSettings>
#include <QApplication>
#include <QTimer>
#include <QCloseEvent>
#include <QMessageBox>


static const int s_maxProgress = 1000;
static QString sUpdatePartsMessage;
								
UpdateDialog::UpdateDialog(QWidget *parent) : QDialog(parent) 
{
	m_versionChecker = NULL;
    m_doQuit = false;
    m_doClose = true;

	this->setWindowTitle(QObject::tr("Check for updates"));
    if (sUpdatePartsMessage.isEmpty()) {
        sUpdatePartsMessage = tr("<p><b>There is a parts library update available!</b></p>"
                                   "<p>Would you like Fritzing to download and install the update now?<br/>"
                                   "See the <a href='https://github.com/fritzing/fritzing-parts/compare/%1...master'>list of changes here.</a></p>"
                                   "<p>Note: the update may take some minutes and you will have to restart Fritzing.<br/>"
                                   "You can also update later via the <i>Help &rarr; Check for Updates</i> menu.</p>");
    }

	QVBoxLayout * vLayout = new QVBoxLayout(this);

	m_feedbackLabel = new QLabel();
	m_feedbackLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
	m_feedbackLabel->setOpenExternalLinks(true);
    m_feedbackLabel->setTextFormat(Qt::RichText);

	vLayout->addWidget(m_feedbackLabel);

    m_progressBar = new QProgressBar();
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(s_maxProgress);

    vLayout->addWidget(m_progressBar);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Close"));
    m_buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Update parts"));

    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(stopClose()));
    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(updateParts()));

    vLayout->addWidget(m_buttonBox);

	this->setLayout(vLayout);
}

UpdateDialog::~UpdateDialog() {
	if (m_versionChecker) {
		delete m_versionChecker;
	}
}

bool UpdateDialog::setAvailableReleases(const QList<AvailableRelease *> & availableReleases)
{
	AvailableRelease * interimRelease = NULL;
	AvailableRelease * mainRelease = NULL;

	foreach (AvailableRelease * availableRelease, availableReleases) {
		if (availableRelease->interim && (interimRelease == NULL)) {
			interimRelease = availableRelease;
			continue;
		}
		if (!availableRelease->interim && (mainRelease == NULL)) {
			mainRelease = availableRelease;
			continue;
		}

		if (mainRelease != NULL && interimRelease != NULL) break;
	}

	if (mainRelease == NULL && interimRelease == NULL) {
		if (m_atUserRequest) {
            m_feedbackLabel->setText(tr("<p>No new versions found.</p>"));
		}
        return false;
	}

	QString style;
	QFile css(":/resources/styles/updatedialog.css");
	if (css.open(QIODevice::ReadOnly)) {
		style = css.readAll();
		css.close();
	}

	QString text = QString("<html><head><style type='text/css'>%1</style></head><body>").arg(style);
			
    QSettings settings;
    if (mainRelease) {
		text += genTable(tr("A new main release is available for downloading:"), mainRelease);
		settings.setValue("lastMainVersionChecked", mainRelease->versionString);
	}
	if (interimRelease) {
		text += genTable(tr("A new interim release is available for downloading:"), interimRelease);
		settings.setValue("lastInterimVersionChecked", interimRelease->versionString);
	}

	text += "</body></html>";

	m_feedbackLabel->setText(text);

    return true;
}


void UpdateDialog::setVersionChecker(VersionChecker * versionChecker) 
{
	if (m_versionChecker != NULL) {
		m_versionChecker->stop();
		delete m_versionChecker;
        m_versionChecker = NULL;
	}

    m_progressBar->setVisible(false);
    m_progressBar->setValue(0);
    // hide the update button, since it is only available under certain circumstances
    m_buttonBox->button(QDialogButtonBox::Ok)->setVisible(false);
    m_feedbackLabel->setText(tr("<p>Checking for new releases...</p>"));
    m_buttonBox->setEnabled(false);

	m_versionChecker = versionChecker;
	connect(m_versionChecker, SIGNAL(releasesAvailable()), this, SLOT(releasesAvailableSlot()));
	connect(m_versionChecker, SIGNAL(xmlError(QXmlStreamReader::Error)), this, SLOT(xmlErrorSlot(QXmlStreamReader::Error)));
    connect(m_versionChecker, SIGNAL(httpError(QNetworkReply::NetworkError)), this, SLOT(httpErrorSlot(QNetworkReply::NetworkError)));
	m_versionChecker->fetch();

}

void UpdateDialog::releasesAvailableSlot() {
    bool available = setAvailableReleases(m_versionChecker->availableReleases());
    if (available) {
        m_buttonBox->setEnabled(true);
        m_buttonBox->button(QDialogButtonBox::Ok)->setVisible(true);
        if (!this->isVisible()) {
            this->exec();
        }
        return;
    }

    bool canWrite = false;
    QDir repoDir(m_repoPath);
    QFile permissionTest(repoDir.absoluteFilePath("test.txt"));
    if (permissionTest.open(QFile::WriteOnly)) {
        qint64 count = permissionTest.write("a");
        permissionTest.close();
        permissionTest.remove();
        if (count > 0) {
            QFile db(repoDir.absoluteFilePath("parts.db"));
            if (db.open(QFile::Append)) {
                canWrite = true;
                db.close();
            }
        }
    }
    if (!canWrite) {
        m_feedbackLabel->setText(tr("<p>Fritzing is unable to check for--and update--new parts.<br/>"
                                    "If you want this functionality, please enable write permission on this folder:<br/> '%1'.</p>"
                                    ).arg(m_repoPath));
        m_buttonBox->setEnabled(true);
        m_buttonBox->button(QDialogButtonBox::Ok)->setVisible(false);
        if (!this->isVisible()) {
            // we are doing the parts check silently, so enable manual update by sending signal
            // otherwise manual update is enabled by closing the dialog
            emit enableAgainSignal(true);
        }
        return;
    }

    m_feedbackLabel->setText(tr("<p>Checking for new parts...</p>"));
    m_doClose = false;
    m_partsCheckerResult.reset();
    available = PartsChecker::newPartsAvailable(m_repoPath, m_shaFromDataBase, m_atUserRequest, m_remoteSha, m_partsCheckerResult);
    m_doClose = true;
    if (!available) {
        m_buttonBox->button(QDialogButtonBox::Ok)->setVisible(false);
        m_buttonBox->setEnabled(true);
        if (m_partsCheckerResult.errorMessage.isEmpty()) {
            m_feedbackLabel->setText(tr("<p>No new releases or new parts found</p>"));
        }
        else {
            m_feedbackLabel->setText(m_partsCheckerResult.errorMessage);
        }
        if (!this->isVisible()) {
            // we are doing the parts check silently, so enable manual update by sending signal
            // otherwise manual update is enabled by closing the dialog
            emit enableAgainSignal(true);
        }
        return;
    }

    switch (m_partsCheckerResult.partsCheckerError) {
        case PARTS_CHECKER_ERROR_NONE:
            m_feedbackLabel->setText(sUpdatePartsMessage.arg(m_shaFromDataBase));
            m_buttonBox->button(QDialogButtonBox::Ok)->setVisible(true);
            break;
        case PARTS_CHECKER_ERROR_REMOTE:
        case PARTS_CHECKER_ERROR_LOCAL_DAMAGE:
        case PARTS_CHECKER_ERROR_USED_GIT:
            m_feedbackLabel->setText(m_partsCheckerResult.errorMessage);
            break;
        case PARTS_CHECKER_ERROR_LOCAL_MODS:
            // pop up secondary dialog listing the files
            {
                ModFileDialog modFileDialog(this->parentWidget());
                modFileDialog.setText(m_partsCheckerResult.errorMessage);
                modFileDialog.addList(tr("New files:"), m_partsCheckerResult.untrackedFiles);
                modFileDialog.addList(tr("Modified Files:"), m_partsCheckerResult.changedFiles);
                connect(&modFileDialog, SIGNAL(cleanRepo(ModFileDialog *)),
                        this, SLOT(onCleanRepo(ModFileDialog *)));

                int result = modFileDialog.exec();
                if (result == QDialog::Rejected) {
                    if (this->isVisible()) {
                        this->hide();
                    }
                    // we are doing the parts check silently, so enable manual update by sending signal
                    // otherwise manual update is enabled by closing the dialog
                    emit enableAgainSignal(true);
                    return;
                }

                // if we got here, then cleaning the repo worked and we can proceed to the update
            }
            break;
    }

    m_buttonBox->setEnabled(true);
    if (!this->isVisible()) {
        this->exec();
    }
}

void UpdateDialog::onCleanRepo(ModFileDialog * modFileDialog) {
    if (!PartsChecker::cleanRepo(m_repoPath, m_partsCheckerResult)) {
        QMessageBox::warning(this->parentWidget(),
                             "Update Parts",
                             tr("Fritzing was unable to clean the files, so the update cannot proceed.<br/>"
                                "You may have to reinstall Fritzing."));

        // we are doing the parts check silently, so enable manual update by sending signal
        // otherwise manual update is enabled by closing the dialog
        emit enableAgainSignal(true);
        modFileDialog->done(QDialog::Rejected);
        return;
    }

    modFileDialog->done(QDialog::Accepted);
    m_feedbackLabel->setText(sUpdatePartsMessage.arg(m_shaFromDataBase));
    m_buttonBox->button(QDialogButtonBox::Ok)->setVisible(true);
}

void UpdateDialog::httpErrorSlot(QNetworkReply::NetworkError) {
    handleError();
}

void UpdateDialog::xmlErrorSlot(QXmlStreamReader::Error  errorCode) {
	Q_UNUSED(errorCode);
	handleError();
}

void UpdateDialog::handleError() 
{
	DebugDialog::debug("handle error");
    m_feedbackLabel->setText(tr("<p>Sorry, unable to retrieve update info</p>"));
    emit enableAgainSignal(true);
	DebugDialog::debug("handle error done");
}

void UpdateDialog::httpPartsErrorSlot(QString error) {
    handlePartsError(error);
}

void UpdateDialog::jsonPartsErrorSlot(QString error) {
    handlePartsError(error);
}

void UpdateDialog::handlePartsError(const QString & error) {

    DebugDialog::debug("handle error " + error);
    m_feedbackLabel->setText(tr("<p>Sorry, unable to retrieve parts update info</p>"));
    emit enableAgainSignal(true);
}

void UpdateDialog::setAtUserRequest(bool atUserRequest) 
{
	m_atUserRequest = atUserRequest;
}

void UpdateDialog::stopClose() {
	m_versionChecker->stop();
    this->close();
    emit enableAgainSignal(true);
}

void UpdateDialog::closeEvent(QCloseEvent * event) {
    if (!m_doClose) {
        event->ignore();
        return;
    }

    if (m_doQuit) {
        QTimer::singleShot(1, Qt::PreciseTimer, qApp, SLOT(quit()));
    }
    QDialog::closeEvent(event);
}

QString UpdateDialog::genTable(const QString & title, AvailableRelease * release) {
	return QString(
				"<p>"
				"<h3><b>%1</b></h3>"
				"<div style='margin-left:10px';margin-top:-5px'>"
				"<table>"
				"<tr>"
				"<td><a href=\"%4\"><b>Version %2</b></a></td>"
				"<td>(%3)<td/>"
				"</tr>"
				"</table>"
				"<table><tr><td>%5</td></tr></table>"
				"</div>"
				"</p>"
			)

			.arg(title)
			.arg(release->versionString)
			.arg(release->dateTime.toString(Qt::DefaultLocaleShortDate))
			.arg(release->link)
			.arg(release->summary.replace("changelog:", "", Qt::CaseInsensitive));
}

void UpdateDialog::setRepoPath(const QString & repoPath, const QString & shaFromDataBase) {
    m_repoPath = repoPath;
    m_shaFromDataBase = shaFromDataBase;
}

void UpdateDialog::updateParts() {
    m_doClose = false;
    m_buttonBox->setDisabled(true);
    m_progressBar->setValue(0);
    m_progressBar->setVisible(true);
    m_feedbackLabel->setText(tr("<p>Downloading new parts...</p>"));
    qApp->processEvents();    // Ensure dialog is drawn before doing I/O

    bool result = PartsChecker::updateParts(m_repoPath, m_remoteSha, this);
    m_buttonBox->button(QDialogButtonBox::Ok)->setVisible(false);
    if (!result) {
        m_doClose = true;
        m_progressBar->setVisible(false);
        m_buttonBox->setEnabled(true);
        m_feedbackLabel->setText(tr("<p>Sorry, unable to download new parts</p>"));
        return;
    }

    m_feedbackLabel->setText(tr("<p>Installing new parts. This may take a few minutes.<br/>Please do not interrupt the process, as your parts folder could be damaged.</p>"));
    m_progressBar->setValue(0);
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(0);
    emit installNewParts();
}

void UpdateDialog::updateProgress(double progress) {
    m_progressBar->setValue(progress * s_maxProgress);
    qApp->processEvents();
}

void UpdateDialog::installFinished(const QString & error) {
    m_progressBar->setVisible(false);
    m_buttonBox->setEnabled(true);
    if (error.isEmpty()) {
        m_feedbackLabel->setText(tr("<p>New parts successfully installed!</p>"
                                    "<p>Fritzing must be restarted, so the 'Close' button will close Fritzing.<br/>"
                                    "The new parts will be available when you run Fritzing again.</p>"));
    }
    else {
        m_feedbackLabel->setText(tr("<p>Sorry, unable to install new parts: %1<br/>"
                                    "Fritzing must nevertheless be restarted, "
                                    "so the 'Close' button will close Fritzing.</p>").arg(error));
    }

    m_doQuit = m_doClose = true;
}
