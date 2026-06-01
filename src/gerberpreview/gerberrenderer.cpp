/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2026 Fritzing

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

#include "gerberrenderer.h"

#include <QBrush>
#include <QPainter>
#include <QPen>

void GerberRenderer::render(QPainter &painter,
                            const GerberDocument &doc,
                            const QColor &color) const
{
	painter.save();
	painter.setRenderHint(QPainter::Antialiasing, true);

	QBrush brush(color);
	painter.setBrush(brush);

	for (const GerberCommand &cmd : doc.commands) {
		switch (cmd.kind) {
			case GerberCommand::Flash: {
				const auto it = doc.apertures.constFind(cmd.apertureCode);
				if (it == doc.apertures.constEnd()) break;
				QPainterPath p = it->path();
				p.translate(cmd.b);
				painter.setPen(Qt::NoPen);
				painter.fillPath(p, brush);
				break;
			}
			case GerberCommand::Stroke: {
				const auto it = doc.apertures.constFind(cmd.apertureCode);
				const double w = (it != doc.apertures.constEnd())
				                 ? it->strokeWidth() : 0.1;
				QPen pen(color, w);
				pen.setCapStyle(Qt::RoundCap);
				pen.setJoinStyle(Qt::RoundJoin);
				pen.setCosmetic(false);          // width is in world units (mm)
				painter.setPen(pen);
				painter.drawLine(cmd.a, cmd.b);
				break;
			}
			case GerberCommand::Region: {
				painter.setPen(Qt::NoPen);
				painter.fillPath(cmd.region, brush);
				break;
			}
		}
	}
	painter.restore();
}

void GerberRenderer::renderDrills(QPainter &painter,
                                  const ExcellonParser::Result &drills,
                                  const QColor &ringColor,
                                  const QColor &holeColor) const
{
	painter.save();
	painter.setRenderHint(QPainter::Antialiasing, true);

	// Plated-through visual: outer copper ring + inner hole.
	// We pick a ring thickness of 0.2 mm or 25% of the diameter,
	// whichever is larger — matches what fab-house viewers show.
	for (const ExcellonHit &hit : drills.hits) {
		const double od = hit.diameter;
		const double ringT = qMax(0.2, od * 0.25);
		const double idOuter = od + ringT;

		painter.setPen(Qt::NoPen);
		painter.setBrush(QBrush(ringColor));
		painter.drawEllipse(hit.pos, idOuter * 0.5, idOuter * 0.5);

		painter.setBrush(QBrush(holeColor));
		painter.drawEllipse(hit.pos, od * 0.5, od * 0.5);
	}
	painter.restore();
}
