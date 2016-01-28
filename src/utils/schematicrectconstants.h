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

#ifndef SCHEMATICRECTCONTANTS_H
#define SCHEMATICRECTCONTANTS_H

#include <QString>
#include <QList>
#include <QDomElement>

// all measurements in millimeters

class SchematicRectConstants {

public:
    static const double PinWidth;  
    static const double PinSmallTextHeight;
    static const double PinBigTextHeight;
    static const double PinTextIndent;   
    static const double PinTextVert;
    static const double PinSmallTextVert;
    static const double LabelTextHeight;
    static const double LabelTextSpace;
    static const double RectStrokeWidth;

    static const QString PinColor;
    static const QString PinTextColor;
    static const QString TitleColor;
    static const QString RectStrokeColor;
    static const QString RectFillColor;

    static const double NewUnit;     

    static const QString FontFamily;

public:
    static QString genSchematicDIP(QList<QDomElement> & powers, QList<QDomElement> & grounds, QList<QDomElement> & lefts,
	            QList<QDomElement> & rights, QList<QDomElement> & vias, QStringList & busNames, 
                QString & boardName, bool usingParam, bool genericSMD, QString (*getConnectorName)(const QDomElement &));
    static QString simpleGetConnectorName(const QDomElement & element);

};


#endif
