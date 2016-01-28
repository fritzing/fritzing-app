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

$Revision$:
$Author$:
$Date$

********************************************************************/

#ifndef BASEREMOVEBUTTON_H_
#define BASEREMOVEBUTTON_H_

#include <QLabel>

class BaseRemoveButton : public QLabel {
	public:
		BaseRemoveButton(QWidget *parent) : QLabel(parent) {
			m_enterIcon = QPixmap(":/resources/images/remove_prop_enter.png");
			m_leaveIcon = QPixmap(":/resources/images/remove_prop_leave.png");
			setPixmap(m_leaveIcon);
		}

	protected:
		virtual void clicked() = 0;

		void mousePressEvent(QMouseEvent * event) {
			clicked();
			QLabel::mousePressEvent(event);
		}

		void enterEvent(QEvent * event) {
			setPixmap(m_enterIcon);
			QLabel::enterEvent(event);
		}

		void leaveEvent(QEvent * event) {
			setPixmap(m_leaveIcon);
			QLabel::leaveEvent(event);
		}

	protected:
		QPixmap m_enterIcon;
		QPixmap m_leaveIcon;
};

#endif /* BASEREMOVEBUTTON_H_ */
