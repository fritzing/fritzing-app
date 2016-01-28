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

#ifndef SCHEMATIC_FRAME_H
#define SCHEMATIC_FRAME_H

#include <QRectF>
#include <QPainterPath>
#include <QPixmap>
#include <QVariant>
#include <QCheckBox>
#include <QComboBox>
#include <QTextEdit>

#include "resizableboard.h"

struct TemplateThing {
    QString svgTemplate;
    QSizeF size;
    double margin;
    double fontSize;
    double strokeWidth;
};

class SchematicFrame : public ResizableBoard 
{
	Q_OBJECT

public:
	// after calling this constructor if you want to render the loaded svg (either from model or from file), MUST call <renderImage>
	SchematicFrame(ModelPart *, ViewLayer::ViewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel);
	~SchematicFrame();

	QString retrieveSvg(ViewLayer::ViewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor);
	bool collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide);
	bool canEditPart();
	void setProp(const QString & prop, const QString & value);
	bool hasPartLabel();
	bool stickyEnabled();
	PluralType isPlural();
	void addedToScene(bool temporary);
	bool rotationAllowed();
	bool rotation45Allowed();
	bool hasPartNumberProperty();
	void setInitialSize();

protected slots:
	void propEntry();
	void dateTimeEntry(QDateTime);
	void sheetEntry(int);

protected:
	double minWidth();
	double minHeight();
	ViewLayer::ViewID theViewID();
	QString makeLayerSvg(ViewLayer::ViewLayerID viewLayerID, double mmW, double mmH, double milsW, double milsH);
	QString makeFirstLayerSvg(double mmW, double mmH, double milsW, double milsH);
	QString makeNextLayerSvg(ViewLayer::ViewLayerID, double mmW, double mmH, double milsW, double milsH);
	bool makeLineEdit(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget);
    ViewLayer::ViewID useViewIDForPixmap(ViewLayer::ViewID, bool swappingEnabled);
	void loadTemplates();

protected:
    static void loadTemplate(const QString & tPath, const QString & fPath, TemplateThing &);

protected:
    QTextEdit * m_textEdit;
	bool m_wrapInitialized;

protected:
};



#endif
