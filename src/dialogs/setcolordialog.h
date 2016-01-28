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


#ifndef SETCOLORDIALOG_H
#define SETCOLORDIALOG_H

#include <QDialog>
#include <QColorDialog>
#include <QLabel>
#include <QCheckBox>

class SetColorDialog : public QDialog
{
Q_OBJECT

public:
	SetColorDialog(const QString & message, QColor & currentColor, QColor & standardColor, bool askPrefs, QWidget *parent = 0);
	~SetColorDialog();

	const QColor & selectedColor();
	bool isPrefsColor();

protected:
	void setColor(const QColor &);
	void setCustomColor(const QColor &);

protected slots:
	void selectCurrent();
	void selectCustom();
	void selectLastCustom();
	void selectStandard();


protected:
	QString m_message;
	QColor m_currentColor;	
	QColor m_standardColor;
	QColor m_selectedColor;
	QColor m_customColor;
	QLabel * m_currentColorLabel;
	QLabel * m_standardColorLabel;
	QLabel * m_customColorLabel;
	QLabel * m_selectedColorLabel;
	QCheckBox * m_prefsCheckBox;
};


#endif 
