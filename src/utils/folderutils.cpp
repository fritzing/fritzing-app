/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2019 Fritzing

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

#include "folderutils.h"
#include "sanitizeforpath.h"
#include "lockmanager.h"
#include "textutils.h"
#include <QDesktopServices>
#include <QCoreApplication>
#include <QSettings>
#include <QTextStream>
#include <QUuid>
#include <QCryptographicHash>
#include <QDateTime>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QStandardPaths>

#include "../debugdialog.h"
#include "utils/misc.h"
#include "fmessagebox.h"
#include <quazip/quazip.h>
#include <quazip/quazipfile.h>


FolderUtils* FolderUtils::singleton = nullptr;
QString FolderUtils::m_openSaveFolder = "";

FolderUtils::FolderUtils() {
	m_openSaveFolder = ___emptyString___;
	m_userFolders
	        << "partfactory"
	        << "backup"
	        << "history"
	        << "fzz"
	        << "local_parts";
	m_documentFolders
	        << "bins";

}

FolderUtils::~FolderUtils() {
}

// finds a subfolder of the application directory searching backward up the tree
QDir  FolderUtils::getApplicationSubFolder(QString search) {
	if (singleton == nullptr) {
		singleton = new FolderUtils();
	}

	QString path = singleton->applicationDirPath();
	path += "/" + search;
	//DebugDialog::debug(QString("path %1").arg(path) );
	QDir dir(path);
	while (!dir.exists()) {
		// if we're running from the debug or release folder, try go up one to find things
		dir.cdUp();
		dir.cdUp();
		if (dir.isRoot()) {
			DebugDialog::debug(QString("Application folder %1 not found").arg(search));
			return QDir();   // didn't find the search folder
		}

		dir.setPath(dir.absolutePath() + "/" + search);
	}

	return dir;
}

QString FolderUtils::getApplicationSubFolderPath(QString search) {
	if (singleton == nullptr) {
		singleton = new FolderUtils();
	}

	QDir dir = getApplicationSubFolder(search);
	return dir.path();
}

QString FolderUtils::getAppPartsSubFolderPath(QString search) {
	if (singleton == nullptr) {
		singleton = new FolderUtils();
	}

	QDir dir = getAppPartsSubFolder(search);
	return dir.path();
}

QDir FolderUtils::getAppPartsSubFolder(QString search) {
	if (singleton == nullptr) {
		singleton = new FolderUtils();
	}

	return singleton->getAppPartsSubFolder2(search);
}

QDir FolderUtils::getAppPartsSubFolder2(QString search) {
	if (m_partsPath.isEmpty()) {
		QDir dir = getApplicationSubFolder("fritzing-parts");
		if (dir.exists() && dir.dirName().endsWith("fritzing-parts")) {
			m_partsPath = dir.absolutePath();
		}
		else {
			QDir dir = getApplicationSubFolder("parts");
			if (dir.exists() && dir.dirName().endsWith("parts")) {
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
	QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
	return dir.absolutePath();
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

QString FolderUtils::getLocalPartsPath() {
	QDir dir(getTopLevelUserDataStorePath());
	return dir.absoluteFilePath("local_parts");
}

bool FolderUtils::createFolderAndCdIntoIt(QDir &dir, QString newFolder) {
	if(!dir.mkdir(newFolder)) return false;
	if(!dir.cd(newFolder)) return false;

	return true;
}

bool FolderUtils::setApplicationPath(const QString & path)
{
	if (singleton == nullptr) {
		singleton = new FolderUtils();
	}

	return singleton->setApplicationPath2(path);
}

bool FolderUtils::setAppPartsPath(const QString & path)
{
	if (singleton == nullptr) {
		singleton = new FolderUtils();
	}

	return singleton->setPartsPath2(path);
}

void FolderUtils::cleanup() {
	if (singleton) {
		delete singleton;
		singleton = nullptr;
	}
}

const QString FolderUtils::getLibraryPath()
{
	if (singleton == nullptr) {
		singleton = new FolderUtils();
	}

	return singleton->libraryPath();
}


const QString FolderUtils::libraryPath()
{
#ifdef Q_OS_MACOS
	// mac plugins are always in the bundle
	return QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../lib");
#endif

	return QDir::cleanPath(applicationDirPath() + "/lib");
}

const QString FolderUtils::applicationDirPath() {
	if (!m_appPath.isEmpty()) {
		return m_appPath;
	}


#ifdef Q_OS_WIN
	m_appPath = QCoreApplication::applicationDirPath();
#else
	// look in standard Fritzing location (applicationDirPath and parent folders) then in standard linux locations
	QStringList candidates;
	candidates.append(QCoreApplication::applicationDirPath());

	QDir dir(QCoreApplication::applicationDirPath());
	if (dir.cdUp()) {

#ifdef Q_OS_MACOS
		candidates.append(dir.absolutePath() + "/Resources");
#endif
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
	Q_FOREACH (QString candidate, candidates) {
		QDir dir(candidate);
		if (!dir.exists("translations")) {
			continue;
		}

		if (dir.exists("help")) {
			m_appPath = candidate;
			break;
		}

	}

	if (m_appPath.isEmpty()) {
		m_appPath = QCoreApplication::applicationDirPath();
		DebugDialog::debug("Data folder not found.", DebugDialog::Warning);
		DebugDialog::debug(" Checked: ");
		for (const QString& candidate : candidates) {
			DebugDialog::debug(candidate);
		}
	}
#endif

	DebugDialog::debug(QString("Using data folder: %1").arg(m_appPath), DebugDialog::Info);
	return m_appPath;
}

bool FolderUtils::setApplicationPath2(const QString & path)
{
	QDir dir(path);
	if (!dir.exists()) return false;

	m_appPath = dir.canonicalPath();
	return true;
}

bool FolderUtils::setPartsPath2(const QString & path)
{
	QDir dir(path);
	if (!dir.exists()) return false;

	m_partsPath = dir.canonicalPath();
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
	DebugDialog::debug(QString("removing folder: %1").arg(dir.path()));

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
	QDir destDir = QFileInfo(filepath).dir();

	QString currFolderBU = QDir::currentPath();
	QDir::setCurrent(dirToCompress.path());
	Q_FOREACH(QFileInfo file, files) {
		if(!file.isFile()) continue;
		if (file.fileName().contains(LockManager::LockedFileName)) continue;

		bool skip = false;
		Q_FOREACH (QString suffix, skipSuffixes) {
			if (file.fileName().endsWith(suffix)) {
				skip = true;
				break;
			}
		}
		if (skip) continue;

		QString destination = destDir.absoluteFilePath(file.fileName());
		// If dirToCompress already is the destination folder, source and
		// destination are the same file. Copying it would mean removing the
		// destination (= the source) and then failing the copy, silently
		// destroying the bundle contents (issue #1158). Leave it in place.
		if (destination == file.absoluteFilePath()) continue;

		inFile.setFileName(file.fileName());
		if(!inFile.open(QIODevice::ReadOnly)) {
			qWarning("inFile.open(): %s", inFile.errorString().toLocal8Bit().constData());
			QDir::setCurrent(currFolderBU);
			return false;
		}
		if (QFileInfo(destination).exists())
			QFile::remove(destination);
		DebugDialog::debug("Destination " + destination);
		if (!inFile.copy(destination)) {
			qWarning("inFile.copy(): %s", inFile.errorString().toLocal8Bit().constData());
			inFile.close();
			QDir::setCurrent(currFolderBU);
			return false;
		}
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
		qWarning() << QString("zip.open(): %1").arg(zip.getZipError());
		return false;
	}

	QFileInfoList files=dirToCompress.entryInfoList();
	QFile inFile;
	QuaZipFile outFile(&zip);
	char c;
	int entriesWritten = 0;

	QString currFolderBU = QDir::currentPath();
	QDir::setCurrent(dirToCompress.path());
	Q_FOREACH(QFileInfo file, files) {
		if(!file.isFile()||file.fileName()==filepath) continue;
		if (file.fileName().contains(LockManager::LockedFileName)) continue;

		bool skip = false;
		Q_FOREACH (QString suffix, skipSuffixes) {
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

		while(inFile.getChar(&c)&&outFile.putChar(c)) {}

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
		entriesWritten++;
	}
	zip.close();
	QDir::setCurrent(currFolderBU);

	if (entriesWritten < 1) {
		// Refuse to overwrite the target with a 0-entry (empty) archive; doing so
		// would silently destroy the user's file (issue #1158). Leave it intact.
		qWarning() << "Saving failed. Refusing to write an empty bundle to" << filepath;
		QFile::remove(tempZipFile);
		return false;
	}

	QFile file(tempZipFile);
	QString randSuffix = TextUtils::getRandText();
	QString temporaryFilepathInTargetDir = addToBasename(filepath, randSuffix);
	bool result = FolderUtils::slamCopy(file, temporaryFilepathInTargetDir);
	if (!result) {
		qWarning("Saving failed. Copying file to target dir failed.");
		return false;
	}

	if(QFileInfo(filepath).exists()) {
		// if we're here the usr has already accepted to overwrite
		QFile::remove(filepath);
	}
	QFile file2(temporaryFilepathInTargetDir);
	result = file2.rename(filepath);
	if (!result) {
		qWarning("Saving failed. Renaming file to target file name failed.");
		return false;
	}
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
		error = QString("zip.open(): %1").arg(zip.getZipError());
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
			error = QString("getCurrentFileInfo(): %1\n").arg(zip.getZipError());
			DebugDialog::debug(error);
			return false;
		}

		if(!file.open(QIODevice::ReadOnly)) {
			error = QString("file.open(): %1").arg(file.getZipError());
			DebugDialog::debug(error);
			return false;
		}
		name=file.getActualFileName();
		DebugDialog::debug(QString("  unzipped: %1").arg(name));
		if(file.getZipError()!=UNZ_OK) {
			error = QString("file.getFileName(): %1").arg(file.getZipError());
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
				else for (auto badCharacter : badCharacters) {
						if (name[i] == badCharacter) {
							name.replace(i, 1, &underscore, 1);
							break;
						}
					}
			}
			out.setFileName(dirToDecompress+"/"+name);
			if(!out.open(QIODevice::WriteOnly)) {
				error = QString("out.open(): %1").arg(out.errorString().toLocal8Bit().constData());
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
			error = QString("file.getFileName(): %1").arg(file.getZipError());
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
			error = QString("file.close(): %1").arg(file.getZipError());
			DebugDialog::debug(error);
			return false;
		}
	}
	zip.close();
	if(zip.getZipError()!=UNZ_OK) {
		error = QString("zip.close(): %1").arg(zip.getZipError());
		DebugDialog::debug(error);
		return false;
	}
	return true;
}


void FolderUtils::collectFiles(const QDir & parent, QStringList & filters, QStringList & files, bool recursive)
{
	QFileInfoList fileInfoList = parent.entryInfoList(filters, QDir::Files | QDir::Hidden | QDir::NoSymLinks);
	Q_FOREACH (QFileInfo fileInfo, fileInfoList) {
		files.append(fileInfo.absoluteFilePath());
	}

	if (recursive) {
		QFileInfoList dirList = parent.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::NoSymLinks);
		Q_FOREACH (QFileInfo dirInfo, dirList) {
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
#elif defined(Q_OS_MACOS)
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

// make sure that the folder structure for parts and bins, exists
void FolderUtils::createUserDataStoreFolders() {
	if (singleton == nullptr) {
		singleton = new FolderUtils();
	}

	QDir userDataStore(getTopLevelUserDataStorePath());
	for (const QString &folder : singleton->m_userFolders) {
		QString path = userDataStore.absoluteFilePath(folder);
		if (!QFileInfo::exists(path)) {
			userDataStore.mkpath(folder);
		}
	}

	QDir documents(getTopLevelDocumentsPath());
	for (const QString &folder : singleton->m_documentFolders) {
		QString path = documents.absoluteFilePath(folder);
		if (!QFileInfo::exists(path)) {
			documents.mkpath(folder);
		}
	}

	// in older versions of Fritzing, local parts and bins were in userDataStore
	// "older" refers to Fritzing versions before 2013.
	for (const QString folder : {"bins", "parts"}) {
		QDir oldDir(userDataStore.absoluteFilePath(folder));
		if (oldDir.exists()) {
			QString oldLocation(oldDir.absolutePath());
			QString newLocation(documents.absoluteFilePath(folder));

			FMessageBox::information(nullptr,
									 QCoreApplication::translate("Legacy", "Move Your Custom Parts"),
									 QCoreApplication::translate("Legacy", "<p>Please move your custom-made parts and bins from the old location:<br/><br/><em>%1</em><br/><br/>"
																		   "to the new Fritzing documents folder at:<br/><br/><em>%2</em><br/><br/>")
										 .arg(oldLocation, newLocation));
			break;
		}
	}
}

QString FolderUtils::addToBasename(const QString &filePath, const QString &addition)
{
	QFileInfo fileInfo(filePath);
	QString baseName = fileInfo.baseName();
	QString suffix = fileInfo.completeSuffix();
	QString path = fileInfo.path();

	return QString("%1/%2%3.%4").arg(path, baseName, addition, suffix);
}

bool FolderUtils::ensureDirectoryExists(const QString & filePath) {
	QDir dir = QFileInfo(filePath).absoluteDir();
	if (!dir.exists()) {
		return dir.mkpath(dir.absolutePath());
	}
	return true;
}

QString FolderUtils::sanitizeForFolder(const QString & name) {
	return sanitizeForPath(name);
}

bool FolderUtils::checkFileLoadability(QWidget* parent, const QString& filePath) {
	QString fileType;
	static const QMap<QString, QString> fileTypes {
		{FritzingSketchExtension, QObject::tr("Fritzing sketch", "This is a file type used for an error message.")},
		{FritzingBundleExtension, QObject::tr("Fritzing bundle", "This is a file type used for an error message.")},
		{FritzingBinExtension, QObject::tr("Fritzing bin", "This is a file type used for an error message.")},
		{FritzingBundledBinExtension, QObject::tr("Fritzing bundled bin", "This is a file type used for an error message.")},
		{FritzingPartExtension, QObject::tr("Fritzing part", "This is a file type used for an error message.")},
		{FritzingBundledPartExtension, QObject::tr("Fritzing bundled part", "This is a file type used for an error message.")}
	};
	for (auto it = fileTypes.begin(); it != fileTypes.end(); ++it) {
		if (filePath.endsWith(it.key())) {
			fileType = it.value();
			break;
		}
	}

	if (!QFileInfo::exists(filePath)) {
		QFileInfo fileInfo(filePath);
		QStringList errorDetails;

		if (fileInfo.isSymLink()) {
			errorDetails << QObject::tr("A symbolic link exists but points to missing file: %1")
						.arg(fileInfo.symLinkTarget());
		}

		QString parentPath = fileInfo.path();
		if (!QFileInfo(parentPath).exists()) {
			errorDetails << QObject::tr("The parent directory does not exist: %1").arg(parentPath);
		}

		// Check for case sensitivity issues
		QDir dir(fileInfo.path());
		QStringList entries = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
		for (const QString& entry : entries) {
			if (entry.compare(fileInfo.fileName(), Qt::CaseInsensitive) == 0 &&
				entry != fileInfo.fileName()) {
				errorDetails << QObject::tr("Found similar filename with different case: %1")
							.arg(entry);
			}
		}

		QString errorDetail = errorDetails.isEmpty() ?
			QString("") : "\n\nPossible issues:\n• " + errorDetails.join("\n• ");

		FMessageBox::warning(
			parent,
			QObject::tr("Fritzing"),
			QObject::tr("Cannot find file '%1'.\n\nFile type: %2\n\nPlease check if the file exists.")
				.arg(filePath)
				.arg(fileType) + errorDetail
		);
		return false;
	}

	QFile file(filePath);
	if (!file.open(QFile::ReadOnly)) {
		FMessageBox::warning(
					parent,
					QObject::tr("Fritzing"),
					QObject::tr("Cannot read file '%1'.\n\nFile type: %2\n\nPlease ensure you have permission to read the file.")
					.arg(filePath)
					.arg(fileType)
					);
		return false;
	}
	file.close();

	if (QFileInfo(filePath).size() == 0) {
		FMessageBox::warning(
					parent,
					QObject::tr("Fritzing"),
					QObject::tr("File '%1' is empty.\n\nFile type: %2\n\nThis could be due to a cloud storage or network drive issue. Please ensure the file has been properly synchronized and saved.")
					.arg(filePath)
					.arg(fileType)
					);
		return false;
	}
	return true;
}

QString FolderUtils::getHistoryPath() {
	return getTopLevelUserDataStorePath() + "/history";
}

void FolderUtils::savePreviousVersionToHistory(const QString &filePath, int maxVersions) {
	QFileInfo sourceInfo(filePath);
	if (!sourceInfo.exists() || sourceInfo.size() == 0) {
		return;
	}

	QString historyDir = getHistoryPath();
	QDir dir(historyDir);
	if (!dir.exists()) {
		dir.mkpath(historyDir);
	}

	// Build history filename: <md5_8chars>_<basename>_<timestamp>.<ext>
	QByteArray pathHash = QCryptographicHash::hash(
		sourceInfo.absoluteFilePath().toUtf8(), QCryptographicHash::Md5);
	QString hashPrefix = pathHash.toHex().left(8);

	QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
	QString historyFileName = QString("%1_%2_%3.%4")
		.arg(hashPrefix,
		     sourceInfo.completeBaseName(),
		     timestamp,
		     sourceInfo.suffix());

	QString historyFilePath = dir.absoluteFilePath(historyFileName);
	if (!QFile::copy(filePath, historyFilePath)) {
		DebugDialog::debug(QString("savePreviousVersionToHistory: failed to copy '%1' to '%2'")
			.arg(filePath, historyFilePath));
		return;
	}

	// Prune: keep at most maxVersions files matching this path hash prefix
	QStringList nameFilter;
	nameFilter << (hashPrefix + "_*");
	QFileInfoList historyFiles = dir.entryInfoList(nameFilter, QDir::Files, QDir::Time);
	while (historyFiles.size() > maxVersions) {
		QFileInfo oldest = historyFiles.takeLast();
		QFile::remove(oldest.absoluteFilePath());
	}
}
