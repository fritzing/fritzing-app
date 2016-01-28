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

$Revision: 6958 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-07 22:56:44 +0200 (So, 07. Apr 2013) $

********************************************************************/

#ifndef PARTFACTORY_H
#define PARTFACTORY_H

#include <QMenu>
#include <QDomDocument>
#include <QDomElement>
#include "../viewlayer.h"
#include "paletteitem.h"


class PartFactory
{
public:
	static class ItemBase * createPart(class ModelPart *, ViewLayer::ViewLayerPlacement, ViewLayer::ViewID, const class ViewGeometry & viewGeometry, long id, QMenu * itemMenu, QMenu * wireMenu, bool doLabel);
	static QString getSvgFilename(class ModelPart *, const QString & filename, bool generate, bool handleSubparts);
	static QString getFzpFilename(const QString & moduleID);
	static void initFolder();
	static void cleanup();
	static class ModelPart * fixObsoleteModuleID(QDomDocument & domDocument, QDomElement & instance, QString & moduleIDRef, class ModelBase * referenceModel);
	static QString folderPath();
	static QString fzpPath();
	static QString partPath();
    static bool svgFileExists(const QString & expectedFileName, QString & path);
    static bool fzpFileExists(const QString & moduleID, QString & path);
    static QString makeSchematicSipOrDipOr(const QStringList & labels, bool hasLayout, bool sip);
	static QString getSvgFilename(const QString & filename);

protected:
	static QString getFzpFilenameAux(const QString & moduleID, GenFzp);
	static QString getSvgFilenameAux(const QString & expectedFileName, GenSvg);
	static class ItemBase * createPartAux(class ModelPart *, ViewLayer::ViewID, const class ViewGeometry & viewGeometry, long id, QMenu * itemMenu, QMenu * wireMenu, bool doLabel);
    static QDomElement showSubpart(QDomElement & root, const QString & subpart);
    static void fixSubpartBounds(QDomElement &, ModelPartShared *);



public:
    static const QString OldSchematicPrefix;
};

#endif
