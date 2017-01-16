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
#include <time.h>

#include <git2.h>

static QString sBoilerPlate1;
static QString sBoilerPlate2;
static QString sNotInMasterBranch;
static QString sDamaged;
static QString sModifiedByGit;
static QString sModified;
static QString sUnreadable;

void PartsCheckerResult::reset() {
    partsCheckerError = PARTS_CHECKER_ERROR_NONE;
    errorMessage.clear();
    untrackedFiles.clear();
    changedFiles.clear();
    gitChangedFiles.clear();
    unreadableFiles.clear();
}

/**
 * Check whether the last commit in the remote repository matches the last commit stored in the database,
 * or check if the last commit in the remote repository  matches the commit saved last time newPartsAvailable returned true
 *
 * repoPath is the local parts folder
 * remoteSha is returned
 */
bool PartsChecker::newPartsAvailable(const QString &repoPath, const QString & shaFromDatabase, bool atUserRequest, QString & remoteSha, PartsCheckerResult &partsCheckerResult)
{
    // from https://github.com/libgit2/libgit2/blob/master/examples/network/ls-remote.c

    if (sBoilerPlate1.isEmpty()) {
        sBoilerPlate1 = "<p>Fritzing cannot update the parts folder automatically.</p>";
        sBoilerPlate2 = sBoilerPlate1 +
                              "<p>If you used git to make changes, use git to update and then regenerate the database<br/>"
                              "by selecting <i>Part > Regenerate parts database...</i>.</p>"
                              "<p>If you did not use git to make changes, then you may need to reinstall Fritzing.</p>";

        sNotInMasterBranch = QObject::tr("The parts folder '%1' has been changed--it is not in the master branch (%2). %3");
        sDamaged = QObject::tr("The parts folder '%1' may have been damaged (%2). %3");
        sModifiedByGit = QObject::tr("The parts folder '%1' has been changed (%2). %3");
        sModified = QObject::tr("There are new or modified files in the parts folder '%1'. These changes will be discarded by the update.");
        sUnreadable = QObject::tr("There are unreadable files in the parts folder '%1'. %2");
    }

    remoteSha = "";

    git_repository *repository = NULL;
    git_remote *remote = NULL;
    int error;
    const git_remote_head **remote_heads;
    size_t remote_heads_len, i;
    git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
    bool available = false;

    partsCheckerResult.partsCheckerError = PARTS_CHECKER_ERROR_NONE;
    git_libgit2_init();

    error = git_repository_open(&repository, repoPath.toUtf8().constData());
    if (error) {
        partsCheckerResult.partsCheckerError = PARTS_CHECKER_ERROR_LOCAL_DAMAGE;
        partsCheckerResult.errorMessage = QObject::tr("Unable to open parts folder '%1' for update. %2").arg(repoPath).arg(sBoilerPlate1);
        goto cleanup;
    }

    if (git_repository_is_bare(repository)) {
        partsCheckerResult.partsCheckerError = PARTS_CHECKER_ERROR_LOCAL_DAMAGE;
        partsCheckerResult.errorMessage = QObject::tr("Parts folder repo '%1' is empty. %2").arg(repoPath).arg(sBoilerPlate1);
        goto cleanup;
    }

    // Find the remote by name
    error = git_remote_lookup(&remote, repository, "origin");
    if (error) {
        partsCheckerResult.partsCheckerError = PARTS_CHECKER_ERROR_LOCAL_DAMAGE;
        partsCheckerResult.errorMessage = QObject::tr("Unable to determine network site for '%1'. %2").arg(repoPath).arg(sBoilerPlate1);
        goto cleanup;
    }

    /**
     * Connect to the remote.
     */
#if LIBGIT2_VER_MINOR > 24
    error = git_remote_connect(remote, GIT_DIRECTION_FETCH, &callbacks, NULL, NULL);
#elif LIBGIT2_VER_MINOR == 24
    error = git_remote_connect(remote, GIT_DIRECTION_FETCH, &callbacks, NULL);
#else
    error = git_remote_connect(remote, GIT_DIRECTION_FETCH, &callbacks);
#endif
    if (error) {
        partsCheckerResult.partsCheckerError = PARTS_CHECKER_ERROR_REMOTE;
        partsCheckerResult.errorMessage = QObject::tr("Unable to access network site for '%1'. %2").arg(repoPath).arg(sBoilerPlate1);
        goto cleanup;
    }

    /**
     * Get the list of references on the remote
     */
    error = git_remote_ls(&remote_heads, &remote_heads_len, remote);
    if (error) {
        partsCheckerResult.partsCheckerError = PARTS_CHECKER_ERROR_REMOTE;
        partsCheckerResult.errorMessage = QObject::tr("Unable to retrieve network references for '%1'. %2").arg(repoPath).arg(sBoilerPlate1);
        goto cleanup;
    }

    for (i = 0; i < remote_heads_len; i++) {
        // we only care about the master branch
        if (strcmp(remote_heads[i]->name, "refs/heads/master") == 0) {
            char oid[GIT_OID_HEXSZ + 1] = {0};
            git_oid_fmt(oid, &remote_heads[i]->oid);
            QString soid(oid);
            remoteSha = soid;
            QSettings settings;
            QString lastPartsSha = settings.value("lastPartsSha", "").toString();
            settings.setValue("lastPartsSha", soid);
            // if soid matches database or matches the last time we notified the user, don't notify again
            available = atUserRequest ? (soid != shaFromDatabase)
                                      : (soid != lastPartsSha && soid != shaFromDatabase);
            if (available) {
                checkIfClean(repoPath, shaFromDatabase, repository, partsCheckerResult);
            }

            goto cleanup;
        }
    }

    partsCheckerResult.partsCheckerError = PARTS_CHECKER_ERROR_REMOTE;
    partsCheckerResult.errorMessage = QObject::tr("Unable to retrieve master network reference for '%1'. %2").arg(repoPath).arg(sBoilerPlate1);

cleanup:
    if (remote) git_remote_free(remote);
    if (repository) git_repository_free(repository);
    git_libgit2_shutdown();
    return available;
}

/**
 * looks for modified or untracked files in the repo
 */
bool PartsChecker::checkIfClean(const QString & repoPath,
                                const QString & shaFromDatabase,
                                git_repository * repository,
                                PartsCheckerResult & partsCheckerResult)
{
    // check local branch is master
    // check local git status is ok
    // check that last commit matches the one in the database

    git_reference *head = NULL;
    git_reference * resolved_head = NULL;
    git_ref_t ref_type;
    QString branchName;
    bool result = false;
    git_status_list *status_list = NULL;
    git_status_options status_options = GIT_STATUS_OPTIONS_INIT;
    int errorNumber = 0;
    QString currentSha;
    char buffer[GIT_OID_HEXSZ + 1] = {0};

    errorNumber++;
    int error = git_repository_head(&head, repository);
    if (error == GIT_EUNBORNBRANCH || error == GIT_ENOTFOUND) {
        // HEAD state changed, assume a git operation
        partsCheckerResult.partsCheckerError = PARTS_CHECKER_ERROR_USED_GIT;
        partsCheckerResult.errorMessage = sNotInMasterBranch.arg(repoPath).arg(errorNumber).arg(sBoilerPlate2);
        goto cleanup;
    }

    errorNumber++;
    if (error) {
        // still dealing with git_repository_head error
        partsCheckerResult.partsCheckerError = PARTS_CHECKER_ERROR_LOCAL_DAMAGE;
        partsCheckerResult.errorMessage = sDamaged.arg(repoPath).arg(errorNumber).arg(sBoilerPlate2);
        goto cleanup;
    }

    errorNumber++;
    branchName = git_reference_shorthand(head);
    if (branchName != "master") {
        partsCheckerResult.partsCheckerError = PARTS_CHECKER_ERROR_USED_GIT;
        partsCheckerResult.errorMessage = sNotInMasterBranch.arg(repoPath).arg(errorNumber).arg(sBoilerPlate2);
        goto cleanup;
    }

    errorNumber++;
    ref_type = git_reference_type(head);
    if (ref_type == GIT_REF_SYMBOLIC) {
        error = git_reference_resolve(&resolved_head, head);
        if (error) {
            partsCheckerResult.partsCheckerError = PARTS_CHECKER_ERROR_LOCAL_DAMAGE;
            partsCheckerResult.errorMessage = sDamaged.arg(repoPath).arg(errorNumber).arg(sBoilerPlate2);
            goto cleanup;
        }

        const git_oid * oid = git_reference_target(resolved_head);
        git_oid_fmt(buffer, oid);
        currentSha = buffer;

    }
    else if (ref_type == GIT_REF_OID) {
        // Get the hash
        const git_oid * oid = git_reference_target(head);
        git_oid_fmt(buffer, oid);
        currentSha = buffer;
    }
    else {
        partsCheckerResult.partsCheckerError = PARTS_CHECKER_ERROR_LOCAL_DAMAGE;
        partsCheckerResult.errorMessage = sDamaged.arg(repoPath).arg(errorNumber).arg(sBoilerPlate2);
        goto cleanup;
    }

    if (currentSha != shaFromDatabase) {
        partsCheckerResult.partsCheckerError = PARTS_CHECKER_ERROR_USED_GIT;
        partsCheckerResult.errorMessage = sModifiedByGit.arg(repoPath).arg(errorNumber).arg(sBoilerPlate2);
        goto cleanup;
    }

    errorNumber++;
    status_options.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS;
    status_options.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    error = git_status_list_new(&status_list, repository, &status_options);
    if (error) {
        partsCheckerResult.partsCheckerError = PARTS_CHECKER_ERROR_LOCAL_DAMAGE;
        partsCheckerResult.errorMessage = sDamaged.arg(repoPath).arg(errorNumber).arg(sBoilerPlate2);
        goto cleanup;
    }

    getChanges(status_list, partsCheckerResult);

    if (partsCheckerResult.unreadableFiles.count() > 0) {
        partsCheckerResult.partsCheckerError = PARTS_CHECKER_ERROR_LOCAL_DAMAGE;
        partsCheckerResult.errorMessage = sUnreadable.arg(repoPath).arg(sBoilerPlate1);
        goto cleanup;
    }

    if (partsCheckerResult.gitChangedFiles.count() > 0) {
        partsCheckerResult.partsCheckerError = PARTS_CHECKER_ERROR_USED_GIT;
        partsCheckerResult.errorMessage = sModifiedByGit.arg(repoPath).arg(errorNumber).arg(sBoilerPlate2);
        goto cleanup;
    }

    if (partsCheckerResult.changedFiles.count() > 0) {
        partsCheckerResult.partsCheckerError = PARTS_CHECKER_ERROR_LOCAL_MODS;
        partsCheckerResult.errorMessage = sModified.arg(repoPath);
        result = true;
        goto cleanup;
    }

    if (partsCheckerResult.untrackedFiles.count() > 0) {
        partsCheckerResult.partsCheckerError = PARTS_CHECKER_ERROR_LOCAL_MODS;
        partsCheckerResult.errorMessage = sModified.arg(repoPath);
        result = true;
        goto cleanup;
    }

    result = true;

cleanup:
    if (head) git_reference_free(head);
    if (resolved_head) git_reference_free(resolved_head);
    if (status_list) git_status_list_free(status_list);

    return result;
}

/**
 * gets the details about modified or new files in the repo
 */
void PartsChecker::getChanges(git_status_list *status_list, PartsCheckerResult & partsCheckerResult)
{
    // from https://libgit2.github.com/libgit2/ex/HEAD/status.html

    size_t i, maxi = git_status_list_entrycount(status_list);
    const git_status_entry *s;
    for (i = 0; i < maxi; ++i) {
        s = git_status_byindex(status_list, i);

        if (s->status == GIT_STATUS_CURRENT) {
            continue;
        }

        if (s->status == GIT_STATUS_IGNORED) {
            continue;
        }

        if (s->status == GIT_STATUS_WT_NEW) {
            partsCheckerResult.untrackedFiles << s->index_to_workdir->old_file.path;
            continue;
        }

        QString path;
        if (s->head_to_index) {
            path = s->head_to_index->old_file.path;
        }
        if (s->index_to_workdir) {
            if (path.isEmpty()) {
                path = s->index_to_workdir->old_file.path;
            }
        }

        if ((s->status & GIT_STATUS_INDEX_NEW)
                || (s->status & GIT_STATUS_INDEX_MODIFIED)
                || (s->status & GIT_STATUS_INDEX_DELETED)
                || (s->status & GIT_STATUS_INDEX_RENAMED)
                || (s->status & GIT_STATUS_INDEX_TYPECHANGE)
                || (s->status & GIT_STATUS_CONFLICTED))
        {
            partsCheckerResult.gitChangedFiles << path;
            continue;
        }

        if ((s->status & GIT_STATUS_WT_MODIFIED)
                || (s->status & GIT_STATUS_WT_DELETED)
                || (s->status & GIT_STATUS_WT_RENAMED)
                || (s->status & GIT_STATUS_WT_TYPECHANGE))
        {
            partsCheckerResult.changedFiles << path;
            continue;
        }

        if (s->status & GIT_STATUS_WT_UNREADABLE) {
            partsCheckerResult.unreadableFiles << path;
            continue;
        }

    }
}

/**
 * git reset --HARD and delete untracked files in the repo
 */
bool PartsChecker::cleanRepo(const QString & repoPath, const PartsCheckerResult & partsCheckerResult) {

    git_repository *repository = NULL;
    git_reference *head = NULL;
    bool result = false;
    git_oid oid;  /* the SHA1 for last commit */
    git_object *object = NULL;
    QDir repoDir(repoPath);

    git_libgit2_init();

    int error = git_repository_open(&repository, repoPath.toUtf8().constData());
    if (error) {
        goto cleanup;
    }

    error = git_reference_name_to_id(&oid, repository, "HEAD" );
    if (error) {
        goto cleanup;
    }


    error = git_object_lookup(&object, repository, &oid, GIT_OBJ_ANY);
    if (error) {
        goto cleanup;
    }


    error = git_reset(repository, object, GIT_RESET_HARD, NULL);
    if (error) {
        goto cleanup;
    }

    foreach (QString fileName, partsCheckerResult.untrackedFiles) {
        bool success = QFile::remove(repoDir.absoluteFilePath(fileName));
        if (!success) {
            goto cleanup;
        }
    }

    result = true;

cleanup:
    if (object) git_object_free(object);
    if (head) git_reference_free(head);
    if (repository) git_repository_free(repository);
    git_libgit2_shutdown();
    return result;
}

/**
 * gets the sha1 hash for the HEAD
 */
QString PartsChecker::getSha(const QString & repoPath) {

    QString sha;
    git_repository * repository = NULL;
    char buffer[GIT_OID_HEXSZ + 1] = {0};

    git_libgit2_init();

    int error = git_repository_open(&repository, repoPath.toUtf8().constData());
    if (error) {
        FMessageBox::warning(NULL, QObject::tr("Regenerating parts database"), QObject::tr("Unable to find parts git repository"));
        goto cleanup;
    }

    git_oid oid;  /* hold the SHA1 for last commit */
    /* resolve HEAD into a SHA1 */
    error = git_reference_name_to_id( &oid, repository, "HEAD" );
    if (error) {
        FMessageBox::warning(NULL, QObject::tr("Regenerating parts database"), QObject::tr("Unable to find parts git repository HEAD"));
        goto cleanup;
    }

    git_oid_fmt(buffer, &oid);
    sha = QString(buffer);

cleanup:
    git_repository_free(repository);
    git_libgit2_shutdown();

    return sha;
}

static int transfer_progress_cb(const git_transfer_progress *stats, void *payload)
{
    double progress = 0;
    if (stats->received_objects == stats->total_objects) {
        progress = 0.5 + (0.5 * stats->indexed_deltas / stats->total_deltas);
    }
    else if (stats->total_objects > 0) {
        progress = 0.5 * stats->received_objects / stats->total_objects;
    }
    if (payload) {
        PartsCheckerUpdateInterface * interface = static_cast<PartsCheckerUpdateInterface *>(payload);
        if (interface) {
            interface->updateProgress(progress);
        }
    }
    return 0;
}

/**
 * git pull origin master
 */
bool PartsChecker::updateParts(const QString & repoPath, const QString & remoteSha, PartsCheckerUpdateInterface * interface) {
    // from https://github.com/libgit2/libgit2/blob/master/examples/network/fetch.c
    // useful for testing: http://stackoverflow.com/questions/3489173/how-to-clone-git-repository-with-specific-revision-changeset

    static char *refspec = const_cast<char *>("master:refs/heads/master");

    bool ok = false;
    git_remote *remote = NULL;
    git_repository * repository = NULL;
    git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;

    if (remoteSha.isEmpty()) {
        DebugDialog::debug("Missing remoteSha");
        return false;
    }

    git_libgit2_init();

    int error = git_repository_open(&repository, repoPath.toUtf8().constData());
    if (error) {
        DebugDialog::debug("unable to open repo " + repoPath);
        goto cleanup;
    }

    // Find the remote by name
    error = git_remote_lookup(&remote, repository, "origin");
    if (error) {
        DebugDialog::debug("unable to lookup repo " + repoPath);
        goto cleanup;
    }

    // Set up the callbacks
    fetch_opts.callbacks.transfer_progress = transfer_progress_cb;
    fetch_opts.callbacks.payload = interface;

    /**
     * Perform the fetch with the configured refspecs from the
     * config. Update the reflog for the updated references with
     * "fetch".
     */

    git_strarray refspecs;
    refspecs.count = 1;
    refspecs.strings = &refspec;
    error = git_remote_fetch(remote, NULL /* &refspecs */, &fetch_opts, "fetch");
    if (error) {
        DebugDialog::debug("unable to fetch " + repoPath);
        goto cleanup;
    }

    // for debugging
    //const git_transfer_progress *stats;
    //stats = git_remote_stats(remote);

    error = doMerge(repository, remoteSha);
    ok = (error == 0);

cleanup:
    git_remote_free(remote);
    git_repository_free(repository);
    git_libgit2_shutdown();
    return ok;
}


/**
 * merge after the fetch
 */
int PartsChecker::doMerge(git_repository * repository, const QString & remoteSha) {
    // from https://github.com/libgit2/libgit2/blob/master/tests/merge/workdir/simple.c
    // https://github.com/MichaelBoselowitz/pygit2-examples/blob/68e889e50a592d30ab4105a2e7b9f28fac7324c8/examples.py#L58

    git_oid their_oids[1];
    git_annotated_commit *their_heads[1];
    git_merge_options merge_options = GIT_MERGE_OPTIONS_INIT;
    git_checkout_options checkout_options = GIT_CHECKOUT_OPTIONS_INIT;
    bool afterMerge = false;
    git_commit *head_commit = NULL;
    git_commit *remote_commit = NULL;
    git_signature * signature = NULL;
    git_tree * saved_tree = NULL;
    git_reference * master_reference = NULL;
    git_reference * new_master_reference = NULL;
    git_reference * head_reference = NULL;
    git_reference * new_head_reference = NULL;
    git_index * index = NULL;
    //char buffer[GIT_OID_HEXSZ + 1] = {0};

    merge_options.file_favor = GIT_MERGE_FILE_FAVOR_THEIRS;
    checkout_options.checkout_strategy = GIT_CHECKOUT_FORCE;

    int error = git_oid_fromstr(&their_oids[0], remoteSha.toUtf8().constData());
    if (error) {
        DebugDialog::debug("unable to convert their oid");
        goto cleanup;
    }

    error = git_annotated_commit_lookup(&their_heads[0], repository, &their_oids[0]);
    if (error) {
        DebugDialog::debug("unable to find their commit");
        goto cleanup;
    }

    git_merge_analysis_t merge_analysis_t;
    git_merge_preference_t merge_preference_t;
    error = git_merge_analysis(&merge_analysis_t, &merge_preference_t, repository, (const git_annotated_commit **) their_heads, 1);
    if (error) {
        DebugDialog::debug("analysis failed");
        goto cleanup;
    }

    if (merge_analysis_t & GIT_MERGE_ANALYSIS_UP_TO_DATE) {
        // no more work needs to be done
        DebugDialog::debug("already up to date");
        goto cleanup;
    }

    error = git_commit_lookup(&remote_commit, repository, &their_oids[0]);
    if (error) {
        DebugDialog::debug("unable to find remote parent commit");
        goto cleanup;
    }

    if (merge_analysis_t & GIT_MERGE_ANALYSIS_FASTFORWARD || merge_analysis_t & GIT_MERGE_ANALYSIS_UNBORN) {
        error = git_checkout_tree(repository, (git_object *) remote_commit, &checkout_options);
        if (error) {
            DebugDialog::debug("ff checkout failed");
            goto cleanup;
        }

        error = git_reference_lookup(&master_reference, repository, "refs/heads/master");
        if (error) {
            DebugDialog::debug("master ref lookup failed");
            goto cleanup;
        }

        error = git_reference_set_target(&new_master_reference, master_reference, &their_oids[0], "Update parts");
        if (error) {
            DebugDialog::debug("ref set target failed");
            goto cleanup;
        }

        error = git_repository_head(&head_reference, repository);
        if (error) {
            DebugDialog::debug("head ref lookup failed");
            goto cleanup;
        }

        error = git_reference_set_target(&new_head_reference, head_reference, &their_oids[0], "Update parts");
        if (error) {
            DebugDialog::debug("head ref set target failed");
            goto cleanup;
        }

        goto cleanup;
    }

    error = git_merge(repository, (const git_annotated_commit **) their_heads, 1, &merge_options, &checkout_options);
    afterMerge = true;
    if (error) {
        DebugDialog::debug("merge failed");
        goto cleanup;
    }

    error = git_repository_index(&index, repository);
    if (error) {
        DebugDialog::debug("unable to get index");
        goto cleanup;
    }

    // we are ignoring conflicts...

    error = git_signature_now(&signature, "Fritzing", "info@fritzing.org");
    if (error) {
        DebugDialog::debug("signature failed");
        goto cleanup;
    }

    git_oid saved_oid;
    error = git_index_write_tree(&saved_oid, index);
    if (error) {
        DebugDialog::debug("save index failed");
        goto cleanup;
    }

    error = git_tree_lookup(&saved_tree, repository, &saved_oid);
    if (error) {
        DebugDialog::debug("save tree lookup failed");
        goto cleanup;
    }

    error = git_repository_head(&head_reference, repository);
    if (error) {
        DebugDialog::debug("head ref lookup failed");
        goto cleanup;
    }

    {
        // put initialization in scope due to compiler complaints
        const git_oid * head_oid = git_reference_target(head_reference);
        error = git_commit_lookup(&head_commit, repository, head_oid);
        if (error) {
            DebugDialog::debug("head lookup failed");
            goto cleanup;
        }
    }

    git_oid new_commit_id;
    error = git_commit_create_v(&new_commit_id, repository, "HEAD", signature, signature,
                              NULL, "Update parts", saved_tree, 2, head_commit, remote_commit);
    if (error) {
        DebugDialog::debug("final commit failed");
        goto cleanup;
    }

cleanup:
    if (afterMerge) {
        git_repository_state_cleanup(repository);
    }

    git_index_free(index);
    git_reference_free(master_reference);
    git_reference_free(new_master_reference);
    git_reference_free(head_reference);
    git_reference_free(new_head_reference);
    git_signature_free(signature);
    git_tree_free(saved_tree);
    git_commit_free(head_commit);
    git_commit_free(remote_commit);
    git_annotated_commit_free(their_heads[0]);

    return error;
}

