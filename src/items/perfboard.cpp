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

$Revision: 6984 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-22 23:44:56 +0200 (Mo, 22. Apr 2013) $

********************************************************************/

#include "perfboard.h"
#include "../utils/graphicsutils.h"
#include "../utils/textutils.h"
#include "../utils/boundedregexpvalidator.h"
#include "../fsvgrenderer.h"
#include "../sketch/infographicsview.h"
#include "../svg/svgfilesplitter.h"
#include "../commands.h"
#include "../debugdialog.h"
#include "moduleidnames.h"
#include "partlabel.h"

#include <qmath.h>
#include <QRegExpValidator>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QMessageBox>


static const int ConnectorIDJump = 1000;
static const int MaxXDimension = 199;
static const int MinXDimension = 5;
static const int MaxYDimension = 199;
static const int MinYDimension = 5;
static const int WarningSize = 2000;

static const QString OneHole("M%1,%2a%3,%3 0 1 %5 %4,0 %3,%3 0 1 %5 -%4,0z\n");

bool Perfboard::m_gotWarning = false;

/////////////////////////////////////////////////////////////////////

Perfboard::Perfboard( ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: Capacitor(modelPart, viewID, viewGeometry, id, itemMenu, doLabel)
{
	m_size = modelPart->localProp("size").toString();
	if (m_size.isEmpty()) {
		m_size = modelPart->properties().value("size", "20.20");
		modelPart->setLocalProp("size", m_size);
	}
}

Perfboard::~Perfboard() {
}

void Perfboard::setProp(const QString & prop, const QString & value) 
{
	if (prop.compare("size") != 0) {
		Capacitor::setProp(prop, value);
		return;
	}
	switch (this->m_viewID) {
		case ViewLayer::BreadboardView:
			if (value.compare(m_size) != 0) {
                QString temp = value;
				QString svg = makeBreadboardSvg(temp);
				reloadRenderer(svg, false);
				//DebugDialog::debug(svg);
			}
			break;

		default:
			break;
	}

	m_size = value;
	modelPart()->setLocalProp("size", value);

    if (m_partLabel) m_partLabel->displayTextsIf();
}

QString Perfboard::makeBreadboardSvg(const QString & size) 
{
	QString BreadboardLayerTemplate = "";
	QString ConnectorTemplate = "";

	if (BreadboardLayerTemplate.isEmpty()) {
		QFile file(":/resources/templates/perfboard_boardLayerTemplate.txt");
		file.open(QFile::ReadOnly);
		BreadboardLayerTemplate = file.readAll();
		file.close();
	}
	if (ConnectorTemplate.isEmpty()) {
		QFile file(":/resources/templates/perfboard_connectorTemplate.txt");
		file.open(QFile::ReadOnly);
		ConnectorTemplate = file.readAll();
		file.close();
	}

	int x, y;
	getXY(x, y, size);

	QString middle;
	QString holes;
	double radius = 17.5;
	int sweepflag = 0;

	int top = 100;
	for (int iy = 0; iy < y; iy++) {
		int left = 100;
		for (int jx = 0; jx < x; jx++) {
			middle += ConnectorTemplate.arg(left).arg(top).arg(jx).arg(iy).arg(QString::number((iy * ConnectorIDJump) + jx));
			holes += OneHole				
				.arg(left - radius)
				.arg(top)
				.arg(radius)
				.arg(2 * radius)
				.arg(sweepflag);

			left += 100;
		}
		top += 100;
	}

	QString svg = BreadboardLayerTemplate
					.arg((x / 10.0) + 0.1)
					.arg((y / 10.0) + 0.1)
					.arg((x * 100) + 100)
					.arg((y * 100) + 100)
					.arg(holes)
					.arg(x * 100 - 8 + 100)
					.arg(y * 100 - 8 + 100)
					.arg(middle);

	return svg;
}

QString Perfboard::genFZP(const QString & moduleid)
{
	QString ConnectorFzpTemplate = "";
	QString FzpTemplate = "";

	if (ConnectorFzpTemplate.isEmpty()) {
		QFile file(":/resources/templates/perfboard_connectorFzpTemplate.txt");
		file.open(QFile::ReadOnly);
		ConnectorFzpTemplate = file.readAll();
		file.close();
	}
	if (FzpTemplate.isEmpty()) {
		QFile file(":/resources/templates/perfboard_fzpTemplate.txt");
		file.open(QFile::ReadOnly);
		FzpTemplate = file.readAll();
		file.close();
	}

	int x, y;
	getXY(x, y, moduleid);

	QString middle;

	for (int iy = 0; iy < y; iy++) {
		for (int jx = 0; jx < x; jx++) {
			middle += ConnectorFzpTemplate.arg(jx).arg(iy).arg(QString::number((iy * ConnectorIDJump) + jx));
		}
	}

	return FzpTemplate.arg(x).arg(y).arg(middle);
}

bool Perfboard::collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide)
{
	if (prop.compare("size", Qt::CaseInsensitive) == 0) {
		returnProp = tr("size");
		returnValue = m_size;
		m_propsMap.insert("size", m_size);

		int x, y;
		getXY(x, y, m_size);

		QFrame * frame = new QFrame();
		QVBoxLayout * vboxLayout = new QVBoxLayout();
		vboxLayout->setAlignment(Qt::AlignLeft);
		vboxLayout->setSpacing(1);
		vboxLayout->setContentsMargins(0, 3, 0, 0);
		vboxLayout->setMargin(0);

		QFrame * subframe1 = new QFrame();
		QHBoxLayout * hboxLayout1 = new QHBoxLayout();
		hboxLayout1->setAlignment(Qt::AlignLeft);
		hboxLayout1->setContentsMargins(0, 0, 0, 0);
		hboxLayout1->setSpacing(2);

		QLabel * l1 = new QLabel(getColumnLabel());	
		l1->setMargin(0);
		l1->setObjectName("infoViewLabel");	
		m_xEdit = new QLineEdit();
		m_xEdit->setEnabled(swappingEnabled);
		QIntValidator * validator = new QIntValidator(m_xEdit);
		validator->setRange(MinXDimension, MaxXDimension);
		m_xEdit->setObjectName("infoViewLineEdit");	
		m_xEdit->setValidator(validator);
		m_xEdit->setMaxLength(5);
		m_xEdit->setText(QString::number(x));

		QFrame * subframe2 = new QFrame();
		QHBoxLayout * hboxLayout2 = new QHBoxLayout();
		hboxLayout2->setAlignment(Qt::AlignLeft);
		hboxLayout2->setContentsMargins(0, 0, 0, 0);
		hboxLayout2->setSpacing(2);

		QLabel * l2 = new QLabel(getRowLabel());
		l2->setMargin(0);
		l2->setObjectName("infoViewLabel");	
		m_yEdit = new QLineEdit();
		m_yEdit->setEnabled(swappingEnabled);
		validator = new QIntValidator(m_yEdit);
		validator->setRange(MinYDimension, MaxYDimension);
		m_yEdit->setObjectName("infoViewLineEdit");	
		m_yEdit->setValidator(validator);
		m_yEdit->setMaxLength(5);
		m_yEdit->setText(QString::number(y));

		hboxLayout1->addWidget(l1);
		hboxLayout1->addWidget(m_xEdit);

		hboxLayout2->addWidget(l2);
		hboxLayout2->addWidget(m_yEdit);

		subframe1->setLayout(hboxLayout1);
		subframe2->setLayout(hboxLayout2);

		if (returnWidget != NULL) vboxLayout->addWidget(qobject_cast<QWidget *>(returnWidget));
		vboxLayout->addWidget(subframe1);
		vboxLayout->addWidget(subframe2);

		m_setButton = new QPushButton (tr("set board size"));
		m_setButton->setObjectName("infoViewButton");
		connect(m_setButton, SIGNAL(pressed()), this, SLOT(changeBoardSize()));
		m_setButton->setEnabled(false);

		vboxLayout->addWidget(m_setButton);

		connect(m_xEdit, SIGNAL(editingFinished()), this, SLOT(enableSetButton()));
		connect(m_yEdit, SIGNAL(editingFinished()), this, SLOT(enableSetButton()));

		frame->setLayout(vboxLayout);

		returnWidget = frame;

		return true;
	}

	return Capacitor::collectExtraInfo(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget, hide);
}


void Perfboard::addedToScene(bool temporary)
{
	if (this->scene()) {
		QString temp = m_size;
		m_size = "";
		setProp("size", temp);
	}
    return Capacitor::addedToScene(temporary);
}

bool Perfboard::canEditPart() {
	return false;
}

void Perfboard::changeBoardSize() 
{
	if (!m_gotWarning) {
		int x = m_xEdit->text().toInt();
		int y = m_yEdit->text().toInt();
		if (x * y >= WarningSize) {
			m_gotWarning = true;
			QMessageBox messageBox(NULL);
			messageBox.setWindowTitle(tr("Performance Warning"));
			messageBox.setText(tr("Performance of perfboards and stripboards with more than approximately 2000 holes can be slow. Are you sure ?\n"
				"\nNote: this warning will not be repeated during this session."
				));
			messageBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
			messageBox.setDefaultButton(QMessageBox::Cancel);
			messageBox.setIcon(QMessageBox::Warning);
			messageBox.setWindowModality(Qt::WindowModal);
			messageBox.setButtonText(QMessageBox::Ok, tr("Set new size"));
			messageBox.setButtonText(QMessageBox::Cancel, tr("Cancel"));
			QMessageBox::StandardButton answer = (QMessageBox::StandardButton) messageBox.exec();

			if (answer != QMessageBox::Ok) {
				getXY(x, y, m_size);
				m_xEdit->setText(QString::number(x));
				m_yEdit->setText(QString::number(y));
				return;
			}
		}
	}

	QString newSize = QString("%1.%2").arg(m_xEdit->text()).arg(m_yEdit->text());
    m_propsMap.insert("size", newSize);

    foreach (QString key, m_propsMap.keys()) {
        DebugDialog::debug("prop " + key + " " + m_propsMap.value(key));
    }

    InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
    if (infoGraphicsView != NULL) {
        infoGraphicsView->swap(family(), "size", m_propsMap, this);
    }
}

ItemBase::PluralType Perfboard::isPlural() {
	return Plural;
}

void Perfboard::enableSetButton() {
	QLineEdit * edit = qobject_cast<QLineEdit *>(sender());
	if (edit == NULL) return;

	int x, y;
	getXY(x, y, m_size);

	int vx = m_xEdit->text().toInt();
	int vy = m_yEdit->text().toInt();

	m_setButton->setEnabled(vx != x || vy != y);
}

QString Perfboard::genModuleID(QMap<QString, QString> & currPropsMap)
{
	QString size = currPropsMap.value("size");
	return size + ModuleIDNames::PerfboardModuleIDName;
}

bool Perfboard::getXY(int & x, int & y, const QString & s) 
{
	QRegExp re("(\\d+)\\.(\\d+)");

	int ix = re.indexIn(s);
	if (ix < 0) return false;

	bool ok;
	x = re.cap(1).toInt(&ok);
	if (!ok) return false;

	y = re.cap(2).toInt(&ok);
	return ok;
}

bool Perfboard::rotation45Allowed() {
	return false;
}

void Perfboard::hoverUpdate()
{
}

void Perfboard::paintHover(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(painter);
	Q_UNUSED(option);
	Q_UNUSED(widget);
}

bool Perfboard::stickyEnabled() {
	return false;
}

bool Perfboard::canFindConnectorsUnder() {
	return false;
}

QString Perfboard::getRowLabel() {
	return tr("rows");
}

QString Perfboard::getColumnLabel() {
	return tr("columns");
}
