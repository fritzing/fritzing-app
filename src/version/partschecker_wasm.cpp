/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2026 Fritzing

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

// PartsChecker for the WebAssembly build, which has no libgit2.
// The parts library is bundled read-only with a prebuilt parts.db, so there is
// no repository to inspect or update.

#include "partschecker.h"

#include <QObject>

void PartsCheckerResult::reset() {
	partsCheckerError = PARTS_CHECKER_ERROR_NONE;
	errorMessage.clear();
	untrackedFiles.clear();
	changedFiles.clear();
	gitChangedFiles.clear();
	unreadableFiles.clear();
}

QString PartsChecker::getSha(const QString &)
{
	// Only reached when parts.db is missing; any non-empty value lets the parts folder be scanned
	return QStringLiteral("wasm");
}

bool PartsChecker::newPartsAvailable(const QString &, const QString &, bool, QString &, PartsCheckerResult & partsCheckerResult)
{
	partsCheckerResult.reset();
	// shown by the update dialog instead of "no new parts"
	partsCheckerResult.errorMessage = QObject::tr("<p>No new releases found.</p>"
	                                              "<p>The parts library is part of the Fritzing web app. To get newer parts, "
	                                              "rebuild the Docker image with <code>--build-arg PARTS_COMMIT=latest</code>.</p>");
	return false;
}

bool PartsChecker::updateParts(const QString &, const QString &, PartsCheckerUpdateInterface *)
{
	return false;
}

bool PartsChecker::cleanRepo(const QString &, const PartsCheckerResult &)
{
	return false;
}
