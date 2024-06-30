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



#include "debugdialog.h"
#include "qevent.h"
#ifndef QT_NO_DEBUG
#include "utils/folderutils.h"
#endif
#include <QEvent>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QtDebug>
#include <QIcon>

DebugDialog* DebugDialog::singleton = nullptr;
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

	QString path;
#ifndef QT_NO_DEBUG
	path = FolderUtils::getTopLevelUserDataStorePath();
#endif
	path += "/debug.txt";

	m_file.setFileName(path);
	m_file.remove();
}

DebugDialog::~DebugDialog()
{
	if (m_textEdit != nullptr) {
		delete m_textEdit;
	}
}

bool DebugDialog::event(QEvent *e) {
	if (e->type() == DebugEventType) {
		this->m_textEdit->append(((DebugEvent *) e)->m_message);
		Q_EMIT debugBroadcast(((DebugEvent *) e)->m_message, ((DebugEvent *) e)->m_debugLevel,((DebugEvent *) e)->m_ancestor);
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

void DebugDialog::debug(QString prefix, const QSettings::Status &status, QObject *ancestor) {
	if (status == QSettings::NoError) {
		return;
	}

	QString statusMessage;
	switch (status) {
	case QSettings::AccessError:
		statusMessage = "AccessError: The application does not have permission to read or write the specified settings.";
		break;
	case QSettings::FormatError:
		statusMessage = "FormatError: The settings data is corrupted.";
		break;
	default:
		statusMessage = "Unknown error.";
		break;
	}
	QString msg = prefix + ": " + statusMessage;

	debug(msg, Debug, ancestor);
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

void DebugDialog::debug_ts(QString message, DebugLevel debugLevel, QObject * ancestor) {
	DebugDialog::debug(QString("[%1] %2").arg(QTime::currentTime().toString("HH:mm:ss.zzz"), message), debugLevel, ancestor);
}

void DebugDialog::debug(QString message, DebugLevel debugLevel, QObject * ancestor) {

	if (!m_enabled) return;


	if (singleton == nullptr) {
		new DebugDialog();
		//singleton->show();
	}

	if (debugLevel < singleton->m_debugLevel) {
		return;
	}

	qDebug() << message;

	if (m_file.open(QIODevice::Append | QIODevice::Text)) {
		QTextStream out(&m_file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
		out.setCodec("UTF-8");
#endif
		out << message << "\n";
		m_file.close();
	}
	auto* de = new DebugEvent(message, debugLevel, ancestor);
	QCoreApplication::postEvent(singleton, de);
}

void DebugDialog::hideDebug() {
	if (singleton != nullptr) {
		singleton->hide();
	}
}

void DebugDialog::showDebug() {
	if (singleton == nullptr) {
		new DebugDialog();
	}

	singleton->show();
}

void DebugDialog::closeDebug() {
	if (singleton != nullptr) {
		singleton->close();
	}
}


bool DebugDialog::visible() {
	if (singleton == nullptr) return false;

	return singleton->isVisible();
}

bool DebugDialog::connectToBroadcast(QObject * receiver, const char* slot) {
	if (singleton == nullptr) {
		new DebugDialog();
	}

	return connect(singleton, SIGNAL(debugBroadcast(QString,QObject*)), receiver, slot ) != nullptr;
}

void DebugDialog::setDebugLevel(DebugLevel debugLevel) {
	if (singleton == nullptr) {
		new DebugDialog();

	}

	singleton->m_debugLevel = debugLevel;
}

void DebugDialog::cleanup() {
	if (singleton != nullptr) {
		delete singleton;
		singleton = nullptr;
	}
}

bool DebugDialog::enabled() {
	return m_enabled;
}

void DebugDialog::setEnabled(bool enabled) {
	m_enabled = enabled;
}

QString DebugDialog::createKeyTag(const QKeyEvent *event) {
	static const QMap<int, QString> KeyNames = {
		{ Qt::Key_Escape, "Esc" },
		{ Qt::Key_Tab, "Tab" },
		{ Qt::Key_Backspace, "Backspace" },
		{ Qt::Key_Return, "Enter" },
		{ Qt::Key_Insert, "Ins" },
		{ Qt::Key_Delete, "Del" },
		{ Qt::Key_Pause, "Pause" },
		{ Qt::Key_Print, "Print" },
		{ Qt::Key_SysReq, "SysRq" },
		{ Qt::Key_Home, "Home" },
		{ Qt::Key_End, "End" },
		{ Qt::Key_Left, "←" },
		{ Qt::Key_Up, "↑" },
		{ Qt::Key_Right, "→" },
		{ Qt::Key_Down, "↓" },
		{ Qt::Key_PageUp, "PageUp" },
		{ Qt::Key_PageDown, "PageDown" },
		{ Qt::Key_Shift, "Shift" },
		{ Qt::Key_Control, "Ctrl" },
		{ Qt::Key_Meta, "Meta" },
		{ Qt::Key_Alt, "Alt" },
		{ Qt::Key_AltGr, "AltGr" },
		{ Qt::Key_CapsLock, "CapsLock" },
		{ Qt::Key_NumLock, "NumLock" },
		{ Qt::Key_ScrollLock, "ScrollLock" },
		{ Qt::Key_F1, "F1" },
		{ Qt::Key_F2, "F2" },
		{ Qt::Key_F3, "F3" },
		{ Qt::Key_F4, "F4" },
		{ Qt::Key_F5, "F5" },
		{ Qt::Key_F6, "F6" },
		{ Qt::Key_F7, "F7" },
		{ Qt::Key_F8, "F8" },
		{ Qt::Key_F9, "F9" },
		{ Qt::Key_F10, "F10" },
		{ Qt::Key_F11, "F11" },
		{ Qt::Key_F12, "F12" },
		{ Qt::Key_F13, "F13" },
		{ Qt::Key_F14, "F14" },
		{ Qt::Key_F15, "F15" },
		{ Qt::Key_F16, "F16" },
		{ Qt::Key_F17, "F17" },
		{ Qt::Key_F18, "F18" },
		{ Qt::Key_F19, "F19" },
		{ Qt::Key_F20, "F20" },
		{ Qt::Key_F21, "F21" },
		{ Qt::Key_F22, "F22" },
		{ Qt::Key_F23, "F23" },
		{ Qt::Key_F24, "F24" }
		// add more keys as needed
	};

	int key = event->key();
	QString keyName = KeyNames.value(key, event->text().toHtmlEscaped());

	QString modifierText;
	if (event->modifiers() & Qt::ShiftModifier && key != Qt::Key_Shift) {
		modifierText += "Shift+";
	}
	if (event->modifiers() & Qt::ControlModifier && key != Qt::Key_Control) {
		modifierText += "Ctrl+";
	}
	if (event->modifiers() & Qt::AltModifier && key != Qt::Key_Alt) {
		modifierText += "Alt+";
	}
	if (event->modifiers() & Qt::MetaModifier && key != Qt::Key_Meta) {
		modifierText += "Meta+";
	}

	if (!modifierText.isEmpty()) {
		keyName = QString("%1%2").arg(modifierText, keyName);
	}

	return QString("<kbd>%1</kbd>").arg(keyName);
}
