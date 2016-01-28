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



#include <QtAlgorithms>

#include "hashpopulatewidget.h"
#include "../debugdialog.h"
#include "../utils/misc.h"

HashLineEdit::HashLineEdit(const QString &text, bool defaultValue, QWidget *parent) : QLineEdit(text, parent) 
{
    connect(this, SIGNAL(editingFinished()), parent, SLOT(lineEditChanged()));
	m_firstText = text;
	m_isDefaultValue = defaultValue;
	if(defaultValue) {
		setStyleSheet("font-style: italic;");
		connect(this,SIGNAL(textChanged(QString)),this,SLOT(updateObjectName()));
	} else {
		setStyleSheet("font-style: normal;");
	}
}

QString HashLineEdit::textIfSetted() {
	if(m_isDefaultValue && !hasChanged()) {
		return "";
	} else {
		return text();
	}
}

void HashLineEdit::updateObjectName() {
	if(m_isDefaultValue) {
		if(!isModified() && !hasChanged()) {
			setStyleSheet("font-style: italic;");
		} else {
			setStyleSheet("font-style: normal;");
		}
	}
}

void HashLineEdit::mousePressEvent(QMouseEvent * event) {
	if(m_isDefaultValue && !isModified() && !hasChanged()) {
		setText("");
	}
	QLineEdit::mousePressEvent(event);
}

bool HashLineEdit::hasChanged() {
	return m_firstText != text();
}

void HashLineEdit::focusOutEvent(QFocusEvent * event) {
	if(text().isEmpty()) {
		setText(m_firstText);
	}
	QLineEdit::focusOutEvent(event);
}

HashPopulateWidget::HashPopulateWidget(const QString & title, const QHash<QString,QString> &initValues, const QStringList &readOnlyKeys, bool keysOnly, QWidget *parent) : QFrame(parent) {
    m_keysOnly = keysOnly;
	m_lastLabel = NULL;
	m_lastValue = NULL;

    QGridLayout *layout = new QGridLayout();
	layout->setColumnStretch(0,0);
	layout->setColumnStretch(1,1);
	layout->setColumnStretch(2,0);

    if (!title.isEmpty()) {
	    layout->addWidget(new QLabel(title),0,0,0);
    }

	QList<QString> keys = initValues.keys();
	qSort(keys);

	for(int i=0; i < keys.count(); i++) {
		HashLineEdit *name = new HashLineEdit(keys[i],false,this);
        HashLineEdit *value = new HashLineEdit(initValues[keys[i]],false,this);
        if (m_keysOnly) value->hide();

        int ix = layout->rowCount();

		if(readOnlyKeys.contains(keys[i])) {
			name->setEnabled(false);
		} else {
			HashRemoveButton *button = createRemoveButton(name, value);
			layout->addWidget(button,ix,3);
		}

		layout->addWidget(name,ix,0);
		layout->addWidget(value,ix,1,1,2);
	}

	addRow(layout);


	this->setLayout(layout);  
}

HashRemoveButton *HashPopulateWidget::createRemoveButton(HashLineEdit* label, HashLineEdit* value) {
	HashRemoveButton *button = new HashRemoveButton(label, value, this);
	connect(button, SIGNAL(clicked(HashRemoveButton*)), this, SLOT(removeRow(HashRemoveButton*)));
	return button;
}

const QHash<QString,QString> &HashPopulateWidget::hash() {
	static QHash<QString,QString> theHash;

	theHash.clear();
	for(int i=1 /*i==0 is title*/; i < gridLayout()->rowCount() /*last one is always an empty one*/; i++) {
		QString label;
		HashLineEdit *labelEdit = lineEditAt(i,0);
		if(labelEdit) {
			label = labelEdit->textIfSetted();
		}

		QString value;
		HashLineEdit *valueEdit = lineEditAt(i,1);
		if(valueEdit) {
			value = valueEdit->textIfSetted();
		}

		if(!label.isEmpty() /*&& !value.isEmpty()*/) {
			theHash.insert(label, value);
		}
	}
	return theHash;
}

HashLineEdit* HashPopulateWidget::lineEditAt(int row, int col) {
	QLayoutItem *litem = gridLayout()->itemAtPosition(row,col);
	return litem ? qobject_cast<HashLineEdit*>(litem->widget()) : NULL;
}

void HashPopulateWidget::addRow(QGridLayout *layout) {
	if(layout == NULL) {
		layout = gridLayout();
	}

	if(m_lastLabel) {
		disconnect(m_lastLabel,SIGNAL(editingFinished()),this,SLOT(lastRowEditionCompleted()));
	}

	if(m_lastValue) {
		disconnect(m_lastValue,SIGNAL(editingFinished()),this,SLOT(lastRowEditionCompleted()));
	}

    int ix = layout->rowCount();

	m_lastLabel = new HashLineEdit(QObject::tr("a label"),true,this);
	layout->addWidget(m_lastLabel,ix,0);
	connect(m_lastLabel,SIGNAL(editingFinished()),this,SLOT(lastRowEditionCompleted()));

	m_lastValue = new HashLineEdit(QObject::tr("a value"),true,this);
	layout->addWidget(m_lastValue,ix,1,1,2);
	connect(m_lastValue,SIGNAL(editingFinished()),this,SLOT(lastRowEditionCompleted()));
    if (m_keysOnly) m_lastValue->hide();

	emit editionStarted();
}

QGridLayout * HashPopulateWidget::gridLayout() {
	return qobject_cast<QGridLayout *>(this->layout());
}

void HashPopulateWidget::lastRowEditionCompleted() {
	if(	m_lastLabel && !m_lastLabel->text().isEmpty() && m_lastLabel->hasChanged()) {
		if(m_lastLabel->text().isEmpty() && m_lastValue->text().isEmpty()) {
			// removeRow() ?;
		} else {
			HashRemoveButton *button = createRemoveButton(m_lastLabel, m_lastValue);
            for (int i = 0; i < gridLayout()->rowCount(); i++) {
                HashLineEdit * label = lineEditAt(i, 0);
                if (m_lastLabel == label) {
                    gridLayout()->addWidget(button,i,3);
                    break;
                }
            }
			addRow();
		}
	}
}

void HashPopulateWidget::removeRow(HashRemoveButton* button) {
	QLayout *lo = layout();
	QList<QWidget*> widgs;
	widgs << button->label() << button->value() << button;
	foreach(QWidget* w, widgs) {
		lo->removeWidget(w);   
		//w->hide();
        //delete w;
	}
	lo->update();
    emit changed();
}

void HashPopulateWidget::lineEditChanged() {
    emit changed();
}
