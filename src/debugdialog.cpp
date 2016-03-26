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

$Revision: 6112 $:
$Author: cohen@irascible.com $:
$Date: 2012-06-28 00:18:10 +0200 (Do, 28. Jun 2012) $

********************************************************************/



#include "debugdialog.h"
#include "utils/folderutils.h"
#include <QEvent>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QtDebug>
#include <QIcon>

DebugDialog* DebugDialog::singleton = NULL;
QFile DebugDialog::m_file;

#ifdef QT_NO_DEBUG
bool DebugDialog::m_enabled = false;
#else
bool DebugDialog::m_enabled = true;
#endif

QEvent::Type DebugEventType = (QEvent::Type) (QEvent::User + 1);

class DebugEvent : public QEvent
{
public:
	QString m_message;
	QObject * m_ancestor;
	DebugDialog::DebugLevel m_debugLevel;

	DebugEvent(QString message, DebugDialog::DebugLevel debugLevel, QObject * ancestor) : QEvent(DebugEventType) {
		this->m_message = message;
		this->m_ancestor = ancestor;
		this->m_debugLevel = debugLevel;
	}
};

DebugDialog::DebugDialog(QWidget *parent)
	: QDialog(parent)
{
	// Let's set the icon
	this->setWindowIcon(QIcon(QPixmap(":resources/images/fritzing_icon.png")));

	singleton = this;
	m_debugLevel = DebugDialog::Debug;
	setWindowTitle(tr("for debugging"));
	resize(400, 300);
	m_textEdit = new QTextEdit(this);
	m_textEdit->setGeometry(QRect(10, 10, 381, 281));

    QString path = FolderUtils::getTopLevelUserDataStorePath();
    path += "/debug.txt";

	m_file.setFileName(path);
	m_file.remove();
}

DebugDialog::~DebugDialog()
{
	if (m_textEdit) {
		delete m_textEdit;
	}
}

bool DebugDialog::event(QEvent *e) {
	if (e->type() == DebugEventType) {
		this->m_textEdit->append(((DebugEvent *) e)->m_message);
		emit debugBroadcast(((DebugEvent *) e)->m_message, ((DebugEvent *) e)->m_debugLevel,((DebugEvent *) e)->m_ancestor);
		// need to delete these events at some point...
		// but it's tricky if the message is being used elsewhere
		return true;
	}
	else {
		return QDialog::event(e);
	}
}

void DebugDialog::resizeEvent(QResizeEvent *e) {
	int w = this->width();
	int h = this->height();
	QRect geom = this->m_textEdit->geometry();
	geom.setWidth(w - geom.left() - geom.left());
	geom.setHeight( h - geom.top() - geom.top());
	this->m_textEdit->setGeometry(geom);
	return QDialog::resizeEvent(e);

}

void DebugDialog::debug(QString prefix, const QPointF &point, DebugLevel debug, QObject *ancestor) {
	QString msg = prefix+QString(" point: x=%1 y=%2").arg(point.x()).arg(point.y());
	DebugDialog::debug(msg,debug,ancestor);
}

void DebugDialog::debug(QString prefix, const QRectF &rect, DebugLevel debug, QObject *ancestor) {
	QString msg = prefix+QString(" rect: x=%1 y=%2 w=%3 h=%4")
		.arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height());
	DebugDialog::debug(msg,debug,ancestor);
}

void DebugDialog::debug(QString prefix, const QPoint &point, DebugLevel debug, QObject *ancestor) {
	QString msg = prefix+QString(" point: x=%1 y=%2").arg(point.x()).arg(point.y());
	DebugDialog::debug(msg,debug,ancestor);
}

void DebugDialog::debug(QString prefix, const QRect &rect, DebugLevel debug, QObject *ancestor) {
	QString msg = prefix+QString(" rect: x=%1 y=%2 w=%3 h=%4")
		.arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height());
	DebugDialog::debug(msg,debug,ancestor);
}

void DebugDialog::debug(QString message, DebugLevel debugLevel, QObject * ancestor) {

	if (!m_enabled) return;


	if (singleton == NULL) {
		new DebugDialog();
		//singleton->show();
	}

	if (debugLevel < singleton->m_debugLevel) {
		return;
	}

	qDebug() << message;

   	if (m_file.open(QIODevice::Append | QIODevice::Text)) {
   		QTextStream out(&m_file);
		out.setCodec("UTF-8");
   		out << message << "\n";
		m_file.close();
	}
	DebugEvent* de = new DebugEvent(message, debugLevel, ancestor);
	QCoreApplication::postEvent(singleton, de);
}

void DebugDialog::hideDebug() {
	if (singleton != NULL) {
		singleton->hide();
	}
}

void DebugDialog::showDebug() {
	if (singleton == NULL) {
		new DebugDialog();
	}

	singleton->show();
}

void DebugDialog::closeDebug() {
	if (singleton != NULL) {
		singleton->close();
	}
}


bool DebugDialog::visible() {
	if (singleton == NULL) return false;

	return singleton->isVisible();
}

bool DebugDialog::connectToBroadcast(QObject * receiver, const char* slot) {
	if (singleton == NULL) {
		new DebugDialog();
	}

	return connect(singleton, SIGNAL(debugBroadcast(const QString &, QObject *)), receiver, slot );
}

void DebugDialog::setDebugLevel(DebugLevel debugLevel) {
	if (singleton == NULL) {
		new DebugDialog();

	}

	singleton->m_debugLevel = debugLevel;
}

void DebugDialog::cleanup() {
	if (singleton) {
		delete singleton;
		singleton = NULL;
	}
}

bool DebugDialog::enabled() {
	return m_enabled;
}

void DebugDialog::setEnabled(bool enabled) {
	m_enabled = enabled;
}

