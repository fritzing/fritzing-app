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



#ifndef ABSTRACTSTATESBUTTON_H_
#define ABSTRACTSTATESBUTTON_H_

#include <QIcon>
#include <QMouseEvent>

#include "../debugdialog.h"

class AbstractStatesButton {
public:
	virtual ~AbstractStatesButton() {}			// clears compiler warning
	void setEnabledIcon() {
		setImage(m_enabledImage);
	}
	void setDisabledIcon() {
		setImage(m_disabledImage);
	}
	void setPressedIcon() {
		setImage(m_pressedImage);
	}

protected:
	virtual QString imagePrefix() = 0;
	virtual QString imageSubfix() {
		return "_icon.png";
	}

	virtual void setImage(const QPixmap & pixmap) = 0;
	virtual void setupIcons(const QString &imageName, bool hasStates=true) {
		if(hasStates) {
			m_enabledImage = QPixmap(imagePrefix()+imageName+"Enabled"+imageSubfix());
			m_disabledImage = QPixmap(imagePrefix()+imageName+"Disabled"+imageSubfix());
			m_pressedImage = QPixmap(imagePrefix()+imageName+"Pressed"+imageSubfix());
		} else {
			m_enabledImage = QPixmap(imagePrefix()+imageName+imageSubfix());
			m_disabledImage = m_enabledImage;
			m_pressedImage =  m_enabledImage;
		}
	}

protected:
	QPixmap m_enabledImage;
	QPixmap m_disabledImage;
	QPixmap m_pressedImage;
};

#endif /* ABSTRACTSTATESBUTTON_H_ */
