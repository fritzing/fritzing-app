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

#include "lockmanager.h"
#include "folderutils.h"
#include "textutils.h"

#include <QTimer>
#include <QPointer>
#include <QMutex>
#include <QMutexLocker>
#include <QDateTime>

const QString LockManager::LockedFileName = "___lockfile___.txt";
const long LockManager::FastTime =  2000;
const long LockManager::SlowTime = 240000;

static LockManager TheLockManager;
static QHash<long, QPointer<QTimer> > TheTimers;
static QMultiHash<long, LockedFile *> TheLockedFiles; 
static QMutex TheMutex;

LockedFile::LockedFile(const QString & filename, long freq) {
	file.setFileName(filename);
	frequency = freq;
}

bool LockedFile::touch() {
	if (file.open(QFile::WriteOnly)) {
		file.write("a");
		file.close();
		return true;
	}

	return false;
}

/////////////////////////////////////////////////

LockManager::LockManager() : QObject()
{
}

LockManager::~LockManager()
{
	foreach (QTimer * timer, TheTimers) {
		if (timer) timer->stop();
	}
	TheTimers.clear();
}

void LockManager::cleanup() {
    foreach (QTimer * timer, TheTimers) {
		if (timer) {
			timer->stop();
			delete timer;
		}
    }
    TheTimers.clear();
}

void LockManager::touchFiles() {
	QTimer * timer = qobject_cast<QTimer *>(sender());
	if (timer == NULL) return;

	QMutexLocker locker(&TheMutex);
	foreach (LockedFile * lockedFile, TheLockedFiles.values(timer->interval())) {
		lockedFile->touch();
	}
}

void LockManager::initLockedFiles(const QString & prefix, QString & folder, QHash<QString, LockedFile *> & lockedFiles, long touchFrequency) {
	// first create our own unique folder and lock it
    QDir backupDir(FolderUtils::getTopLevelUserDataStorePath());
    backupDir.cd(prefix);
	QString lockedSubfolder = TextUtils::getRandText();
	backupDir.mkdir(lockedSubfolder);
	folder = backupDir.absoluteFilePath(lockedSubfolder);
	if (touchFrequency > 0) {
		LockedFile * lockedFile = makeLockedFile(folder + "/" + LockedFileName, touchFrequency);
		lockedFiles.insert(lockedSubfolder, lockedFile);
	}
}

LockedFile * LockManager::makeLockedFile(const QString & path, long touchFrequency) {
	LockedFile * lockedFile = new LockedFile(path, touchFrequency);
	lockedFile->touch();
	TheMutex.lock();
	TheLockedFiles.insert(touchFrequency, lockedFile);		
	TheMutex.unlock();
	QTimer * timer = TheTimers.value(touchFrequency, NULL);
	if (timer == NULL) {
		timer = new QTimer();
		timer->setInterval(touchFrequency);
		timer->setSingleShot(false);
		QObject::connect(timer, SIGNAL(timeout()), &TheLockManager, SLOT(touchFiles()));
		timer->start();
        TheTimers.insert(touchFrequency, timer);
	}
	return lockedFile;
}


void LockManager::releaseLockedFiles(const QString & folder, QHash<QString, LockedFile *> & lockedFiles) 
{
	// remove backup files; this is a clean exit
	releaseLockedFiles(folder, lockedFiles, true);
}

void LockManager::releaseLockedFiles(const QString & folder, QHash<QString, LockedFile *> & lockedFiles, bool remove) 
{
	QDir backupDir(folder);
	backupDir.cdUp();
	foreach (QString sub, lockedFiles.keys()) {
        LockedFile * lockedFile = lockedFiles.value(sub);
		TheMutex.lock();
		TheLockedFiles.remove(lockedFile->frequency, lockedFile);
		TheMutex.unlock();
        if (remove) {
			FolderUtils::rmdir(backupDir.absoluteFilePath(sub));
		}
		delete lockedFile;
	}
	lockedFiles.clear();
}

void LockManager::checkLockedFiles(const QString & prefix, QFileInfoList & backupList, QHash<QString, LockedFile *> & lockedFiles, bool recurse, long touchFrequency)
{
    QDir backupDir(FolderUtils::getTopLevelUserDataStorePath());
    backupDir.cd(prefix);
	QFileInfoList dirList = backupDir.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::NoSymLinks);
	foreach (QFileInfo dirInfo, dirList) {
		QDir dir(dirInfo.filePath());
		QStringList filters;
		//DebugDialog::debug(QString("looking in backup dir %1").arg(dir.absolutePath()));
		QFileInfoList fileInfoList = dir.entryInfoList(filters, QDir::Files | QDir::Hidden | QDir::NoSymLinks);
		bool gotRecurse = false;
		if (recurse && fileInfoList.isEmpty()) {
			gotRecurse = checkLockedFilesAux(dir, filters);
		}

		if (fileInfoList.isEmpty() && !gotRecurse) {
			// could mean this backup folder is just being created by another process
			// could also mean it's leftover crap.
			// check the date and only delete if it's old
			
			QDateTime lastModified = dirInfo.lastModified();
			if (lastModified < QDateTime::currentDateTime().addMSecs(-2000 - touchFrequency)) {
				FolderUtils::rmdir(dirInfo.filePath());
			}
			
			continue;
		}
		
		QFileInfo lockInfo(dir.absoluteFilePath(LockedFileName));
		if (lockInfo.exists()) {
			QDateTime lastModified = lockInfo.lastModified();
			if (lastModified >= QDateTime::currentDateTime().addMSecs(-2000 - touchFrequency)) {
				// somebody else owns the file
				continue;
			}
		}

		// we own the file
		QString folder;
		LockedFile * lockedFile = makeLockedFile(dir.absoluteFilePath(LockedFileName), touchFrequency);
		lockedFiles.insert(dirInfo.fileName(), lockedFile);		
		foreach (QFileInfo fileInfo, fileInfoList) {
			backupList << fileInfo;
		}
	}
}

bool LockManager::checkLockedFilesAux(const QDir & parent, QStringList & filters)
{
	QFileInfoList dirList = parent.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::NoSymLinks);
	foreach (QFileInfo dirInfo, dirList) {
		QDir dir(dirInfo.filePath());
		//DebugDialog::debug(QString("looking in backup dir %1").arg(dir.absolutePath()));
		QFileInfoList fileInfoList = dir.entryInfoList(filters, QDir::Files | QDir::Hidden | QDir::NoSymLinks);
		if (!fileInfoList.isEmpty()) return true;

		if (checkLockedFilesAux(dir, filters)) return true;
	}

	return false;
}
