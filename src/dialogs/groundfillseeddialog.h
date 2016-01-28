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

$Revision: 6112 $:
$Author: cohen@irascible.com $:
$Date: 2012-06-28 00:18:10 +0200 (Do, 28. Jun 2012) $

********************************************************************/


#ifndef GROUNDFILLSEEDDIALOG_H
#define GROUNDFILLSEEDDIALOG_H

#include <QDialog>
#include <QList>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>

class GroundFillSeedDialog : public QDialog
{
	Q_OBJECT

public:
	GroundFillSeedDialog(class PCBSketchWidget *, QList<class ConnectorItem *> &, const QString & intro, QWidget *parent = 0);
	~GroundFillSeedDialog();

	void getResults(QList<bool> & results);
	bool getFill();

public slots:
	void clickedSlot(QListWidgetItem *);
	void changedSlot(QListWidgetItem *);
	void accept();
	void reject();
	void doFill(bool);

protected:
	void showEqualPotential(ConnectorItem * connectorItem, bool show);

protected:
	PCBSketchWidget * m_sketchWidget;
	QList<ConnectorItem *> m_connectorItems;
	QListWidget * m_listWidget;
	ConnectorItem * m_activeConnectorItem;
	bool m_doFill;
	QPushButton * m_doFillButton;

};

#endif 
