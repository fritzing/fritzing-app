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


#include <QTextStream>
#include <QFile>
#include <QtDebug>

#include "fapplication.h"
#include "version/version.h"
#include "debugdialog.h"
#include "utils/folderutils.h"

#ifdef Q_OS_WIN
#ifndef QT_NO_DEBUG
#include "windows.h"
#endif
#endif

QtMessageHandler originalMsgHandler;

void writeCrashMessage(const char * msg) {
	QString path = FolderUtils::getTopLevelUserDataStorePath();
	path += "/fritzingcrash.txt";
	QFile file(path);
	if (file.open(QIODevice::Append | QIODevice::Text)) {
		QTextStream out(&file);
		out << QString(msg) << "\n";
		file.close();
	}
}

void writeCrashMessage(const QString & msg) {
	writeCrashMessage(msg.toStdString().c_str());
}

void fMessageHandler(QtMsgType type, const QMessageLogContext & context, const QString & msg)
{
	if (type == QtFatalMsg) {
		writeCrashMessage(msg);
	}
	originalMsgHandler(type, context, msg);
}


int main(int argc, char *argv[])
{
#ifdef _MSC_VER // just for the MS compiler
#define WIN_CHECK_LEAKS
#endif

#ifdef Q_OS_WIN
	originalMsgHandler = qInstallMessageHandler(fMessageHandler);
#ifndef QT_NO_DEBUG
#ifdef WIN_CHECK_LEAKS
	HANDLE hLogFile;
	QString path = FolderUtils::getTopLevelUserDataStorePath() + "/fritzing_leak_log.txt";
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
	//_CrtSetBreakAlloc(323809);  // sets a break when this memory id is allocated
#endif
#endif
#endif

	int result = 0;
	try {
		//QApplication::setGraphicsSystem("raster");
		QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
		FApplication * app = new FApplication(argc, argv);
		switch (app->init()) {
		case FInitResultNormal: {
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
			break;
		}
		case FInitResultHelp: {
			QTextStream cout(stdout);
			cout <<
			     "Usage: Fritzing [-f FOLDER] [OPTION]... [FILE]\n"
			     "\n"
			     "Fritzing is an open-source hardware initiative that makes electronics accessible as a creative material for anyone. "
			     "We offer this software tool, a community website and services in the spirit of Processing and Arduino, "
			     "fostering a creative ecosystem that allows users to document their prototypes, share them with others, "
			     "teach electronics in a classroom, and layout and manufacture professional PCBs.\n"
			     "\n"
			     "For more information on Fritzing and its related activities visit <https://fritzing.org>.\n"
			     "\n"
			     "Options:\n"
			     "\n"
			     "User options:\n"
			     "  -d, -debug                    run Fritzing in debug mode, providing additional debug information\n"
			     //" drc filename : runs a design rule check on the given sketch file\n"
			     "  -f, -folder FOLDER            use Fritzing parts, sketches, bins and translations in folders under FOLDER\n"
			     "  -geda FOLDER                  convert all gEDA footprint (.fp) files in FOLDER to Fritzing SVGs\n"
			     "  -g, -gerber FOLDER            export all sketches in FOLDER to Gerber, in the same folder\n"
			     "  -h, -help                     print this help message\n"
			     "  -kicad FOLDER                 convert all Kicad footprint (.mod) files in FOLDER to Fritzing SVGs\n"
			     "  -kicadschematic FOLDER        convert all Kicad schematic (.lib) files in FOLDER to Fritzing SVGs\n"
			     "  -port NUMBER                  run Fritzing as a server process on port NUMBER\n"
			     "  -svg FOLDER                   export all sketches in FOLDER to SVGs of all views, in the same folder\n"
			     "\n"
			     "Administrator option:\n"
			     "  -db, -database FILE           rebuild the internal parts database FILE\n"
			     "\n"
			     "Developer options:\n"
			     "  -e, -examples FOLDER          prepare all sketches in FOLDER to be included as examples\n"
			     "  -ep FILE                      add menu item for external process using executable FILE\n"
			     "  -eparg ARGS                   with -ep, external process arguments ARGS\n"
			     "  -epname NAME                  with -ep, external process menu item NAME\n"
			     "\n"
			     "The -geda, -kicad, -kicadschematic, -gerber SVG options all exit Fritzing after the conversion process is complete;\n"
			     "these options are mutually exclusive.\n"
			     "\n"
#ifndef PKGDATADIR
			     "Usually, the Fritzing executable is stored in the same folder that contains the parts/bins/sketches/translations folders,\n"
			     "or the executable is in a child folder of the p/b/s/t folder.\n"
			     "If this is not the case, use the -f option to point to the p/b/s/t folder.\n"
			     "\n"
#endif
			     "The -ep option creates a menu item to launch an external process,\n"
			     "and puts the standard output of that process into a dialog window in Fritzing.\n"
			     "The process path follows the -ep argument; the name of the menu item follows the -epname argument;\n"
			     "and any arguments to pass to the external process are provided in the -eparg arguments.\n"
			     "\n"
			     "Report bugs or suggest improvements using the issue tracker <https://github.com/fritzing/fritzing-app/issues> or the user forum <https://forum.fritzing.org>.\n";
			break;
		}
		case FInitResultVersion: {
			QTextStream cout(stdout);
			cout <<
			     "Fritzing " << Version::versionString() << "\n";
			break;
		}
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
