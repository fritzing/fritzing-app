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

$Revision$:
$Author$:
$Date$

********************************************************************/

#include "fsplashscreen.h"
#include "utils/misc.h"
#include "debugdialog.h"

#include <QTextDocument>
#include <QTextCursor>
#include <QTimer>

#include <time.h>

FSplashScreen::FSplashScreen(const QPixmap & pixmap, Qt::WindowFlags f ) : QSplashScreen(pixmap, f)
{
	QFile file(":/resources/images/splash/splash.xml");
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        DebugDialog::debug("unable to load splash.xml: " + file.errorString());
        return;
    }

    QString errorStr;
    int errorLine;
    int errorColumn;
    QDomDocument domDocument;

    if (!domDocument.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
		DebugDialog::debug(QString("unable to parse splash.xml: %1 %2 %3").arg(errorStr).arg(errorLine).arg(errorColumn));
		return;
	}

	QDomElement root = domDocument.documentElement();
	int sliceDelay = root.attribute("sliceDelaySeconds", "0").toInt();
	if (sliceDelay > 0) {
		QTimer::singleShot(sliceDelay * 1000, this, SLOT(displaySlice()));
	}

	QDomElement item = root.firstChildElement("item");
	while (!item.isNull()) {
		QString id = item.attribute("id");
		if (!id.isEmpty()) {
			int x = item.attribute("x", "0").toInt();
			int y = item.attribute("y", "0").toInt();
			int width = item.attribute("width", "0").toInt();
			int height = item.attribute("height", "0").toInt();
			QString colorName = item.attribute("color");
			MessageThing * messageThing = new MessageThing();
			messageThing->rect.setCoords(x, y, x + width, y + height);
			if (colorName.isEmpty()) {
				messageThing->color = QColor(0, 0, 0);
			}
			else {
				messageThing->color.setNamedColor(colorName);
			}
			m_items.insert(id, messageThing);
		}
		item = item.nextSiblingElement("item");
	}
}

FSplashScreen::~FSplashScreen() {
	foreach (MessageThing * messageThing, m_messages) {
		delete messageThing;
	}
	foreach (MessageThing * messageThing, m_items) {
		delete messageThing;
	}
	foreach (PixmapThing * pixmapThing, m_pixmaps) {
		delete pixmapThing;
	}
}


void FSplashScreen::showMessage(const QString &message, const QString & id, int alignment)
{
	MessageThing * itemMessageThing = m_items.value(id);
	if (itemMessageThing == NULL) return;

	MessageThing * messageThing = new MessageThing;
	messageThing->alignment = alignment;
	messageThing->color = itemMessageThing->color;
	messageThing->rect = itemMessageThing->rect;;
	messageThing->message = message;
	m_messages.append(messageThing);
	repaint();
}


int FSplashScreen::showPixmap(const QPixmap & pixmap, const QString & id)
{
	MessageThing * itemMessageThing = m_items.value(id);
	if (itemMessageThing == NULL) return -1;

	PixmapThing * pixmapThing = new PixmapThing;
	pixmapThing->rect = QRect(itemMessageThing->rect.topLeft(), pixmap.size());
	pixmapThing->pixmap = pixmap;
	m_pixmaps.append(pixmapThing);
	repaint();

	return m_pixmaps.count() - 1;
}

void FSplashScreen::showProgress(int index, double progress) {
	if (index < 0) return;
	if (index >= m_pixmaps.count()) return;

	int w = (int) (this->width() * progress);
	PixmapThing * pixmapThing = m_pixmaps[index];
	pixmapThing->rect.setWidth(w);
	repaint();
}


void FSplashScreen::drawContents ( QPainter * painter )
{
	// copied from QSplashScreen::drawContents
	painter->setRenderHint ( QPainter::Antialiasing, true );				// TODO: might need to be in the stylesheet?

	// pixmaps first, since they go beneath text
	foreach (PixmapThing * pixmapThing, m_pixmaps) {
		painter->drawPixmap(pixmapThing->rect, pixmapThing->pixmap);
		//DebugDialog::debug(QString("pixmapthing %1 %2").arg(pixmapThing->pixmap.width()).arg(pixmapThing->pixmap.height()), pixmapThing->rect);
	}

	foreach (MessageThing * messageThing, m_messages) {
		painter->setPen(messageThing->color);
		if (Qt::mightBeRichText(messageThing->message)) {
			QTextDocument doc;
	#ifdef QT_NO_TEXTHTMLPARSER
			doc.setPlainText(messageThing->message);
	#else
			doc.setHtml(messageThing->message);
	#endif
			doc.setTextWidth(messageThing->rect.width());
			QTextCursor cursor(&doc);
			cursor.select(QTextCursor::Document);
			QTextBlockFormat fmt;
			fmt.setAlignment(Qt::Alignment(messageThing->alignment));
			cursor.mergeBlockFormat(fmt);
			painter->save();
			painter->translate(messageThing->rect.topLeft());
			doc.drawContents(painter);
			painter->restore();
		} else {
			painter->drawText(messageThing->rect, messageThing->alignment, messageThing->message);
		}
	}
}

void FSplashScreen::displaySlice()
{
	QString fname = ":/resources/images/splash/fab_slice%1.png";
	int highest = 0;
	for (int i = 1; i < 100; i++) {
		QFileInfo info(fname.arg(i));
		if (info.exists()) {
			highest = i;
		}
		else break;
	}

	if (highest == 0) return;

	QPixmap bar(":/resources/images/splash/fab_logo_bar.png");
	if (bar.isNull()) return;

	srand ( time(NULL) );
	int ix = (rand() % highest) + 1;

	QPixmap slice(fname.arg(ix));
	if (slice.isNull()) return;

	showPixmap(bar, "logoBar");
	showPixmap(slice, "slice");
}

