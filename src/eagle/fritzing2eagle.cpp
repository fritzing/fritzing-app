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

$Revision: 6469 $:
$Author: irascibl@gmail.com $:
$Date: 2012-09-23 00:54:30 +0200 (So, 23. Sep 2012) $

********************************************************************/


#include <QLabel>
#include <QScrollArea>
#include <QFont>
#include <QChar>
#include <QTime>
#include <QKeyEvent>
#include <QScrollBar>
#include <QMessageBox>


#include "../items/itembase.h"
#include "../sketch/pcbsketchwidget.h"
#include "fritzing2eagle.h"



Fritzing2Eagle* Fritzing2Eagle::singleton = NULL;

Fritzing2Eagle::Fritzing2Eagle(PCBSketchWidget *m_pcbGraphicsView)
{
	QList <ItemBase*> partList;
	
	m_pcbGraphicsView->collectParts(partList);
	
	singleton = this;
}

/*
void Fritzing2Eagle::showOutputInfo(PCBSketchWidget m_pcbGraphicsView) 
{
	if (singleton == NULL) {
		new Fritzing2Eagle();
	}
	
}
*/


