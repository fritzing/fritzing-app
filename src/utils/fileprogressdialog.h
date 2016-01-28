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


#ifndef FILEPROGRESSDIALOG_H
#define FILEPROGRESSDIALOG_H

#include <QDialog>
#include <QProgressBar>
#include <QLabel>
#include <QDomElement>
#include <QTimer>

class FileProgressDialog : public QDialog
{
Q_OBJECT

public:
	FileProgressDialog(QWidget *parent = 0);
    FileProgressDialog(const QString & title, int initialMaximum, QWidget *parent);
	~FileProgressDialog();

	int value();
	void setBinLoadingCount(int);
	void setBinLoadingChunk(int);
    void setIncValueMod(int);
    void setIndeterminate();

protected:
	void closeEvent(QCloseEvent *);
	void resizeEvent(QResizeEvent *);

public slots:
	void setMinimum(int);
	void setMaximum(int);
    void addMaximum(int);
	void setValue(int);
	void incValue();
	void setMessage(const QString & message);
	void sendCancel();

	void loadingInstancesSlot(class ModelBase *, QDomElement & instances);
	void loadingInstanceSlot(class ModelBase *, QDomElement & instance);
	void settingItemSlot();

protected slots:
    void updateIndeterminate();


signals:
	void cancel();

protected:
	void init(const QString & title, int initialMaximum);

protected:
	QProgressBar * m_progressBar;	
	QLabel * m_message;

	int m_binLoadingCount;
	int m_binLoadingIndex;
	int m_binLoadingStart;
    int m_binLoadingChunk;
    int m_incValueMod;
	double m_binLoadingInc;
	double m_binLoadingValue;
    QTimer m_timer;
};


#endif 
