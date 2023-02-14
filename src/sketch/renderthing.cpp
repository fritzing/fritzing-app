#include "renderthing.h"

#include "fgraphicsscene.h"

QList<QGraphicsItem *> RenderThing::getItems(QGraphicsScene * scene) {
	QList<QGraphicsItem *> items;
	if (selectedItems) {
		items = scene->selectedItems();
	}
	else if (!m_board) {
		items = scene->items();
	}
	else {
		items = scene->collidingItems(m_board);
		items << m_board;
	}
	return items;
}

void RenderThing::setBoard(QGraphicsItem *board) {
	m_board = board;
	if (board) {
		offsetRect = m_board->sceneBoundingRect();
	}
}
