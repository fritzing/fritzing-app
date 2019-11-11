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
	FileProgressDialog(QWidget *parent = nullptr);
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
	QProgressBar * m_progressBar = nullptr;
	QLabel * m_message = nullptr;

	int m_binLoadingCount = 0;
	int m_binLoadingIndex = 0;
	int m_binLoadingStart = 0;
	int m_binLoadingChunk = 0;
	int m_incValueMod = 0;
	double m_binLoadingInc = 0.0;
	double m_binLoadingValue = 0.0;
	QTimer m_timer;
};


#endif
