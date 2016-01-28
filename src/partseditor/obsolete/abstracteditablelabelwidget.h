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
along with Fritzing.  If not,AbstractEditableLabelWidget see <http://www.gnu.org/licenses/>.

********************************************************************

$Revision: 6904 $:
$Author: irascibl@gmail.com $:
$Date: 2013-02-26 16:26:03 +0100 (Di, 26. Feb 2013) $

********************************************************************/



#include <QWidget>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QGridLayout>

#include "editablelabel.h"
#include "../waitpushundostack.h"

#ifndef ABSTRACTEDITABLELABELWIDGET_H_
#define ABSTRACTEDITABLELABELWIDGET_H_

class AbstractEditableLabelWidget : public QFrame {
Q_OBJECT
	public:
		AbstractEditableLabelWidget(QString text, WaitPushUndoStack *undoStack, QWidget *parent=0, QString title="", bool edited=false, bool noSpacing=false);
		QString text();

	protected slots:
		void editionStarted(QString text);
		void informEditionCompleted();
		void editionCanceled();

	signals:
		void editionCompleted(QString text);
		void editionStarted();
		void editionFinished();

	protected:
		void toStandardMode();
		void toEditionMode();
		void setNoSpacing(QLayout *layout);

		virtual QString editionText()=0;
		virtual void setEditionText(QString text)=0;
		virtual QWidget* myEditionWidget()=0;
		virtual void setEmptyTextToEdit()=0;

		QLabel *m_title;
		EditableLabel *m_label;

		QPushButton *m_acceptButton;
		QPushButton *m_cancelButton;

		class WaitPushUndoStack * m_undoStack;

		bool m_noSpacing;
		bool m_edited;
		volatile bool m_isInEditionMode;
};

#endif /* ABSTRACTEDITABLELABELWIDGET_H_ */
