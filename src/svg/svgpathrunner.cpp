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

#include <QDebug>
#include "svgpathrunner.h"

QHash<QChar, PathCommand *> SVGPathRunner::pathCommands;

SVGPathRunner::SVGPathRunner()
{
	if (pathCommands.size() == 0) {
		initStates();
	}
}

SVGPathRunner::~SVGPathRunner()
{
}

bool SVGPathRunner::runPath(QVector<QVariant> & pathData, void * userData) {
	PathCommand * currentCommand = NULL;
	QList<double> args;

	foreach (QVariant variant, pathData) {
		if (variant.type() == QVariant::Char) {
			PathCommand * newCommand = pathCommands.value(variant.toChar(), NULL);
			if (newCommand == NULL) return false;

			if (currentCommand != NULL) {
				if (currentCommand->argCount == 0) {
					if (args.count() != 0) return false;
				}
				else if (args.count() % currentCommand->argCount != 0) return false;

				emit commandSignal(currentCommand->command, currentCommand->relative, args, userData);
			}

			args.clear();
			currentCommand = newCommand;
		}
		else if (variant.type() == QVariant::Double) {
			if (currentCommand == NULL) return false;
			args.append(variant.toDouble());
		}
	}

	if (currentCommand != NULL) {
		if (currentCommand->argCount == 0) {
			if (args.count() != 0) return false;
		}
		else if (args.count() % currentCommand->argCount != 0) return false;

		emit commandSignal(currentCommand->command, currentCommand->relative, args, userData);
	}

	return true;
}

void SVGPathRunner::initStates() {
	pathCommands.clear();

	PathCommand * pathCommand = new PathCommand;
	pathCommand->command = 'M';
	pathCommand->relative = false;
	pathCommand->argCount = 2;
	pathCommands.insert(pathCommand->command, pathCommand);

	pathCommand = new PathCommand;
	pathCommand->command = 'm';
	pathCommand->relative = true;
	pathCommand->argCount = 2;
	pathCommands.insert(pathCommand->command, pathCommand);

	pathCommand = new PathCommand;
	pathCommand->command = 'A';
	pathCommand->relative = false;
	pathCommand->argCount = 7;
	pathCommands.insert(pathCommand->command, pathCommand);

	pathCommand = new PathCommand;
	pathCommand->command = 'a';
	pathCommand->relative = true;
	pathCommand->argCount = 7;
	pathCommands.insert(pathCommand->command, pathCommand);

	pathCommand = new PathCommand;
	pathCommand->command = 'Z';
	pathCommand->relative = false;
	pathCommand->argCount = 0;
	pathCommands.insert(pathCommand->command, pathCommand);

	pathCommand = new PathCommand;
	pathCommand->command = 'z';
	pathCommand->relative = true;
	pathCommand->argCount = 0;
	pathCommands.insert(pathCommand->command, pathCommand);

	pathCommand = new PathCommand;
	pathCommand->command = 'L';
	pathCommand->relative = false;
	pathCommand->argCount = 2;
	pathCommands.insert(pathCommand->command, pathCommand);

	pathCommand = new PathCommand;
	pathCommand->command = 'l';
	pathCommand->relative = true;
	pathCommand->argCount = 2;
	pathCommands.insert(pathCommand->command, pathCommand);

	pathCommand = new PathCommand;
	pathCommand->command = 'H';
	pathCommand->relative = false;
	pathCommand->argCount = 1;
	pathCommands.insert(pathCommand->command, pathCommand);

	pathCommand = new PathCommand;
	pathCommand->command = 'h';
	pathCommand->relative = true;
	pathCommand->argCount = 1;
	pathCommands.insert(pathCommand->command, pathCommand);

	pathCommand = new PathCommand;
	pathCommand->command = 'V';
	pathCommand->relative = false;
	pathCommand->argCount = 1;
	pathCommands.insert(pathCommand->command, pathCommand);

	pathCommand = new PathCommand;
	pathCommand->command = 'v';
	pathCommand->relative = true;
	pathCommand->argCount = 1;
	pathCommands.insert(pathCommand->command, pathCommand);

	pathCommand = new PathCommand;
	pathCommand->command = 'C';
	pathCommand->relative = false;
	pathCommand->argCount = 6;
	pathCommands.insert(pathCommand->command, pathCommand);

	pathCommand = new PathCommand;
	pathCommand->command = 'c';
	pathCommand->relative = true;
	pathCommand->argCount = 6;
	pathCommands.insert(pathCommand->command, pathCommand);

	pathCommand = new PathCommand;
	pathCommand->command = 'S';
	pathCommand->relative = false;
	pathCommand->argCount = 4;
	pathCommands.insert(pathCommand->command, pathCommand);

	pathCommand = new PathCommand;
	pathCommand->command = 's';
	pathCommand->relative = true;
	pathCommand->argCount = 4;
	pathCommands.insert(pathCommand->command, pathCommand);

	pathCommand = new PathCommand;
	pathCommand->command = 'Q';
	pathCommand->relative = false;
	pathCommand->argCount = 4;
	pathCommands.insert(pathCommand->command, pathCommand);

	pathCommand = new PathCommand;
	pathCommand->command = 'q';
	pathCommand->relative = true;
	pathCommand->argCount = 4;
	pathCommands.insert(pathCommand->command, pathCommand);

	pathCommand = new PathCommand;
	pathCommand->command = 'T';
	pathCommand->relative = false;
	pathCommand->argCount = 2;
	pathCommands.insert(pathCommand->command, pathCommand);

	pathCommand = new PathCommand;
	pathCommand->command = 't';
	pathCommand->relative = true;
	pathCommand->argCount = 2;
	pathCommands.insert(pathCommand->command, pathCommand);

}
