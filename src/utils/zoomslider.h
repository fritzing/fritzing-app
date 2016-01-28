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

#ifndef ZOOMSLIDER_H_
#define ZOOMSLIDER_H_

#include <QEvent>
#include <QFrame>
#include <QString>
#include <QSlider>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTimer>

class ZoomLabel : public QLabel {
        Q_OBJECT

public:
        ZoomLabel(QWidget * parent);
        ~ZoomLabel();

		void setImages(const QString & normal, const QString & pressed);
		void setAutoRepeat(bool);

        void mousePressEvent(QMouseEvent *);
        void mouseMoveEvent(QMouseEvent *);
        void mouseReleaseEvent(QMouseEvent *);

signals:
        void clicked();

protected slots:
		void repeat();

protected:
		QTimer m_timer;
		QPixmap m_pressed;
		QPixmap m_normal;
		bool m_autoRepeat;
		bool m_mouseIsDown;
		bool m_mouseIsIn;
		bool m_repeated;
};

class ZoomSlider: public QFrame {
Q_OBJECT

public:
	ZoomSlider(int maxValue, QWidget * parent=0);

	void setValue(double);
	double value();

	void zoomOut();
	void zoomIn();

protected slots:
	//void emitZoomChanged();
	//void updateBackupFieldsIfOptionSelected(int index);
	void sliderValueChanged(int newValue);
	void sliderTextEdited(const QString & newText);
	void minusClicked();
	void plusClicked();

protected:
	void step(int direction);
	void sliderTextEdited(const QString & newValue, bool doEmit);
	void showEvent(QShowEvent * event);
	static void loadFactors();

protected:
	//int itemIndex(QString value);
	//void updateBackupFields();

	static QList<double> ZoomFactors;
	//bool m_userStillWriting;
	//QString m_valueBackup;
	//int m_indexBackup;
	QSlider * m_slider;
	QLineEdit * m_lineEdit;
    ZoomLabel * m_plusButton;
    ZoomLabel * m_minusButton;
	QLabel * m_suffix;
	bool m_firstTime;

public:
	static double ZoomStep;

signals:
	void zoomChanged(double newZoom);
};

#endif /* ZOOMSLIDER_H_ */
