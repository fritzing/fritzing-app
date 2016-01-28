/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2014 Fachhochschule Potsdam - http://fh-potsdam.de

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
#include "../debugdialog.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QSettings>
#include <QApplication>
#include <QTimer>
#include <QCloseEvent>


static const int s_maxProgress = 1000;
								
UpdateDialog::UpdateDialog(QWidget *parent) : QDialog(parent) 
{
	m_versionChecker = NULL;
    m_doQuit = false;
    m_doClose = true;

	this->setWindowTitle(QObject::tr("Check for updates"));

	QVBoxLayout * vLayout = new QVBoxLayout(this);

	m_feedbackLabel = new QLabel();
	m_feedbackLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
	m_feedbackLabel->setOpenExternalLinks(true);

	vLayout->addWidget(m_feedbackLabel);

    m_progressBar = new QProgressBar();
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(s_maxProgress);

    vLayout->addWidget(m_progressBar);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Ignore"));
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
			m_feedbackLabel->setText(tr("No new versions found."));
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
    m_buttonBox->button(QDialogButtonBox::Ok)->setVisible(false);
    m_feedbackLabel->setText(tr("Checking for a new release..."));
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
        m_feedbackLabel->setText(tr("Fritzing is unable to check for--and update--new parts. "
                                    "If you want this functionality, please enable write permission on the folder '%1'."
                                    ).arg(m_repoPath));
        m_buttonBox->setEnabled(true);
        return;
    }

    m_feedbackLabel->setText(tr("Checking for new parts..."));
    m_doClose = false;
    available = PartsChecker::newPartsAvailable(m_repoPath, m_shaFromDataBase, m_atUserRequest, m_remoteSha);
    m_doClose = true;
    if (!available) {
        m_feedbackLabel->setText(tr("No new releases or new parts found"));
        m_buttonBox->setEnabled(true);
        return;
    }

    m_feedbackLabel->setTextFormat(Qt::RichText);
    m_feedbackLabel->setText(tr("<p><b>There is a parts library update available!</b></p>"
                               "<p>See the <a href='https://github.com/fritzing/fritzing-parts/compare/%1...master'>list of changes.</a> "
                               "Would you like Fritzing to download and install them?</p>"
                               "<p>Note: this may take a moment "
                               "and you will have to restart Fritzing.</p>").arg(m_shaFromDataBase));
    m_buttonBox->button(QDialogButtonBox::Ok)->setVisible(true);
    m_buttonBox->setEnabled(true);
    if (!this->isVisible()) {
        this->exec();
    }
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
    m_feedbackLabel->setText(tr("Sorry, unable to retrieve update info"));
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
    m_feedbackLabel->setText(tr("Sorry, unable to retrieve parts update info"));
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
    m_feedbackLabel->setText(tr("Downloading new parts..."));

    bool result = PartsChecker::updateParts(m_repoPath, m_remoteSha, this);
    m_buttonBox->button(QDialogButtonBox::Ok)->setVisible(false);
    if (!result) {
        m_doClose = true;
        m_progressBar->setVisible(false);
        m_buttonBox->setEnabled(true);
        m_feedbackLabel->setText(tr("Sorry, unable to download new parts"));
        return;
    }

    m_feedbackLabel->setText(tr("Installing new parts. This may take a few minutes..."));
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
        m_feedbackLabel->setText(tr("New parts successfully installed.\n"
                                    "Fritzing must be restarted, so the 'Close' button will close Fritzing.\n"
                                    "The new parts will be available when you run Fritzing again."));
    }
    else {
        m_feedbackLabel->setText(tr("Sorry, unable to install new parts: %1\n"
                                    "Fritzing must nevertheless be restarted, "
                                    "so the 'Close' button will close Fritzing.\n").arg(error));
    }

    m_doQuit = m_doClose = true;
}
