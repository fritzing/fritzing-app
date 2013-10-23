/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2012 Fachhochschule Potsdam - http://fh-potsdam.de

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


#include <QTextStream>
#include <QFile>
#include <QtDebug>

#include "fapplication.h"
#include "debugdialog.h"
#include "utils/folderutils.h"

#ifdef Q_WS_WIN
#ifndef QT_NO_DEBUG
#include "windows.h"
#endif
#endif

QtMsgHandler originalMsgHandler;

void writeCrashMessage(const char * msg) {
	QString path = FolderUtils::getUserDataStorePath("");
	path += "/fritzingcrash.txt";
	QFile file(path);
	if (file.open(QIODevice::Append | QIODevice::Text)) {
		QTextStream out(&file);
		out << QString(msg) << "\n";
		file.close();
	}
}

void fMessageHandler(QtMsgType type, const char *msg)
 {
	switch (type) {
		case QtDebugMsg:
			originalMsgHandler(type, msg);
			break;
		case QtWarningMsg:
			originalMsgHandler(type, msg);
			break;
		case QtCriticalMsg:
			originalMsgHandler(type, msg);
			break;
		case QtFatalMsg:
			{
				writeCrashMessage(msg);
			}

			// don't abort
			originalMsgHandler(QtWarningMsg, msg);
	}
 }



int main(int argc, char *argv[])
{
#ifdef _MSC_VER // just for the MS compiler
#define WIN_CHECK_LEAKS
#endif

#ifdef Q_WS_WIN
	originalMsgHandler = qInstallMsgHandler(fMessageHandler);
#ifndef QT_NO_DEBUG
#ifdef WIN_CHECK_LEAKS
	HANDLE hLogFile;
    QString path = FolderUtils::getUserDataStorePath("") + "/fritzing_leak_log.txt";
    std::wstring wstr = path.toStdWString();
    LPCWSTR ptr = wstr.c_str();
	hLogFile = CreateFile(ptr, GENERIC_WRITE,
		  FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
		  FILE_ATTRIBUTE_NORMAL, NULL);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, hLogFile);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, hLogFile);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, hLogFile);
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	//_CrtSetBreakAlloc(323809);					// sets a break when this memory id is allocated
#endif
#endif
#endif

	int result = 0;
	try {
		//QApplication::setGraphicsSystem("raster");
		FApplication * app = new FApplication(argc, argv);
		if (app->init()) {
			//DebugDialog::setDebugLevel(DebugDialog::Error);
			if (app->runAsService()) {
				// for example: -g C:\Users\jonathan\fritzing2\fz\Test_multiple.fz -go C:\Users\jonathan\fritzing2\fz\gerber
				result = app->serviceStartup();
                if (result == 1) {
					result = app->exec();
                }
			}
			else {
				result = app->startup();
				if (result == 0) {
					result = app->exec();
				}
			}
			app->finish();
		}
		else {
			qDebug() << "\n"
				"usage:\n"
				"\n"
				"[path to Fritzing file to be loaded]*\n"
				"[-f {path to folder containing Fritzing parts/sketches/bins/translations folders}]\n"
				"[-geda {path to folder containing gEDA footprint (.fp) files to be converted to Fritzing SVGs}]\n"
				"[-kicad {path to folder containing Kicad footprint (.mod) files to be converted to Fritzing SVGs}]\n"
				"[-kicadschematic {path to folder containing Kicad schematic (.lib) files to be converted to Fritzing SVGs}]\n"
				"[-gerber {path to folder containing fzz files to export; gerber output files will be placed in that same folder}]\n"
				"[-svg {path to folder containing fzz files to export; gerber output files will be placed in that same folder}]\n"
				"[-ep {external process path} [-eparg {argument passed to external process}]* -epname {name for the menu item}]\n"
				"\n"
				"The -geda/-kicad/-kicadschematic/-gerber/svg options all exit Fritzing after the conversion process is complete;\n"
				"these options are mutually exclusive.\n"
				"\n"
				"Usually, the Fritzing executable is stored in the same folder that contains the parts/bins/sketches/translations folders,\n"
				"or the executable is in a child folder of the p/b/s/t folder.\n"
				"If this is not the case, use the -f option to point to the p/b/s/t folder.\n"
				"\n"
				"The -ep option creates a menu item to launch an external process,\n"
				"and puts the standard output of that process into a dialog window in Fritzing.\n"
				"The process path follows the -ep argument; the name of the menu item follows the -epname argument;\n"
				"and any arguments to pass to the external process are provided in the -eparg argments.\n"
				"\n";
		}
		delete app;
	}
	catch (char const *str) {
		writeCrashMessage(str);
	}
	catch (...) {
		result = -1;
	}

	return result;
}

