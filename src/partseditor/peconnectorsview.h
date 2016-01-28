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

$Revision: 6499 $:
$Author: irascibl@gmail.com $:
$Date: 2012-10-02 07:21:40 +0200 (Di, 02. Okt 2012) $

********************************************************************/



#ifndef CONNECTORSVIEW_H
#define CONNECTORSVIEW_H

#include <QFrame>
#include <QTimer>
#include <QLabel>
#include <QScrollArea>
#include <QGridLayout>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QMutex>

#include "../items/itembase.h"
#include "../items/wire.h"
#include "../connectors/connectoritem.h"
#include "../connectors/connector.h"
#include "../referencemodel/referencemodel.h"

class PEConnectorsView : public QFrame
{
Q_OBJECT
public:
	PEConnectorsView(QWidget * parent = 0);
	~PEConnectorsView();

    void initConnectors(QList<QDomElement> * connectorList);
	bool anyModified();
	void setSMD(bool);

signals:
    void connectorMetadataChanged(struct ConnectorMetadata *);
    void removedConnectors(QList<struct ConnectorMetadata *> &);
    void connectorCountChanged(int);
    void connectorsTypeChanged(Connector::ConnectorType);
	void smdChanged(const QString &);

protected slots:
    void nameEntry();
    void descriptionEntry();
    void typeEntry();
    void connectorCountEntry();
    void removeConnector();
	void allTypeEntry();
	void smdEntry();
	void uncheckRadios();

protected:
    void changeConnector();

protected:
    QFrame * m_scrollFrame;
	QScrollArea * m_scrollArea;
	QLineEdit * m_numberEdit;
    int m_connectorCount;
    QMutex m_mutex;
	QList<QRadioButton *> m_radios;
	QRadioButton * m_smd;
	QRadioButton * m_tht;
};

#endif
