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

#ifndef VIRTUALWIRE_H
#define VIRTUALWIRE_H

#include "clipablewire.h"

class VirtualWire : public ClipableWire
{
	Q_OBJECT

public:
	VirtualWire( ModelPart * modelPart, ViewLayer::ViewID,  const ViewGeometry & , long id, QMenu* itemMenu  ); 
	~VirtualWire();
	
	void setHidden(bool hidden);
	void setInactive(bool inactivate);
	void tempRemoveAllConnections();
	void setColorWasNamed(bool);
	bool colorWasNamed();
    QPainterPath shape() const;
	
protected:
	void paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget );	
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void connectionChange(ConnectorItem * onMe, ConnectorItem * onIt, bool connect);
 	class FSvgRenderer * setUpConnectors(class ModelPart *, ViewLayer::ViewID);
	void hideConnectors();	
	void inactivateConnectors();

public:
    static const double ShapeWidthExtra;

protected:
	bool m_colorWasNamed;

};

#endif
