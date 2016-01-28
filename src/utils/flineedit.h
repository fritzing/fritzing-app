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

#ifndef FLINEEDIT_H
#define FLINEEDIT_H

#include <QLineEdit>
#include <QMouseEvent>

class FLineEdit : public QLineEdit {
Q_OBJECT

public:
	FLineEdit(QWidget * parent = NULL);
	~FLineEdit();

protected:
	void mousePressEvent( QMouseEvent * event );
	void enterEvent( QEvent *);
	void leaveEvent( QEvent *);

signals:
	void mouseEnter();
	void mouseLeave();
	void editable(bool);

protected slots:
	void editingFinishedSlot();
	
protected:
	bool m_readOnly;						// was using readOnly() but mac didn't like it

};


#endif
