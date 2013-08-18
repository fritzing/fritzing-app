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

$Revision: 6112 $:
$Author: cohen@irascible.com $:
$Date: 2012-06-28 00:18:10 +0200 (Do, 28. Jun 2012) $

********************************************************************/

#include <QVBoxLayout>
#include <QPixmap>
#include <QIcon>

#include "tipsandtricks.h"

TipsAndTricks* TipsAndTricks::singleton = NULL;

TipsAndTricks::TipsAndTricks(QWidget *parent)
	: QDialog(parent)
{
	// Let's set the icon
	this->setWindowIcon(QIcon(QPixmap(":resources/images/fritzing_icon.png")));

	singleton = this;
	setWindowTitle(tr("Tips and Tricks"));
	resize(400, 300);
	m_textEdit = new QTextEdit();
	m_textEdit->setReadOnly(true);

	QVBoxLayout * vLayout = new QVBoxLayout(this);
	vLayout->addWidget(m_textEdit);

	QString html =
tr("<html><body>") +
tr("<h3>Fritzing Tips and Tricks</h3>") +
tr("<ul>") +
tr("<li><h4>parts</h4>") +
tr("<ul>") +
tr("<li>If you can't find a part in the Parts Bin, the Generic IC is your friend.  Drag it onto your sketch, then use the widgets in the Inspector to: choose from among 25 different through-hole and SMD packages; change the pin label; and--for DIPs and SIPs--change the number of pins.  You can also change the pin names with the Pin Label editor</li>") +
tr("<li>An icon in the parts bin may actually represent multiple related parts.  So when you drag an icon from the parts bin into a sketch, make sure you look at the inspector.  The inspector will display the range of choices available for you to modify a part, or swap it for a related part. The parts bin icon will also be a little 'stack' and not just a flat icon.</li>") +
tr("</ul></li>") +
tr("<li><h4>moving and selection</h4>") +
tr("<ul>") +
tr("<li>To constrain the motion of a part to horizontal or vertical, hold down the shift key as you drag it.</li>") +
tr("<li>If you're having trouble selecting a part or a wire (segment), try selecting the part that's in the way and send it to the back: use the Raise and Lower functions on the Part menu or the context menu (right-click menu).</li>") +
tr("<li>To more precisely move a selection of parts, use the arrow keys.  Shift-arrow moves by 10 units.</li>") +
tr("</ul></li>") +
tr("<li><h4>curves and rubber band legs</h4>") +
tr("<ul>") +
tr("<li>In Breadboard view, to drag a part with rubber-band legs while keeping it connected to the breadboard, hold the Alt (Linux: Meta) key down when you start dragging.</li>") +
tr("<li>In Breadboard view, to add a curve to a wire or rubber-band leg, drag with the Control (Mac: Command) key down.  You can set whether curvy wires are the default in Preferences.</li>") +
tr("<li>In Breadboard view, to drag out a wire from the end of a rubber-band leg, drag with the Alt (Linux: Meta) key down.</li>") +
tr("</ul></li>") +
tr("<li><h4>rotation</h4>") +
tr("<ul>") +
tr("<li>To free-rotate a part in Breadboard or PCB view, select it, then hover your mouse near one of the corners until you see the rotate cursor. Mouse down and that corner will follow your mouse as you drag.</li>") +
tr("<li>To free-rotate a logo text or image item in PCB view hold down the Alt (Linux: meta) key and free-rotate as usual.</li>") +
tr("</ul></li>") +
tr("<li><h4>layers and views</h4>") +
tr("<ul>") +
tr("<li>To drag the canvas, hold down the space bar and drag with the mouse.</li>") +
tr("<li>To toggle the visibility of layer in a view, go to the view menu and choose one of the view layer items.  Or open up the <b>Layers</b> palette from the <b>Window</b> menu.</li>") +
tr("<li>When you export images from Fritzing, you can choose which layers are exported. Before you choose 'Export...', go into the 'View' menu and hide the layers you don't want to be visible.</li>") +
tr("</ul></li>") +
tr("<li><h4>part labels</h4>") +
tr("<ul>") +
tr("<li>To edit a part label, double-click it, or use the text input widget in the inspector window.</li>") +
tr("<li>To display different properties in a part label, as well as rotate it, or change the font, right-click the label.</li>") +
tr("<li>To move a part label independently from its part, select the part first--both the part and the label will be highlighted. Once the label is selected you can drag it.</li>") +
tr("</ul></li>") +
tr("<li><h4>wires and bendpoints</h4>") +
tr("<ul>") +
tr("<li>To add a bendpoint to a wire, double-click where you want the bendpoint.</li>") +
tr("<li>To delete a bendpoint from a wire, double-click it.</li>") +
tr("<li>In Schematic or PCB view, if you drag from a bendpoint with the Alt (Linux: Meta) key down, you will drag out a new wire from that bendpoint.</li>") +
tr("<li>To drag a wire segment (a section of a wire between two bendpoints), drag it with the Alt (Linux: Meta) key down.  If you also hold down the shift key, the wire segment will be constrained to horizontal or vertical motion.</li>") +
tr("<li>Use shift-drag on a wire end or bendpoint to constrain its wire segment to an angle of 45 degrees (or some multiple of 45 degrees).  If the wire segment is connected to other wire segments, the segment you're dragging will snap to make 90 degree angles with the neighboring wire segment.</li>") +
tr("</ul></li>") +
tr("</ul>") +
tr("</body></html>") 
;

	m_textEdit->setHtml(html);

}

TipsAndTricks::~TipsAndTricks()
{
}

void TipsAndTricks::hideTipsAndTricks() {
	if (singleton != NULL) {
		singleton->hide();
	}
}

void TipsAndTricks::showTipsAndTricks() {
	if (singleton == NULL) {
		new TipsAndTricks();
	}

	singleton->show();
}

void TipsAndTricks::cleanup() {
	if (singleton) {
		delete singleton;
		singleton = NULL;
	}
}
