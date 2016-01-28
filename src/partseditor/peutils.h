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

$Revision: 6904 $:
$Author: irascibl@gmail.com $:
$Date: 2013-02-26 16:26:03 +0100 (Di, 26. Feb 2013) $

********************************************************************/


#ifndef PEUTILS_H
#define PEUTILS_H

#include "../connectors/connector.h"

#include <QWidget>
#include <QDomElement>

struct ConnectorMetadata {
    Connector::ConnectorType connectorType;
    QString connectorName;
    QString connectorDescription;
    QString connectorID;
    int index;
};

class PEUtils 
{
public:
	static QString convertUnitsStr(double val);
	static double convertUnits(double val);
	static double unconvertUnits(double val);
    static QWidget * makeConnectorForm(const QDomElement & connector, int index, QObject * slotHolder, bool alternating);
	static bool fillInMetadata(int senderIndex, QWidget * parentWidget, ConnectorMetadata & cmd);

public:
	static QString Units;
    const static int Spacing;

};

#endif 
