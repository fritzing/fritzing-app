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

#include "triplenavigator.h"
#include "../debugdialog.h"
#include "../utils/misc.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>

TripleNavigator::TripleNavigator( QWidget * parent )
	: QFrame(parent)
{
	m_splitter = new QSplitter(Qt::Horizontal, this);
	m_splitter->setChildrenCollapsible(false);

	m_bottomMargin = new QFrame(this);
	m_bottomMargin->setFixedHeight(6);
	m_bottomMargin->setObjectName("tripleNavigatorBottomMargin");

	QFrame *realArea = new QFrame(this);
	QHBoxLayout *layout = new QHBoxLayout(realArea);
	layout->addWidget(m_splitter);
	layout->setMargin(1);

	QVBoxLayout *mainLO = new QVBoxLayout(this);
	mainLO->setMargin(0);
	mainLO->setSpacing(0);

    mainLO->addWidget(realArea);
    mainLO->addWidget(m_bottomMargin);
}

void TripleNavigator::showBottomMargin(bool show) {
	m_bottomMargin->setVisible(show);
}

void TripleNavigator::addView(MiniViewContainer * miniViewContainer, const QString & title)
{
	miniViewContainer->setTitle(title);
	TripleNavigatorFrame * frame = new TripleNavigatorFrame(miniViewContainer, title, this);

	for (int i = 0; i < m_splitter->count(); i++) {
		frame->hook(((TripleNavigatorFrame *) m_splitter->widget(i))->miniViewContainer());
	}

	// hook everybody to everybody else
	m_splitter->addWidget(frame);
	for (int i = 0; i < m_splitter->count(); i++) {
		((TripleNavigatorFrame *) m_splitter->widget(i))->hook(miniViewContainer);
	}
}

////////////////////////////////////

TripleNavigatorFrame::TripleNavigatorFrame(MiniViewContainer * miniViewContainer, const QString & title, QWidget * parent) : QFrame(parent)
{
	Q_UNUSED(title);
	m_miniViewContainer = miniViewContainer;
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setMargin(0);
	this->setLayout(layout);
	layout->addWidget(miniViewContainer);

	installEventFilter(this);
}

void TripleNavigatorFrame::hook(MiniViewContainer * miniViewContainer) {
	connect(miniViewContainer, SIGNAL(navigatorMousePressedSignal(MiniViewContainer *)),
		    m_miniViewContainer->miniView(), SLOT(navigatorMousePressedSlot(MiniViewContainer *)) );
	connect(miniViewContainer, SIGNAL(navigatorMouseEnterSignal(MiniViewContainer *)),
		    m_miniViewContainer->miniView(), SLOT(navigatorMouseEnterSlot(MiniViewContainer *)) );
	connect(miniViewContainer, SIGNAL(navigatorMouseLeaveSignal(MiniViewContainer *)),
		    m_miniViewContainer->miniView(), SLOT(navigatorMouseLeaveSlot(MiniViewContainer *)) );
}

MiniViewContainer * TripleNavigatorFrame::miniViewContainer() {
	return m_miniViewContainer;
}

bool TripleNavigatorFrame::eventFilter(QObject *obj, QEvent *event)
{
	if (m_miniViewContainer) {
		switch (event->type()) {
			case QEvent::MouseButtonPress:
			case QEvent::NonClientAreaMouseButtonPress:
				if (obj == this || isParent(this, obj)) {
					m_miniViewContainer->miniViewMousePressedSlot();
				}
				break;
			case QEvent::Enter:
				if (obj == this || isParent(this, obj)) {
					m_miniViewContainer->miniViewMouseEnterSlot();
				}
				break;
			case QEvent::Leave:
				if (obj == this || isParent(this, obj)) {
					m_miniViewContainer->miniViewMouseLeaveSlot();
				}
				break;
			default:
				break;
		}
	}

	return QObject::eventFilter(obj, event);
}
