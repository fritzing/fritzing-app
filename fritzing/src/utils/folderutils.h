/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2013 Fachhochschule Potsdam - http://fh-potsdam.de

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

$Revision: 6912 $:
$Author: irascibl@gmail.com $:
$Date: 2013-03-09 08:18:59 +0100 (Sa, 09. Mrz 2013) $

********************************************************************/

#ifndef FOLDERUTILS_H
#define FOLDERUTILS_H

#include <QString>
#include <QDir>
#include <QStringList>
#include <QFileDialog>

#include "misc.h"

class FolderUtils
{

public:
	static QDir *getApplicationSubFolder(QString);
	static QString getApplicationSubFolderPath(QString);
	static QString getUserDataStorePath(QString folder = ___emptyString___);
	static const QStringList & getUserDataStoreFolders();
	static bool createFolderAnCdIntoIt(QDir &dir, QString newFolder);
	static bool setApplicationPath(const QString & path);
	static const QString getLibraryPath();
	static QString getOpenFileName( QWidget * parent = 0, const QString & caption = QString(), const QString & dir = QString(), const QString & filter = QString(), QString * selectedFilter = 0, QFileDialog::Options options = 0 );
	static QStringList getOpenFileNames( QWidget * parent = 0, const QString & caption = QString(), const QString & dir = QString(), const QString & filter = QString(), QString * selectedFilter = 0, QFileDialog::Options options = 0 );
	static QString getSaveFileName( QWidget * parent = 0, const QString & caption = QString(), const QString & dir = QString(), const QString & filter = QString(), QString * selectedFilter = 0, QFileDialog::Options options = 0 );
	static void setOpenSaveFolder(const QString& path);
	static void setOpenSaveFolderAux(const QString& path); 
	static const QString openSaveFolder();
	static bool isEmptyFileName(const QString &filename, const QString &unsavedFilename);
	static void rmdir(const QString &dirPath);
	static void rmdir(QDir & dir);
	static bool createZipAndSaveTo(const QDir &dirToCompress, const QString &filename, const QStringList & skipSuffixes);
	static bool unzipTo(const QString &filepath, const QString &dirToDecompress, QString & error);
	static void replicateDir(QDir srcDir, QDir targDir);
	static void cleanup();
    static void collectFiles(const QDir & parent, QStringList & filters, QStringList & files, bool recursive);
	static void makePartFolderHierarchy(const QString & prefixFolder, const QString & destFolder);
    static void copyBin(const QString & source, const QString & dest);
    static bool slamCopy(QFile &, const QString & dest);
    static void showInFolder(const QString & path);

protected:
	FolderUtils();
	~FolderUtils();

	const QStringList & userDataStoreFolders();
	bool setApplicationPath2(const QString & path);
	const QString applicationDirPath();
	const QString libraryPath();

protected:
	static FolderUtils* singleton;
	static QString m_openSaveFolder;

protected:
	QStringList m_folders;
	QString m_appPath;
};

#endif
