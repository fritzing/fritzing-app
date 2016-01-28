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

#ifndef LOGOITEM_H
#define LOGOITEM_H

#include <QRectF>
#include <QPainterPath>
#include <QPixmap>
#include <QVariant>
#include <QCheckBox>
#include <QComboBox>
#include <QTime>

#include "resizableboard.h"

class LogoItem : public ResizableBoard 
{
	Q_OBJECT

public:
	LogoItem(ModelPart *, ViewLayer::ViewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel);
	~LogoItem();

	QString retrieveSvg(ViewLayer::ViewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor);
	bool collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide);
	bool resizeMM(double w, double h, const LayerHash & viewLayers);
	QString getProperty(const QString & key);
	void setLogo(QString logo, bool force);
	const QString & logo();
	bool canEditPart();
	void setProp(const QString & prop, const QString & value);
	bool hasPartLabel();
	void loadImage(const QString & fileName, bool addName);
	virtual bool reloadImage(const QString & svg, const QSizeF & aspectRatio, const QString & fileName, bool addName);
	bool stickyEnabled();
	PluralType isPlural();
	void addedToScene(bool temporary);
	void setHeight(double h);
	void paintHover(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    bool hasLogo();
    QString getNewLayerFileName(const QString & newLayer);
    QString getInspectorTitle();
    void setInspectorTitle(const QString & oldText, const QString & newText);

protected slots:
	void logoEntry();
	void widthEntry();
	void heightEntry();

protected:
	virtual QString hackSvg(const QString & svg, const QString & logo);
	void initImage();
	void prepLoadImageAux(const QString & fileName, bool addName);
	virtual ViewLayer::ViewLayerID layer();
	virtual QString colorString();
	virtual QString layerName();
	bool rerender(const QString & svg);
	double minWidth();
	double minHeight();
	bool freeRotationAllowed(Qt::KeyboardModifiers modifiers);
	bool freeRotationAllowed();
	ResizableBoard::Corner findCorner(QPointF p, Qt::KeyboardModifiers);
    virtual QString getShapeForRenderer(const QString & svg);
    virtual bool canRetrieveLayer(ViewLayer::ViewLayerID viewLayerID);
    bool checkImage(const QString & filename);
	QStringList & getImageNames();
	QStringList & getNewImageNames();
	virtual bool isBottom();
	virtual QString flipSvg(const QString & svg);
	QString flipSvgAux(QString & newSvg);
	QStringList collectValues(const QString & family, const QString & prop, QString & value);
    void logoEntryAux(const QString & newText);

protected:
	QString m_logo;
	bool m_hasLogo;
	QString m_originalFilename;
	QTime m_inLogoEntry;
    bool m_standardizeColors;
};

class CopperLogoItem : public LogoItem
{
Q_OBJECT
	
public:
	CopperLogoItem(ModelPart *, ViewLayer::ViewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel);
	~CopperLogoItem();

protected:
	ViewLayer::ViewLayerID layer();
	QString colorString();
	QStringList & getImageNames();
	QString flipSvg(const QString & svg);
};

class SchematicLogoItem : public LogoItem
{
Q_OBJECT
	
public:
	SchematicLogoItem(ModelPart *, ViewLayer::ViewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel);
	~SchematicLogoItem();

protected:
	ViewLayer::ViewLayerID layer();
	QString colorString();
    bool isBottom();
};

class BreadboardLogoItem : public LogoItem
{
Q_OBJECT
	
public:
	BreadboardLogoItem(ModelPart *, ViewLayer::ViewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel);
	~BreadboardLogoItem();

    bool collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide);
	void setProp(const QString & prop, const QString & value);

public slots:
    void changeTextColor();

protected:
	ViewLayer::ViewLayerID layer();
	QString colorString();
    bool isBottom();

protected:
    QString m_color;
};

class BoardLogoItem : public LogoItem
{
Q_OBJECT
public:
	BoardLogoItem(ModelPart *, ViewLayer::ViewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel);
	~BoardLogoItem();

    bool resizeMM(double w, double h, const LayerHash & viewLayers);
	bool reloadImage(const QString & svg, const QSizeF & aspectRatio, const QString & fileName, bool addName);
	bool collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide);

protected:
	ViewLayer::ViewLayerID layer();
	QString colorString();
	QStringList & getImageNames();
	QStringList & getNewImageNames();
    QString getShapeForRenderer(const QString & svg);
    bool canRetrieveLayer(ViewLayer::ViewLayerID viewLayerID);
    void reloadLayerKin(double mmW, double mmH);
    bool checkImage(const QString & filename);
    bool isBottom();
};



#endif
