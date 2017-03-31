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

$Revision: 6912 $:
$Author: irascibl@gmail.com $:
$Date: 2013-03-09 08:18:59 +0100 (Sa, 09. Mrz 2013) $

********************************************************************/



#ifndef BREADBOARDSKETCHWIDGET_H
#define BREADBOARDSKETCHWIDGET_H

#include "sketchwidget.h"

class BreadboardSketchWidget : public SketchWidget
{
	Q_OBJECT

public:
    BreadboardSketchWidget(ViewLayer::ViewID, QWidget *parent=0);

	void addViewLayers();
	void initWire(Wire *, int penWidth);
	bool canDisconnectAll();
	bool ignoreFemale();
	void addDefaultParts();
	void showEvent(QShowEvent * event);
	double getWireStrokeWidth(Wire *, double wireWidth);
	void getBendpointWidths(class Wire *, double w, double & w1, double & w2, bool & negativeOffsetRect);
    void colorWiresByLength(bool);
    bool coloringWiresByLength();

protected:
	void setWireVisible(Wire * wire);
	bool collectFemaleConnectees(ItemBase *, QSet<ItemBase *> &);
	void findConnectorsUnder(ItemBase * item);
	bool checkUnder();
	bool disconnectFromFemale(ItemBase * item, QHash<long, ItemBase *> & savedItems, ConnectorPairHash &, bool doCommand, bool rubberBandLegEnabled, QUndoCommand * parentCommand);
	BaseCommand::CrossViewType wireSplitCrossView();
	bool canDropModelPart(ModelPart * modelPart); 
	void getLabelFont(QFont &, QColor &, ItemBase *);
	void setNewPartVisible(ItemBase *);
	double defaultGridSizeInches();
	ViewLayer::ViewLayerID getLabelViewLayerID(ItemBase *);
	double getTraceWidth();
	const QString & traceColor(ViewLayer::ViewLayerPlacement);

    bool m_colorWiresByLength;
};

#endif
