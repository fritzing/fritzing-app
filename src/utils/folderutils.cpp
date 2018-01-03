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

$Revision: 7004 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-29 13:10:59 +0200 (Mo, 29. Apr 2013) $

********************************************************************/

#include "folderutils.h"
#include "lockmanager.h"
#include "textutils.h"
#include "fmessagebox.h"
#include <QDesktopServices>
#include <QCoreApplication>
#include <QSettings>
#include <QTextStream>
#include <QUuid>
#include <QCryptographicHash>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>

#include "../debugdialog.h"
#ifdef QUAZIP_INSTALLED
    #include <quazip/quazip.h>
    #include <quazip/quazipfile.h>
#else
    #include "../lib/quazip/quazip.h"
    #include "../lib/quazip/quazipfile.h"
#endif
#include "../lib/qtsysteminfo/QtSystemInfo.h"


FolderUtils* FolderUtils::singleton = NULL;
QString FolderUtils::m_openSaveFolder = "";

FolderUtils::FolderUtils() {
	m_openSaveFolder = ___emptyString___;
    m_userFolders
        << "partfactory"
        << "backup"
        << "fzz";
    m_documentFolders
        << "bins"
        << "parts/user" << "parts/contrib"
        << "parts/svg/user/icon" << "parts/svg/user/breadboard" << "parts/svg/user/schematic" << "parts/svg/user/pcb"
        << "parts/svg/contrib/icon" << "parts/svg/contrib/breadboard" << "parts/svg/contrib/schematic" << "parts/svg/contrib/pcb";

}

FolderUtils::~FolderUtils() {
}

// finds a subfolder of the application directory searching backward up the tree
QDir  FolderUtils::getApplicationSubFolder(QString search) {
	if (singleton == NULL) {
		singleton = new FolderUtils();
	}

	QString path = singleton->applicationDirPath();
    path += "/" + search;
	//DebugDialog::debug(QString("path %1").arg(path) );
    QDir dir(path);
    while (!dir.exists()) {
    	// if we're running from the debug or release folder, go up one to find things
        dir.cdUp();
        dir.cdUp();
        if (dir.isRoot()) return QDir();   // didn't find the search folder

        dir.setPath(dir.absolutePath() + "/" + search);
   	}

   	return dir;
}

QString FolderUtils::getApplicationSubFolderPath(QString search) {
	if (singleton == NULL) {
		singleton = new FolderUtils();
	}

    QDir dir = getApplicationSubFolder(search);
    return dir.path();
}

QString FolderUtils::getAppPartsSubFolderPath(QString search) {
    if (singleton == NULL) {
        singleton = new FolderUtils();
    }

    QDir dir = getAppPartsSubFolder(search);
    return dir.path();
}

QDir FolderUtils::getAppPartsSubFolder(QString search) {
    if (singleton == NULL) {
        singleton = new FolderUtils();
    }

    return singleton->getAppPartsSubFolder2(search);
}

QDir FolderUtils::getAppPartsSubFolder2(QString search) {
    if (m_partsPath.isEmpty()) {
        QDir dir = getApplicationSubFolder("fritzing-parts");
        if (dir.exists()) {
            m_partsPath = dir.absolutePath();
        }
        else {
            QDir dir = getApplicationSubFolder("parts");
            if (dir.exists()) {
                m_partsPath = dir.absolutePath();
            }
        }
    }


    QString path = search.isEmpty() ? m_partsPath : m_partsPath + "/" + search;
    //DebugDialog::debug(QString("path %1").arg(path) );
    QDir dir(path);

    return dir;
}

QString FolderUtils::getTopLevelUserDataStorePath() {
    QString path = QSettings(QSettings::IniFormat,QSettings::UserScope,"Fritzing","Fritzing").fileName();
    return QFileInfo(path).dir().absolutePath();
}

QString FolderUtils::getTopLevelDocumentsPath() {
    // must add a fritzing subfolder
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    return dir.absoluteFilePath("Fritzing");
}

QString FolderUtils::getUserBinsPath() {
    QDir dir(getTopLevelDocumentsPath() + "/bins");
    return QFileInfo(dir, "").absoluteFilePath();
}

QString FolderUtils::getUserPartsPath() {
    QDir dir(getTopLevelDocumentsPath());
    return dir.absoluteFilePath("parts");
}

bool FolderUtils::createFolderAndCdIntoIt(QDir &dir, QString newFolder) {
	if(!dir.mkdir(newFolder)) return false;
	if(!dir.cd(newFolder)) return false;

	return true;
}

bool FolderUtils::setApplicationPath(const QString & path)
{
    if (singleton == NULL) {
        singleton = new FolderUtils();
    }

    return singleton->setApplicationPath2(path);
}

bool FolderUtils::setAppPartsPath(const QString & path)
{
    if (singleton == NULL) {
        singleton = new FolderUtils();
    }

    return singleton->setPartsPath2(path);
}

void FolderUtils::cleanup() {
	if (singleton) {
		delete singleton;
		singleton = NULL;
	}
}

const QString FolderUtils::getLibraryPath() 
{
	if (singleton == NULL) {
		singleton = new FolderUtils();
	}

	return singleton->libraryPath();
}


const QString FolderUtils::libraryPath() 
{
#ifdef Q_OS_MAC
	// mac plugins are always in the bundle
	return QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../lib");
#endif

	return QDir::cleanPath(applicationDirPath() + "/lib");		
}

const QString FolderUtils::applicationDirPath() {
	if (m_appPath.isEmpty()) {
#ifdef Q_OS_WIN
        m_appPath = QCoreApplication::applicationDirPath();
#else
		// look in standard Fritzing location (applicationDirPath and parent folders) then in standard linux locations
		QStringList candidates;
		candidates.append(QCoreApplication::applicationDirPath());
		QDir dir(QCoreApplication::applicationDirPath());
		if (dir.cdUp()) {
			candidates.append(dir.absolutePath());
			if (dir.cdUp()) {
				candidates.append(dir.absolutePath());
				if (dir.cdUp()) {
					candidates.append(dir.absolutePath());
				}
			}
		}
		
#ifdef PKGDATADIR
		candidates.append(QLatin1String(PKGDATADIR));
#else
		candidates.append("/usr/share/fritzing");
		candidates.append("/usr/local/share/fritzing");
#endif
		candidates.append(QDir::homePath() + "/.local/share/fritzing");
		foreach (QString candidate, candidates) {
            //DebugDialog::debug(QString("candidate:%1").arg(candidate));
			QDir dir(candidate);
            if (!dir.exists("translations")) continue;

            if (dir.exists("help")) {
				m_appPath = candidate;
				return m_appPath;
			}

		}

		m_appPath = QCoreApplication::applicationDirPath();
        DebugDialog::debug("data folders not found");

#endif
	}

	return m_appPath;
}

bool FolderUtils::setApplicationPath2(const QString & path)
{
    QDir dir(path);
    if (!dir.exists()) return false;

    m_appPath = path;
    return true;
}

bool FolderUtils::setPartsPath2(const QString & path)
{
    QDir dir(path);
    if (!dir.exists()) return false;

    m_partsPath = path;
    return true;
}

void FolderUtils::setOpenSaveFolder(const QString& path) {
	setOpenSaveFolderAux(path);
	QSettings settings;
	settings.setValue("openSaveFolder", m_openSaveFolder);
}

void FolderUtils::setOpenSaveFolderAux(const QString& path) 
{
	QFileInfo fileInfo(path);
	if(fileInfo.isDir()) {
		m_openSaveFolder = path;
	} else {
		m_openSaveFolder = fileInfo.path().remove(fileInfo.fileName());
	}
}


const QString FolderUtils::openSaveFolder() {
	if(m_openSaveFolder.isEmpty()) {
		QSettings settings;
		QString tempFolder = settings.value("openSaveFolder").toString();
		if (!tempFolder.isEmpty()) {
			QFileInfo fileInfo(tempFolder);
			if (fileInfo.exists()) {
				m_openSaveFolder = tempFolder;
				return m_openSaveFolder;
			}
			else {
				settings.remove("openSaveFolder");
			}
		}

        return getTopLevelDocumentsPath();

	} else {
		return m_openSaveFolder;
	}
}


QString FolderUtils::getOpenFileName( QWidget * parent, const QString & caption, const QString & dir, const QString & filter, QString * selectedFilter, QFileDialog::Options options )
{
	QString result = QFileDialog::getOpenFileName(parent, caption, dir, filter, selectedFilter, options);
	if (!result.isNull()) {
		setOpenSaveFolder(result);
	}
	return result;
}

QStringList FolderUtils::getOpenFileNames( QWidget * parent, const QString & caption, const QString & dir, const QString & filter, QString * selectedFilter, QFileDialog::Options options )
{
	QStringList result = QFileDialog::getOpenFileNames(parent, caption, dir, filter, selectedFilter, options);
	if (result.count() > 0) {
		setOpenSaveFolder(result.at(0));
	}
	return result;
}

QString FolderUtils::getSaveFileName( QWidget * parent, const QString & caption, const QString & dir, const QString & filter, QString * selectedFilter, QFileDialog::Options options )
{
	//DebugDialog::debug(QString("getopenfilename %1 %2 %3 %4").arg(caption).arg(dir).arg(filter).arg(*selectedFilter));
	QString result = QFileDialog::getSaveFileName(parent, caption, dir, filter, selectedFilter, options);
	if (!result.isNull()) {
		setOpenSaveFolder(result);
	}
	return result;
}

bool FolderUtils::isEmptyFileName(const QString &fileName, const QString &untitledFileName) {
	return (fileName.isEmpty() || fileName.isNull() || fileName.startsWith(untitledFileName));
}

void FolderUtils::replicateDir(QDir srcDir, QDir targDir) {
    // copy all files from srcDir source to targDir
	QStringList files = srcDir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
	for(int i=0; i < files.size(); i++) {
		QFile tempFile(srcDir.path() + "/" +files.at(i));
		DebugDialog::debug(QObject::tr("Copying file %1").arg(tempFile.fileName()));
		QFileInfo fi(files.at(i));
		QString newFilePath = targDir.path() + "/" + fi.fileName();
		if(QFileInfo(tempFile.fileName()).isDir()) {
			QDir newTargDir = QDir(newFilePath);
			newTargDir.mkpath(newTargDir.absolutePath());
			newTargDir.cd(files.at(i));
			replicateDir(QDir(tempFile.fileName()),newTargDir);
		} else {
			if(!tempFile.copy(newFilePath)) {
				DebugDialog::debug(QObject::tr("File %1 already exists: it won't be overwritten").arg(newFilePath));
			}
		}
	}
}

// NOTE: This function cannot remove directories that have non-empty name filters set on it.
void FolderUtils::rmdir(const QString &dirPath) {
	QDir dir = QDir(dirPath);
	rmdir(dir);
}

void FolderUtils::rmdir(QDir & dir) {
	//DebugDialog::debug(QString("removing folder: %1").arg(dir.path()));

	QStringList files = dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
	for(int i=0; i < files.size(); i++) {
		QFile tempFile(dir.path() + "/" +files.at(i));
		//DebugDialog::debug(QString("removing from original folder: %1").arg(tempFile.fileName()));
		if(QFileInfo(tempFile.fileName()).isDir()) {
			QDir dir = QDir(tempFile.fileName());
			rmdir(dir);
		} else {
			tempFile.remove(tempFile.fileName());
		}
	}
	dir.rmdir(dir.path());
}


bool FolderUtils::createFZAndSaveTo(const QDir &dirToCompress, const QString &filepath, const QStringList & skipSuffixes) {
	DebugDialog::debug("saveASfz "+dirToCompress.path()+" into "+filepath);

	QFileInfoList files=dirToCompress.entryInfoList();
	QFile inFile;
	
	char c;

	QString currFolderBU = QDir::currentPath();
	QDir::setCurrent(dirToCompress.path());
	foreach(QFileInfo file, files) {
		if(!file.isFile()||file.fileName()==filepath) continue;
		if (file.fileName().contains(LockManager::LockedFileName)) continue;

        bool skip = false;
        foreach (QString suffix, skipSuffixes) {
            if (file.fileName().endsWith(suffix)) {
                skip = true;
                break;
            }
        }
        if (skip) continue;

		inFile.setFileName(file.fileName());

		if(!inFile.open(QIODevice::ReadOnly)) {
			qWarning("inFile.open(): %s", inFile.errorString().toLocal8Bit().constData());
			return false;
		}
		QString destination = QFileInfo(filepath).dir().filePath(inFile.fileName());
		if (QFileInfo(destination).exists())
			QFile::remove(destination);
		DebugDialog::debug("Destination " + destination);
		inFile.copy(destination);

		inFile.close();
	}
	QDir::setCurrent(currFolderBU);

	return true;
}


bool FolderUtils::createZipAndSaveTo(const QDir &dirToCompress, const QString &filepath, const QStringList & skipSuffixes) {
	DebugDialog::debug("zipping "+dirToCompress.path()+" into "+filepath);

	QString tempZipFile = QDir::temp().path()+"/"+TextUtils::getRandText()+".zip";
	DebugDialog::debug("temp file: "+tempZipFile);
	QuaZip zip(tempZipFile);
	if(!zip.open(QuaZip::mdCreate)) {
		qWarning("zip.open(): %d", zip.getZipError());
		return false;
	}

	QFileInfoList files=dirToCompress.entryInfoList();
	QFile inFile;
	QuaZipFile outFile(&zip);
	char c;

	QString currFolderBU = QDir::currentPath();
	QDir::setCurrent(dirToCompress.path());
	foreach(QFileInfo file, files) {
		if(!file.isFile()||file.fileName()==filepath) continue;
		if (file.fileName().contains(LockManager::LockedFileName)) continue;

        bool skip = false;
        foreach (QString suffix, skipSuffixes) {
            if (file.fileName().endsWith(suffix)) {
                skip = true;
                break;
            }
        }
        if (skip) continue;

//#pragma message("remove fzz check")
//if (file.fileName().endsWith(".fzz")) continue;

		inFile.setFileName(file.fileName());

		if(!inFile.open(QIODevice::ReadOnly)) {
			qWarning("inFile.open(): %s", inFile.errorString().toLocal8Bit().constData());
			return false;
		}
		if(!outFile.open(QIODevice::WriteOnly, QuaZipNewInfo(inFile.fileName(), inFile.fileName()))) {
			qWarning("outFile.open(): %d", outFile.getZipError());
			return false;
		}

		while(inFile.getChar(&c)&&outFile.putChar(c)){}

		if(outFile.getZipError()!=UNZ_OK) {
			qWarning("outFile.putChar(): %d", outFile.getZipError());
			return false;
		}
		outFile.close();
		if(outFile.getZipError()!=UNZ_OK) {
			qWarning("outFile.close(): %d", outFile.getZipError());
			return false;
		}
		inFile.close();
	}
	zip.close();
	QDir::setCurrent(currFolderBU);

	if(QFileInfo(filepath).exists()) {
		// if we're here the usr has already accepted to overwrite
		QFile::remove(filepath);
	}
	QFile file(tempZipFile);
	FolderUtils::slamCopy(file, filepath);
	file.remove();

	if(zip.getZipError()!=0) {
		qWarning("zip.close(): %d", zip.getZipError());
		return false;
	}
	return true;
}



bool FolderUtils::unzipTo(const QString &filepath, const QString &dirToDecompress, QString & error) {
    static QChar badCharacters[] = { '\\', '/', ':', '*', '?', '"', '<', '>', '|' };
    static QChar underscore('_');

	QuaZip zip(filepath);
	if(!zip.open(QuaZip::mdUnzip)) {
        error = QString("zip.open(): %d").arg(zip.getZipError());
		DebugDialog::debug(error);
		return false;
	}

	zip.setFileNameCodec("IBM866");
	DebugDialog::debug(QString("unzipping %1 entries from %2").arg(zip.getEntriesCount()).arg(filepath));
	QuaZipFileInfo info;
	QuaZipFile file(&zip);
	QFile out;
	QString name;
	char c;
	for(bool more=zip.goToFirstFile(); more; more=zip.goToNextFile()) {
		if(!zip.getCurrentFileInfo(&info)) {
			error = QString("getCurrentFileInfo(): %d\n").arg(zip.getZipError());
			DebugDialog::debug(error);
			return false;
		}

		if(!file.open(QIODevice::ReadOnly)) {
			error = QString("file.open(): %d").arg(file.getZipError());
			DebugDialog::debug(error);
			return false;
		}
		name=file.getActualFileName();
		if(file.getZipError()!=UNZ_OK) {
			error = QString("file.getFileName(): %d").arg(file.getZipError());
			DebugDialog::debug(error);
			return false;
		}

		out.setFileName(dirToDecompress+"/"+name);
		// this will fail if "name" contains subdirectories, but we don't mind that
		if(!out.open(QIODevice::WriteOnly)) {
            for (int i = 0; i < name.length(); i++) {
                if (name[i].unicode() < 32) {
                    name.replace(i, 1, &underscore, 1);
                }
                else for (unsigned int j = 0; j < (sizeof(badCharacters) / sizeof(QChar)); j++) {
                    if (name[i] == badCharacters[j]) {
                        name.replace(i, 1, &underscore, 1);
                        break;
                    }
                }
            }
            out.setFileName(dirToDecompress+"/"+name);
            if(!out.open(QIODevice::WriteOnly)) {
                error = QString("out.open(): %s").arg(out.errorString().toLocal8Bit().constData());
			    DebugDialog::debug(error);
			    return false;
            }
		}

		// Slow like hell (on GNU/Linux at least), but it is not my fault.
		// Not ZIP/UNZIP package's fault either.
		// The slowest thing here is out.putChar(c).
		// TODO: now that out.putChar has been replaced with a buffered write, is it still slow under Linux?

#define BUFFERSIZE 1024
		char buffer[BUFFERSIZE];
		int ix = 0;
		while(file.getChar(&c)) {
			buffer[ix++] = c;
			if (ix == BUFFERSIZE) {
				out.write(buffer, ix);
				ix = 0;
			}
		}
		if (ix > 0) {
			out.write(buffer, ix);
		}

		out.close();
		if(file.getZipError()!=UNZ_OK) {
			error = QString("file.getFileName(): %d").arg(file.getZipError());
			DebugDialog::debug(error);
			return false;
		}
		if(!file.atEnd()) {
			error = "read all but not EOF";
			DebugDialog::debug(error);
			return false;
		}
		file.close();
		if(file.getZipError()!=UNZ_OK) {
			error = QString("file.close(): %d").arg(file.getZipError());
			DebugDialog::debug(error);
			return false;
		}
	}
	zip.close();
	if(zip.getZipError()!=UNZ_OK) {
		error = QString("zip.close(): %d").arg(zip.getZipError());
		DebugDialog::debug(error);
		return false;
	}
	return true;
}


void FolderUtils::collectFiles(const QDir & parent, QStringList & filters, QStringList & files, bool recursive)
{
	QFileInfoList fileInfoList = parent.entryInfoList(filters, QDir::Files | QDir::Hidden | QDir::NoSymLinks);
	foreach (QFileInfo fileInfo, fileInfoList) {
		files.append(fileInfo.absoluteFilePath());
	}

    if (recursive) {
        QFileInfoList dirList = parent.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::NoSymLinks);
        foreach (QFileInfo dirInfo, dirList) {
            QDir dir(dirInfo.filePath());
            //DebugDialog::debug(QString("looking in backup dir %1").arg(dir.absolutePath()));

            collectFiles(dir, filters, files, recursive);
        }
    }
}

void FolderUtils::makePartFolderHierarchy(const QString & prefixFolder, const QString & destFolder) {
	QDir dir(prefixFolder);

	dir.mkdir(destFolder);
	dir.mkdir("svg");
	dir.cd("svg");
	dir.mkdir(destFolder);
	dir.cd(destFolder);
	dir.mkdir("icon");
	dir.mkdir("breadboard");
	dir.mkdir("schematic");
	dir.mkdir("pcb");
}

void FolderUtils::copyBin(const QString & dest, const QString & source) {
    if(QFileInfo(dest).exists()) return;

    // this copy action, is not working on windows, because is a resources file
    if(!QFile(source).copy(dest)) {
#ifdef Q_OS_WIN // may not be needed from qt 4.5.2 on
        DebugDialog::debug("Failed to copy a file from the resources");
        QDir binsFolder = QFileInfo(dest).dir().absolutePath();
        QStringList binFiles = binsFolder.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
        foreach(QString binName, binFiles) {
            if(binName.startsWith("qt_temp.")) {
                QString filePath = binsFolder.absoluteFilePath(binName);
                bool success = QFile(filePath).rename(dest);
                Q_UNUSED(success);
                break;
            }
        }
#endif
    }
    QFlags<QFile::Permission> ps = QFile::permissions(dest);
    QFile::setPermissions(
        dest,
        QFile::WriteOwner | QFile::WriteUser | ps
#ifdef Q_OS_WIN
        | QFile::WriteOther | QFile::WriteGroup
#endif

    );
}

bool FolderUtils::slamCopy(QFile & file, const QString & dest) {
    QFileInfo info(file);
    if (info.absoluteFilePath() == dest) {
        // source = dest
        return true;
    }

    bool result = file.copy(dest);
    if (result) return result;

    file.remove(dest);
    return file.copy(dest);
}

void FolderUtils::showInFolder(const QString & path)
{
    // http://stackoverflow.com/questions/3490336/how-to-reveal-in-finder-or-show-in-explorer-with-qt
    // http://stackoverflow.com/questions/9581330/change-selection-in-explorer-window
    // Mac, Windows support folder or file.
#if defined(Q_OS_WIN)
    const QString explorer = "explorer.exe";
    QString param = QLatin1String("/e,/select,");
    param += QDir::toNativeSeparators(path);
    QProcess::startDetached(explorer, QStringList(param));
#elif defined(Q_OS_MAC)
    QStringList scriptArgs;
    scriptArgs << QLatin1String("-e")
               << QString::fromLatin1("tell application \"Finder\" to reveal POSIX file \"%1\"")
                                     .arg(path);
    QProcess::execute(QLatin1String("/usr/bin/osascript"), scriptArgs);
    scriptArgs.clear();
    scriptArgs << QLatin1String("-e")
               << QLatin1String("tell application \"Finder\" to activate");
    QProcess::execute("/usr/bin/osascript", scriptArgs);
#else
    QDesktopServices::openUrl( QUrl::fromLocalFile( QFileInfo(path).absolutePath() ) );   
#endif
}

void FolderUtils::createUserDataStoreFolders() {
    // make sure that the folder structure for parts and bins, exists

    if (singleton == NULL) {
        singleton = new FolderUtils();
    }

    QDir userDataStore(getTopLevelUserDataStorePath());
    foreach(QString folder, singleton->m_userFolders) {
        QString path = userDataStore.absoluteFilePath(folder);
        if(!QFileInfo(path).exists()) {
            userDataStore.mkpath(folder);
        }
    }

    QDir documents(getTopLevelDocumentsPath());
    QStringList documentFolders(singleton->m_documentFolders);
    foreach(QString folder, documentFolders) {
        QString path = documents.absoluteFilePath(folder);
        if(!QFileInfo(path).exists()) {
            documents.mkpath(folder);
        }
    }

    // in older versions of Fritzing, local parts and bins were in userDataStore
    QList<QDir> toRemove;
    QStringList folders;
    folders << "bins" << "parts";
    bool foundOld = false;
    foreach(QString folder, folders ) {
        foundOld || QFileInfo(userDataStore.absoluteFilePath(folder)).exists();
    }

    if (foundOld) {
        // inform user about the move
        FMessageBox::information(NULL, QCoreApplication::translate("FolderUtils", "Moving your custom parts"),
            QCoreApplication::translate("FolderUtils", "<p>Your custom-made parts and bins are moved from the hidden "
               "app data folder to your fritzing documents folder at <br/><br/><em>%1</em><br/><br/>"
               "This way we hope to make it easier for you to find and edit them manually.</p>")
               .arg(documents.absolutePath()));

        // copy these into the new locations
        foreach(QString folder, folders ) {
            QDir source(userDataStore.absoluteFilePath(folder));
            QDir target(documents.absoluteFilePath(folder));
            if (source.exists()) {
                replicateDir(source, target);
                toRemove << source;
            }
        }

        // now remove the obsolete locations
        foreach (QDir dir, toRemove) {
            rmdir(dir);
        }
    }
}
