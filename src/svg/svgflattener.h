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

#ifndef SVGFLATTENER_H
#define SVGFLATTENER_H

#include "svgfilesplitter.h"
#include <QMatrix>
#include <QSvgRenderer>

class SvgFlattener : public SvgFileSplitter
{
public:
    SvgFlattener();

    void flattenChildren(QDomElement & element);
    void unRotateChild(QDomElement & element,QMatrix transform);

public:
	static void flipSMDSvg(const QString & filename, const QString & svg, QDomDocument & flipDoc, const QString & elementID, const QString & altElementID, double printerScale, Qt::Orientations);
	static void replaceElementID(const QString & filename, const QString & svg, QDomDocument & flipDoc, const QString & elementID, const QString & altElementID);

protected:
	static QString flipSMDElement(QDomDocument & domDocument, QDomElement & element, const QString & att, QDomElement altAtt, const QString & altElementID, double printerScale, Qt::Orientations);
    static bool hasOtherTransform(QDomElement & element);
    static bool hasTranslate(QDomElement & element);
    static bool loadDocIf(const QString & filename, const QString & svg, QDomDocument & domDocument);


protected slots:
    void rotateCommandSlot(QChar command, bool relative, QList<double> & args, void * userData);

};

#endif // SVGFLATTENER_H
