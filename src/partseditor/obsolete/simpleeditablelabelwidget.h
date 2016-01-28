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



#ifndef SIMPLEEDITABLELABELWIDGET_H_
#define SIMPLEEDITABLELABELWIDGET_H_

#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QUndoStack>

class SimpleEditableLabelWidget : public QFrame {
	Q_OBJECT
	public:
		SimpleEditableLabelWidget(QUndoStack *undoStack, QWidget *parent=0, const QString &text = "", bool edited=false);
		void setText(const QString &text, bool markAsEdited = true);
		QString text();

	protected slots:
		void toStandardMode(bool markAsEdited = true);
		void toEditionMode();

	signals:
		void textChanged(const QString& text);

	protected:
		void swapWidgets(QWidget *toShow, QWidget *toHide);
		void swapMode();

		void mousePressEvent(QMouseEvent *event);
		void keyPressEvent(QKeyEvent *event);

		void updateUndoStackIfNecessary();

	protected:
		QLabel *m_label;
		QLineEdit *m_lineEdit;
		QUndoStack *m_undoStack;

		bool m_hasBeenEdited;
		volatile bool m_isInEditionMode;
};

#endif /* SIMPLEEDITABLELABELWIDGET_H_ */
