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

$Revision: 6112 $:
$Author: cohen@irascible.com $:
$Date: 2012-06-28 00:18:10 +0200 (Do, 28. Jun 2012) $

********************************************************************/

#ifndef MINIVIEW_H
#define MINIVIEW_H

#include <QFrame>
#include <QBrush>
#include <QPainter>
#include <QPointer>
#include <QGraphicsView>
#include <QTimer>
#include <QPixmap>

class MiniView : public QFrame
{
	Q_OBJECT
	
public:
	MiniView(QWidget *parent=0);
	virtual ~MiniView(); 

	void setView(QGraphicsView *);	
	QGraphicsView* view();
	void setTitle(const QString & title);
	QRectF sceneRect();
	
protected:
	void resizeEvent ( QResizeEvent * event ); 
	void mousePressEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *);

public slots:
	void updateSceneRect();
	void updateScene();
	void reallyUpdateScene();
	void navigatorMousePressedSlot(class MiniViewContainer *);
	void navigatorMouseEnterSlot(class MiniViewContainer *);
	void navigatorMouseLeaveSlot(class MiniViewContainer *);

signals:
	void rectChangedSignal();
	void miniViewMousePressedSignal();
	
protected:
	QPointer<QGraphicsView> m_graphicsView;
	QString m_title;
	int m_titleWeight;
	QColor m_titleColor;
	bool m_selected;
	int m_lastHeight;
	QTimer m_updateSceneTimer;
	QRectF m_sceneRect;
};

#endif
