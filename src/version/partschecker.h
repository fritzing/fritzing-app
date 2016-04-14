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

#ifndef PARTSCHECKER_H
#define PARTSCHECKER_H

#include <QString>
#include <QStringList>

class PartsCheckerUpdateInterface {
public:
    // progress ranges from 0 to 1
    virtual void updateProgress(double progress) = 0;
};

enum PartsCheckerError {
    PARTS_CHECKER_ERROR_NONE = 0,
    PARTS_CHECKER_ERROR_REMOTE,
    PARTS_CHECKER_ERROR_USED_GIT,
    PARTS_CHECKER_ERROR_LOCAL_DAMAGE,
    PARTS_CHECKER_ERROR_LOCAL_MODS
};

struct PartsCheckerResult {

    PartsCheckerError partsCheckerError;
    QString errorMessage;
    QStringList untrackedFiles;
    QStringList changedFiles;
    QStringList gitChangedFiles;
    QStringList unreadableFiles;

    void reset();
};

class PartsChecker
{
public:
    /**
     * returns the sha1 for the last commit; used only when creating the parts db
     */
    static QString getSha(const QString & repoPath);

    /**
     * Check if there are any new parts available (since the last new parts check)
     *
     * remoteSha and PartsErrorResult are also returned
     */
    static bool newPartsAvailable(const QString &repoPath, const QString &shaFromDataBase, bool atUserRequest, QString &remoteSha, PartsCheckerResult & partsCheckerResult);

    /**
     * trigger the update, with progress going to the PartsCheckerUpdateInterface
     */
    static bool updateParts(const QString & repoPath, const QString & remoteSha, PartsCheckerUpdateInterface *);

    /**
     * Remove untracked files (as listed in partsCheckerResult) and perform git reset --hard
     */
    static bool cleanRepo(const QString & repoPath, const PartsCheckerResult & partsCheckerResult);

protected:
    static int doMerge(struct git_repository * repository, const QString & remoteSha);
    static bool checkIfClean(const QString & repoPath, const QString &shaFromDatabase, struct git_repository *repository, PartsCheckerResult &partsCheckerResult);
    static void getChanges(struct git_status_list *status_list, PartsCheckerResult & partsCheckerResult);

};

#endif // PARTSCHECKER_H
