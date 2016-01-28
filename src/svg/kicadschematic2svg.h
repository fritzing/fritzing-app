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

#ifndef KICADSCHEMATIC2SVG_H
#define KICADSCHEMATIC2SVG_H

#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QRectF>

#include "kicad2svg.h"

class KicadSchematic2Svg : public Kicad2Svg
{

public:
	KicadSchematic2Svg();
	QString convert(const QString & filename, const QString &defName);

public:
	static QStringList listDefs(const QString & filename);

protected:
	QString convertField(const QString & line);
	QString convertText(const QString & line);
	QString convertField(const QString & xString, const QString & yString, const QString & fontSizeString, const QString & orientation, const QString & hjustify, const QString & vjustify, const QString & text);
	QString convertRect(const QString & line); 
	QString convertCircle(const QString & line);  
	QString convertPin(const QString & line, int textOffset, bool drawPinName, bool drawPinNumber, int pinIndex); 
	QString convertArc(const QString & line); 
	QString convertPoly(const QString & line); 
	QString addFill(const QString & line, const QString & NF, const QString & strokeString);
	QStringList splitLine(const QString & line);
};


#endif // KICADSCHEMATIC2SVG_H
