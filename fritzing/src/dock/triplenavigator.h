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

#ifndef TRIPLENAVIGATOR_H
#define TRIPLENAVIGATOR_H

#include <QFrame>
#include <QSplitter>
#include <QLabel>

#include "miniviewcontainer.h"

class TripleNavigator : public QFrame
{
	Q_OBJECT

public:
	TripleNavigator(QWidget * parent = 0);
	void addView(MiniViewContainer *, const QString & title);
	void showBottomMargin(bool show);

protected:
	QSplitter * m_splitter;
	QFrame *m_bottomMargin;
};

class TripleNavigatorFrame : public QFrame
{
	Q_OBJECT

public:
	TripleNavigatorFrame(MiniViewContainer *, const QString & title, QWidget * parent = 0);

	void hook(MiniViewContainer * miniViewContainer);
	MiniViewContainer * miniViewContainer();

protected:
	bool eventFilter(QObject *obj, QEvent *event);

protected:
	MiniViewContainer * m_miniViewContainer;
};

#endif
