/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2013 Fachhochschule Potsdam - http://fh-potsdam.de

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

$Revision: 6904 $:
$Author: irascibl@gmail.com $:
$Date: 2013-02-26 16:26:03 +0100 (Di, 26. Feb 2013) $

********************************************************************/


#include "welcomeview.h"


//////////////////////////////////////

WelcomeView::WelcomeView(QWidget * parent) : QTextEdit(parent) 
{
	QString breadboardHelpText = tr(
                "<br/>"
        "The <b>Breadboard View</b> is meant to look like a <i>real-life</i> breadboard prototype."
	"<br/><br/>"
        "Begin by dragging a part from the Parts Bin, which is over at the top right. "
        "Then pull in more parts, connecting them by clicking on the connectors and dragging wires. "
        "The process is similar to how you would arrange things in the physical world. "
	"<br/><br/>"
        "After you're finished creating your sketch in the breadboard view, try the other views. "
        "You can switch by clicking the other views in either the View Switcher or the Navigator on the lower right. "
        "Because different views have different purposes, parts will look different in the other views.");

	QString schematicHelpText = tr(
        "Welcome to the <b>Schematic View</b>"
	"<br/><br/>"
        "This is a more abstract way to look at components and connections than the Breadboard View. "
        "You have the same elements as you have on your breadboard, "
        "they just look different. This representation is closer to the traditional diagrams used by engineers."
        "<br/><br/>"
        "You can press &lt;Shift&gt;-click with the mouse to create bend points and tidy up your connections. "
        "The Schematic View can help you check that you have made the right connections between components. "
        "You can also print out your schematic for documentation.");

	QString pcbHelpText = tr(
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


    this->setText(breadboardHelpText + "<br /><br />" + schematicHelpText + "<br /><br />" + pcbHelpText);
}

WelcomeView::~WelcomeView() {

}
