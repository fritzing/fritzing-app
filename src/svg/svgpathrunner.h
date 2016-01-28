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

#ifndef SVGPATHRUNNER_H
#define SVGPATHRUNNER_H

#include <QVariant>
#include <QVector>

struct PathCommand {
	bool relative;
	int argCount;
	QChar command;
};

class SVGPathRunner : public QObject
{
	Q_OBJECT

public:
    SVGPathRunner();
    ~SVGPathRunner();

public:
	bool runPath(QVector<QVariant> & pathData, void * userData);

signals:
	// note: must connect to this signal via Qt::DirectConnection since args is modified immediately after the signal
	void commandSignal(QChar command, bool relative, QList<double> & args, void * userData);

protected:
	static void initStates();

protected:
	static QHash<QChar, PathCommand *> pathCommands;

};

#endif // SVGPATHRUNNER_H
