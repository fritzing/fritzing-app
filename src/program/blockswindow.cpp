#include "blockswindow.h"

#include <QWebEngineView>
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QWebEngineDownloadRequest>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QSettings>
#include <QCloseEvent>
#include <QVBoxLayout>
#include <QWidget>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QLocale>
#include <QFileDialog>
#include "programwindow.h"
#include "programtab.h"
#include "platformarduino.h"
#include <QApplication>
#include <QClipboard>
#include "../debugdialog.h"

BlocksPage::BlocksPage(QObject *parent)
	: QWebEnginePage(parent)
	, m_checkTimer(nullptr)
{
	m_checkTimer = new QTimer(this);
	connect(m_checkTimer, SIGNAL(timeout()), this, SLOT(checkForCode()));
	m_checkTimer->start(100); // Check every 100ms
}

bool BlocksPage::acceptNavigationRequest(const QUrl &url, QWebEnginePage::NavigationType type, bool isMainFrame)
{
	// Allow all normal navigations, including downloads
	return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
}

QWebEnginePage *BlocksPage::createWindow(QWebEnginePage::WebWindowType /*type*/)
{
	// Allow opening new windows/tabs (necessary for downloads)
	// Return nullptr for Qt to handle downloads via downloadRequested
	return nullptr;
}

void BlocksPage::checkForCode()
{
	// Check if code has been stored in the global JavaScript variable
	runJavaScript("window.blocklyCodeToCopy || ''", [this](const QVariant &result) {
		QString code = result.toString();
		if (!code.isEmpty() && code != m_lastCode) {
			m_lastCode = code;
			DebugDialog::debug(QString("BlocksPage::checkForCode: Found code, length=%1, first 100 chars: %2").arg(code.length()).arg(code.left(100)));
			// Clear the JavaScript variable to avoid processing the same code multiple times
			runJavaScript("window.blocklyCodeToCopy = '';");
			Q_EMIT codeReceived(code);
		}
	});
}

BlocksWindow::BlocksWindow(QWidget *parent)
	: QDialog(parent)
	, m_javaScriptInjected(false)
{
	QFile styleSheet(":/resources/styles/programwindow.qss");

	this->setObjectName("blocksWindow");
	this->setWindowTitle(tr("Blocks Editor"));
	this->setModal(false); // Make the window non-modal
	
	// Add minimize and maximize buttons
	Qt::WindowFlags flags = windowFlags();
	flags |= Qt::WindowMinimizeButtonHint;
	flags |= Qt::WindowMaximizeButtonHint;
	setWindowFlags(flags);

	if (!styleSheet.open(QIODevice::ReadOnly)) {
		qWarning("Unable to open :/resources/styles/programwindow.qss");
	} else {
		QString ss = styleSheet.readAll();
#ifdef Q_OS_MACOS
		int paneLoc = 4;
		int tabBarLoc = 0;
#else
		int paneLoc = -1;
		int tabBarLoc = 5;
#endif
		ss = ss.arg(paneLoc).arg(tabBarLoc);
		this->setStyleSheet(ss);
	}

	// Create a layout for the dialog
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	// Create a custom page that intercepts navigations
	BlocksPage *customPage = new BlocksPage(this);
	connect(customPage, SIGNAL(codeReceived(QString)), this, SLOT(onCodeReceived(QString)));

	// Create the QWebEngineView with the custom page
	m_webView = new QWebEngineView(this);
	m_webView->setPage(customPage);

	// Configure WebEngine settings
	QWebEngineSettings *settings = m_webView->settings();
	settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
	settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
	settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
	settings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);

	// Handle downloads
	QWebEngineProfile *profile = m_webView->page()->profile();
	connect(profile, &QWebEngineProfile::downloadRequested, [this](QWebEngineDownloadRequest *download) {
		QString suggestedFileName = download->downloadFileName();
		if (suggestedFileName.isEmpty()) {
			suggestedFileName = "download";
		}
		
		// Open a dialog to choose where to save the file
		QString savePath = QFileDialog::getSaveFileName(
			this,
			tr("Save File"),
			QDir::home().absoluteFilePath(suggestedFileName),
			tr("All Files (*.*)")
		);
		
		if (!savePath.isEmpty()) {
			download->setDownloadFileName(savePath);
			download->accept();
			DebugDialog::debug(QString("BlocksWindow: Download started: %1").arg(savePath));
		} else {
			// User cancelled
			download->cancel();
		}
	});

	// Connect the load finished signal
	connect(m_webView->page(), SIGNAL(loadFinished(bool)), this, SLOT(onLoadFinished(bool)));

	layout->addWidget(m_webView);

	// Load index.html with language and board parameters
	QString appDirPath = QCoreApplication::applicationDirPath();
	QDir appDir(appDirPath);
	QString blocklyPath = appDir.absoluteFilePath("blockly");
	QString indexHtmlPath = QDir(blocklyPath).absoluteFilePath("index_fritzing.html");

	QFileInfo fileInfo(indexHtmlPath);
	if (fileInfo.exists() && fileInfo.isFile()) {
		QUrl url = QUrl::fromLocalFile(fileInfo.canonicalFilePath());
		
		// Retrieve selected language from preferences
		QSettings settings;
		QString language = settings.value("language").toString();
		if (language.isEmpty()) {
			language = QLocale::system().name();
		}
		
		// Extract language code (e.g., "fr" from "fr_FR")
		QString languageCode = language.toLower();
		int underscorePos = languageCode.indexOf('_');
		if (underscorePos > 0) {
			languageCode = languageCode.left(underscorePos);
		}
		
		// Add language parameter to the URL
		QUrlQuery query;
		query.addQueryItem("lang", languageCode);
		
		// Retrieve selected board from ProgramTab
		ProgramWindow *programWindow = qobject_cast<ProgramWindow *>(this->parent());
		if (programWindow != nullptr) {
			ProgramTab *currentTab = programWindow->getCurrentTab();
			if (currentTab != nullptr) {
				Platform *platform = currentTab->platform();
				if (platform != nullptr && platform->getName() == "Arduino") {
					QString boardName = currentTab->board();
					if (!boardName.isEmpty()) {
						QMap<QString, QString> boards = platform->getBoards();
						QString boardId = boards.value(boardName);
						if (!boardId.isEmpty()) {
							// Transform "arduino:avr:uno" to "arduino_uno"
							// Take the last two parts separated by ":"
							QStringList parts = boardId.split(':');
							if (parts.size() >= 2) {
								QString boardParam = parts[parts.size() - 3] + "_" + parts[parts.size() - 1];
								query.addQueryItem("board", boardParam);
								DebugDialog::debug(QString("BlocksWindow: Adding board parameter: %1").arg(boardParam));
							}
						}
					}
				}
			}
		}
		
		url.setQuery(query);
		m_webView->load(url);
	}

	// Restore window geometry
	QSettings settings_obj;
	if (!settings_obj.value("blockswindow/geometry").isNull()) {
		restoreGeometry(settings_obj.value("blockswindow/geometry").toByteArray());
	} else {
		// Default size
		resize(1024, 768);
	}
}

BlocksWindow::~BlocksWindow()
{
	// Save window geometry
	QSettings settings;
	settings.setValue("blockswindow/geometry", saveGeometry());
}

void BlocksWindow::closeEvent(QCloseEvent *event)
{
	// Save geometry before closing
	QSettings settings;
	settings.setValue("blockswindow/geometry", saveGeometry());
	QDialog::closeEvent(event);
}

void BlocksWindow::onLoadFinished(bool success)
{
	if (success) {
		// Reset flag to allow reinjection after refresh
		m_javaScriptInjected = false;
		// Wait a bit for the DOM to be fully loaded
		QTimer::singleShot(500, this, SLOT(injectJavaScript()));
	}
}

void BlocksWindow::injectJavaScript()
{
	if (m_javaScriptInjected || m_webView == nullptr) {
		return;
	}

	// Inject JavaScript to intercept button click
	// Use a custom URL that will be intercepted by acceptNavigationRequest
	QString script = R"(
		(function() {
			// Function to set up the copy button
			function setupCopyButton() {
				var copyButton = document.getElementById('btn_CopyCode');
				if (copyButton) {
					// Check if listener has already been added by checking a data attribute
					if (copyButton.hasAttribute('data-qt-listener-added')) {
						return true; // Already configured
					}
					
					// Mark the button as having a listener
					copyButton.setAttribute('data-qt-listener-added', 'true');
					
					// Add the listener without cloning the button to preserve other listeners
					copyButton.addEventListener('click', function(e) {
						// Retrieve content of pre_previewArduino div
						var preElement = document.getElementById('pre_previewArduino');
						if (preElement) {
							var code = preElement.textContent || preElement.innerText || '';
							// Store code in a global variable that Qt can read
							window.blocklyCodeToCopy = code;
						}
						// Do not prevent propagation to allow other handlers to function
					}, true); // Use capture phase to execute our handler first
					return true;
				}
				return false;
			}

			// Try immediately
			if (!setupCopyButton()) {
				// If the button doesn't exist yet, wait a bit and retry
				setTimeout(function() {
					setupCopyButton();
				}, 100);
			}
		})();
	)";
	
	m_webView->page()->runJavaScript(script);
	
	m_javaScriptInjected = true;
}

void BlocksWindow::onCodeReceived(const QString &code)
{
	DebugDialog::debug(QString("BlocksWindow::onCodeReceived called with code length: %1").arg(code.length()));
	
	// Copy to clipboard
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(code);
	
	// Send code to the main window's code tab
	ProgramWindow *programWindow = qobject_cast<ProgramWindow *>(parent());
	if (programWindow != nullptr) {
		DebugDialog::debug("BlocksWindow::onCodeReceived - programWindow found, calling insertCodeIntoCurrentTab");
		programWindow->insertCodeIntoCurrentTab(code);
	} else {
		DebugDialog::debug("BlocksWindow::onCodeReceived - programWindow is nullptr!");
	}
}
