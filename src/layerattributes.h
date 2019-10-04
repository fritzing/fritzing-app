/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2019 Fritzing

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

********************************************************************/



#ifndef LAYERATTRIBUTES_H
#define LAYERATTRIBUTES_H

#include <QString>
#include <QDomElement>

#include "viewlayer.h"

class LayerAttributes {

public:
	LayerAttributes() noexcept : orientation(Qt::Vertical), createShape(true), doConnectors(false) { }
	const QString & filename() const noexcept { return m_filename; }
	void setFilename(const QString &);
	const QByteArray & loaded() const noexcept { return m_loaded; }
	void clearLoaded();
	void setLoaded(const QByteArray &);


public:
	Qt::Orientations orientation;
	bool createShape;
	bool doConnectors;
	QString error;
	ViewLayer::ViewID viewID;
	ViewLayer::ViewLayerID viewLayerID;
	ViewLayer::ViewLayerPlacement viewLayerPlacement;
protected:
	QString m_filename;
	QByteArray m_loaded;
};

#endif
