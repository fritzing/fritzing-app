/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2013 Fachhochschule Potsdam - http://fh-potsdam.de

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

#ifndef VIEWSWITCHER_H_
#define VIEWSWITCHER_H_

#include <QLabel>
#include <QFrame>
#include <QHBoxLayout>
#include <QBitmap>
#include <QHash>

class ViewSwitcher;

class ViewSwitcherButton : public QLabel {
	Q_OBJECT
 
	public:
		ViewSwitcherButton(const QString &view, const QString & text, int maxWidth, Qt::Alignment, int index, ViewSwitcher *parent);
		void setFocus(bool active);
		void setActive(bool selected);
		void setHover(bool hover);
		int index();

		static void cleanup();

	signals:
		void clicked(ViewSwitcherButton*);

	protected:
		void enterEvent(QEvent *event);
		void leaveEvent(QEvent *event);
		void mousePressEvent(QMouseEvent *event);
		void updateImage();

	protected:
		bool m_focus;
		bool m_active;
		bool m_hover;
		int m_index;
		QString m_resourcePath;
		ViewSwitcher *m_parent;

	protected:
		static QString ResourcePathPattern;
		static QHash<QString, QPixmap *> Pixmaps;
};

class ViewSwitcher : public QFrame {
	Q_OBJECT
	public:
		ViewSwitcher(bool includeCodeView);
		~ViewSwitcher();

		const QBitmap * getMask();
		void connectClose(QObject * target, const char* slot);
		QRect buttonHolderGeometry();

		static void cleanup();

	signals:
		void viewSwitched(int index);

	public slots:
		void updateHoverState(ViewSwitcherButton* hoverOne = NULL);
		void viewSwitchedTo(int);

	protected slots:
		void updateState(ViewSwitcherButton* clickedOne, bool doEmit=true);

	protected:
		void enterEvent(QEvent *event);
		void leaveEvent(QEvent *event);
		void resizeEvent(QResizeEvent * event);

		ViewSwitcherButton *createButton(const QString &view, const QString &text, int maxWidth, Qt::Alignment);
		void createMask();

	protected:
		QHBoxLayout *m_layout;
		QList<ViewSwitcherButton*> m_buttons;
		static QBitmap * m_mask;
		QWidget * m_buttonHolder;
};

#endif /* VIEWSWITCHER_H_ */
