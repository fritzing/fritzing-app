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

$Revision: 6904 $:
$Author: irascibl@gmail.com $:
$Date: 2013-02-26 16:26:03 +0100 (Di, 26. Feb 2013) $

********************************************************************/

#include "pcbxml.h"
#include <QSize>
#include <QPainter>
#include <QTextStream>

#include "../debugdialog.h"

// this module converts fritzing footprint xml files to svg

// TODO: mask and keepout layers
// TODO: elementarc
// TODO: square pins and round pads?

PcbXML::PcbXML( const QDomElement & pcbDocument ) 
{
	// TODO: need some execption handling for bad elements missing attrs, etc.
	m_svg = new SVGDomDocument();
	m_svgroot = m_svg->documentElement();
	
	// FIXME: what happens when there is no mark - should be 0
	m_markx = pcbDocument.attribute("MX").toInt();
	m_marky = pcbDocument.attribute("MY").toInt();
	m_units = pcbDocument.attribute("units");
	
	m_minx = m_markx;
	m_miny = m_marky;
	m_maxx = m_markx;
	m_maxy = m_marky;
	m_pinCount = 0;
	m_padCount = 0;
    
	m_silkscreen = m_svg->createGroup("silkscreen");
	m_copper = m_svg->createGroup("copper0");
	m_keepout = m_svg->createGroup("keepout");
	m_mask = m_svg->createGroup("soldermask");  
	m_outline = m_svg->createGroup("outline");  
    
	QDomNodeList tagList = pcbDocument.childNodes();
	
	//TODO: eventually need to support recursing into the tree here
	//TODO: support footprints with multiple root elements (module style)
	for(uint i = 0; i < tagList.length(); i++){
		drawNode(tagList.item(i));
	}

	m_svg->setHeight(m_maxy-m_miny, m_units);
	m_svg->setWidth(m_maxx-m_minx, m_units);
	m_svg->setViewBox(0,0,m_maxx-m_minx,m_maxy-m_miny);
	
	shiftCoordinates();

	m_svgFile = QDir::tempPath () + "/footprint.svg";
	DebugDialog::debug(QString("temp path %1").arg(QDir::tempPath () + "/test.svg" ) );
	m_svg->save(m_svgFile);
}

QString PcbXML::getSvgFile(){
	return m_svgFile;
}

void PcbXML::drawNode(QDomNode node){
	DebugDialog::debug("drawing node:");

	QString tag = node.nodeName().toLower();

	if(tag=="pin"){
		DebugDialog::debug("\tPin");
		drawPin(node);
	}
	else if(tag=="pad"){
		DebugDialog::debug("\tPad");
		drawPad(node);
	}
	else if(tag=="elementline"){
		DebugDialog::debug("\tElementLine");
		drawElementLine(node);
	}
	else if(tag=="elementarc"){
		DebugDialog::debug("\tElementArc");
		drawElementArc(node);
	}
	else if(tag=="mark"){
		DebugDialog::debug("\tMark");
		drawMark(node);
	}
	else {
		DebugDialog::debug("cannot draw - unrecognized tag");
	}
}

void PcbXML::drawPin(QDomNode node){
	QDomElement element = node.toElement();
	QDomElement pin = m_svg->createElement("circle");
	int x;
	int y;
	
	if( element.hasAttribute("aX") ){
		x = element.attribute("aX").toInt();	
		y = element.attribute("aY").toInt();
	}
	else {
		x = element.attribute("rX").toInt() + m_markx;
		y = element.attribute("rY").toInt() + m_marky;
	}
	
	int drill = element.attribute("Drill").toInt()/2;
	int thickness = element.attribute("Thickness").toInt()/2 - drill;
	int radius = drill + thickness/2;
	QString id = QString("connector%1pin").arg(m_pinCount);
	
	// BUG: what's up with str -> hex conversion???
	bool verify;
	bool square=false;
	if(element.hasAttribute("NFlags")) {
		square = element.attribute("NFlags").toInt(&verify,16) & 0x0100;
	}
	else if(element.hasAttribute("SFlags")) {
		square = element.attribute("SFlags").toInt(&verify,16) & 0x0100;
	}
	//DebugDialog::debug(QString("NFlags: %1 Value:%2 Square:%3").arg(element.attribute("NFlags")).arg(element.attribute("NFlags").toInt(&verify,16)).arg(square));
	
	m_pinCount++;
	
	if(square){
		QDomElement sq = m_svg->createElement("rect");
		sq.setAttribute("x",x-radius);
		sq.setAttribute("y",y-radius);
		sq.setAttribute("fill", "none");
		sq.setAttribute("width",radius*2);
		sq.setAttribute("height",radius*2);
		sq.setAttribute("stroke", "rgb(255, 191, 0)");
		sq.setAttribute("stroke-width", thickness);
		QDomNode tempsq = m_copper.appendChild(sq);
	}
	
	pin.setAttribute("cx", x);
	pin.setAttribute("cy", y);
	pin.setAttribute("r", radius);
	pin.setAttribute("stroke", "rgb(255, 191, 0)");
	pin.setAttribute("stroke-width", thickness);
	pin.setAttribute("id", id);
	pin.setAttribute("fill", "none");
	
	QDomNode temp = m_copper.appendChild(pin);
	minMax(x,y,radius+(thickness/2));
}

void PcbXML::drawPad(QDomNode node){
	QDomElement element = node.toElement();
	QDomElement pad = m_svg->createElement("rect");
	
	// TODO: mask and keepout layers
	int x1 = qMin(element.attribute("rX1").toInt(),element.attribute("rX2").toInt()) + m_markx;
	int x2 = qMax(element.attribute("rX1").toInt(),element.attribute("rX2").toInt()) + m_markx;
	int y1 = qMin(element.attribute("rY1").toInt(),element.attribute("rY2").toInt()) + m_marky;
	int y2 = qMax(element.attribute("rY1").toInt(),element.attribute("rY2").toInt()) + m_marky;
	int thickness = element.attribute("Thickness").toInt();
	QString id = "connector" + QString::number(m_padCount) + "pad";
	
	m_padCount++;
	
	int x = x1 - (thickness/2);
	int y = y1 - (thickness/2);
	int width = x2 - x1 + thickness;
	int height = y2 -y1 + thickness;
	
	pad.setAttribute("x", x);
	pad.setAttribute("y", y);
	pad.setAttribute("width", width);
	pad.setAttribute("height", height);
	pad.setAttribute("fill", "rgb(255, 191, 0)");
	pad.setAttribute("id", id);
	
	QDomNode temp = m_copper.appendChild(pad);
	minMax(x,y,0);
	minMax(x+width,y+height,0);
}

void PcbXML::drawElementLine(QDomNode node){
	QDomElement element = node.toElement();
	QDomElement line = m_svg->createElement("line");
	
	int x1 = element.attribute("X1").toInt() + m_markx;	
	int y1 = element.attribute("Y1").toInt() + m_marky;
	int x2 = element.attribute("X2").toInt() + m_markx;	
	int y2 = element.attribute("Y2").toInt() + m_marky;
	int thickness = element.attribute("Thickness").toInt();
	
	line.setAttribute("x1", x1);
	line.setAttribute("y1", y1);
	line.setAttribute("x2", x2);
	line.setAttribute("y2", y2);
	line.setAttribute("stroke", "white");
	line.setAttribute("stroke-width", thickness);
	QDomNode temp = m_silkscreen.appendChild(line);
	minMax(x1,y1,thickness);
	minMax(x2,y2,thickness);
}

void PcbXML::drawElementArc(QDomNode node){
	QDomElement element = node.toElement();
	
	// TODO: implement this with cubic bezier in svg	
	
	int x = element.attribute("X").toInt() + m_markx;	
	int y = element.attribute("Y").toInt() + m_marky;
	//int width = element.attribute("width").toInt()/2;
	//int height = element.attribute("height").toInt()/2;
	//int startangle = element.attribute("StartAngle").toInt();
	//int endangle = element.attribute("Delta").toInt() + startangle;
	//int thickness = element.attribute("Thickness").toInt();
	
	QString path = "M" + QString::number(x) + "," + QString::number(y) + " ";
	
}

// ignore for now
void PcbXML::drawMark(QDomNode /*node*/){
	//QDomElement element = node.toElement();QDomElement element = node.toElement();
	//	
	//int ax = element.attribute("aX").toInt();		int ax = element.attribute("aX").toInt();	
	//int ay = element.attribute("aY").toInt();	int ay = element.attribute("aY").toInt();
	//int radius = element.attribute("Drill").toInt()/2;		int radius = element.attribute("Drill").toInt()/2;	
	//int thickness = element.attribute("Thickness").toInt();	int thickness = element.attribute("Thickness").toInt();
	return;
}

// checks to see if input is greater or less than current min max  
// adjust viewbox accordingly
void PcbXML::minMax(int x, int y, int width){
	m_minx = qMin(x-width, m_minx);
	m_miny = qMin(y-width, m_miny);
	m_maxx = qMax(x+width, m_maxx);
	m_maxy = qMax(y+width, m_maxy);
}

// given minimum x and y coordinates shift all elements to upper left corner
void PcbXML::shiftCoordinates(){
	// shift lines
	QDomNodeList lineList = m_svgroot.elementsByTagName("line");
	for(uint i = 0; i < lineList.length(); i++){
		QDomElement line = lineList.item(i).toElement();
		float x1 = line.attribute("x1").toFloat() - m_minx;
		line.setAttribute("x1", x1);
		float x2 = line.attribute("x2").toFloat() - m_minx;
		line.setAttribute("x2", x2);
		float y1 = line.attribute("y1").toFloat() - m_miny;
		line.setAttribute("y1", y1);
		float y2 = line.attribute("y2").toFloat() - m_miny;
		line.setAttribute("y2", y2);
	}
	
	// circles
	QDomNodeList circleList = m_svgroot.elementsByTagName("circle");
	for(uint i = 0; i < circleList.length(); i++){
		QDomElement circle = circleList.item(i).toElement();
		float cx = circle.attribute("cx").toFloat() - m_minx;
		circle.setAttribute("cx",cx);
		float cy = circle.attribute("cy").toFloat() - m_miny;
		circle.setAttribute("cy",cy);
	}
	
	// rects
	QDomNodeList rectList = m_svgroot.elementsByTagName("rect");
	for(uint i = 0; i < rectList.length(); i++){
		QDomElement rect = rectList.item(i).toElement();
		float x = rect.attribute("x").toFloat() - m_minx;
		rect.setAttribute("x", x);
		float y = rect.attribute("y").toFloat() - m_miny;
		rect.setAttribute("y", y);
	}
	
	//TODO: arcs
	
	return;
}
