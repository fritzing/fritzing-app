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

#ifndef SVGVIEW_H
#define SVGVIEW_H
//
#include <QFrame>
#include <QDomDocument>
#include <QSvgRenderer>
#include <QSvgWidget>
#include <QGraphicsScene>

#include "pcbxml.h"

QT_FORWARD_DECLARE_CLASS(QGraphicsView)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QSlider)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class SVGView : public QFrame
{
Q_OBJECT
public:
	SVGView(const QString &name, QWidget *parent = 0);
	QGraphicsView *view() const;

private slots:
    void print();
	void importPCBXML();
    void zoomIn();
    void zoomOut();
    void rotateLeft();
    void rotateRight();

private:
    QGraphicsView *m_graphicsView;
    QLabel *m_label;
    QToolButton *m_printButton;
    QToolButton *m_loadPCBXMLButton;
	QSvgRenderer *m_renderer;
	QSvgWidget *m_pcbWidget;
	PcbXML	*m_pcbXML;
    
    QGraphicsScene m_scene;
	QDomDocument *m_domDocument; // footprint xml file
	
	// graphics layers (svg groups really)
    
    qreal m_zoom;
    qreal m_rotation;

    void setupMatrix();
    void drawPCBXML(QDomElement * pcbDocument);

};
#endif
