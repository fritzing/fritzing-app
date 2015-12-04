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

#include "partschecker.h"
#include "version.h"
#include "../debugdialog.h"
#include "../utils/fmessagebox.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QSettings>
#include <QTimer>
#include <QDebug>
#include <QSettings>

#include <git2.h>

bool PartsChecker::newPartsAvailable(const QString &repoPath, const QString & shaFromDataBase, bool atUserRequest)
{
    // from https://github.com/libgit2/libgit2/blob/master/examples/network/ls-remote.c

    git_repository *repository = NULL;
    git_remote *remote = NULL;
    int error;
    const git_remote_head **remote_heads;
    size_t remote_heads_len, i;
    git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
    bool available = false;

    git_libgit2_init();

    error = git_repository_open(&repository, repoPath.toUtf8().constData());
    if (error) {
        DebugDialog::debug("unable to open repo " + repoPath);
        goto cleanup;
    }

    // Find the remote by name
    error = git_remote_lookup(&remote, repository, "origin");
    if (error) {
        error = git_remote_create_anonymous(&remote, repository, "origin");
        if (error) {
            DebugDialog::debug("unable to lookup repo " + repoPath);
            goto cleanup;
        }
    }

    /**
     * Connect to the remote and call the printing function for
     * each of the remote references.
     */
    //callbacks.credentials = cred_acquire_cb;

    error = git_remote_connect(remote, GIT_DIRECTION_FETCH, &callbacks, NULL);
    if (error) {
        DebugDialog::debug("unable to connect to repo " + repoPath);
        goto cleanup;
    }

    /**
     * Get the list of references on the remote and print out
     * their name next to what they point to.
     */
    error = git_remote_ls(&remote_heads, &remote_heads_len, remote);
    if (error) {
        DebugDialog::debug("unable to list repo " + repoPath);
        goto cleanup;
    }

    for (i = 0; i < remote_heads_len; i++) {
        if (strcmp(remote_heads[i]->name, "refs/heads/master") == 0) {
            char oid[GIT_OID_HEXSZ + 1] = {0};
            git_oid_fmt(oid, &remote_heads[i]->oid);
            QString soid(oid);

            QSettings settings;
            QString lastPartsSha = settings.value("lastPartsSha", "").toString();
            settings.setValue("lastPartsSha", soid);
            // if soid matches database or matches the last time we notified the user, don't notify again
            available = atUserRequest ? (soid != shaFromDataBase)
                                      : (soid != lastPartsSha && soid != shaFromDataBase);
            break;
        }
    }

cleanup:
    git_remote_free(remote);
    git_repository_free(repository);
    git_libgit2_shutdown();
    return available;
}

QString PartsChecker::getSha(const QString & repoPath) {

    QString sha;
    git_repository * repository = NULL;

    git_libgit2_init();

    int error = git_repository_open(&repository, repoPath.toUtf8().constData());
    if (error) {
        FMessageBox::warning(NULL, QObject::tr("Regenerating parts database"), QObject::tr("Unable to find parts git repository"));
        goto cleanup;
    }

    git_oid oid;  /* the SHA1 for last commit */

    /* resolve HEAD into a SHA1 */
    error = git_reference_name_to_id( &oid, repository, "HEAD" );
    if (error) {
        FMessageBox::warning(NULL, QObject::tr("Regenerating parts database"), QObject::tr("Unable to find parts git repository HEAD"));
        goto cleanup;
    }

    char buffer[GIT_OID_HEXSZ + 1] = {0};
    git_oid_fmt(buffer, &oid);
    sha = QString(buffer);

cleanup:
    git_repository_free(repository);
    git_libgit2_shutdown();

    return sha;
}
