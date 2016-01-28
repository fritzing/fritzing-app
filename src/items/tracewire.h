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

#ifndef TRACEWIRE_H
#define TRACEWIRE_H

#include "clipablewire.h"

class TraceWire : public ClipableWire
{
Q_OBJECT
public:
	TraceWire( ModelPart * modelPart, ViewLayer::ViewID,  const ViewGeometry & , long id, QMenu* itemMenu  ); 
	~TraceWire();

	bool collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide);
	bool canSwitchLayers();
	void setSchematic(bool schematic);

public:
	static TraceWire * getTrace(ConnectorItem *);
	static class QComboBox * createWidthComboBox(double currentMils, QWidget * parent);
	static int widthEntry(const QString & text, QObject * sender);
    QHash<QString, QString> prepareProps(ModelPart *, bool wantDebug, QStringList & keys);
	QStringList collectValues(const QString & family, const QString & prop, QString & value);

public:
	enum WireDirection {
		NoDirection,
		Vertical,
		Horizontal,
		Diagonal
	};

	void setWireDirection(TraceWire::WireDirection);
	TraceWire::WireDirection wireDirection();

public:
	static const int MinTraceWidthMils;
	static const int MaxTraceWidthMils;


protected:
    void setColorFromElement(QDomElement & element);

protected slots:
	void widthEntry(const QString & text);

protected:
	WireDirection m_wireDirection;


};

#endif
