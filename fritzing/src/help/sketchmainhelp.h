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

/*
 * Central sketch help
 */

#ifndef SKETCHMAINHELP_H_
#define SKETCHMAINHELP_H_

#include <QLabel>
#include <QFrame>
#include <QGraphicsProxyWidget>
#include <QPixmap>

class SketchMainHelpCloseButton : public QLabel {
	Q_OBJECT
	public:
		SketchMainHelpCloseButton(const QString &imagePath, QWidget *parent);
		void doShow();
		void doHide();

	signals:
		void clicked();

	protected:
		void mousePressEvent(QMouseEvent * event);
		QPixmap m_pixmap;
};

class SketchMainHelp;

class SketchMainHelpPrivate : public QFrame {
	Q_OBJECT

	public:
		SketchMainHelpPrivate(const QString &viewString, const QString &htmlText, SketchMainHelp *parent);

	protected slots:
		void doClose();
		void setTransparent();

	protected:
		void enterEvent(QEvent * event);
		void leaveEvent(QEvent * event);
		void enterEventAux();
		void leaveEventAux();
		bool forwardMousePressEvent(QMouseEvent *);

	protected:
		friend class SketchMainHelp;

		SketchMainHelp *m_parent;
		SketchMainHelpCloseButton *m_closeButton;
		volatile bool m_shouldGetTransparent;
};

class SketchMainHelp : public QGraphicsProxyWidget {
public:
	SketchMainHelp(const QString &viewString, const QString &htmlText, bool doShow);
	~SketchMainHelp();

	void doClose();
	void setTransparent();
	void doSetVisible(bool visible);
	bool getVisible();
	bool setMouseWithin(bool);
	const QPixmap & getPixmap();
	bool forwardMousePressEvent(QMouseEvent *);

protected:
	void loadState();
	void saveState();
	SketchMainHelpPrivate *m_son;
	bool m_visible;
	bool m_mouseWithin;
	QPixmap * m_pixmap;

public:
	static double OpacityLevel;
};

#endif /* SKETCHMAINHELP_H_ */
