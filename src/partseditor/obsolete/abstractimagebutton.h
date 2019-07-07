/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2019 Fritzing

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

********************************************************************/

#ifndef ABSTRACTIMAGEBUTTON_H_
#define ABSTRACTIMAGEBUTTON_H_

#include <QLabel>

#include "abstractstatesbutton.h"

class AbstractImageButton : public QLabel, public AbstractStatesButton {
	Q_OBJECT
public:
	AbstractImageButton(QWidget *parent=0)
		: QLabel(parent)
	{
	};
	virtual ~AbstractImageButton() {}

signals:
	void clicked();

protected:
	virtual QString imagePrefix() = 0;

	void setImage(const QPixmap & pixmap) {
		setPixmap(pixmap);
	}

	void mousePressEvent(QMouseEvent * event) {
		setPressedIcon();
		QLabel::mousePressEvent(event);
	}

	void mouseReleaseEvent(QMouseEvent * event) {
		if(isEnabled()) {
			emit clicked();
			setEnabledIcon();
		}
		QLabel::mouseReleaseEvent(event);
	}
};

#endif /* ABSTRACTIMAGEBUTTON_H_ */
