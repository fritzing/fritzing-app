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


#ifndef SYMBOLPALETTEITEM_H
#define SYMBOLPALETTEITEM_H

#include "paletteitem.h"

/*
#include <QTime>

class FocusBugLineEdit : public QLineEdit {
    Q_OBJECT

public:
	FocusBugLineEdit(QWidget * parent = NULL);
	~FocusBugLineEdit();

signals:
	void safeEditingFinished();

protected slots:
	void editingFinishedSlot();
	
protected:
	QTime m_lastEditingFinishedEmit; 					

};
*/

class SymbolPaletteItem : public PaletteItem 
{
	Q_OBJECT

public:
	SymbolPaletteItem(ModelPart *, ViewLayer::ViewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel);
	~SymbolPaletteItem();

	ConnectorItem* newConnectorItem(class Connector *connector);
	void busConnectorItems(class Bus * bus, ConnectorItem *, QList<ConnectorItem *> & items);
	double voltage();
	void setProp(const QString & prop, const QString & value);
	void setVoltage(double);
	QString retrieveSvg(ViewLayer::ViewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor);
	bool collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide);
	QString getProperty(const QString & key);
	ConnectorItem * connector0();
	ConnectorItem * connector1();
	PluralType isPlural();
	void addedToScene(bool temporary);
	bool hasPartNumberProperty();
	virtual bool isOnlyNetLabel();
	bool hasPartLabel();
	bool getAutoroutable();
	void setAutoroutable(bool);
    void setLabel(const QString &);
    QString getLabel();
    QString getDirection();

public:
	static double DefaultVoltage;

public slots:
	void voltageEntry(const QString & text);
	void labelEntry();

protected:
	void removeMeFromBus(double voltage);
	double useVoltage(ConnectorItem * connectorItem);
	virtual QString makeSvg(ViewLayer::ViewLayerID);
	QString replaceTextElement(QString svg);
    ViewLayer::ViewID useViewIDForPixmap(ViewLayer::ViewID, bool swappingEnabled);
    void resetLayerKin();

protected:
	double m_voltage;
	QPointer<ConnectorItem> m_connector0;
	QPointer<ConnectorItem> m_connector1;
	bool m_voltageReference;
	bool m_isNetLabel;
};


class NetLabel : public SymbolPaletteItem 
{
Q_OBJECT

public:
	NetLabel(ModelPart *, ViewLayer::ViewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel);
	~NetLabel();

    void addedToScene(bool temporary);
	QString retrieveSvg(ViewLayer::ViewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor);
	PluralType isPlural();
	bool isOnlyNetLabel();
    QString getInspectorTitle();
    void setInspectorTitle(const QString & oldText, const QString & newText);

protected:
    QString makeSvg(ViewLayer::ViewLayerID);

};

#endif
