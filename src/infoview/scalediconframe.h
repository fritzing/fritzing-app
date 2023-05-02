/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2023 Fritzing GmbH

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

#ifndef SCALEDICONFRAME_H
#define SCALEDICONFRAME_H

#include <QLabel>
#include <QHBoxLayout>
#include <QPixmap>
#include <QFrame>

class ScaledIconFrame : public QFrame
{
	Q_OBJECT

public:
	explicit ScaledIconFrame(QWidget *parent);
	void setIcons(const QPixmap * icon1, const QPixmap * icon2, const QPixmap * icon3);

	static const int STANDARD_ICON_IMG_WIDTH;
	static const int STANDARD_ICON_IMG_HEIGHT;

protected:
	void resizeEvent(QResizeEvent *event) override;

private:
	QLabel * addLabel(QHBoxLayout *hboxLayout, const QPixmap &pixmap);

	QLabel * m_icon1;
	QLabel * m_icon2;
	QLabel * m_icon3;
	QHBoxLayout * m_iconLayout;

	void updateLayout();
	QPixmap NoIcon;

};

#endif // SCALEDICONFRAME_H
