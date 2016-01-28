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

$Revision: 6904 $:
$Author: irascibl@gmail.com $:
$Date: 2013-02-26 16:26:03 +0100 (Di, 26. Feb 2013) $

********************************************************************/

#ifndef CONNECTORRECTANGLE_H_
#define CONNECTORRECTANGLE_H_

#include "cornerhandler.h"

class ConnectorRectangle : public QObject {
	Q_OBJECT

public:
	enum State {
		Normal = 0x00000,
		Highlighted = 0x00001,
		Hover = 0x00002,
		Selected = 0x00003
	};

	ConnectorRectangle(QGraphicsItem* owner, bool withHandlers = true);
	~ConnectorRectangle();
	QGraphicsItem *owner();
	void resizeRect(double x1, double y1, double x2, double y2);
	bool isResizable();

	void resizingStarted();
	void resizingFinished();

	double currentScale();
	void setMinSize(double minWidth, double minHeight);

	void setHandlersVisible(bool visible);
	QRectF handlerRect(Qt::Corner corner);
	QRectF errorIconRect();
	void paint(QPainter *painter);

signals:
	void resizeSignal(double x1, double y1, double x2, double y2);
	void isResizableSignal(bool & resizable);

protected:
	void setHandlerRect(CornerHandler* handler);
	void placeHandlers();

	QGraphicsItem *m_owner;

	CornerHandler *m_topLeftHandler;
	CornerHandler *m_topRightHandler;
	CornerHandler *m_bottomRightHandler;
	CornerHandler *m_bottomLeftHandler;
	QList<CornerHandler*> m_cornerHandlers;

	double m_minWidth;
	double m_minHeight;

	bool m_firstPaint;

public:
	static double ErrorIconSize;
};

#endif /* CONNECTORRECTANGLE_H_ */
