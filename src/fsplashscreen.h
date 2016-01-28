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

#ifndef FSPLASHSCREEN_H
#define FSPLASHSCREEN_H

#include <QSplashScreen>
#include <QPainter>
#include <QHash>

struct MessageThing {
	QString message;
	QRect rect;
	int alignment;
	QColor color;
};

struct PixmapThing {
	QPixmap pixmap;
	QRect rect;
};

class FSplashScreen : public QSplashScreen {
	Q_OBJECT
public:
	FSplashScreen(const QPixmap & pixmap = QPixmap(), Qt::WindowFlags f = 0);
	~FSplashScreen();

	void showMessage(const QString &message, const QString & id, int alignment = Qt::AlignLeft);
	int showPixmap(const QPixmap &pixmap, const QString & id);
	void showProgress(int index, double progress);			// progress is from 0.0 to 1.0;

protected:
	void drawContents ( QPainter * painter );

public slots:
	void displaySlice();

protected:
    QPixmap m_pixmap;
	QList<MessageThing *> m_messages;
	QList<PixmapThing *> m_pixmaps;
	QHash<QString, MessageThing *> m_items;
};

#endif
