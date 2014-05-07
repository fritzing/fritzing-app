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
#include <QDialogButtonBox>
#include <QPushButton>
#include <QSettings>
								
UpdateDialog::UpdateDialog(QWidget *parent) : QDialog(parent) 
{
#ifndef NO_VERSION_CHECK
	m_versionChecker = NULL;

	this->setWindowTitle(QObject::tr("Check for updates"));

	QVBoxLayout * vLayout = new QVBoxLayout(this);

	m_feedbackLabel = new QLabel();
	m_feedbackLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
	m_feedbackLabel->setOpenExternalLinks(true);

	vLayout->addWidget(m_feedbackLabel);

    QDialogButtonBox * buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
	buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Close"));

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(stopClose()));

	vLayout->addWidget(buttonBox);

	this->setLayout(vLayout);
#endif
}

UpdateDialog::~UpdateDialog() {
#ifndef NO_VERSION_CHECK
	if (m_versionChecker) {
		delete m_versionChecker;
	}
#endif
}

#ifndef NO_VERSION_CHECK

void UpdateDialog::setAvailableReleases(const QList<AvailableRelease *> & availableReleases) 
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
		return;
	}

	QSettings settings;
	QString style;
	QFile css(":/resources/styles/updatedialog.css");
	if (css.open(QIODevice::ReadOnly)) {
		style = css.readAll();
		css.close();
	}

	QString text = QString("<html><head><style type='text/css'>%1</style></head><body>").arg(style);
			
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

	this->show();

}


void UpdateDialog::setVersionChecker(VersionChecker * versionChecker) 
{
	if (m_versionChecker != NULL) {
		m_versionChecker->stop();
		delete m_versionChecker;
	}

	m_feedbackLabel->setText(tr("Checking..."));

	m_versionChecker = versionChecker;
	connect(m_versionChecker, SIGNAL(releasesAvailable()), this, SLOT(releasesAvailableSlot()));
	connect(m_versionChecker, SIGNAL(xmlError(QXmlStreamReader::Error)), this, SLOT(xmlErrorSlot(QXmlStreamReader::Error)));
	connect(m_versionChecker, SIGNAL(httpError(QHttp::Error)), this, SLOT(httpErrorSlot(QHttp::Error)));
	m_versionChecker->fetch();

}

void UpdateDialog::releasesAvailableSlot() {
	setAvailableReleases(m_versionChecker->availableReleases());
	emit enableAgainSignal(true);
}

void UpdateDialog::httpErrorSlot(QHttp::Error  statusCode) {
	Q_UNUSED(statusCode);
	handleError();
}

void UpdateDialog::xmlErrorSlot(QXmlStreamReader::Error  errorCode) {
	Q_UNUSED(errorCode);
	handleError();
}

void UpdateDialog::handleError() 
{
	DebugDialog::debug("handle error");
	if (m_atUserRequest) {
		m_feedbackLabel->setText(tr("Sorry, unable to retrieve update info")); 
	}
	else {
		// automatic update check: don't bother the user
	}
	emit enableAgainSignal(true);
	DebugDialog::debug("handle error done");
}

void UpdateDialog::setAtUserRequest(bool atUserRequest) 
{
	m_atUserRequest = atUserRequest;
}

void UpdateDialog::stopClose() {
	m_versionChecker->stop();
	this->close();
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

#endif
