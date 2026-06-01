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

#include "panelpresets.h"

#include <QObject>

namespace PanelPresets {

QVector<Preset> list()
{
	// Smallest-area first. Vendor tags on the right where the upper
	// bound is documented. The auto-fit walker stops at the first hit,
	// so this order is also a soft "what does the user usually want"
	// ranking for the candidate page.
	return {
		{  50.0,  50.0, QObject::tr( "50x50 mm" ) },
		{  75.0,  75.0, QObject::tr( "75x75 mm (OSH Park 2-layer min)" ) },
		{ 100.0, 100.0, QObject::tr( "100x100 mm (JLCPCB / PCBWay sweet spot)" ) },
		{ 100.0, 150.0, QObject::tr( "100x150 mm" ) },
		{ 150.0, 100.0, QObject::tr( "150x100 mm" ) },
		{ 100.0, 200.0, QObject::tr( "100x200 mm" ) },
		{ 150.0, 150.0, QObject::tr( "150x150 mm" ) },
		{ 200.0, 150.0, QObject::tr( "200x150 mm" ) },
		{ 200.0, 200.0, QObject::tr( "200x200 mm" ) },
		{ 200.0, 250.0, QObject::tr( "200x250 mm" ) },
		{ 200.0, 300.0, QObject::tr( "200x300 mm" ) },
		{ 250.0, 250.0, QObject::tr( "250x250 mm" ) },
		{ 254.0, 254.0, QObject::tr( "254x254 mm (10x10 in OSH Park super swift)" ) },
		{ 300.0, 300.0, QObject::tr( "300x300 mm" ) },
		{ 400.0, 500.0, QObject::tr( "400x500 mm (JLCPCB max)" ) },
		{ 500.0, 500.0, QObject::tr( "500x500 mm" ) },
		{ 540.0, 510.0, QObject::tr( "540x510 mm (PCBWay max)" ) },
	};
}

} // namespace PanelPresets
