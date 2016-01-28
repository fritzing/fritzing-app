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

$Revision: 6954 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-05 10:22:00 +0200 (Fr, 05. Apr 2013) $

********************************************************************/



#ifndef LAYERATTRIBUTES_H
#define LAYERATTRIBUTES_H

#include <QString>
#include <QDomElement>

#include "viewlayer.h"

class LayerAttributes {
	
public:
	LayerAttributes();
	
	const QString & filename();
	void setFilename(const QString &);
	const QByteArray & loaded() const;
	void clearLoaded();
	void setLoaded(const QByteArray &);

protected:
	QString m_filename;
	QByteArray m_loaded;

public:
    QString error;
    ViewLayer::ViewID viewID;
    ViewLayer::ViewLayerID viewLayerID;
    ViewLayer::ViewLayerPlacement viewLayerPlacement;
    bool doConnectors;
    Qt::Orientations orientation;
    bool createShape;
};

#endif
