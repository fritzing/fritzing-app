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

#ifndef QUOTEDIALOG_H
#define QUOTEDIALOG_H

#include <QTableWidget>
#include <QDialog>
#include <QLabel>

struct CountCost {
	int count;
	double cost;
};

class LabelThing : public QLabel
{
	Q_OBJECT

public:
	LabelThing(const QString & text, const QString & released, const QString & pressed, const QString & hover, QWidget * parent = NULL);

	void enterEvent(QEvent * event);
	void leaveEvent(QEvent * event);
	void mousePressEvent(QMouseEvent * event);
	void mouseReleaseEvent(QMouseEvent * event);
	void paintEvent(QPaintEvent * event);

public:
	enum State {
		RELEASED,
		PRESSED,
		HOVER
	};

Q_SIGNALS:
	void clicked();

protected:
	QPixmap m_releasedImage;
	QPixmap m_pressedImage;
	QPixmap m_hoverImage;
	State m_state;
};

class QuoteDialog : public QDialog {
	Q_OBJECT

public:
	QuoteDialog(bool full, QWidget *parent = 0);
	~QuoteDialog();

	void setText();

public:
	static void setDimensions(double width, double height, int boardCount, const QString & boardTitle);
	static void setCountCost(int index, int count, double cost);
	static QString countArgs();
	static void setQuoteSucceeded(bool);
	static bool quoteSucceeded();

protected:
	static void initCounts();

public:
	static constexpr int MessageCount = 4;

protected Q_SLOTS:
	void visitFritzingFab();

protected:
	QLabel * m_messageLabel;
	QTableWidget * m_tableWidget;
};

#endif
