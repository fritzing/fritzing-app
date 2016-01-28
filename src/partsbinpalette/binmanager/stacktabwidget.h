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


#ifndef STACKTABWIDGET_H_
#define STACKTABWIDGET_H_

#include <QTabWidget>
#include <QTabBar>
#include <QPushButton>
#include <QPointer>

// originally extracted from http://wiki.qtcentre.org/index.php?title=Movable_Tabs
class StackTabWidget : public QTabWidget {
	Q_OBJECT
	public:
		StackTabWidget(QWidget *parent);
		class StackTabBar *stackTabBar();

	public slots:
		void informCurrentChanged(int index);
		void informTabCloseRequested(int index);

	signals:
		void currentChanged(StackTabWidget *, int index);
		void tabCloseRequested(StackTabWidget *, int index);

};

#endif /* STACKTABWIDGET_H_ */
