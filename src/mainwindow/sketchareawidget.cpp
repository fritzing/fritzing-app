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


#include <QHBoxLayout>
#include <QLabel>

#include "sketchareawidget.h"

const QString SketchAreaWidget::RoutingStateLabelName = "routingStateLabel";

SketchAreaWidget::SketchAreaWidget(QWidget *contentView, QMainWindow *parent)
	: QFrame(parent)
{
    init(contentView, parent, true, true);
}

SketchAreaWidget::SketchAreaWidget(QWidget *contentView, QMainWindow *parent, bool hasToolBar, bool hasStatusBar)
    : QFrame(parent)
{
    init (contentView, parent, hasToolBar, hasStatusBar);
}

SketchAreaWidget::~SketchAreaWidget() {
	// TODO Auto-generated destructor stub
}

void SketchAreaWidget::init(QWidget *contentView, QMainWindow *parent, bool hasToolBar, bool hasStatusBar) {
    m_contentView = contentView;
    contentView->setParent(this);

    createLayout();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(contentView);
    layout->addWidget(m_toolbar);
    if (!hasToolBar) m_toolbar->hide();
    layout->addWidget(m_statusBarArea);
    if (!hasStatusBar) m_statusBarArea->hide();
    m_statusBarArea->setFixedHeight(parent->statusBar()->height()*3/4);
}

QWidget *SketchAreaWidget::contentView() {
	return m_contentView;
}

void SketchAreaWidget::createLayout() {
	m_toolbar = new QFrame(this);
	m_toolbar->setObjectName("sketchAreaToolbar");
    m_toolbar->setFixedHeight(66);

	QFrame *leftButtons = new QFrame(m_toolbar);
	m_leftButtonsContainer = new QHBoxLayout(leftButtons);
    m_leftButtonsContainer->setMargin(0);
    m_leftButtonsContainer->setSpacing(0);

	QFrame *middleButtons = new QFrame(m_toolbar);
	middleButtons->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::MinimumExpanding);
	m_middleButtonsContainer = new QVBoxLayout(middleButtons);
	m_middleButtonsContainer->setSpacing(0);
    m_middleButtonsContainer->setMargin(0);

	QFrame *rightButtons = new QFrame(m_toolbar);
	m_rightButtonsContainer = new QHBoxLayout(rightButtons);
    m_rightButtonsContainer->setMargin(0);
    m_rightButtonsContainer->setSpacing(0);

	QHBoxLayout *toolbarLayout = new QHBoxLayout(m_toolbar);
    toolbarLayout->setMargin(0);
	toolbarLayout->setSpacing(0);
	toolbarLayout->addWidget(leftButtons);
	toolbarLayout->addWidget(middleButtons);
	toolbarLayout->addWidget(rightButtons);

	m_statusBarArea = new QFrame(this);
	m_statusBarArea->setObjectName("statusBarContainer");
	QVBoxLayout *statusbarlayout = new QVBoxLayout(m_statusBarArea);
	statusbarlayout->setMargin(0);
	statusbarlayout->setSpacing(0);
}

void SketchAreaWidget::setToolbarWidgets(QList<QWidget*> widgets) {
    bool goLeft = true;
	foreach(QWidget* widget, widgets) {
		if(widget->objectName() != RoutingStateLabelName) {
            if (goLeft) m_leftButtonsContainer->addWidget(widget);
            else m_rightButtonsContainer->addWidget(widget);
		} else {
            m_middleButtonsContainer->addSpacerItem(new QSpacerItem(0,1,QSizePolicy::Maximum));
			m_middleButtonsContainer->addWidget(widget);
            m_middleButtonsContainer->addSpacerItem(new QSpacerItem(0,1,QSizePolicy::Maximum));
            goLeft = false;
		}
	}
}

void SketchAreaWidget::addStatusBar(QStatusBar *statusBar) {
	m_statusBarArea->layout()->addWidget(statusBar);
}

QWidget *SketchAreaWidget::separator(QWidget* parent) {
	QLabel *separator = new QLabel(parent);
	separator->setPixmap(QPixmap(":/resources/images/toolbar_icons/toolbar_separator.png"));
    separator->setStyleSheet("margin-left: 1px; margin-right: 1px;");
    separator->setObjectName("ToolBarSeparator");
	return separator;
}

void SketchAreaWidget::setRoutingStatusLabel(ExpandingLabel * rsl) {
	m_routingStatusLabel = rsl;
}

ExpandingLabel * SketchAreaWidget::routingStatusLabel() {
	return m_routingStatusLabel;
}

QFrame * SketchAreaWidget::toolbar() {
	return m_toolbar;
}
