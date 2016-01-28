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



#ifndef ADDREMOVELISTWIDGET_H_
#define ADDREMOVELISTWIDGET_H_

#include <QGroupBox>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>

class AddRemoveListWidget : public QGroupBox {
	Q_OBJECT

	public:
		AddRemoveListWidget(QString title, QWidget *parent=0);
		int count();
		QListWidgetItem* itemAt(int rowIdx);
		QStringList& getItemsText();
		void setItemsText(const QStringList& texts);

	protected slots:
		void addItem();
		void addItem(QString itemText);
		void removeSelectedItems();

	protected:
		QLabel *m_label;

		QPushButton *m_addButton;
		QPushButton *m_removeButton;

		QListWidget *m_list;
};

#endif /* ADDREMOVELISTWIDGET_H_ */
