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

#ifndef FRITZING2EAGLE_H
#define FRITZING2EAGLE_H

#include <QWidget>
#include <QTime>

class Fritzing2Eagle {
//	Q_OBJECT

public:
	Fritzing2Eagle(PCBSketchWidget *m_pcbGraphicsView);

	/*
	public:
		static void showOutputInfo(PCBSketchWidget m_pcbGraphicsView);
	*/

protected:
	static Fritzing2Eagle* singleton;
};

#endif
