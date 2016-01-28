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

#ifndef PETOOLVIEW_H
#define PETOOLVIEW_H

#include <QFrame>
#include <QRadioButton>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QLabel>
#include <QTreeWidget>
#include <QDomDocument>
#include <QDoubleSpinBox>
#include <QPointer>

#include "../viewlayer.h"

class PEDoubleSpinBox : public QDoubleSpinBox
{
Q_OBJECT
public:
    PEDoubleSpinBox(QWidget * parent = 0);

    void stepBy (int steps);

signals:
    void getSpinAmount(double &);
};

class PEToolView : public QFrame
{
Q_OBJECT
public:
	PEToolView(QWidget * parent = NULL);
	~PEToolView();

    void initConnectors(QList<QDomElement> * connectorList);
    int currentConnectorIndex();
	void setCurrentConnector(const QDomElement &);
    void setCurrentConnector(int index);
    void setTerminalPointCoords(QPointF);
    void setTerminalPointLimits(QSizeF);
	void setChildrenVisible(bool vis);
    void enableConnectorChanges(bool enableTerminalPointDrag, bool enableTerminalPointControls, bool enableInfo, bool enableAssign);
	void showAssignedConnectors(const QDomDocument * svgDoc, ViewLayer::ViewID);

signals:
    void switchedConnector(int);
    void removedConnector(const QDomElement &);
    void pickModeChanged(bool);
    void busModeChanged(bool);
    void terminalPointChanged(const QString & how);
    void terminalPointChanged(const QString & coord, double value);
    void getSpinAmount(double &);
    void connectorMetadataChanged(struct ConnectorMetadata *);

protected slots:
    void switchConnector(QTreeWidgetItem * current, QTreeWidgetItem * previous);
    void pickModeChangedSlot();
    void busModeChangedSlot(bool);
    void descriptionEntry();
    void typeEntry();
    void nameEntry();
    void buttonChangeTerminalPoint();
    void terminalPointEntry();
    void getSpinAmountSlot(double &);
    void removeConnector();

protected:
	void changeConnector();
	void hideConnectorListStuff();


protected:
    QPointer<QTreeWidget> m_connectorListWidget;
    QList<QPushButton *> m_buttons;
    QPointer<class PEGraphicsItem> m_pegi;
    QList<QDomElement> * m_connectorList;
    QPointer<QGroupBox> m_connectorInfoGroupBox;
    QPointer<QBoxLayout> m_connectorInfoLayout;
    QPointer<QWidget> m_connectorInfoWidget;
	QPointer<QCheckBox> m_busModeBox;
    QPointer<QDoubleSpinBox> m_terminalPointX;
    QPointer<QDoubleSpinBox> m_terminalPointY;
    QPointer<QLabel> m_units;
	QPointer<QGroupBox> m_terminalPointGroupBox;
	QPointer<QLabel> m_terminalPointDragState;
    QPointer<QPushButton> m_assignButton;
};

#endif
