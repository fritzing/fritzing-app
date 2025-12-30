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

#ifndef BLOCKSWINDOW_H
#define BLOCKSWINDOW_H

#include <QDialog>
#include <QPointer>
#include <QWebEnginePage>

QT_BEGIN_NAMESPACE
class QWebEngineView;
class QTimer;
QT_END_NAMESPACE

class BlocksPage : public QWebEnginePage
{
	Q_OBJECT

public:
	explicit BlocksPage(QObject *parent = nullptr);
	QWebEnginePage *createWindow(QWebEnginePage::WebWindowType type) override;

protected:
	bool acceptNavigationRequest(const QUrl &url, QWebEnginePage::NavigationType type, bool isMainFrame) override;

public Q_SLOTS:
	void checkForCode();

Q_SIGNALS:
	void codeReceived(const QString &code);

private:
	QTimer *m_checkTimer;
	QString m_lastCode;
};

class BlocksWindow : public QDialog
{
	Q_OBJECT

public:
	explicit BlocksWindow(QWidget *parent = nullptr);
	~BlocksWindow();

protected:
	void closeEvent(QCloseEvent *event) override;

private Q_SLOTS:
	void onLoadFinished(bool success);
	void injectJavaScript();
	void onCodeReceived(const QString &code);

private:
	QPointer<QWebEngineView> m_webView;
	bool m_javaScriptInjected;
};

#endif // BLOCKSWINDOW_H

