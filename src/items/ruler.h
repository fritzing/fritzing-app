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

$Revision: 6984 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-22 23:44:56 +0200 (Mo, 22. Apr 2013) $

********************************************************************/

#ifndef RULER_H
#define RULER_H

#include <QRectF>
#include <QPainterPath>
#include <QPixmap>
#include <QVariant>
#include <QComboBox>
#include <QDoubleValidator>

#include "paletteitem.h"

class Ruler : public PaletteItem 
{
	Q_OBJECT

public:
	// after calling this constructor if you want to render the loaded svg (either from model or from file), MUST call <renderImage>
	Ruler(ModelPart *, ViewLayer::ViewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel);
	~Ruler();

	void resizeMM(double magnitude, double unitsFlag, const LayerHash & viewLayers);
	QString retrieveSvg(ViewLayer::ViewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor);
	bool collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide);
	bool hasCustomSVG();
	bool stickyEnabled();
    bool hasPartLabel();
	PluralType isPlural();
	void addedToScene(bool temporary);
	bool hasPartNumberProperty();
	bool canFindConnectorsUnder();

public slots:
	void widthEntry();
    void unitsEntry();

protected:
	QString makeSvg(double inches);
    ViewLayer::ViewID useViewIDForPixmap(ViewLayer::ViewID, bool swappingEnabled);
	
protected:
	QPointer<QLineEdit> m_widthEditor;
    QPointer<QRadioButton> m_unitsEditor;
	QPointer<QDoubleValidator> m_widthValidator;
};

#endif
