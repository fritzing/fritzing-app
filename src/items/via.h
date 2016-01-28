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

#ifndef VIA_H
#define VIA_H

#include "hole.h"

class Via : public Hole 
{
	Q_OBJECT

public:
	Via(ModelPart *, ViewLayer::ViewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel);
	~Via();

	void setAutoroutable(bool);
	bool getAutoroutable();
	ConnectorItem * connectorItem();
	void saveInstanceLocation(QXmlStreamWriter & streamWriter);

public:
	static const QString AutorouteViaHoleSize;
	static const QString AutorouteViaRingThickness;
	static QString DefaultAutorouteViaHoleSize;
	static QString DefaultAutorouteViaRingThickness;

public:
	static void initHoleSettings(HoleSettings & holeSettings);

protected:
	QString makeID();
	void setBoth(const QString & holeDiameter, const QString &  thickness);

protected:
	static void setBothConnectors(ItemBase * itemBase, SvgIdLayer * svgIdLayer);

};

#endif
