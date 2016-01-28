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

$Revision: 6141 $:
$Author: cohen@irascible.com $:
$Date: 2012-07-04 21:20:05 +0200 (Mi, 04. Jul 2012) $

********************************************************************/

#ifndef LOCKMANAGER_H
#define LOCKMANAGER_H

#include <QFile>
#include <QHash>
#include <QFileInfoList>


struct LockedFile {
	QFile file;
	long frequency;

	LockedFile(const QString & filename, long freq);
	bool touch();
};

class LockManager : public QObject {
Q_OBJECT

public:
	LockManager();
	~LockManager();

public:
	static void initLockedFiles(const QString & prefix, QString & folder, QHash<QString, LockedFile *> & lockedFiles, long touchFrequency);
	static void releaseLockedFiles(const QString & folder, QHash<QString, LockedFile *> & lockedFiles);
	static void checkLockedFiles(const QString & prefix, QFileInfoList & backupList, QHash<QString, LockedFile *> & lockedFiles, bool recurse, long touchFrequency);
    static void cleanup();

public:
	static const QString LockedFileName;
	static const long FastTime;
	static const long SlowTime;

public slots:
	void touchFiles();

protected:
	static bool checkLockedFilesAux(const QDir & parent, QStringList & filters);
	static void releaseLockedFiles(const QString & folder, QHash<QString, LockedFile *> & lockedFiles, bool remove);
	static LockedFile * makeLockedFile(const QString & folder, long touchFrequency);

};


#endif
