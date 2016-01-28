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
$Date: 2013-03-09 08:18:59 +0100 (Sa, 09 Mrz 2013) $

********************************************************************/

#include "schematicsubpart.h"


SchematicSubpart::SchematicSubpart( ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: PaletteItem(modelPart, viewID, viewGeometry, id, itemMenu, doLabel)
{
}

SchematicSubpart::~SchematicSubpart() {
}

void SchematicSubpart::hoverEnterEvent (QGraphicsSceneHoverEvent * event)
{
	PaletteItem::hoverEnterEvent(event);
    if (m_superpart) {
        foreach (ItemBase * itemBase, m_superpart->subparts()) {
            if (itemBase != this) {
                SchematicSubpart * subpart = qobject_cast<SchematicSubpart *>(itemBase);
                if (subpart) subpart->simpleHoverEnterEvent(event);
            }
        }
    }
}

void SchematicSubpart::hoverLeaveEvent (QGraphicsSceneHoverEvent * event)
{
	PaletteItem::hoverLeaveEvent(event);
    if (m_superpart) {
        foreach (ItemBase * itemBase, m_superpart->subparts()) {
            if (itemBase != this) {
                SchematicSubpart * subpart = qobject_cast<SchematicSubpart *>(itemBase);
                if (subpart) subpart->simpleHoverLeaveEvent(event);
            }
        }
    }
}

void SchematicSubpart::simpleHoverEnterEvent (QGraphicsSceneHoverEvent * event)
{
    PaletteItem::hoverEnterEvent(event);
}

void SchematicSubpart::simpleHoverLeaveEvent (QGraphicsSceneHoverEvent * event)
{
    PaletteItem::hoverLeaveEvent(event);
}

