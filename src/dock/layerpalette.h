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

#ifndef LAYERPALETTE_H
#define LAYERPALETTE_H

#include <QFrame>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QSpacerItem>
#include <QScrollArea>

#include "../viewlayer.h"

class ViewLayerCheckBox : public QCheckBox 
{
Q_OBJECT
public:
	ViewLayerCheckBox(QWidget * parent = NULL);
	~ViewLayerCheckBox();

	void setViewLayer(ViewLayer *);
	ViewLayer * viewLayer();

protected:
	ViewLayer * m_viewLayer;
};

class LayerPalette : public QScrollArea
{
Q_OBJECT
public:
	LayerPalette(QWidget * parent = NULL);
	~LayerPalette();

	void updateLayerPalette(LayerHash & viewLayers, LayerList & keys);
	void resetLayout(LayerHash & viewLayers, LayerList & keys);
	void setShowAllLayersAction(QAction *);
	void setHideAllLayersAction(QAction *);

protected slots:
	void setLayerVisibility(bool vis);
	void setAllLayersVisible(bool vis);

protected:
	QCheckBox * m_showAllWidget;
	QList <ViewLayerCheckBox *> m_checkBoxes;
	QVBoxLayout * m_mainLayout;
	QGroupBox * m_groupBox;

    QAction *m_showAllLayersAct;
	QAction *m_hideAllLayersAct;
};

#endif
