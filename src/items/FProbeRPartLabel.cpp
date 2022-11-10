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

#include"FProbeRPartLabel.h"

#include "../debugdialog.h"
#include "../items/partlabel.h"

FProbeRPartLabel::FProbeRPartLabel(SketchWidget * sketchWidget) :
	FProbe("RPartLabel"),
	m_sketchWidget(sketchWidget) {
}

QVariant FProbeRPartLabel::read() {
	ItemBase * itemBase = nullptr;
	Q_FOREACH (QGraphicsItem * item, m_sketchWidget->scene()->items()) {
		auto* base = dynamic_cast<ItemBase *>(item);
		if (base == nullptr) continue;
		if (base->label() != "R") continue;
		if (base->viewID() != ViewLayer::PCBView) continue;
		itemBase = base;
	}

	if (itemBase != nullptr) {
		PartLabel * partLabel = itemBase->partLabel();
		if(partLabel != nullptr) {
			QDomElement labelGeometry;
			partLabel->getLabelGeometry(labelGeometry);
			QPointF p;
			bool ok = false;
			double x = labelGeometry.attribute("x").toDouble(&ok);
			if (ok) p.setX(x);
			double y = labelGeometry.attribute("y").toDouble(&ok);
			if (ok) p.setY(y);
			DebugDialog::debug(QString("part label pos: %1 %2").arg(p.x()).arg(p.y()));
			return QVariant(p);
		}
	}

	return QVariant();
}
