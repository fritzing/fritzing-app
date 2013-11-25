/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2012 Fachhochschule Potsdam - http://fh-potsdam.de

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

$Revision: 6483 $:
$Author: irascibl@gmail.com $:
$Date: 2012-09-26 15:45:37 +0200 (Mi, 26. Sep 2012) $

********************************************************************/



#ifndef WELCOMEVIEW_H
#define WELCOMEVIEW_H

#include <QFrame>
#include <QLabel>
#include <QList>
#include <QWidget>
#include <QNetworkReply>
#include <QDomDocument>
#include <QDragEnterEvent>

class WelcomeView : public QFrame
{
Q_OBJECT

public:
	WelcomeView(QWidget * parent = 0);
	~WelcomeView();

	void showEvent(QShowEvent * event);
    void dragEnterEvent(QDragEnterEvent *event);

protected:
	void initLayout();
	QWidget * initRecent();
	QWidget * initBlog();
	QWidget * initKit();
	QWidget * initTip();
	void updateRecent();
	void readBlog(const QDomDocument &);
    QWidget * makeRecentItem(const QString & objectName, const QString & iconText, const QString & textText, QLabel * & icon, QLabel * & text);

signals:
	void newSketch();
	void openSketch();
	void recentSketch(const QString & filename, const QString & actionText);

protected slots:
	void clickRecent(const QString &);
    void gotBlogSnippet(QNetworkReply *);
    void gotBlogImage(QNetworkReply *);
	void clickBlog(const QString &);

protected:
	QList<QLabel *> m_recentList;
	QList<QLabel *> m_recentIconList;
    QList<QFrame *> m_blogEntryList;
    QList<QLabel *> m_blogEntryTitleList;
    QList<QLabel *> m_blogEntryTextList;
    QList<QLabel *> m_blogEntryPictureList;
    QList<QLabel *> m_blogEntryDateList;
	QLabel * m_tip;
	QFrame * m_blog;
	QFrame * m_creatorKit;
};


#endif
