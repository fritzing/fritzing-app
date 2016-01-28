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

#ifndef MYSTERYPART_H
#define MYSTERYPART_H

#include <QRectF>
#include <QPainterPath>
#include <QPixmap>
#include <QVariant>

#include "paletteitem.h"

class MysteryPart : public PaletteItem 
{
	Q_OBJECT

public:
	// after calling this constructor if you want to render the loaded svg (either from model or from file), MUST call <renderImage>
	MysteryPart(ModelPart *, ViewLayer::ViewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel);
	~MysteryPart();

	QString retrieveSvg(ViewLayer::ViewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor);
	bool collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide);
	QString getProperty(const QString & key);
	void setProp(const QString & prop, const QString & value);
	void setChipLabel(QString label, bool force);
	QString chipLabel();
    const QString & title();
	bool hasCustomSVG();
	PluralType isPlural();
	void addedToScene(bool temporary);
	bool changePinLabels(bool singleRow, bool sip);

public slots:
	void chipLabelEntry();
	void swapEntry(const QString & text);

public:
	static QString genSipFZP(const QString & moduleid);
	static QString genDipFZP(const QString & moduleid);
	static QString genModuleID(QMap<QString, QString> & currPropsMap);
	static QString makeSchematicSvg(const QString & expectedFileName);
	static QString makeSchematicSvg(const QStringList & labels, bool sip);
	static QString obsoleteMakeSchematicSvg(const QStringList & labels, bool sip);
	static QString makeBreadboardSvg(const QString & expectedFileName);
	static QString makeBreadboardSipSvg(const QString & expectedFileName);
	static QString makeBreadboardDipSvg(const QString & expectedFileName);
	static QString makePcbDipSvg(const QString & expectedFileName);

protected:
	QString makeSvg(const QString & chipLabel, bool replace);
	QStringList collectValues(const QString & family, const QString & prop, QString & value);
	virtual bool isDIP();
	virtual bool otherPropsChange(const QMap<QString, QString> & propsMap);
	virtual const QStringList & spacings();
	virtual QString retrieveSchematicSvg(QString & svg);

protected:
    static QString hackFzpHoleSize(const QString & fzp, const QString & moduleid); 
    static QString genxFZP(const QString & moduleid, const QString & templateName, int minPins, int maxPins, int step);

protected:
	static int NoExcusePins;

protected:
	QString m_chipLabel;
	QString m_title;
};

#endif
