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

#include "svgview.h"
#include <QtGui>
//
SVGView::SVGView(const QString &name, QWidget *parent)
	: QFrame(parent)
{
	setFrameStyle(Sunken | StyledPanel);
    m_graphicsView = new QGraphicsView;
    m_graphicsView->setRenderHint(QPainter::Antialiasing, true);
    m_graphicsView->setDragMode(QGraphicsView::RubberBandDrag);
    m_graphicsView->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    int size = style()->pixelMetric(QStyle::PM_ToolBarIconSize);
    QSize iconSize(size, size);

    m_domDocument = new QDomDocument;

	m_zoom = 1;
	m_rotation = 0;

    QToolButton *zoomInIcon = new QToolButton;
    zoomInIcon->setAutoRepeat(true);
    zoomInIcon->setAutoRepeatInterval(33);
    zoomInIcon->setAutoRepeatDelay(0);
    zoomInIcon->setIcon(QPixmap(":/zoomin.png"));
    zoomInIcon->setIconSize(iconSize);
    QToolButton *zoomOutIcon = new QToolButton;
    zoomOutIcon->setAutoRepeat(true);
    zoomOutIcon->setAutoRepeatInterval(33);
    zoomOutIcon->setAutoRepeatDelay(0);
    zoomOutIcon->setIcon(QPixmap(":/zoomout.png"));
    zoomOutIcon->setIconSize(iconSize);

    QToolButton *rotateLeftIcon = new QToolButton;
    rotateLeftIcon->setIcon(QPixmap(":/rotateleft.png"));
    rotateLeftIcon->setIconSize(iconSize);
    QToolButton *rotateRightIcon = new QToolButton;
    rotateRightIcon->setIcon(QPixmap(":/rotateright.png"));
    rotateRightIcon->setIconSize(iconSize);

    // Label layout
    QHBoxLayout *labelLayout = new QHBoxLayout;
    m_label = new QLabel(name);

	//TODO: put proper icons in here
    m_printButton = new QToolButton;
    m_printButton->setIcon(QIcon(QPixmap(":/resources/images/document-print.png")));
    m_printButton->setText(tr("Print"));
    m_printButton->setToolTip(tr("Print"));

	m_loadPCBXMLButton = new QToolButton;
    m_loadPCBXMLButton->setIcon(QIcon(QPixmap(":/resources/images/applications-accessories.png")));
	m_loadPCBXMLButton->setText(tr("Import XML"));
	m_loadPCBXMLButton->setToolTip(tr("Import XML"));

    labelLayout->addWidget(m_label);
    labelLayout->addStretch();
    labelLayout->addWidget(m_printButton);
    labelLayout->addWidget(m_loadPCBXMLButton);

    QGridLayout *topLayout = new QGridLayout;
    topLayout->addLayout(labelLayout, 0, 0);
    topLayout->addWidget(m_graphicsView, 1, 0);
    setLayout(topLayout);

    connect(rotateLeftIcon, SIGNAL(clicked()), this, SLOT(rotateLeft()));
    connect(rotateRightIcon, SIGNAL(clicked()), this, SLOT(rotateRight()));
    connect(zoomInIcon, SIGNAL(clicked()), this, SLOT(zoomIn()));
    connect(zoomOutIcon, SIGNAL(clicked()), this, SLOT(zoomOut()));
    connect(m_printButton, SIGNAL(clicked()), this, SLOT(print()));
    connect(m_loadPCBXMLButton, SIGNAL(clicked()), this, SLOT(importPCBXML()));

    setupMatrix();
}
//

void SVGView::importPCBXML(){
	QString path = QFileDialog::getOpenFileName(this,
         tr("Select Footprint XML File"), "",
         tr("Fritzing Footprint XML Files (*.fzfp);;All Files (*)"));
	QFile file(path);
	if (!file.open(QFile::ReadOnly | QFile::Text)) {
    	QMessageBox::warning(NULL, QObject::tr("Fritzing"),
                     QObject::tr("Cannot read file %1:\n%2.")
                     .arg(path)
                     .arg(file.errorString()));
    }

    QString errorStr;
    int errorLine;
    int errorColumn;

    if (!m_domDocument->setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
        QMessageBox::critical(NULL, QObject::tr("Fritzing"),
                                 QObject::tr("Parse error (3) at line %1, column %2:\n%3\n%4")
                                 .arg(errorLine)
                                 .arg(errorColumn)
                                 .arg(errorStr)
								 .arg(path));
        return;
    }

    QDomElement root = m_domDocument->documentElement();
   	if (root.isNull()) {
        QMessageBox::critical(NULL, QObject::tr("Fritzing"), QObject::tr("The file %1 is not a Fritzing file (12).").arg(path));
   		return;
	}

    if (root.tagName().toLower() != "element") {
        QMessageBox::critical(NULL, QObject::tr("Fritzing"), QObject::tr("The file %1 is not a Fritzing Footprint XML file.").arg(path));
        return;
    }

    drawPCBXML(&root);
}

void SVGView::drawPCBXML(QDomElement * rootElement) {
	m_pcbXML = new PcbXML(*rootElement);
	m_pcbWidget = new QSvgWidget(m_pcbXML->getSvgFile());
	m_scene.addWidget(m_pcbWidget);

	m_graphicsView->setScene(&m_scene);
	m_graphicsView->show();

    //connect(pcbWidget, SIGNAL(repaintNeeded()), this, SLOT(pcbWidget->update()));
}

void SVGView::setupMatrix(){
	// TODO: add support for scaling and rotation
    QMatrix matrix;
    matrix.scale(qreal(1), qreal(1));
    matrix.rotate(m_rotation);

    m_graphicsView->setMatrix(matrix);
}

QGraphicsView *SVGView::view() const
{
    return m_graphicsView;
}

void SVGView::print()
{
#ifndef QT_NO_PRINTER
    QPrinter printer;
    QPrintDialog dialog(&printer, this);
    if (dialog.exec() == QDialog::Accepted) {
        QPainter painter(&printer);
        m_graphicsView->render(&painter);
    }
#endif
}

void SVGView::zoomIn()
{
	m_zoom++;
    //zoomSlider->setValue(zoomSlider->value() + 1);
}

void SVGView::zoomOut()
{
	m_zoom--;
    //zoomSlider->setValue(zoomSlider->value() - 1);
}

void SVGView::rotateRight(){

}

void SVGView::rotateLeft(){

}
