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


#ifndef PINLABELDIALOG_H
#define PINLABELDIALOG_H

#include <QDialog>
#include <QList>
#include <QFrame>
#include <QStringList>
#include <QGridLayout>
#include <QUndoStack>
#include <QUndoCommand>
#include <QLineEdit>
#include <QPushButton>

class PinLabelDialog : public QDialog
{
	Q_OBJECT

public:
	PinLabelDialog(const QStringList & labels, bool singleRow, const QString & chipLabel, bool isCore, QWidget *parent = 0);
	~PinLabelDialog();

	const QStringList & labels();
	void setLabelText(int index, QLineEdit *, const QString & text);
	bool doSaveAs();

protected slots:
	void labelChanged();
	void buttonClicked(QAbstractButton *);
	void undoChanged(bool);
	void labelEdited(const QString &) ;

protected:
	QFrame * initLabels(const QStringList & labels, bool singleRow, const QString & chipLabel);
	void makeOnePinEntry(int index, const QString & label, Qt::Alignment alignment, int row, QGridLayout *);
	void keyPressEvent(QKeyEvent *e);

protected:
	bool m_isCore;
	QStringList m_labels;
	QUndoStack m_undoStack;
	QPushButton * m_saveAsButton;
	QPushButton * m_undoButton;
	QPushButton * m_redoButton;
	bool m_doSaveAs;

};

class PinLabelUndoCommand : public QUndoCommand {

public:
	PinLabelUndoCommand(PinLabelDialog *, int index, QLineEdit *, const QString & previous, const QString & next);
	
	void undo();
	void redo();

protected:
	QString m_previous;
	QString m_next;
	PinLabelDialog * m_pinLabelDialog;
	int m_index;
	QLineEdit * m_lineEdit;

};

#endif 
