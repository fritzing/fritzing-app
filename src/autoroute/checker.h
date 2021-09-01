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

#ifndef CHECKER_H
#define CHECKER_H

#include <QString>

#include "../mainwindow/mainwindow.h"

class Checker
{
public:
	static int checkDonuts(MainWindow *, bool displayMessage);
	static int checkText(MainWindow *, bool displayMessage);

protected:
	static void writeCheckerOutput(const QString & message);
	static void collectFilenames(const QString & filenames);
};

#endif
