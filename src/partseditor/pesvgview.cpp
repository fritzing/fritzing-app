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

#include "pesvgview.h"
#include "pegraphicsitem.h"
#include "peutils.h"
#include "../utils/textutils.h"
#include "../utils/graphicsutils.h"
#include "../debugdialog.h"

#include <QHBoxLayout>
#include <QTextStream>
#include <QSplitter>
#include <QPushButton>
#include <QLineEdit>
#include <QFile>

//////////////////////////////////////


PESvgView::PESvgView(QWidget * parent) : QFrame(parent)
{
    this->setObjectName("peSVG");

    m_pegi = NULL;
  
    QVBoxLayout * mainLayout = new QVBoxLayout;

    m_filename = new QLabel();
    mainLayout->addWidget(m_filename);

    QFrame * boundsFrame = new QFrame;
    QHBoxLayout * boundsLayout = new QHBoxLayout;

    QLabel * label = new QLabel("x:");
    boundsLayout->addWidget(label);
    m_x = new QLabel;
    boundsLayout->addWidget(m_x);
    boundsLayout->addSpacing(PEUtils::Spacing);

    label = new QLabel("y:");
    boundsLayout->addWidget(label);
    m_y = new QLabel;
    boundsLayout->addWidget(m_y);
    boundsLayout->addSpacing(PEUtils::Spacing);

    label = new QLabel(tr("width:"));
    boundsLayout->addWidget(label);
    m_width = new QLabel;
    boundsLayout->addWidget(m_width);
    boundsLayout->addSpacing(PEUtils::Spacing);

    label = new QLabel(tr("height:"));
    boundsLayout->addWidget(label);
    m_height = new QLabel;
    boundsLayout->addWidget(m_height);
    boundsLayout->addSpacing(PEUtils::Spacing);

    m_units = new QLabel();
    boundsLayout->addWidget(m_units);

    boundsLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));
    boundsFrame->setLayout(boundsLayout);
    mainLayout->addWidget(boundsFrame);

    m_svgElement = new QLabel;
    m_svgElement->setWordWrap(false);
    m_svgElement->setTextFormat(Qt::PlainText);
    mainLayout->addWidget(m_svgElement);

    mainLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));

	//this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    this->setLayout(mainLayout);


}

PESvgView::~PESvgView() 
{
}

void PESvgView::highlightElement(PEGraphicsItem * pegi) {
    m_pegi = pegi;
    if (pegi == NULL) {
        m_svgElement->setText("");
        m_x->setText("");
        m_y->setText("");
        m_width->setText("");
        m_height->setText("");
        return;
    }

    QString string;
    QTextStream stream(&string);
    pegi->element().save(stream, 0);
    string = TextUtils::killXMLNS(string);
    int ix = string.indexOf("\n");
    if (ix > 0) {
        int jx = string.indexOf("\n", ix + 1);
        if (jx >= 0) {
            string.truncate(jx - 1);
        }
        else {
            string.truncate(ix + 200);
        }
    }
    else {
        string.truncate(200);
    }

    m_svgElement->setText(string);
    QPointF p = pegi->offset();
    m_x->setText(PEUtils::convertUnitsStr(p.x()));
    m_y->setText(PEUtils::convertUnitsStr(p.y()));
    QRectF r = pegi->rect();
    m_width->setText(PEUtils::convertUnitsStr(r.width()));
    m_height->setText(PEUtils::convertUnitsStr(r.height()));
    m_units->setText(QString("(%1)").arg(PEUtils::Units));

}

void PESvgView::setChildrenVisible(bool vis)
{
	foreach (QWidget * widget, findChildren<QWidget *>()) {
		widget->setVisible(vis);
	}
}

void PESvgView::setFilename(const QString & filename) {
    m_filename->setText(filename);
}
