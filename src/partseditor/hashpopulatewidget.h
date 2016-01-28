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



#ifndef HASHPOPULATEWIDGET_H_
#define HASHPOPULATEWIDGET_H_

#include <QFrame>
#include <QHash>
#include <QLineEdit>
#include <QGridLayout>

#include "baseremovebutton.h"

class HashLineEdit : public QLineEdit {
	Q_OBJECT
	public:
		HashLineEdit(const QString &text, bool defaultValue = false, QWidget *parent = 0);
		bool hasChanged();
		QString textIfSetted();

	protected slots:
		void updateObjectName();

	protected:
		void mousePressEvent(QMouseEvent * event);
		void focusOutEvent(QFocusEvent * event);

		QString m_firstText;
		bool m_isDefaultValue;
};

class HashRemoveButton : public BaseRemoveButton {
	Q_OBJECT
	public:
		HashRemoveButton(QWidget* label, QWidget* value, QWidget *parent)
			: BaseRemoveButton(parent)
		{
			m_label = label;
			m_value = value;
		}

		QWidget *label() {return m_label;}
		QWidget *value() {return m_value;}

	signals:
		void clicked(HashRemoveButton*);

	protected:
		void clicked() {
			emit clicked(this);
		}
		QWidget *m_label;
		QWidget *m_value;
};

class HashPopulateWidget : public QFrame {
	Q_OBJECT
	public:
		HashPopulateWidget(const QString & title, const QHash<QString,QString> &initValues, const QStringList &readOnlyKeys, bool keysOnly, QWidget *parent);
		const QHash<QString,QString> & hash();
		HashLineEdit* lineEditAt(int row, int col);

	protected slots:
		void lastRowEditionCompleted();
		void removeRow(HashRemoveButton*);
        void lineEditChanged();

	signals:
		void editionStarted();
        void changed();

	protected:
		void addRow(QGridLayout *layout = 0);
		QGridLayout* gridLayout();
		HashRemoveButton *createRemoveButton(HashLineEdit* label, HashLineEdit* value);

		HashLineEdit *m_lastLabel;
		HashLineEdit *m_lastValue;

        bool m_keysOnly;
};

#endif /* HASHPOPULATEWIDGET_H_ */
