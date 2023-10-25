/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2022 Fritzing GmbH

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

#include"FProbeR1PosPCB.h"

#include "../debugdialog.h"

FProbeR1PosPCB::FProbeR1PosPCB(SketchWidget * sketchWidget) :
	FProbe("R1PosPCB"),
	m_sketchWidget(sketchWidget) {
}

QVariant FProbeR1PosPCB::read() {
	ItemBase * itemBase = nullptr;
	Q_FOREACH (QGraphicsItem * item, m_sketchWidget->scene()->items()) {
		auto* base = dynamic_cast<ItemBase *>(item);
		if (base == nullptr) continue;
		if (base->label() != "R") continue;
		if (base->viewID() != ViewLayer::PCBView) continue;
		itemBase = base;
	}

	if (itemBase != nullptr) {
		DebugDialog::debug(QString("itembase pos: %1 %2").arg(itemBase->pos().x()).arg(itemBase->pos().y()));
		return QVariant(itemBase->pos());
	}

	return QVariant();
}
