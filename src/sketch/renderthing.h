#ifndef RENDERTHING_H
#define RENDERTHING_H

#include <QGraphicsView>

struct RenderThing {
	bool selectedItems;
	double printerScale;
	bool blackOnly;
	QRectF imageRect;
	QRectF offsetRect;
	double dpi;
	bool renderBlocker;
	QRectF itemsBoundingRect;
	bool empty;
	bool hideTerminalPoints;

	QList<QGraphicsItem *> getItems(QGraphicsScene * scene);
	void setBoard(QGraphicsItem * board);
private:
	QGraphicsItem * m_board;
};

#endif // RENDERTHING_H
