/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2019 Fritzing

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

********************************************************************/

#ifndef GEDAELEMENT2SVG_H
#define GEDAELEMENT2SVG_H

#include <QString>
#include <QStringList>
#include <QVariant>

#include "x2svg.h"

class GedaElement2Svg : public X2Svg
{

public:
	GedaElement2Svg();
	QString convert(const QString & filename, bool allowPadsAndPins);

protected:
	int countArgs(QVector<QVariant> & stack, int ix);
	QString convertPin(QVector<QVariant> & stack, int ix, int argCount, bool mils, QString & pinID);
	QString convertPad(QVector<QVariant> & stack, int ix, int argCount, bool mils, QString & pinID);
	QString convertArc(QVector<QVariant> & stack, int ix, int argCount, bool mils);
	void fixQuad(int quad, double & px, double & py);
	int reflectQuad(int angle, int & quad);
	QString getPinID(QString & number, QString & name, bool isPad);
	QString makeCopper(QStringList ids, QHash<QString, QString> &, const QString & filename);

protected:
	int m_nonConnectorNumber = 0;

};

#endif // GEDAELEMENT2SVG_H
