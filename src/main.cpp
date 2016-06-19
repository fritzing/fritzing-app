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

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
QtMsgHandler originalMsgHandler;
#define ORIGINAL_MESSAGE_HANDLER(TYPE, MSG) originalMsgHandler((TYPE), (MSG))
#else
QtMessageHandler originalMsgHandler;
#define ORIGINAL_MESSAGE_HANDLER(TYPE, MSG) originalMsgHandler((TYPE), context, (MSG))
#endif

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

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
void writeCrashMessage(const QString & msg) {
        writeCrashMessage(msg.toStdString().c_str());
}
#endif

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
void fMessageHandler(QtMsgType type, const char *msg)
#else
void fMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString & msg)
#endif
{
	switch (type) {
		case QtDebugMsg:
			ORIGINAL_MESSAGE_HANDLER(type, msg);
			break;
		case QtWarningMsg:
			ORIGINAL_MESSAGE_HANDLER(type, msg);
			break;
		case QtCriticalMsg:
			ORIGINAL_MESSAGE_HANDLER(type, msg);
			break;
		case QtFatalMsg:
			{
				writeCrashMessage(msg);
			}

			// don't abort
			ORIGINAL_MESSAGE_HANDLER(QtWarningMsg, msg);
	}
 }



int main(int argc, char *argv[])
{
#ifdef _MSC_VER // just for the MS compiler
#define WIN_CHECK_LEAKS
#endif

#ifdef Q_OS_WIN
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
	originalMsgHandler = qInstallMsgHandler(fMessageHandler);
#else
    originalMsgHandler = qInstallMessageHandler(fMessageHandler);
#endif
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
	//_CrtSetBreakAlloc(323809);					// sets a break when this memory id is allocated
#endif
#endif
#endif

	int result = 0;
	try {
		//QApplication::setGraphicsSystem("raster");
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
		QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
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
			qDebug() <<
                "Fritzing version" << Version::versionString() << "- Qt version" << QT_VERSION_STR << "\n"
                "\n"
                "usage: fritzing [-d] [-f path] filename\n"
                "       fritzing [-f path] -geda folder\n"
                "       fritzing [-f path] -gerber folder\n"
                "       fritzing [-f path] -kicad folder\n"
                "       fritzing [-f path] -kicadschematic folder\n"
                "       fritzing [-f path] -svg folder\n"
                "       fritzing [-f path] -port number\n"
                "\n"
                "user options:\n"
                "  d,debug            :  runs Fritzing in debug mode, providing additional debug information\n"
                //"  drc filename       :  runs a design rule check on the given sketch file\n"
                "  f,folder           :  path to folder containing Fritzing parts, sketches, bins, & translations folders}]\n"
                "  geda path          :  converts all gEDA footprint (.fp) files in folder <path> to Fritzing SVGs}]\n"
                "  g,gerber path      :  exports all sketches in folder <path> to Gerber, in the same folder\n"
                "  h,help             :  print this help message\n"
                "  kicad path         :  converts all Kicad footprint (.mod) files in folder <path> to Fritzing SVGs}]\n"
                "  kicadschematic path:  converts all Kicad schematic (.lib) files in folder <path> to Fritzing SVGs}]\n"
                "  port               :  runs Fritzing as a server process under <port>\n"
                "  svg path           :  exports all sketches in folder <path> to SVGs of all views, in the same folder\n"
                "\n"
                "developer options:\n"
                "  db path            :  rebuilds the internal parts database at the given path\n"
                "  e,examples path    :  prepares all sketches in the folder to be included as examples\n"
                "  ep path            :  external process at <path>\n"
                "  eparg args         :  external process arguments\n"
                "  epname name        :  external process menu item name\n"
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
				"and any arguments to pass to the external process are provided in the -eparg argments.";
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
