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

$Revision: 6904 $:
$Author: irascibl@gmail.com $:
$Date: 2013-02-26 16:26:03 +0100 (Di, 26. Feb 2013) $

********************************************************************/

#ifndef PARTSCHECKER_H
#define PARTSCHECKER_H

#include <QString>

class PartsCheckerUpdateInterface {
public:
    // progress ranges from 0 to 1
    virtual void updateProgress(double progress) = 0;
};

class PartsChecker
{
public:
    static QString getSha(const QString & repoPath);
    static bool newPartsAvailable(const QString &repoPath, const QString &shaFromDataBase, bool atUserRequest, QString &remoteSha);
    static bool updateParts(const QString & repoPath, const QString & remoteSha, PartsCheckerUpdateInterface *);

protected:
    static int doMerge(struct git_repository * repository, const QString & remoteSha);
};

#endif // PARTSCHECKER_H
