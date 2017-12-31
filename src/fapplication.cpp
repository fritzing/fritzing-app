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

$Revision: 6975 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-19 12:51:22 +0200 (Fr, 19. Apr 2013) $

********************************************************************/

#include "fapplication.h"
#include "debugdialog.h"
#include "utils/misc.h"
#include "mainwindow/mainwindow.h"
#include "fsplashscreen.h"
#include "version/version.h"
#include "dialogs/prefsdialog.h"
#include "fsvgrenderer.h"
#include "version/versionchecker.h"
#include "version/updatedialog.h"
#include "itemdrag.h"
#include "items/wire.h"
#include "partsbinpalette/binmanager/binmanager.h"
#include "help/tipsandtricks.h"
#include "utils/folderutils.h"
#include "utils/lockmanager.h"
#include "utils/fmessagebox.h"
#include "dialogs/translatorlistmodel.h"
#include "partsbinpalette/partsbinview.h"
#include "partsbinpalette/svgiconwidget.h"
#include "partsbinpalette/partsbinpalettewidget.h"
#include "items/moduleidnames.h"
#include "partsbinpalette/searchlineedit.h"
#include "utils/ratsnestcolors.h"
#include "utils/cursormaster.h"
#include "utils/textutils.h"
#include "utils/graphicsutils.h"
#include "infoview/htmlinfoview.h"
#include "svg/gedaelement2svg.h"
#include "svg/kicadmodule2svg.h"
#include "svg/kicadschematic2svg.h"
#include "svg/gerbergenerator.h"
#include "installedfonts.h"
#include "items/pinheader.h"
#include "items/partfactory.h"
#include "items/propertydef.h"
#include "dialogs/recoverydialog.h"
#include "processeventblocker.h"
#include "autoroute/panelizer.h"
#include "sketch/sketchwidget.h"
#include "sketch/pcbsketchwidget.h"
#include "help/firsttimehelpdialog.h"
#include "help/aboutbox.h"
#include "version/partschecker.h"

// dependency injection :P
#include "referencemodel/sqlitereferencemodel.h"
#define CurrentReferenceModel SqliteReferenceModel

#include <QSettings>
#include <QKeyEvent>
#include <QFileInfo>
#include <QDesktopServices>
#include <QLocale>
#include <QFileOpenEvent>
#include <QThread>
#include <QMessageBox>
#include <QTextStream>
#include <QFontDatabase>
#include <QtDebug>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QMultiHash>
#include <QTemporaryFile>
#include <QDir>
#include <time.h>

#ifdef LINUX_32
#define PLATFORM_NAME "linux-32bit"
#endif
#ifdef LINUX_64
#define PLATFORM_NAME "linux-64bit"
#endif
#ifdef Q_OS_WIN
#ifdef WIN64
#define PLATFORM_NAME "windows-64bit"
#else
#define PLATFORM_NAME "windows"
#endif
#endif
#ifdef Q_OS_MAC
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)) || defined(QT_MAC_USE_COCOA)
#define PLATFORM_NAME "mac-os-x-105"
#else
#define PLATFORM_NAME "mac-os-x-104"
#endif
#endif

#ifdef Q_OS_WIN
#ifndef QT_NO_DEBUG
#define WIN_DEBUG
#endif
#endif

static const double LoadProgressStart = 0.085;
static const double LoadProgressEnd = 0.6;


////////////////////////////////////////////////////

FServer::FServer(QObject *parent)
    : QTcpServer(parent)
{
}

void FServer::incomingConnection(qintptr socketDescriptor)
{
	DebugDialog::debug("incomingConnection called");
    emit newConnection(socketDescriptor);
}

////////////////////////////////////////////////////

QMutex FServerThread::m_busy;

FServerThread::FServerThread(qintptr socketDescriptor, QObject *parent) : QThread(parent), m_socketDescriptor(socketDescriptor)
{
}

 void FServerThread::run()
 {
    QTcpSocket * socket = new QTcpSocket();
    if (!socket->setSocketDescriptor(m_socketDescriptor)) {
        emit error(socket->error());
        DebugDialog::debug(QString("Socket error %1 %2").arg(socket->error()).arg(socket->errorString()));
        socket->deleteLater();
        return;
    }

    socket->waitForReadyRead();
    QString header;
    while (socket->canReadLine()) {
        header += socket->readLine();
    }

    DebugDialog::debug("header " + header);

    QStringList tokens = header.split(QRegExp("[ \r\n][ \r\n]*"), QString::SkipEmptyParts);
    if (tokens.count() <= 0) {
        writeResponse(socket, 400, "Bad Request", "", "");
        return;
    }

    if (tokens[0] != "GET") {
        writeResponse(socket, 405, "Method Not Allowed", "", "");
        return;
    }

    if (tokens.count() < 2) {
        writeResponse(socket, 400, "Bad Request", "", "");
        return;
    }

    QStringList params = tokens.at(1).split("/", QString::SkipEmptyParts);
    QString command = params.takeFirst();
    if (params.count() == 0) {
        writeResponse(socket, 400, "Bad Request", "", "");
        return;
    }

    QString subFolder = params.join("/");
	bool fixSubFolder = false;
    QString mimeType;
    if (command == "svg") {
		 mimeType = "image/svg+xml";
    }
    else if (command == "gerber") {
    }
    else if (command == "svg-tcp") {
		fixSubFolder = true;
    }
    else if (command == "gerber-tcp") {
		fixSubFolder = true;
    }
    else {
        writeResponse(socket, 400, "Bad Request", "", "");
        return;
    }

	if (fixSubFolder) {
		// replace "/" that was removed from "http:/blah" above
		int ix = subFolder.indexOf(":/");
		if (ix >= 0) {
			subFolder.insert(ix + 1, "/");
		}
	}

    int waitInterval = 100;     // 100ms to wait
    int timeoutSeconds = 2 * 60;    // timeout after 2 minutes
    int attempts = timeoutSeconds * 1000 / waitInterval;  // timeout a
    bool gotLock = false;
    for (int i = 0; i < attempts; i++) {
        if (m_busy.tryLock()) {
            gotLock = true;
            break;
        }
    }

    if (!gotLock) {
        writeResponse(socket, 503, "Service Unavailable", "", "Server busy.");
        return;
    }

    DebugDialog::debug(QString("emitting do command %1 %2").arg(command).arg(subFolder));
    QString result;
	int status;
    emit doCommand(command, subFolder, result, status);

    m_busy.unlock();

	if (status != 200) {
		writeResponse(socket, status, "failed", "", result);
	}
	else if (command.endsWith("tcp")) {
		QString filename = result;
		mimeType = "application/zip";
		QFile file(filename);
		if (file.open(QFile::ReadOnly)) {
			QString response = QString("HTTP/1.0 %1 %2\r\n").arg(200).arg("ok");
			response += QString("Content-Type: %1; charset=\"utf-8\"\r\n").arg(mimeType);
			response += QString("Content-Length: %1\r\n").arg(file.size());
			response += QString("\r\n");    
			socket->write(response.toUtf8());
			int buffersize = 8192;
			long remaining = file.size();
			while (remaining >= buffersize) {
				QByteArray bytes = file.read(buffersize);
				socket->write(bytes);
				remaining -= buffersize;
			}
			if (remaining > 0) {
				QByteArray bytes = file.read(remaining);
				socket->write(bytes);
			}
			socket->disconnectFromHost();
			socket->waitForDisconnected();
			socket->deleteLater();
			file.close();
		}
		else {
			writeResponse(socket, 500, "failed", "", "local zip failure (2)");
		}

		QFileInfo info(filename);
		QDir dir = info.dir();
		FolderUtils::rmdir(dir);
	}
	else {
		writeResponse(socket, 200, "Ok", mimeType, result);
	}
}

void FServerThread::writeResponse(QTcpSocket * socket, int code, const QString & codeString, const QString & mimeType, const QString & message) 
{
    QString type = mimeType;
    if (type.isEmpty()) type = "text/plain";
    QString response = QString("HTTP/1.0 %1 %2\r\n").arg(code).arg(codeString);
    response += QString("Content-Type: %1; charset=\"utf-8\"\r\n").arg(type);
    response += QString("Content-Length: %1\r\n").arg(message.count());
    response += QString("\r\n%1").arg(message);
       
    socket->write(response.toUtf8());
    socket->disconnectFromHost();
    socket->waitForDisconnected();
    socket->deleteLater();
}

////////////////////////////////////////////////////

RegenerateDatabaseThread::RegenerateDatabaseThread(const QString & dbFileName, QDialog * progressDialog, ReferenceModel * referenceModel) {
    m_dbFileName = dbFileName;
    m_referenceModel = referenceModel;
    m_progressDialog = progressDialog;
}

const QString RegenerateDatabaseThread::error() const {
    return m_error;
}

QDialog * RegenerateDatabaseThread::progressDialog() const {
    return m_progressDialog;
}

ReferenceModel * RegenerateDatabaseThread::referenceModel() const {
    return m_referenceModel;
}

void RegenerateDatabaseThread::run() {
    QTemporaryFile file(QString("%1/XXXXXX.db").arg(QDir::tempPath()));
    QString fileName;
    if (file.open()) {
        fileName = file.fileName();
        file.close();
    }
    else {
        m_error = tr("Unable to open temporary file");
        return;
    }

    bool ok = ((FApplication *) qApp)->loadReferenceModel(fileName, true, m_referenceModel);
    if (!ok) {
        m_error = tr("Database failure");
        return;
    }

    if (QFile::exists(m_dbFileName)) {
        ok = QFile::remove(m_dbFileName);
        if (!ok) {
            m_error = tr("Unable to remove original db file %1").arg(m_dbFileName);
            return;
        }
    }

    ok = QFile::copy(fileName, m_dbFileName);
    if (!ok) {
        m_error = tr("Unable to copy database file %1").arg(m_dbFileName);
        return;
    }
}

////////////////////////////////////////////////////

FApplication::FApplication( int & argc, char ** argv) : QApplication(argc, argv)
{
    m_fServer = NULL;
	m_spaceBarIsPressed = false;
	m_mousePressed = false;
	m_referenceModel = NULL;
	m_started = false;
	m_updateDialog = NULL;
	m_lastTopmostWindow = NULL;
	m_serviceType = NoService;
	m_splash = NULL;

	m_arguments = arguments();
}

bool FApplication::init() {

	//foreach (QString argument, m_arguments) {
		//DebugDialog::debug(QString("argument %1").arg(argument));
	//}

	m_serviceType = NoService;

	QList<int> toRemove;
	for (int i = 0; i < m_arguments.length(); i++) {
		if ((m_arguments[i].compare("-h", Qt::CaseInsensitive) == 0) || 
			(m_arguments[i].compare("-help", Qt::CaseInsensitive) == 0) || 
			(m_arguments[i].compare("--help", Qt::CaseInsensitive) == 0)) 
		{
			return false;
		}

		if ((m_arguments[i].compare("-e", Qt::CaseInsensitive) == 0) ||
			(m_arguments[i].compare("-examples", Qt::CaseInsensitive) == 0)||
			(m_arguments[i].compare("--examples", Qt::CaseInsensitive) == 0)) {
            DebugDialog::setEnabled(true);
			m_serviceType = ExampleService;
			m_outputFolder = " ";					// otherwise program will bail out
			toRemove << i;
		}

		if ((m_arguments[i].compare("-d", Qt::CaseInsensitive) == 0) ||
			(m_arguments[i].compare("-debug", Qt::CaseInsensitive) == 0)||
			(m_arguments[i].compare("--debug", Qt::CaseInsensitive) == 0)) {
			DebugDialog::setEnabled(true);
			toRemove << i;
		}

		if (i + 1 >= m_arguments.length()) continue;

        if ((m_arguments[i].compare("-f", Qt::CaseInsensitive) == 0) ||
            (m_arguments[i].compare("-folder", Qt::CaseInsensitive) == 0)||
            (m_arguments[i].compare("--folder", Qt::CaseInsensitive) == 0))
        {
            FolderUtils::setApplicationPath(m_arguments[i + 1]);
            // delete these so we don't try to process them as files later
            toRemove << i << i + 1;
        }

        if ((m_arguments[i].compare("-pp", Qt::CaseInsensitive) == 0) ||
            (m_arguments[i].compare("-pa", Qt::CaseInsensitive) == 0) ||
            (m_arguments[i].compare("-parts", Qt::CaseInsensitive) == 0) ||
            (m_arguments[i].compare("--parts", Qt::CaseInsensitive) == 0) ||
            (m_arguments[i].compare("--partsparent", Qt::CaseInsensitive) == 0))
        {
            FolderUtils::setAppPartsPath(m_arguments[i + 1]);
            // delete these so we don't try to process them as files later
            toRemove << i << i + 1;
        }

        if ((m_arguments[i].compare("-ov", Qt::CaseInsensitive) == 0) ||
            (m_arguments[i].compare("-ow", Qt::CaseInsensitive) == 0) ||
            (m_arguments[i].compare("-of", Qt::CaseInsensitive) == 0) ||
            (m_arguments[i].compare("--ov", Qt::CaseInsensitive) == 0) ||
            (m_arguments[i].compare("--ow", Qt::CaseInsensitive) == 0) ||
            (m_arguments[i].compare("--of", Qt::CaseInsensitive) == 0) ||
            (m_arguments[i].compare("--overridefolder", Qt::CaseInsensitive) == 0)
           )
        {
            PaletteModel::setFzpOverrideFolder(m_arguments[i + 1]);
            // delete these so we don't try to process them as files later
            toRemove << i << i + 1;
        }

        if ((m_arguments[i].compare("-geda", Qt::CaseInsensitive) == 0) ||
			(m_arguments[i].compare("--geda", Qt::CaseInsensitive) == 0)) {
			m_serviceType = GedaService;
			m_outputFolder = m_arguments[i + 1];
			toRemove << i << i + 1;
		}

		//if ((m_arguments[i].compare("-drc", Qt::CaseInsensitive) == 0) ||
		//	(m_arguments[i].compare("--drc", Qt::CaseInsensitive) == 0)) {
		//	m_serviceType = DRCService;
		//	m_outputFolder = m_arguments[i + 1];
		//	toRemove << i << i + 1;
		//}

		if ((m_arguments[i].compare("-db", Qt::CaseInsensitive) == 0) ||
            (m_arguments[i].compare("-database", Qt::CaseInsensitive) == 0) ||
			(m_arguments[i].compare("--database", Qt::CaseInsensitive) == 0)) {
			m_serviceType = DatabaseService;
			m_outputFolder = m_arguments[i + 1];
			toRemove << i << i + 1;
		}

		if ((m_arguments[i].compare("-kicad", Qt::CaseInsensitive) == 0) ||
			(m_arguments[i].compare("--kicad", Qt::CaseInsensitive) == 0)) {
			m_serviceType = KicadFootprintService;
			m_outputFolder = m_arguments[i + 1];
			toRemove << i << i + 1;
		}

		if ((m_arguments[i].compare("-kicadschematic", Qt::CaseInsensitive) == 0) ||
			(m_arguments[i].compare("--kicadschematic", Qt::CaseInsensitive) == 0)) {
			m_serviceType = KicadSchematicService;
			m_outputFolder = m_arguments[i + 1];
			toRemove << i << i + 1;
		}

		if ((m_arguments[i].compare("-svg", Qt::CaseInsensitive) == 0) ||
			(m_arguments[i].compare("--svg", Qt::CaseInsensitive) == 0)) {
			m_serviceType = SvgService;
            DebugDialog::setEnabled(true);
			m_outputFolder = m_arguments[i + 1];
			toRemove << i << i + 1;
		}

		if ((m_arguments[i].compare("-port", Qt::CaseInsensitive) == 0) ||
			(m_arguments[i].compare("--port", Qt::CaseInsensitive) == 0)) {
            DebugDialog::setEnabled(true);
            bool ok;
            int p = m_arguments[i + 1].toInt(&ok);
            if (ok) {
                m_portNumber = p;
            }

            toRemove << i << i + 1;

            if (i + 2 < m_arguments.count()) {
                if (ok) {
                    m_portRootFolder = m_arguments[i + 2];
                    m_serviceType = PortService;
                }
                toRemove << i + 2;
            }

			m_outputFolder = m_arguments[i + 1];
		}



		if ((m_arguments[i].compare("-g", Qt::CaseInsensitive) == 0) ||
			(m_arguments[i].compare("-gerber", Qt::CaseInsensitive) == 0)||
			(m_arguments[i].compare("--gerber", Qt::CaseInsensitive) == 0)) {
			m_serviceType = GerberService;
            DebugDialog::setEnabled(true);
			m_outputFolder = m_arguments[i + 1];
			toRemove << i << i + 1;
		}

		if ((m_arguments[i].compare("-p", Qt::CaseInsensitive) == 0) ||
			(m_arguments[i].compare("-panel", Qt::CaseInsensitive) == 0)||
			(m_arguments[i].compare("--panel", Qt::CaseInsensitive) == 0)) {
			m_serviceType = PanelizerService;
			m_panelFilename = m_arguments[i + 1];
			m_outputFolder = " ";					// otherwise program will bail out
            m_panelizerCustom = false;
			toRemove << i << i + 1;
		}

		if ((m_arguments[i].compare("-pc", Qt::CaseInsensitive) == 0)) {
			m_serviceType = PanelizerService;
			m_panelFilename = m_arguments[i + 1];
			m_outputFolder = " ";					// otherwise program will bail out
            m_panelizerCustom = true;
			toRemove << i << i + 1;
		}

		if ((m_arguments[i].compare("-i", Qt::CaseInsensitive) == 0) ||
			(m_arguments[i].compare("-inscription", Qt::CaseInsensitive) == 0)||
			(m_arguments[i].compare("--inscription", Qt::CaseInsensitive) == 0)) {
			m_serviceType = InscriptionService;
			m_panelFilename = m_arguments[i + 1];
			m_outputFolder = " ";					// otherwise program will bail out
			toRemove << i << i + 1;
		}

		if (m_arguments[i].compare("-ep", Qt::CaseInsensitive) == 0) {
			m_externalProcessPath = m_arguments[i + 1];
			toRemove << i << i + 1;
		}

		if (m_arguments[i].compare("-eparg", Qt::CaseInsensitive) == 0) {
			m_externalProcessArgs << m_arguments[i + 1];
			toRemove << i << i + 1;
		}

		if (m_arguments[i].compare("-epname", Qt::CaseInsensitive) == 0) {
			m_externalProcessName = m_arguments[i + 1];
			toRemove << i << i + 1;
		}

	}

	while (toRemove.count() > 0) {
		int ix = toRemove.takeLast();
		m_arguments.removeAt(ix);
	}

	m_started = false;
	m_lastTopmostWindow = NULL;

	connect(&m_activationTimer, SIGNAL(timeout()), this, SLOT(updateActivation()));
	m_activationTimer.setInterval(10);
	m_activationTimer.setSingleShot(true);

	QCoreApplication::setOrganizationName("Fritzing");
	QCoreApplication::setOrganizationDomain("fritzing.org");
	QCoreApplication::setApplicationName("Fritzing");

	installEventFilter(this);

	// tell app where to search for plugins (jpeg export and sql lite)
	m_libPath = FolderUtils::getLibraryPath();
	QApplication::addLibraryPath(m_libPath);	

	/*QFile file("libpath.txt");
	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QTextStream out(&file);
		out << m_libPath;
		file.close();
	}*/

	// !!! translator must be installed before any widgets are created !!!
	m_translationPath = FolderUtils::getApplicationSubFolderPath("translations");

	bool loaded = findTranslator(m_translationPath);
	Q_UNUSED(loaded);

	Q_INIT_RESOURCE(phoenixresources);

	MainWindow::initNames();
	FSvgRenderer::initNames();
	ViewLayer::initNames();
	RatsnestColors::initNames();
	Wire::initNames();
	ItemBase::initNames();
	ViewLayer::initNames();
	Connector::initNames();
	BinManager::initNames();
	PaletteModel::initNames();
	SvgIconWidget::initNames();
	PinHeader::initNames();
	if (m_serviceType == NoService) {
		CursorMaster::initCursors();
	}

#ifdef Q_OS_MAC
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)) || defined(QT_MAC_USE_COCOA)
	m_buildType = " Cocoa";
#else
	m_buildType = " Carbon";
#endif
#else
    m_buildType = QString(PLATFORM_NAME).contains("64") ? "64" : "32";
#endif
    AboutBox::initBuildType(m_buildType);

	return true;
}

FApplication::~FApplication(void)
{
	cleanupBackups();
		
	clearModels();

    if (m_updateDialog) {
        delete m_updateDialog;
    }

    FSvgRenderer::cleanup();
	ViewLayer::cleanup();
	ViewLayer::cleanup();
	ItemBase::cleanup();
	Wire::cleanup();
	DebugDialog::cleanup();
	ItemDrag::cleanup();
	Version::cleanup();
	TipsAndTricks::cleanup();
	FirstTimeHelpDialog::cleanup();
	TranslatorListModel::cleanup();
	FolderUtils::cleanup();
	SearchLineEdit::cleanup();
	RatsnestColors::cleanup();
	HtmlInfoView::cleanup();
	SvgIconWidget::cleanup();
	PartFactory::cleanup();
    PartsBinView::cleanup();
    PropertyDefMaster::cleanup();
    CursorMaster::cleanup();
    LockManager::cleanup();
    PartsBinPaletteWidget::cleanup();
}

void FApplication::clearModels() {
	if (m_referenceModel) {
		m_referenceModel->clearPartHash();
		delete m_referenceModel;
	}
}

bool FApplication::spaceBarIsPressed() {
	return ((FApplication *) qApp)->m_spaceBarIsPressed;
}


bool FApplication::eventFilter(QObject *obj, QEvent *event)
{
	// check whether the space bar is down.
    //qDebug() << "event" << event->type();

	Q_UNUSED(obj);

	switch (event->type()) {
		case QEvent::MouseButtonPress:
            //DebugDialog::debug("button press");
			m_mousePressed = true;
			break;
		case QEvent::MouseButtonRelease:
            //DebugDialog::debug("button release");
			m_mousePressed = false;
			break;
        case QEvent::Drop:
            // at least under Windows, the MouseButtonRelease event is not triggered if the Drop event is triggered
            m_mousePressed = false;
            break;
		case QEvent::KeyPress:
			{
                //DebugDialog::debug(QString("key pressed %1 %2").arg(m_mousePressed).arg(QApplication::mouseButtons()));
				if (!m_mousePressed && !m_spaceBarIsPressed) {
					QKeyEvent * kevent = static_cast<QKeyEvent *>(event);
					if (!kevent->isAutoRepeat() && (kevent->key() == Qt::Key_Space)) {
						m_spaceBarIsPressed = true;
                        //DebugDialog::debug("spacebar pressed");
                        CursorMaster::instance()->block();
                        setOverrideCursor(Qt::OpenHandCursor);
						emit spaceBarIsPressedSignal(true);
					}
				}
			}
			break;
		case QEvent::KeyRelease:
			{
                //DebugDialog::debug(QString("key released %1 %2").arg(m_mousePressed).arg(QApplication::mouseButtons()));
				if (m_spaceBarIsPressed) {
					QKeyEvent * kevent = static_cast<QKeyEvent *>(event);
					if (!kevent->isAutoRepeat() && (kevent->key() == Qt::Key_Space)) {
						m_spaceBarIsPressed = false;
                        //DebugDialog::debug("spacebar pressed");
                        restoreOverrideCursor();
                        CursorMaster::instance()->unblock();
						emit spaceBarIsPressedSignal(false);
					}
				}
			}
			break;
		default:
			break;
	}

	return false;
}


bool FApplication::event(QEvent *event)
{
    switch (event->type()) {
		case QEvent::FileOpen:
			{
				QString path = static_cast<QFileOpenEvent *>(event)->file();
				DebugDialog::debug(QString("file open %1").arg(path));
				if (m_started) {
					loadNew(path);
				}
				else {
					m_filesToLoad.append(path);
				}

			}
			return true;
		default:
			return QApplication::event(event);
    }
}

bool FApplication::findTranslator(const QString & translationsPath) {
	QSettings settings;
	QString suffix = settings.value("language").toString();
	if (suffix.isEmpty()) {
		suffix = QLocale::system().name();	   // Returns the language and country of this locale as a string of the form "language_country", where language is a lowercase, two-letter ISO 639 language code, and country is an uppercase, two-letter ISO 3166 country code.
	}
	else {
		QLocale::setDefault(QLocale(suffix));
	}

    bool loaded = m_translator.load(QString("fritzing_") + suffix.toLower(), translationsPath);
    DebugDialog::debug(QString("translation %1 loaded %2 from %3").arg(suffix).arg(loaded).arg(translationsPath));
	if (loaded) {
		QApplication::installTranslator(&m_translator);
	}

	return loaded;
}

void FApplication::registerFonts() {
	registerFont(":/resources/fonts/DroidSans.ttf", true);
	registerFont(":/resources/fonts/DroidSans-Bold.ttf", false);
	registerFont(":/resources/fonts/DroidSansMono.ttf", false);
	registerFont(":/resources/fonts/OCRA.ttf", true);

    // "Droid Sans" 
    // "Droid Sans Mono" 
		
    /*
		QFontDatabase database;
		QStringList families = database.families (  );
		foreach (QString string, families) {
			DebugDialog::debug(string);			// should print out the name of the fonts you loaded
		}
*/


}

bool FApplication::loadReferenceModel(const QString & databaseName, bool fullLoad) {
    m_referenceModel = new CurrentReferenceModel();
    ItemBase::setReferenceModel(m_referenceModel);
    connect(m_referenceModel, SIGNAL(loadedPart(int, int)), this, SLOT(loadedPart(int, int)));
    return loadReferenceModel(databaseName, fullLoad, m_referenceModel);
}

bool FApplication::loadReferenceModel(const QString & databaseName, bool fullLoad, ReferenceModel * referenceModel)
{
    QDir dir = FolderUtils::getAppPartsSubFolder("");
    QString dbPath = dir.absoluteFilePath("parts.db");
    QFileInfo info(dbPath);
    bool dbExists = info.exists();

    QString sha;
    if (fullLoad) {
        // fullLoad == true means we are creating the parts database
        // so get the sha for last commit of the parts folder and store it in the database
        // this sha will be used to determine whether the user's parts folder can be updated from the remote repo
        sha = PartsChecker::getSha(dir.absolutePath());
        if (sha.isEmpty()) {
            return false;
        }

        referenceModel->setSha(sha);
    }

    // loads local parts, resource parts, and any other parts in files not in the db--these part override db parts with the same moduleID
    bool ok = referenceModel->loadAll(databaseName, fullLoad, dbExists);
    if (ok && databaseName.isEmpty()) {
        QFile file(dir.absoluteFilePath("parts.db"));
        if (file.exists()) {
            referenceModel->loadFromDB(dbPath);
        }
    }

    return ok;
}

MainWindow * FApplication::openWindowForService(bool lockFiles, int initialTab) {
	// our MainWindows use WA_DeleteOnClose so this has to be added to the heap (via new) rather than the stack (for local vars)
	MainWindow * mainWindow = MainWindow::newMainWindow(m_referenceModel, "", false, lockFiles, initialTab);   // this is also slow
	mainWindow->setReportMissingModules(false);
    mainWindow->noBackup();
    mainWindow->noSchematicConversion();

	return mainWindow;
}

int FApplication::serviceStartup() {

	if (m_outputFolder.isEmpty()) {
		return -1;
	}

	switch (m_serviceType) {
        case PortService:
            initService();
            {
                MainWindow * sketch = MainWindow::newMainWindow(m_referenceModel, "", true, true, -1);
                if (sketch) {
                    sketch->show();
                    sketch->clearFileProgressDialog();
                }
            }
            return 1;

		case GedaService:
			runGedaService();
			return 0;
	
		case DRCService:
			runDRCService();
			return 0;
	
		case DatabaseService:
			runDatabaseService();
			return 0;
	
		case KicadFootprintService:
			runKicadFootprintService();
			return 0;

		case KicadSchematicService:
			runKicadSchematicService();
			return 0;

		case GerberService:
			runGerberService();
			return 0;

		case SvgService:
			runSvgService();
			return 0;

		case PanelizerService:
			runPanelizerService();
			return 0;

		case InscriptionService:
			runInscriptionService();
			return 0;

		case ExampleService:
			runExampleService();
			return 0;

		default:
			DebugDialog::debug("unknown service");
			return -1;
	}
}

void FApplication::runGerberService()
{
	initService();
    runGerberServiceAux();
}

void FApplication::runGerberServiceAux()
{
	QDir dir(m_outputFolder);
	QString s = dir.absolutePath();
	QStringList filters;
	filters << "*" + FritzingBundleExtension;
	QStringList filenames = dir.entryList(filters, QDir::Files);
	foreach (QString filename, filenames) {
		QString filepath = dir.absoluteFilePath(filename);
		MainWindow * mainWindow = openWindowForService(false, 3);
		m_started = true;
        
		FolderUtils::setOpenSaveFolderAux(m_outputFolder);
		if (mainWindow->loadWhich(filepath, false, false, false, "")) {
            QFileInfo info(filepath);
            GerberGenerator::exportToGerber(info.completeBaseName(), m_outputFolder, NULL, mainWindow->pcbView(), false);
		}

		mainWindow->setCloseSilently(true);
		mainWindow->close();
	}
}

void FApplication::initService() 
{
    createUserDataStoreFolderStructures();
	registerFonts();
	loadReferenceModel("", false);
}

void FApplication::runSvgService()
{
    initService();
    runSvgServiceAux();
}

void FApplication::runSvgServiceAux()
{
	QDir dir(m_outputFolder);
	QString s = dir.absolutePath();
	QStringList filters;
	filters << "*" + FritzingBundleExtension;
	QStringList filenames = dir.entryList(filters, QDir::Files);
	foreach (QString filename, filenames) {
		QString filepath = dir.absoluteFilePath(filename);
        MainWindow * mainWindow = openWindowForService(false, -1);
		m_started = true;
        
		FolderUtils::setOpenSaveFolderAux(m_outputFolder);
		if (mainWindow->loadWhich(filepath, false, false, false, "")) {
            QFileInfo info(filepath);
            QList<ViewLayer::ViewID> ids;
            ids << ViewLayer::BreadboardView << ViewLayer::SchematicView << ViewLayer::PCBView;
            foreach (ViewLayer::ViewID id, ids) {
                QString fn = QString("%1_%2.svg").arg(info.completeBaseName()).arg(ViewLayer::viewIDNaturalName(id));
                QString svgPath = dir.absoluteFilePath(fn);
                mainWindow->setCurrentView(id);
                mainWindow->exportSvg(GraphicsUtils::StandardFritzingDPI, false, false, svgPath);
            }
		}

		mainWindow->setCloseSilently(true);
		mainWindow->close();
	}
}

void FApplication::runDatabaseService()
{
    createUserDataStoreFolderStructures();

    DebugDialog::setEnabled(true);

    QString partsDB = m_outputFolder;  // m_outputFolder is actually a full path ending in ".db"
    QFile::remove(partsDB);
    loadReferenceModel(partsDB, true);
}

void FApplication::runGedaService() {
	try {
		QDir dir(m_outputFolder);
		QStringList filters;
		filters << "*.fp";
		QStringList filenames = dir.entryList(filters, QDir::Files);
		foreach (QString filename, filenames) {
			QString filepath = dir.absoluteFilePath(filename);
			QString newfilepath = filepath;
			newfilepath.replace(".fp", ".svg");
			GedaElement2Svg geda;
			QString svg = geda.convert(filepath, false);
            TextUtils::writeUtf8(newfilepath, svg);
		}
	}
	catch (const QString & msg) {
		DebugDialog::debug(msg);
	}
	catch (...) {
		DebugDialog::debug("who knows");
	}
}


void FApplication::runDRCService() {
    m_started = true;
    initService();
    DebugDialog::setEnabled(true);

    try {
		QDir dir(m_outputFolder);
		QStringList filters;
		filters << "*.fzz";
		QStringList filenames = dir.entryList(filters, QDir::Files);
		foreach (QString filename, filenames) {
			QString filepath = dir.absoluteFilePath(filename);
	        MainWindow * mainWindow = openWindowForService(false, 3);
            if (mainWindow == NULL) continue;

            mainWindow->setCloseSilently(true);

	        if (!mainWindow->loadWhich(filepath, false, false, false, "")) {
		        DebugDialog::debug(QString("failed to load '%1'").arg(filepath));
		        mainWindow->close();
                delete mainWindow;
                continue;
	        }

	        mainWindow->showPCBView();

            int moved = mainWindow->pcbView()->checkLoadedTraces();
            if (moved > 0) {
                QMessageBox::warning(NULL, QObject::tr("Fritzing"), QObject::tr("%1 wires moved from their saved position in %2.").arg(moved).arg(filepath));
                DebugDialog::debug(QString("\ncheckloadedtraces %1\n").arg(filepath)); 
            }
	

            Panelizer::checkDonuts(mainWindow, true);
            Panelizer::checkText(mainWindow, true);

            QList<ItemBase *> boards = mainWindow->pcbView()->findBoard();	
	        foreach (ItemBase * boardItem, boards) {
                mainWindow->pcbView()->selectAllItems(false, false);
                boardItem->setSelected(true);
                mainWindow->newDesignRulesCheck(false);
            }

            mainWindow->close();
            delete mainWindow;
		}
	}
	catch (const QString & msg) {
		DebugDialog::debug(msg);
	}
	catch (...) {
		DebugDialog::debug("who knows");
	}
}

void FApplication::runKicadFootprintService() {
	QDir dir(m_outputFolder);
	QStringList filters;
	filters << "*.mod";
	QStringList filenames = dir.entryList(filters, QDir::Files);
	foreach (QString filename, filenames) {
		QString filepath = dir.absoluteFilePath(filename);
		QStringList moduleNames = KicadModule2Svg::listModules(filepath);
		foreach (QString moduleName, moduleNames) {
			KicadModule2Svg kicad;
			try {
				QString svg = kicad.convert(filepath, moduleName, false);
				if (svg.isEmpty()) {
					DebugDialog::debug("svg is empty " + filepath + " " + moduleName);
					continue;
				}
				foreach (QChar c, QString("<>:\"/\\|?*")) {
					moduleName.remove(c);
				}

				QString newFilePath = dir.absoluteFilePath(moduleName + "_" + filename);
				newFilePath.replace(".mod", ".svg");

				if (!TextUtils::writeUtf8(newFilePath, svg)) {
					DebugDialog::debug("unable to open file " + newFilePath);
				}
			}
			catch (const QString & msg) {
				DebugDialog::debug(msg);
			}
			catch (...) {
				DebugDialog::debug("who knows");
			}
		}
	}
}

void FApplication::runKicadSchematicService() {
	QDir dir(m_outputFolder);
	QStringList filters;
	filters << "*.lib";
	QStringList filenames = dir.entryList(filters, QDir::Files);
	foreach (QString filename, filenames) {
		QString filepath = dir.absoluteFilePath(filename);
		QStringList defNames = KicadSchematic2Svg::listDefs(filepath);
		foreach (QString defName, defNames) {
			KicadSchematic2Svg kicad;
			try {
				QString svg = kicad.convert(filepath, defName);
				if (svg.isEmpty()) {
					DebugDialog::debug("svg is empty " + filepath + " " + defName);
					continue;
				}
				foreach (QChar c, QString("<>:\"/\\|?*")) {
					defName.remove(c);
				}

				//DebugDialog::debug(QString("converting %1 %2").arg(defName).arg(filename));

				QString newFilePath = dir.absoluteFilePath(defName + "_" + filename);
				newFilePath.replace(".lib", ".svg");
                
				QFile file(newFilePath);
				if (!TextUtils::writeUtf8(newFilePath, svg)) {
					DebugDialog::debug("unable to open file " + newFilePath);
				}
			}
			catch (const QString & msg) {
				DebugDialog::debug(msg);
			}
			catch (...) {
				DebugDialog::debug("who knows");
			}
		}
	}
}

int FApplication::startup()
{
	//DebugDialog::setEnabled(true);

	QString splashName = ":/resources/images/splash/splash_screen_start.png";
	QDateTime now = QDateTime::currentDateTime();
	if (now.date().month() == 4 && now.date().day() == 1) {
		QString aSplashName = ":/resources/images/splash/april1st.png";
		QFileInfo info(aSplashName);
		if (info.exists()) {
			splashName = aSplashName;
		}
	}

	QPixmap pixmap(splashName);
	FSplashScreen splash(pixmap);
	m_splash = &splash;
	ProcessEventBlocker::processEvents();								// seems to need this (sometimes?) to display the splash screen

	initSplash(splash);
	ProcessEventBlocker::processEvents();

	// DebugDialog::debug("Data Location: "+QDesktopServices::storageLocation(QDesktopServices::DataLocation));

	registerFonts();

	if (m_progressIndex >= 0) splash.showProgress(m_progressIndex, LoadProgressStart);
	ProcessEventBlocker::processEvents();

	#ifdef Q_OS_WIN
		// associate .fz file with fritzing app on windows (xp only--vista is different)
		// TODO: don't change settings if they're already set?
		// TODO: only do this at install time?
		QSettings settings1("HKEY_CLASSES_ROOT\\Fritzing", QSettings::NativeFormat);
		settings1.setValue(".", "Fritzing Application");
		foreach (QString extension, fritzingExtensions()) {
			QSettings settings2("HKEY_CLASSES_ROOT\\" + extension, QSettings::NativeFormat);
			settings2.setValue(".", "Fritzing");
		}
		QSettings settings3("HKEY_CLASSES_ROOT\\Fritzing\\shell\\open\\command", QSettings::NativeFormat);
		settings3.setValue(".", QString("\"%1\" \"%2\"")
							.arg(QDir::toNativeSeparators(QApplication::applicationFilePath()))
							.arg("%1") );
	#endif

	cleanFzzs();

    createUserDataStoreFolderStructures();

    ProcessEventBlocker::processEvents();

    loadReferenceModel("", false);

    ProcessEventBlocker::processEvents();

	QString prevVersion;
	{
		// put this in a block so that QSettings is closed
		QSettings settings;
		prevVersion = settings.value("version").toString();
		QString currVersion = Version::versionString();
		if(prevVersion != currVersion) {
			QVariant pid = settings.value("pid");
            QVariant language = settings.value("language");
			settings.clear();
			if (!pid.isNull()) {
				settings.setValue("pid", pid);
			}
			if (!language.isNull()) {
				settings.setValue("language", language);
			}
		}
	}

	//bool fabEnabled = settings.value(ORDERFABENABLED, QVariant(false)).toBool();
	//if (!fabEnabled) {
		QNetworkAccessManager * manager = new QNetworkAccessManager(this);
		connect(manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(gotOrderFab(QNetworkReply *)));
		manager->get(QNetworkRequest(QUrl(QString("http://fab.fritzing.org/launched%1").arg(Version::makeRequestParamsString(true)))));
	//}

	if (m_progressIndex >= 0) splash.showProgress(m_progressIndex, LoadProgressEnd);



	//DebugDialog::debug("after createUserDataStoreFolderStructure");

	if (m_progressIndex >= 0) splash.showProgress(m_progressIndex, 0.65);
	ProcessEventBlocker::processEvents();

	DebugDialog::debug("load something");
	loadSomething(prevVersion);
	m_started = true;

	if (m_progressIndex >= 0) splash.showProgress(m_progressIndex, 0.99);
	ProcessEventBlocker::processEvents();

    splash.hide();
	m_splash = NULL;

    m_updateDialog = new UpdateDialog();
    m_updateDialog->setRepoPath(FolderUtils::getAppPartsSubFolderPath(""), m_referenceModel->sha());
    connect(m_updateDialog, SIGNAL(enableAgainSignal(bool)), this, SLOT(enableCheckUpdates(bool)));
    connect(m_updateDialog, SIGNAL(installNewParts()), this, SLOT(installNewParts()));
    checkForUpdates(false);

	return 0;
}

void FApplication::registerFont(const QString &fontFile, bool reallyRegister) {
	int id = QFontDatabase::addApplicationFont(fontFile);
	if(id > -1 && reallyRegister) {
		QStringList familyNames = QFontDatabase::applicationFontFamilies(id);
		QFileInfo finfo(fontFile);
		foreach (QString family, familyNames) {
			InstalledFonts::InstalledFontsNameMapper.insert(family, finfo.completeBaseName());
			InstalledFonts::InstalledFontsList << family;
			DebugDialog::debug(QString("registering font family: %1 %2").arg(family).arg(finfo.completeBaseName()));
		}
	}
}

void FApplication::finish()
{
	QString currVersion = Version::versionString();
	QSettings settings;
    settings.setValue("version", currVersion);
}

void FApplication::loadNew(QString path) {
	MainWindow * mw = MainWindow::newMainWindow(m_referenceModel, path, true, true, -1);
	if (!mw->loadWhich(path, false, true, true, "")) {
		mw->close();
	}
	mw->clearFileProgressDialog();
}

void FApplication::loadOne(MainWindow * mw, QString path, int loaded) {
	if (loaded == 0) {
		mw->showFileProgressDialog(path);
		mw->loadWhich(path, true, true, true, "");
	}
	else {
		loadNew(path);
	}
}

void FApplication::preferences() {
    // delay keeps OS 7 from crashing?

    QTimer::singleShot(30, this, SLOT(preferencesAfter()));
}

void FApplication::preferencesAfter() 
{
	QDir dir(m_translationPath);
	QStringList nameFilters;
	nameFilters << "*.qm";
    QFileInfoList languages = dir.entryInfoList(nameFilters, QDir::Files | QDir::NoSymLinks);
	QSettings settings;
	QString language = settings.value("language").toString();
	if (language.isEmpty()) {
		language = QLocale::system().name();
	}

	MainWindow * mainWindow = NULL;
	foreach (MainWindow * mw, orderedTopLevelMainWindows()) {
		mainWindow = mw;
		break;
	}

	if (mainWindow == NULL) return;			// shouldn't happen (maybe on the mac)

	PrefsDialog prefsDialog(language, NULL);			// TODO: use the topmost MainWindow as parent
	int ix = 0;
	foreach (SketchWidget * sketchWidget, mainWindow->sketchWidgets()) {
		prefsDialog.initViewInfo(ix++,  sketchWidget->viewName(), sketchWidget->getShortName(), 
									sketchWidget->curvyWires());
	}

    QList<Platform *> platforms = mainWindow->programmingWidget()->getAvailablePlatforms();

    prefsDialog.initLayout(languages, platforms);
	if (QDialog::Accepted == prefsDialog.exec()) {
		updatePrefs(prefsDialog);
	}
}

void FApplication::updatePrefs(PrefsDialog & prefsDialog)
{
	QSettings settings;

	if (prefsDialog.cleared()) {
		settings.clear();
		return;
	}

	QHash<QString, QString> hash = prefsDialog.settings();
	QList<MainWindow *> mainWindows = orderedTopLevelMainWindows();
	foreach (QString key, hash.keys()) {
		settings.setValue(key, hash.value(key));
		if (key.compare("connectedColor") == 0) {
			QColor c(hash.value(key));
			ItemBase::setConnectedColor(c);
			foreach (MainWindow * mainWindow, mainWindows) {
				mainWindow->redrawSketch();
			}
		}
		else if (key.compare("unconnectedColor") == 0) {
			QColor c(hash.value(key));
			ItemBase::setUnconnectedColor(c);
			foreach (MainWindow * mainWindow, mainWindows) {
				mainWindow->redrawSketch();
			}
		}
		else if (key.compare("wheelMapping") == 0) {
			ZoomableGraphicsView::setWheelMapping((ZoomableGraphicsView::WheelMapping) hash.value(key).toInt());
		}
		else if (key.compare("autosavePeriod") == 0) {
			MainWindow::setAutosavePeriod(hash.value(key).toInt());
		}
		else if (key.compare("autosaveEnabled") == 0) {
			MainWindow::setAutosaveEnabled(hash.value(key).toInt());
		}
		else if (key.contains("curvy", Qt::CaseInsensitive)) {
			foreach (MainWindow * mainWindow, mainWindows) {
				foreach (SketchWidget * sketchWidget, mainWindow->sketchWidgets()) {
					if (key.contains(sketchWidget->getShortName())) {
						sketchWidget->setCurvyWires(hash.value(key).compare("1") == 0);
					}
				}
			}
		}
	}

}

void FApplication::initSplash(FSplashScreen & splash) {
	QPixmap progress(":/resources/images/splash/splash_progressbar.png");

	m_progressIndex = splash.showPixmap(progress, "progress");
	if (m_progressIndex >= 0) splash.showProgress(m_progressIndex, 0);

	// put this above the progress indicator

	QString msg1 = QObject::tr("<font face='Lucida Grande, Tahoma, Sans Serif' size='2' color='#eaf4ed'>"
                               "&#169; 2007-%1 Fritzing"
							   "</font>")
                            .arg(Version::year());
    splash.showMessage(msg1, "fritzingText", Qt::AlignLeft | Qt::AlignTop);

	QString msg2 = QObject::tr("<font face='Lucida Grande, Tahoma, Sans Serif' size='2' color='#eaf4ed'>"
							   "Version %1.%2.%3 (%4%5) %6"
							   "</font>")
						.arg(Version::majorVersion())
						.arg(Version::minorVersion())
						.arg(Version::minorSubVersion())
						.arg(Version::modifier())
						.arg(Version::shortDate())
						.arg(m_buildType);
    splash.showMessage(msg2, "versionText", Qt::AlignRight | Qt::AlignTop);

#ifdef Q_OS_MAC
            // remove the splash screen flag on OS-X as workaround for the reported bug
            // https://bugreports.qt.io/browse/QTBUG-49576
            splash.setWindowFlags(splash.windowFlags() & (~Qt::SplashScreen));
#endif

    splash.show();
}

struct Thing {
        QString moduleID;
        ViewLayer::ViewID viewID;
        ViewLayer::ViewLayerID viewLayerID;
};


void FApplication::checkForUpdates() {
	checkForUpdates(true);
}

void FApplication::checkForUpdates(bool atUserRequest)
{
    enableCheckUpdates(false);

	VersionChecker * versionChecker = new VersionChecker();

	QSettings settings;
	if (!atUserRequest) {
		// if I've already been notified about these updates, don't bug me again
		QString lastMainVersionChecked = settings.value("lastMainVersionChecked").toString();
		if (!lastMainVersionChecked.isEmpty()) {
			versionChecker->ignore(lastMainVersionChecked, false);
		}
		QString lastInterimVersionChecked = settings.value("lastInterimVersionChecked").toString();
		if (!lastInterimVersionChecked.isEmpty()) {
			versionChecker->ignore(lastInterimVersionChecked, true);
		}
	}

    QString atom = QString("http://fritzing.org/download/feed/atom/%1/%2")
		.arg(PLATFORM_NAME)
		.arg(Version::makeRequestParamsString(true));
    DebugDialog::debug(atom);
    versionChecker->setUrl(atom);
	m_updateDialog->setAtUserRequest(atUserRequest);
	m_updateDialog->setVersionChecker(versionChecker);

	if (atUserRequest) {
        m_updateDialog->exec();
	}
}

void FApplication::enableCheckUpdates(bool enabled)
{
	//DebugDialog::debug("before enable check updates");
    foreach (QWidget *widget, QApplication::topLevelWidgets()) {
        MainWindow *mainWindow = qobject_cast<MainWindow *>(widget);
		if (mainWindow) {
			mainWindow->enableCheckUpdates(enabled);
		}
	}
	//DebugDialog::debug("after enable check updates");
}

void FApplication::createUserDataStoreFolderStructures() {
    FolderUtils::createUserDataStoreFolders();
    FolderUtils::copyBin(BinManager::MyPartsBinLocation, BinManager::MyPartsBinTemplateLocation);
    FolderUtils::copyBin(BinManager::SearchBinLocation, BinManager::SearchBinTemplateLocation);
	PartFactory::initFolder();
}


void FApplication::changeActivation(bool activate, QWidget * originator) {
	if (!activate) return;

	//DebugDialog::debug(QString("change activation %1 %2").arg(activate).arg(originator->metaObject()->className()));

	FritzingWindow * fritzingWindow = qobject_cast<FritzingWindow *>(originator);
	if (fritzingWindow == NULL) {
		fritzingWindow = qobject_cast<FritzingWindow *>(originator->parent());
	}
	if (fritzingWindow == NULL) return;

	m_orderedTopLevelWidgets.removeOne(fritzingWindow);
	m_orderedTopLevelWidgets.push_front(fritzingWindow);

	m_activationTimer.stop();
	m_activationTimer.start();
}

void FApplication::updateActivation() {
	//DebugDialog::debug("updating activation");

	FritzingWindow * prior = m_lastTopmostWindow; 
	m_lastTopmostWindow = NULL;
	if (m_orderedTopLevelWidgets.count() > 0) {
		m_lastTopmostWindow = qobject_cast<FritzingWindow *>(m_orderedTopLevelWidgets.at(0));
	}

	if (prior == m_lastTopmostWindow) {
		//DebugDialog::debug("done updating activation");
		return;
	}

	//DebugDialog::debug(QString("last:%1, new:%2").arg((long) prior, 0, 16).arg((long) m_lastTopmostWindow.data(), 0, 16));

	MainWindow * priorMainWindow = qobject_cast<MainWindow *>(prior);
	if (priorMainWindow != NULL) {			
		priorMainWindow->saveDocks();
	}

	MainWindow * lastTopmostMainWindow = qobject_cast<MainWindow *>(m_lastTopmostWindow);
	if (lastTopmostMainWindow != NULL) {
		lastTopmostMainWindow->restoreDocks();
		//DebugDialog::debug("restoring active window");
	}

	//DebugDialog::debug("done 2 updating activation");

}

void FApplication::topLevelWidgetDestroyed(QObject * object) {
	QWidget * widget = qobject_cast<QWidget *>(object);
	if (widget) {
		m_orderedTopLevelWidgets.removeOne(widget);
	}
}

void FApplication::closeAllWindows2() {
/*
Ok, near as I can tell, here's what's going on.  When you quit fritzing, the function 
QApplication::closeAllWindows() is invoked.  This goes through the top-level window 
list in random order and calls close() on each window, until some window says "no".  
The QGraphicsProxyWidgets must contain top-level windows, and at least on the mac, their response to close() 
seems to be setVisible(false).  The random order explains why different icons 
disappear, or sometimes none at all.  

So the hack for now is to call the windows in non-random order.

Eventually, maybe the SvgIconWidget class could be rewritten so that it's not using QGraphicsProxyWidget, 
which is really not intended for hundreds of widgets. 

(SvgIconWidget has been rewritten)
*/


// this code modified from QApplication::closeAllWindows()


    bool did_close = true;
    QWidget *w;
    while((w = QApplication::activeModalWidget()) && did_close) {
        if(!w->isVisible())
            break;
        did_close = w->close();
    }
	if (!did_close) return;

    QWidgetList list = QApplication::topLevelWidgets();
    for (int i = 0; did_close && i < list.size(); ++i) {
        w = list.at(i);
        FritzingWindow *fWindow = qobject_cast<FritzingWindow *>(w);
		if (fWindow == NULL) continue;

        if (w->isVisible() && w->windowType() != Qt::Desktop) {
            did_close = w->close();
            list = QApplication::topLevelWidgets();
            i = -1;
        }
    }
	if (!did_close) return;

    list = QApplication::topLevelWidgets();
    for (int i = 0; did_close && i < list.size(); ++i) {
        w = list.at(i);
        if (w->isVisible() && w->windowType() != Qt::Desktop) {
            did_close = w->close();
            list = QApplication::topLevelWidgets();
            i = -1;
        }
    }

}

bool FApplication::runAsService() {
    if (m_serviceType == PortService) {
        DebugDialog::setEnabled(true);
        initServer();
        //return false;
    }

	return m_serviceType != NoService;
}

void FApplication::loadedPart(int loaded, int total) {
	if (total == 0) return;
	if (m_splash == NULL) return;

	//DebugDialog::debug(QString("loaded %1 %2").arg(loaded).arg(total));
	if (m_progressIndex >= 0) m_splash->showProgress(m_progressIndex, LoadProgressStart + ((LoadProgressEnd - LoadProgressStart) * loaded / (double) total));
}

void FApplication::externalProcessSlot(QString &name, QString &path, QStringList &args) {
	name = m_externalProcessName;
	path = m_externalProcessPath;
	args = m_externalProcessArgs;
}

bool FApplication::notify(QObject *receiver, QEvent *e)
{
    try {
        //qDebug() << QString("notify %1 %2").arg(receiver->metaObject()->className()).arg(e->type());
        return QApplication::notify(receiver, e);
    }
	catch (char const *str) {
        FMessageBox::critical(NULL, tr("Fritzing failure"), tr("Fritzing caught an exception %1 from %2 in event %3")
			.arg(str).arg(receiver->objectName()).arg(e->type()));
	}
    catch (std::exception& exp) {
        // suggested in https://code.google.com/p/fritzing/issues/detail?id=2698
        qDebug() << QString("notify %1 %2").arg(receiver->metaObject()->className()).arg(e->type());
        FMessageBox::critical(NULL, tr("Fritzing failure"), tr("Fritzing caught an exception from %1 in event %2: %3").arg(receiver->objectName()).arg(e->type()).arg(exp.what()));
    }
    catch (...) {
        FMessageBox::critical(NULL, tr("Fritzing failure"), tr("Fritzing caught an exception from %1 in event %2").arg(receiver->objectName()).arg(e->type()));
    }
	closeAllWindows2();
	QApplication::exit(-1);
	abort();
    return false;
}

void FApplication::loadSomething(const QString & prevVersion) {
    // At this point we're trying to determine what sketches to load from one of the following sources:
    // Only one of these sources will actually provide sketches to load and they're listed in order of priority:

    //		We found sketch backups to recover
    //		Files were double-clicked
    //		The last opened sketch (obsolete)
    //		A new blank sketch


    Q_UNUSED(prevVersion);

	initFilesToLoad();   // sets up m_filesToLoad from the command line on PC and Linux; mac uses a FileOpen event instead

	initBackups();

	DebugDialog::debug("checking for backups");
    QList<MainWindow*> sketchesToLoad = recoverBackups();


    if (sketchesToLoad.isEmpty()) {
		// Check for double-clicked files to load
		DebugDialog::debug(QString("check files to load %1").arg(m_filesToLoad.count()));

        foreach (QString filename, m_filesToLoad) {
            DebugDialog::debug(QString("Loading non-service file %1").arg(filename));
            MainWindow *mainWindow = MainWindow::newMainWindow(m_referenceModel, filename, true, true, -1);
            mainWindow->loadWhich(filename, true, true, true, "");
            if (filename.endsWith(FritzingSketchExtension) || filename.endsWith(FritzingBundleExtension)) {
            }
            else {
                mainWindow->addDefaultParts();
            }
            sketchesToLoad << mainWindow;
        }
	}

    ProcessEventBlocker::processEvents();

    // Find any previously open sketches to reload
    if (sketchesToLoad.isEmpty()) {
        // new logic here, we no longer open the most recent sketch, since it can be reached in one click from the welcome page

        // DebugDialog::debug(QString("load last open"));
        //sketchesToLoad = loadLastOpenSketch();
	}

	MainWindow * newBlankSketch = NULL;
	if (sketchesToLoad.isEmpty()) {
		DebugDialog::debug(QString("create empty sketch"));
		newBlankSketch = MainWindow::newMainWindow(m_referenceModel, "", true, true, -1);
		if (newBlankSketch) {
			// make sure to start an empty sketch with a board
			newBlankSketch->addDefaultParts();   // do this before call to show()
			sketchesToLoad << newBlankSketch;
		}
	}

    ProcessEventBlocker::processEvents();
	DebugDialog::debug(QString("finish up sketch loading"));

    // Finish loading the sketches and show them to the user
	foreach (MainWindow* sketch, sketchesToLoad) {
		sketch->show();
		sketch->clearFileProgressDialog();
	}

    if (newBlankSketch) {
		newBlankSketch->hideTempPartsBin();
        // new empty sketch defaults to welcome view
        newBlankSketch->showWelcomeView();
	}
}

QList<MainWindow *> FApplication::loadLastOpenSketch() {
	QList<MainWindow *> sketches;
	QSettings settings;
	if(settings.value("lastOpenSketch").isNull()) return sketches;

	QString lastSketchPath = settings.value("lastOpenSketch").toString();
	if(!QFileInfo(lastSketchPath).exists()) {
		settings.remove("lastOpenSketch");
		return sketches;
	}

    DebugDialog::debug(QString("Loading last open sketch %1").arg(lastSketchPath));
    settings.remove("lastOpenSketch");				// clear the preference, in case the load crashes
    MainWindow *mainWindow = MainWindow::newMainWindow(m_referenceModel, lastSketchPath, true, true, -1);
    mainWindow->loadWhich(lastSketchPath, true, true, true, "");
    sketches << mainWindow;
    settings.setValue("lastOpenSketch", lastSketchPath);	// the load works, so restore the preference
	return sketches;
}

QList<MainWindow *> FApplication::recoverBackups()
{	
	QFileInfoList backupList;
	LockManager::checkLockedFiles("backup", backupList, m_lockedFiles, false, LockManager::FastTime);
	for (int i = backupList.size() - 1; i >=0; i--) {
		QFileInfo fileInfo = backupList.at(i);
		if (!fileInfo.fileName().endsWith(FritzingSketchExtension)) {
			backupList.removeAt(i);
		}
	}

	QList<MainWindow*> recoveredSketches;
    if (backupList.size() == 0) return recoveredSketches;

    RecoveryDialog recoveryDialog(backupList);
	int result = (QMessageBox::StandardButton)recoveryDialog.exec();
    QList<QTreeWidgetItem*> fileItems = recoveryDialog.getFileList();
    DebugDialog::debug(QString("Recovering %1 files from recoveryDialog").arg(fileItems.size()));
    foreach (QTreeWidgetItem * item, fileItems) {
		QString backupName = item->data(0, Qt::UserRole).value<QString>();
        if (result == QDialog::Accepted && item->isSelected()) {
			QString originalBaseName = item->text(0);
            DebugDialog::debug(QString("Loading recovered sketch %1").arg(originalBaseName));
			
			QString originalPath = item->data(1, Qt::UserRole).value<QString>();
			QString fileExt;
			QString bundledFileName = FolderUtils::getSaveFileName(NULL, tr("Please specify an .fzz file name to save to (cancel will delete the backup)"), originalPath, tr("Fritzing (*%1)").arg(FritzingBundleExtension), &fileExt);
			if (!bundledFileName.isEmpty()) {
				MainWindow *currentRecoveredSketch = MainWindow::newMainWindow(m_referenceModel, originalBaseName, true, true, -1);
    			currentRecoveredSketch->mainLoad(backupName, bundledFileName, true);
				currentRecoveredSketch->saveAsShareable(bundledFileName, true);
				currentRecoveredSketch->setCurrentFile(bundledFileName, true, true);
				recoveredSketches << currentRecoveredSketch;

				/*
				if (originalPath.startsWith(untitledFileName())) {
					DebugDialog::debug(QString("Comparing untitled documents: %1 %2").arg(filename).arg(untitledFileName()));
					QRegExp regexp("\\d+");
					int ix = regexp.indexIn(filename);
					int untitledSketchNumber = ix >= 0 ? regexp.cap(0).toInt() : 1;
					untitledSketchNumber++;
					DebugDialog::debug(QString("%1 untitled documents open, currently thinking %2").arg(untitledSketchNumber).arg(UntitledSketchIndex));
					UntitledSketchIndex = UntitledSketchIndex >= untitledSketchNumber ? UntitledSketchIndex : untitledSketchNumber;
				}
				*/



			}
        }

		QFile::remove(backupName);
    }

	return recoveredSketches;
}

void FApplication::initFilesToLoad()
{
	for (int i = 1; i < m_arguments.length(); i++) {
		QFileInfo fileinfo(m_arguments[i]);
		if (fileinfo.exists() && !fileinfo.isDir()) {
			m_filesToLoad << m_arguments[i];
		}
	}
}

void FApplication::initBackups() {
	LockManager::initLockedFiles("backup", MainWindow::BackupFolder, m_lockedFiles, LockManager::FastTime);
}

void FApplication::cleanupBackups() {
	LockManager::releaseLockedFiles(MainWindow::BackupFolder, m_lockedFiles);
}

void FApplication::gotOrderFab(QNetworkReply * networkReply) {
	int responseCode = networkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	if (responseCode == 200) {
		QSettings settings;
		settings.setValue(ORDERFABENABLED, QVariant(true));
	}
    networkReply->manager()->deleteLater();
    networkReply->deleteLater();
}

void FApplication::runPanelizerService()
{	
	m_started = true;
	Panelizer::panelize(this, m_panelFilename, m_panelizerCustom);
}

void FApplication::runInscriptionService()
{	
	m_started = true;
    bool drc = false;
    bool noMessages = false;
    foreach (QString arg, m_arguments) {
        if (arg.compare("-drc", Qt::CaseInsensitive) == 0) {
            drc = true;
        }
        if (arg.compare("-nm", Qt::CaseInsensitive) == 0) {
            noMessages = true;
        }
    }
	Panelizer::inscribe(this, m_panelFilename, drc, noMessages);
}

QList<MainWindow *> FApplication::orderedTopLevelMainWindows() {
	QList<MainWindow *> mainWindows;
	foreach (QWidget * widget, m_orderedTopLevelWidgets) {
		MainWindow * mainWindow = qobject_cast<MainWindow *>(widget);
		if (mainWindow) mainWindows.append(mainWindow);
	}
	return mainWindows;
}

void FApplication::runExampleService()
{	
	m_started = true;

	initService();

	QDir sketchesDir(FolderUtils::getApplicationSubFolderPath("sketches"));
	runExampleService(sketchesDir);
}

void FApplication::runExampleService(QDir & dir) {
	QStringList nameFilters;
	nameFilters << ("*" + FritzingBundleExtension);   //  FritzingSketchExtension
	QFileInfoList fileList = dir.entryInfoList(nameFilters, QDir::Files | QDir::NoSymLinks);
	foreach (QFileInfo fileInfo, fileList) {
		QString path = fileInfo.absoluteFilePath();
		DebugDialog::debug("sketch file " + path);

		MainWindow * mainWindow = openWindowForService(false, -1);
		if (mainWindow == NULL) continue;

		FolderUtils::setOpenSaveFolderAux(dir.absolutePath());

		if (!mainWindow->loadWhich(path, false, false, true, "")) {
			DebugDialog::debug(QString("failed to load"));
		}
		else {
			QList<ItemBase *> items = mainWindow->selectAllObsolete(false);
			if (items.count() > 0) {
                mainWindow->swapObsolete(false, items);
            }
			mainWindow->saveAsAux(path);    //   path + "z"
			mainWindow->setCloseSilently(true);
			mainWindow->close();
		}
	}

	QFileInfoList dirList = dir.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::NoSymLinks);
	foreach (QFileInfo dirInfo, dirList) {
		QDir dir(dirInfo.filePath());
		runExampleService(dir);
	}
}

void FApplication::cleanFzzs() {
	QHash<QString, LockedFile *> lockedFiles;
	QString folder;
	LockManager::initLockedFiles("fzz", folder, lockedFiles, LockManager::SlowTime);
	QFileInfoList backupList;
	LockManager::checkLockedFiles("fzz", backupList, lockedFiles, true, LockManager::SlowTime);
	LockManager::releaseLockedFiles(folder, lockedFiles);
}

void FApplication::initServer() {
    FMessageBox::BlockMessages = true;
    m_fServer = new FServer(this);
    connect(m_fServer, SIGNAL(newConnection(qintptr)), this, SLOT(newConnection(qintptr)));
	DebugDialog::debug("Server active");
    m_fServer->listen(QHostAddress::Any, m_portNumber);
}

void FApplication::newConnection(qintptr socketDescription) {
     FServerThread *thread = new FServerThread(socketDescription, this);
     connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
     connect(thread, SIGNAL(doCommand(const QString &, const QString &, QString &, int &)), 
			this, SLOT(doCommand(const QString &, const QString &, QString &, int &)), Qt::BlockingQueuedConnection);
     thread->start();
}


void FApplication::doCommand(const QString & command, const QString & params, QString & result, int & status) {
	status = 200;
	result = "";

    QDir dir(m_portRootFolder);

	QString subfolder = params;
	if (command.endsWith("tcp")) {
		subfolder = TextUtils::getRandText();
		dir.mkdir(subfolder);
	}

    dir.cd(subfolder);
    m_outputFolder = dir.absolutePath();

	if (command.endsWith("tcp")) {
		QEventLoop loop;
		QNetworkAccessManager networkManager;
		QObject::connect(&networkManager, SIGNAL(finished(QNetworkReply*)), &loop, SLOT(quit()));

		QUrl url = params;
		QNetworkReply* reply = networkManager.get(QNetworkRequest(url));

		loop.exec();

		status = 404;

		if (reply->error() == QNetworkReply::NoError) {
			if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toUInt() == 200) {
				if (reply->isReadable()) {
					int buffersize = 8192;
					QStringList components = params.split("/");
					QString filename = components.last();
					QFile file(dir.absoluteFilePath(filename));
					if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
						status = 200;
						while (reply->bytesAvailable() >= buffersize) {
							QByteArray bytes = reply->read(buffersize);
							file.write(bytes);
						}
						if (reply->bytesAvailable() > 0) {
							file.write(reply->readAll());
						}
						file.close();
					}
					else {
						result = "unable to save to local file";
					}
				}
				else {
					result = "response unreadable";
				}
			}
			else {
				result = QString("bad response from url server %1").arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toUInt());
			}
		}
		else {
			result = QString("url get failed %1").arg(reply->error());
		}

		reply->deleteLater();
	}

	if (!result.isEmpty()) {
		// network problem
		return;
	}

	if (command.startsWith("svg")) {
        runSvgServiceAux();
	    QStringList nameFilters;
	    nameFilters << ("*.svg");
	    QFileInfoList fileList = dir.entryInfoList(nameFilters, QDir::Files | QDir::NoSymLinks);
        if (fileList.count() > 0) {
            QFile file(fileList.at(0).absoluteFilePath());
            if (file.open(QFile::ReadOnly)) {
                result = file.readAll();
            }
        }
    }
    else if (command.startsWith("gerber")) {
        runGerberServiceAux();
	    QStringList nameFilters;
	    nameFilters << ("*.txt");
	    QFileInfoList fileList = dir.entryInfoList(nameFilters, QDir::Files | QDir::NoSymLinks);
        if (fileList.count() > 0) {
            QFile file(fileList.at(0).absoluteFilePath());
            if (file.open(QFile::ReadOnly)) {
                result = file.readAll();
            }
        }
    }


	if (command.endsWith("tcp")) {
		QStringList skipSuffixes(".zip");
		skipSuffixes << ".fzz";
		QString filename = dir.absoluteFilePath(subfolder + ".zip");
		if (FolderUtils::createZipAndSaveTo(dir, filename, skipSuffixes)) {
			status = 200;
			result = filename;
		}
		else {
			status = 500;
			result = "local zip failure";
		}
    }
}

void FApplication::regeneratePartsDatabase() {
    QMessageBox messageBox(NULL);
    messageBox.setWindowTitle(tr("Regenerate parts database?"));
    messageBox.setText(tr("Regenerating the parts database will take some minutes and you will have to restart Fritzing\n\n") +
                        tr("Would you like to regenerate the parts database?\n")
                        );
    messageBox.setInformativeText("This option is a last resort in case Fritzing's is more-or-less unable to display parts. "
                                  "You may be better off downloading the latest Fritzing release.");
    messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    messageBox.setDefaultButton(QMessageBox::Yes);
    messageBox.setIcon(QMessageBox::Question);
    messageBox.setWindowModality(Qt::WindowModal);
    messageBox.setButtonText(QMessageBox::Yes, tr("Regenerate"));
    messageBox.setButtonText(QMessageBox::No, tr("Cancel"));
    if ((QMessageBox::StandardButton) messageBox.exec() != QMessageBox::Yes) {
        return;
    }

    FileProgressDialog * fileProgressDialog = new FileProgressDialog(tr("Regenerating parts database..."), 0, NULL);
    // these don't seem very accurate (i.e. when progress is at 100%, there is still a lot of work pending)
    // so we are leaving progress indeterminate at present
    //connect(referenceModel, SIGNAL(partsToLoad(int)), fileProgressDialog, SLOT(setMaximum(int)));
    //connect(referenceModel, SIGNAL(loadedPart(int,int)), fileProgressDialog, SLOT(setValue(int)));

    regeneratePartsDatabaseAux(fileProgressDialog);
}

void FApplication::regeneratePartsDatabaseAux(QDialog * progressDialog) {
    ReferenceModel * referenceModel = new CurrentReferenceModel();
    QDir dir = FolderUtils::getAppPartsSubFolder("");
    QString dbPath = dir.absoluteFilePath("parts.db");
    RegenerateDatabaseThread *thread = new RegenerateDatabaseThread(dbPath, progressDialog, referenceModel);
    connect(thread, SIGNAL(finished()), this, SLOT(regenerateDatabaseFinished()));
    FMessageBox::BlockMessages = true;
    thread->start();
}

void FApplication::regenerateDatabaseFinished() {
   RegenerateDatabaseThread * thread = qobject_cast<RegenerateDatabaseThread *>(sender());
   if (thread == NULL) return;

   QDialog * progressDialog = thread->progressDialog();
   if (progressDialog == m_updateDialog) {
       m_updateDialog->installFinished(thread->error());
   }
   else {
       if (thread->error().isEmpty()) {
            QTimer::singleShot(50, Qt::PreciseTimer, this, SLOT(quit()));
       }
       else {
           thread->referenceModel()->deleteLater();
           QMessageBox::warning(NULL, QObject::tr("Regenerate database failed"), thread->error());
       }

       if (progressDialog) {
           thread->progressDialog()->close();
           thread->progressDialog()->deleteLater();
       }
   }

   thread->deleteLater();
}

void FApplication::installNewParts() {
    regeneratePartsDatabaseAux(m_updateDialog);
}
