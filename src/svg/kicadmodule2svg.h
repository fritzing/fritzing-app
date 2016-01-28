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

#ifndef KICADMODULE2SVG_H
#define KICADMODULE2SVG_H

#include <QString>
#include <QStringList>
#include <QTextStream>

#include "kicad2svg.h"

class KicadModule2Svg : public Kicad2Svg
{

public:
	KicadModule2Svg();
	QString convert(const QString & filename, const QString & moduleName, bool allowPadsAndPins);

public:
	static QStringList listModules(const QString & filename);

public:
	enum PadLayer {
		ToCopper0,
		ToCopper1,
		UnableToTranslate
	};

protected:
	KicadModule2Svg::PadLayer convertPad(QTextStream & stream, QString & pad, QList<int> & numbers);
	int drawDSegment(const QString & ds, QString & line);
	int drawDArc(const QString & ds, QString & arc);
	int drawDCircle(const QString & ds, QString & arc);
	QString drawOblong(int posX, int posY, double xSize, double ySize, int drillX, int drillY, const QString & padType, KicadModule2Svg::PadLayer);
	QString drawVerticalOblong(int posX, int posY, double xSize, double ySize, int drillX, int drillY, const QString & padType, KicadModule2Svg::PadLayer);
	QString drawHorizontalOblong(int posX, int posY, double xSize, double ySize, int drillX, int drillY, const QString & padType, KicadModule2Svg::PadLayer);
	void checkLimits(int posX, int xSize, int posY, int ySize);
	QString drawRPad(int posX, int posY, int xSize, int ySize, int drillX, int drillY, const QString & padName, int padNumber, const QString & padType, KicadModule2Svg::PadLayer);
	QString drawCPad(int posX, int posY, int xSize, int ySize, int drillX, int drillY, const QString & padName, int padNumber, const QString & padType, KicadModule2Svg::PadLayer);
	QString getColor(KicadModule2Svg::PadLayer padLayer);
	QString getID(int padNumber, KicadModule2Svg::PadLayer padLayer);

protected:
	int m_nonConnectorNumber;
};


#endif // KICADMODULE2SVG_H
