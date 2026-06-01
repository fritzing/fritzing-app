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

#ifndef PANELPRESETS_H
#define PANELPRESETS_H

#include <QString>
#include <QVector>

/**
 * @brief Single source of truth for panelizer preset sizes.
 *
 * Previously duplicated between PanelSizePage and PanelizerWizard's
 * auto-fit walker. Both now read from PanelPresets::list().
 *
 * Order matters: presets are walked smallest-first by auto-fit, so
 * earlier entries should have smaller total area (W*H).
 */
namespace PanelPresets {

struct Preset {
	double  widthMm;
	double  heightMm;
	QString label;     // human-readable; vendor-tagged where useful
};

/// Full vendor-tagged preset list, in smallest-first order.
QVector<Preset> list();

} // namespace PanelPresets

#endif // PANELPRESETS_H
