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

#ifndef ZOOMCONTROLS_H_
#define ZOOMCONTROLS_H_

#include <QLabel>
#include <QBoxLayout>

#include "../sketch/zoomablegraphicsview.h"

class ZoomButton : public QLabel {
	Q_OBJECT

public:
	enum ZoomType {ZoomIn, ZoomOut};

public:
	ZoomButton(QBoxLayout::Direction dir, ZoomButton::ZoomType type, ZoomableGraphicsView* view, QWidget *parent);



Q_SIGNALS:
	void clicked();

protected Q_SLOTS:
	void zoom();

protected:
	void enterEvent(QEvent *event);
	void leaveEvent(QEvent *event);
	void mousePressEvent(QMouseEvent *event);

	ZoomableGraphicsView *m_owner;
	double m_step;
	ZoomButton::ZoomType m_type;
};

class ZoomControlsPrivate : public QFrame {
	Q_OBJECT
public:
	ZoomControlsPrivate(ZoomableGraphicsView*, QBoxLayout::Direction = QBoxLayout::TopToBottom, QWidget *parent=0);

	ZoomButton *zoomInButton();
	ZoomButton *zoomOutButton();

protected:
	ZoomButton *m_zoomInButton;
	ZoomButton *m_zoomOutButton;

	QBoxLayout *m_boxLayout;
};

class ZoomControls : public ZoomControlsPrivate {
	Q_OBJECT
public:
	ZoomControls(ZoomableGraphicsView *view, QWidget *parent);

protected Q_SLOTS:
	void updateLabel(double zoom);

protected:
	QLabel *m_zoomLabel;
};

#endif /* ZOOMCONTROLS_H_ */
