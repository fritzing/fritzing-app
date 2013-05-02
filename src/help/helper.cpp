/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2012 Fachhochschule Potsdam - http://fh-potsdam.de

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

$Revision: 6565 $:
$Author: irascibl@gmail.com $:
$Date: 2012-10-15 12:10:48 +0200 (Mo, 15. Okt 2012) $

********************************************************************/

#include <QScrollBar>

#include "helper.h"
#include "../debugdialog.h"
#include "../sketch/breadboardsketchwidget.h"
#include "../sketch/schematicsketchwidget.h"
#include "../sketch/pcbsketchwidget.h"


QString Helper::BreadboardHelpText;
QString Helper::SchematicHelpText;
QString Helper::PCBHelpText;

void Helper::initText() {

	BreadboardHelpText = tr(
        "The <b>Breadboard View</b> is meant to look like a <i>real-life</i> breadboard prototype."
	"<br/><br/>"
        "Begin by dragging a part from the Parts Bin, which is over at the top right. "
        "Then pull in more parts, connecting them by clicking on the connectors and dragging wires. "
        "The process is similar to how you would arrange things in the physical world. "
	"<br/><br/>"
        "After you're finished creating your sketch in the breadboard view, try the other views. "
        "You can switch by clicking the other views in either the View Switcher or the Navigator on the lower right. "
        "Because different views have different purposes, parts will look different in the other views.");
	SchematicHelpText = tr(
        "Welcome to the <b>Schematic View</b>"
	"<br/><br/>"
        "This is a more abstract way to look at components and connections than the Breadboard View. "
        "You have the same elements as you have on your breadboard, "
        "they just look different. This representation is closer to the traditional diagrams used by engineers."
        "<br/><br/>"
        "You can press &lt;Shift&gt;-click with the mouse to create bend points and tidy up your connections. "
        "The Schematic View can help you check that you have made the right connections between components. "
        "You can also print out your schematic for documentation.");
	PCBHelpText = tr(
        "The <b>PCB View</b> is where you design how the components will appear on a physical PCB (Printed Circuit Board)."
	"<br/><br/>"
        "PCBs can be made at home or in a small lab using DIY etching processes. "
        "They also can be sent to professional PCB manufacturing services for more precise fabrication. "
        "<br/>"
		"<table><tr><td>"
		"The first thing you will need is a board to place your parts on. "
		"There should already be one beneath this widget, but if not, "
		"drag in the board icon from the parts bin (image at right). "
		"</td><td>"
		"<img src=\":resources/parts/svg/core/icon/rectangle_pcb.svg\" />"
		"</td></tr></table>"
        "<br/><br/>"
	    "To lay out your PCB, rearrange all the components so they fit nicely on the board. "
        "Then try to shift them around to minimize the length and confusion of connections. "
        "You can also resize rectangular boards. "
		"<br/>"
		"<table><tr><td>"
        "Once the parts are sorted out, you connect them with copper traces. "
		"You can drag out a trace from individual connections or use "
        "the autorouter to generate them. "
		"The Autoroute button is at the bottom of the window (image at right)."
		"</td><td>"
		"<img src=\":resources/images/icons/toolbarAutorouteEnabled_icon.png\" />"
		"</td></tr></table>");
}


Helper::Helper(MainWindow *owner, bool doShow) : QObject(owner) {
	m_owner = owner;
	m_breadMainHelp = new SketchMainHelp("Breadboard", BreadboardHelpText, doShow);

	m_schemMainHelp = new SketchMainHelp("Schematic", SchematicHelpText, doShow);

	m_pcbMainHelp = new SketchMainHelp("PCB", PCBHelpText, doShow);

	m_stillWaitingFirstDrop = true;
	m_stillWaitingFirstViewSwitch = true;

	m_prevVScroolW = 0;
	m_prevHScroolH = 0;

	connectToView(m_owner->m_breadboardGraphicsView);
	connectToView(m_owner->m_schematicGraphicsView);
	connectToView(m_owner->m_pcbGraphicsView);

	m_owner->m_breadboardGraphicsView->addFixedToCenterItem2(m_breadMainHelp);
	m_owner->m_schematicGraphicsView->addFixedToCenterItem2(m_schemMainHelp);
	m_owner->m_pcbGraphicsView->addFixedToCenterItem2(m_pcbMainHelp);

}

Helper::~Helper() {
	delete m_breadMainHelp;
	delete m_schemMainHelp;
	delete m_pcbMainHelp;


}

void Helper::connectToView(SketchWidget* view) {
	connect(view, SIGNAL(dropSignal(const QPoint &)), this, SLOT(somethingDroppedIntoView(const QPoint &)));
}


void Helper::somethingDroppedIntoView(const QPoint & pos) {
	Q_UNUSED(pos);

	if(m_stillWaitingFirstDrop) {
		m_stillWaitingFirstDrop = false; 
		m_breadMainHelp->doSetVisible(false);
		m_schemMainHelp->doSetVisible(false);
		m_pcbMainHelp->doSetVisible(false);
	} else {
		m_breadMainHelp->setTransparent();
		m_schemMainHelp->setTransparent();
		m_pcbMainHelp->setTransparent();
		disconnect(m_owner->m_currentGraphicsView, SIGNAL(dropSignal(const QPoint &)), this, SLOT(somethingDroppedIntoView(const QPoint &)));
	}
	m_owner->update();   // fixes update problem, thanks to bryant.mairs
}


void Helper::viewSwitched() {
	if(m_stillWaitingFirstViewSwitch) {
		m_stillWaitingFirstViewSwitch = false;
	} else {
	}
}

void Helper::toggleHelpVisibility(int index) {
	SketchMainHelp * which = helpForIndex(index);
	if (which == NULL) return;

	which->doSetVisible(!which->isVisible());
}

void Helper::setHelpVisibility(int index, bool show) {
	SketchMainHelp * which = helpForIndex(index);
	if (which == NULL) return;

	which->doSetVisible(show);
}

bool Helper::helpVisible(int index) {
	SketchMainHelp * which = helpForIndex(index);
	if (which == NULL) return false;

	return which->isVisible();
}

SketchMainHelp *Helper::helpForIndex(int index) {
	SketchMainHelp * which = NULL;
	switch (index) {
		case 0:
			which = m_breadMainHelp;
			break;
		case 1:
			which = m_schemMainHelp;
			break;
		case 2:
			which = m_pcbMainHelp;
			break;
		default:
			break;
	}

	return which;
}
