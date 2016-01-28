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



#ifndef SKETCHAREAWIDGET_H_
#define SKETCHAREAWIDGET_H_

#include <QFrame>
#include <QHBoxLayout>
#include <QStatusBar>
#include <QMainWindow>

class SketchAreaWidget : public QFrame {
    Q_OBJECT
public:
	SketchAreaWidget(QWidget *contentView, QMainWindow *parent);
    SketchAreaWidget(QWidget *contentView, QMainWindow *parent, bool hasToolBar, bool hasStatusBar);
	virtual ~SketchAreaWidget();

	QWidget* contentView();

	void setToolbarWidgets(QList<QWidget*> buttons);
	void addStatusBar(QStatusBar *);
	static QWidget *separator(QWidget* parent);
	class ExpandingLabel * routingStatusLabel();
	void setRoutingStatusLabel(ExpandingLabel *);
	QFrame * toolbar();

protected:
    void init(QWidget *contentView, QMainWindow *parent, bool hasToolBar, bool hasStatusBar);
	void createLayout();

public:
	static const QString RoutingStateLabelName;

protected:
	QWidget *m_contentView;

	QFrame *m_toolbar;
	QHBoxLayout *m_leftButtonsContainer;
	QVBoxLayout *m_middleButtonsContainer;
	QHBoxLayout *m_rightButtonsContainer;
	QFrame *m_statusBarArea;
	class ExpandingLabel * m_routingStatusLabel;

};

#endif /* SKETCHAREAWIDGET_H_ */
