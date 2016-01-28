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

$Revision: 6980 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-22 01:45:43 +0200 (Mo, 22. Apr 2013) $

********************************************************************/

#ifndef GROUNDPLANE_H
#define GROUNDPLANE_H

#include "paletteitem.h"

class GroundPlane : public PaletteItem
{

public:
	GroundPlane( ModelPart * modelPart, ViewLayer::ViewID,  const ViewGeometry & , long id, QMenu* itemMenu, bool doLabel); 

 	bool setUpImage(ModelPart* modelPart, const LayerHash & viewLayers, LayerAttributes &);
	void saveParams();
	void getParams();
	QString retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor);
	class ConnectorItem * connector0();
	bool hasCustomSVG();
	void setProp(const QString & prop, const QString & value);
	QString svg();
	bool hasPartLabel();
	void loadLayerKin( const LayerHash & viewLayers, ViewLayer::ViewLayerPlacement viewLayerPlacement);
	void addedToScene(bool temporary);
	bool hasPartNumberProperty();
	bool rotationAllowed();
	bool rotation45Allowed();
	PluralType isPlural();
	bool canEditPart();
	void setDropOffset(QPointF offset);
	void setShape(QPainterPath &);
	QPainterPath shape() const;


public:
	static QString fillTypeIndividual;
	static QString fillTypeGround;
	static QString fillTypePlain;	
	static QString fillTypeNone;

protected:
	void setSvg(const QString &);
	void setSvgAux(const QString &);
	QString generateSvg();
    ViewLayer::ViewID useViewIDForPixmap(ViewLayer::ViewID, bool swappingEnabled);

protected:
	ConnectorItem * m_connector0;
	QPointF m_dropOffset;
    QPainterPath m_shape;
};

#endif
