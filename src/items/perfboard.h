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

#ifndef PERFBOARD_H
#define PERFBOARD_H

#include <QRectF>
#include <QPainterPath>
#include <QPixmap>
#include <QVariant>
#include <QLineEdit>
#include <QPushButton>

#include "capacitor.h"

class Perfboard : public Capacitor 
{
	Q_OBJECT

public:
	// after calling this constructor if you want to render the loaded svg (either from model or from file), MUST call <renderImage>
	Perfboard(ModelPart *, ViewLayer::ViewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel);
	~Perfboard();

	bool collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide);
	bool canEditPart();
	PluralType isPlural();
	void addedToScene(bool temporary);
	void setProp(const QString & prop, const QString & value);
	void hoverUpdate();
	void paintHover(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget); 
	bool stickyEnabled();
	bool canFindConnectorsUnder();
	bool rotation45Allowed();

protected:
	virtual QString getRowLabel();
	virtual QString getColumnLabel();

public:
	static QString genFZP(const QString & moduleID);
	static QString makeBreadboardSvg(const QString & size);
	static QString genModuleID(QMap<QString, QString> & currPropsMap);


protected slots:
	void changeBoardSize();
	void enableSetButton();

protected:
	static bool getXY(int & x, int & y, const QString & s);

protected:
	static bool m_gotWarning;

protected:
	QString m_size;
	QPointer<QLineEdit> m_xEdit;
	QPointer<QLineEdit> m_yEdit;
	QPointer<QPushButton> m_setButton;
};

#endif
