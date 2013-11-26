/*********************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2013 Fachhochschule Potsdam - http://fh-potsdam.de

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

//	TODO
//
//		multiple windows: update blog in all windows when one updates--share download data?
//		tips and tricks: save list of viewed tricks?
//

#include "welcomeview.h"
#include "../debugdialog.h"
#include "../help/tipsandtricks.h"

#include <QTextEdit>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QPixmap>
#include <QSpacerItem>
#include <QSettings>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QDesktopServices>
#include <QDomDocument>
#include <QDomNodeList>
#include <QDomElement>

void zeroMargin(QLayout * layout) {
    layout->setMargin(0);
    layout->setSpacing(0);
}

//////////////////////////////////////

WelcomeView::WelcomeView(QWidget * parent) : QFrame(parent) 
{
    setAcceptDrops(false);
    initLayout();

	connect(this, SIGNAL(newSketch()), this->window(), SLOT(createNewSketch()));
	connect(this, SIGNAL(openSketch()), this->window(), SLOT(mainLoad()));
	connect(this, SIGNAL(recentSketch(const QString &, const QString &)), this->window(), SLOT(openRecentOrExampleFile(const QString &, const QString &)));

    QNetworkAccessManager * manager = new QNetworkAccessManager(this);
	connect(manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(gotBlogSnippet(QNetworkReply *)));
	manager->get(QNetworkRequest(QUrl("http://blog.fritzing.org/recent-posts-app/")));

	TipsAndTricks::initTipSets();
    m_tip->setText(QString("<a href='tip' style='text-decoration:none; color:#2e94af;'>%1</a>").arg(TipsAndTricks::randomTip()));
}

WelcomeView::~WelcomeView() {
}

void WelcomeView::initLayout()
{
	QGridLayout * mainLayout = new QGridLayout();

	QWidget * recent = initRecent();
	mainLayout->addWidget(recent, 0, 0);

	QWidget * widget = initBlog();
	mainLayout->addWidget(widget, 0, 1);

	widget = initKit();
	mainLayout->addWidget(widget, 1, 1);

	widget = initTip();
	mainLayout->addWidget(widget, 1, 0);

	this->setLayout(mainLayout);
}


QWidget * WelcomeView::initRecent() {
	QFrame * frame = new QFrame;
    frame->setObjectName("recentFrame");
	QVBoxLayout * frameLayout = new QVBoxLayout;

    frameLayout->setMargin(0);
    frameLayout->setSpacing(0);


	QStringList names;
    names << "recentTitle" << "recentFileFrame" << "recentFileFrame" << "recentFileFrame" << "recentFileFrame" << "recentFileFrame" << "recentFileFrame" << "recentSpace" << "recentNewSketch" << "recentOpenSketch";

	foreach (QString name, names) {
		QWidget * widget = NULL;
        QLabel * icon = NULL;
        QLabel * text = NULL;
        if (name == "recentSpace") {
			widget = new QLabel;
		}
		else if (name == "recentTitle") {
            widget = new QLabel(tr("Recent Sketches"));
		}
        else if (name == "recentFileFrame") {
            widget = makeRecentItem(name,"","",icon,text);
            m_recentIconList << icon;
            m_recentList << text;
		}
        else if (name == "recentNewSketch") {
			widget = makeRecentItem(name, 
                QString("<a href='new' style='text-decoration:none; color:#666;'><img src=':/resources/images/icons/WS-new-icon.png' /></a>"),
                QString("<a href='new' style='text-decoration:none; color:#666;'>%1</a>").arg(tr("New Sketch >>")),
                icon,
                text);
		}
		else if (name == "recentOpenSketch") {
			widget = makeRecentItem(name, 
                QString("<a href='open' style='text-decoration:none; color:#666;'><img src=':/resources/images/icons/WS-open-icon.png' /></a>"),
                QString("<a href='open' style='text-decoration:none; color:#666;'>%1</a>").arg(tr("Open Sketch >>")),
                icon,
                text);
		}
		widget->setObjectName(name);
		frameLayout->addWidget(widget);
	}

    frameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));

	frame->setLayout(frameLayout);
	return frame;
}

QWidget * WelcomeView::makeRecentItem(const QString & objectName, const QString & iconText, const QString & textText, QLabel * & icon, QLabel * & text) {
    QFrame * rFrame = new QFrame;
    QHBoxLayout * rFrameLayout = new QHBoxLayout;

    icon = new QLabel(iconText);
    icon->setObjectName("recentIcon");
    connect(icon, SIGNAL(linkActivated(const QString &)), this, SLOT(clickRecent(const QString &)));
    rFrameLayout->addWidget(icon);

	text = new QLabel(textText);
    text->setObjectName(objectName);
    text->setObjectName("recentText");
    rFrameLayout->addWidget(text);
    connect(text, SIGNAL(linkActivated(const QString &)), this, SLOT(clickRecent(const QString &)));

    rFrame->setLayout(rFrameLayout);
    return rFrame;
}

QWidget * WelcomeView::initKit() {

    QFrame * frame = new QFrame();
    frame->setObjectName("shopFrame");
    QVBoxLayout * frameLayout = new QVBoxLayout;

    frameLayout->setMargin(0);
    frameLayout->setSpacing(0);


	// use parent/child relation to manage overlapping widgets
    QFrame * titleFrame = new QFrame();
    titleFrame->setObjectName("shopTitleFrame");
    QHBoxLayout * titleFrameLayout = new QHBoxLayout;

    titleFrameLayout->setMargin(0);
    titleFrameLayout->setSpacing(0);

   /* overlapFrame->setGeometry(0, 0, pixmap.width(), 30);*/

    QLabel * title = new QLabel(tr("Fritzing Creator Kit"));
    title->setObjectName("shopTitle");
    titleFrameLayout->addWidget(title);

    QLabel * url = new QLabel(QString("<a href='http://fritzing.org/shop/'  style='text-decoration:none; display:block; font-weight:bold; color:#323232;'>%1</a>").arg(tr("| Shop >>")));
    url->setObjectName("shopTitleGoto");
    connect(url, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));
    titleFrameLayout->addWidget(url);


    titleFrameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));


    titleFrame->setLayout(titleFrameLayout);

    frameLayout->addWidget(titleFrame);

    QLabel * label = new QLabel("<img src=':/resources/images/welcome_kit.png' />");
    label->setObjectName("shopImage");
    frameLayout->addWidget(label);

    frameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));

    QFrame * footerFrame = new QFrame();
    footerFrame->setObjectName("shopFooterFrame");

    QHBoxLayout * footerFrameLayout = new QHBoxLayout;

    footerFrameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));

    QLabel * footerLabel = new QLabel(QString("<a href='http://fritzing.org/creatorkit/'  style='text-decoration:none; color:#802742;'>%1</a>").arg(tr("Get your CreatorKit now.   ")));
    footerLabel->setObjectName("shopLogoText");
    footerFrameLayout->addWidget(footerLabel);
    connect(footerLabel, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));

    QLabel * footerLogoLabel = new QLabel(tr("<a href='http://fritzing.org/creatorkit/'><img src=':/resources/images/icons/WS-shopLogo.png'/></a>"));
    footerLogoLabel->setObjectName("shopLogo");
    footerFrameLayout->addWidget(footerLogoLabel);
    connect(footerLogoLabel, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));

    footerFrame->setLayout(footerFrameLayout);
    frameLayout->addWidget(footerFrame);

    frame->setLayout(frameLayout);
    return frame;
}


QWidget * WelcomeView::initBlog() {

    QFrame * frame = new QFrame();
	frame->setObjectName("blogFrame");
	QVBoxLayout * frameLayout = new QVBoxLayout;
    zeroMargin(frameLayout);


    QFrame * titleFrame = new QFrame();
	titleFrame->setObjectName("blogTitleFrame");


	QHBoxLayout * titleFrameLayout = new QHBoxLayout;
    zeroMargin(titleFrameLayout);


    QLabel * titleLabel = new QLabel(tr("News and Stories"));
	titleLabel->setObjectName("blogTitle");
	titleFrameLayout->addWidget(titleLabel);

    QLabel * label = new QLabel(QString("<a href='http://blog.fritzing.org'  style='text-decoration:none; font-weight:bold; color:#323232;'>%1</a>").arg(tr("| Blog >>")));
	label->setObjectName("blogTitleGoto");
	titleFrameLayout->addWidget(label);
	connect(label, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));

	titleFrameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));

    titleFrame->setLayout(titleFrameLayout);
	frameLayout->addWidget(titleFrame);

	for (int i = 0; i < 3; i++) {
         QFrame * blogEntry = new QFrame;
         m_blogEntryList << blogEntry;
         blogEntry->setObjectName("blogEntry");
         QHBoxLayout * blogEntryLayout = new QHBoxLayout;

            /* QFrame * blogEntryPicture = new QFrame; */

            QLabel * picLabel = new QLabel;
            picLabel->setObjectName("blogEntryPicture");
            blogEntryLayout->addWidget(picLabel);
            m_blogEntryPictureList << picLabel;
            connect(picLabel, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));

            QFrame * blogEntryTextFrame = new QFrame;
            blogEntryTextFrame ->setObjectName("blogEntryTextFrame");
            QVBoxLayout * blogEntryTextLayout = new QVBoxLayout;

                QLabel * label = new QLabel();
                label->setObjectName("blogEntryTitle");
                blogEntryTextLayout->addWidget(label);
                m_blogEntryTitleList << label;
                connect(label, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));

                label = new QLabel();
                label->setObjectName("blogEntryText");
                blogEntryTextLayout->addWidget(label);
                m_blogEntryTextList << label;

                label = new QLabel();
                label->setObjectName("blogEntryDate");
                blogEntryTextLayout->addWidget(label);
                m_blogEntryDateList << label;

            blogEntryTextFrame->setLayout(blogEntryTextLayout);
            blogEntryLayout->addWidget(blogEntryTextFrame);

        blogEntry->setLayout(blogEntryLayout);
        frameLayout->addWidget(blogEntry);
	}

  /*  frameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));*/

    QFrame * footerFrame = new QFrame();
    footerFrame->setObjectName("blogFooterFrame");

        QHBoxLayout * footerFrameLayout = new QHBoxLayout;

        footerFrameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));


        QLabel * footerLabel = new QLabel(QString("<a href='http://blog.fritzing.org'  style='text-decoration:none; color:#802742;'>%1</a>").arg(tr("News on the Fritzing Project.   ")));
        footerLabel->setObjectName("blogLogoText");
        footerFrameLayout->addWidget(footerLabel);

        footerLabel = new QLabel(tr("<a href='http://blog.fritzing.org'><img src=':/resources/images/icons/WS-blogLogo.png' /></a>"));
        footerLabel->setObjectName("blogLogo");
        footerFrameLayout->addWidget(footerLabel);

    footerFrame->setLayout(footerFrameLayout);

    frameLayout->addWidget(footerFrame);

    frame->setLayout(frameLayout);

    foreach (QFrame * blogEntry, m_blogEntryList) blogEntry->setVisible(false);

    return frame;
}

void WelcomeView::showEvent(QShowEvent * event) {
	QFrame::showEvent(event);
	updateRecent();
}

void WelcomeView::updateRecent() {
	if (m_recentList.count() == 0) return;

	QSettings settings;
	QStringList files = settings.value("recentFileList").toStringList();
	int ix = 0;
	for (int i = 0; i < files.size(); ++i) {
		QFileInfo finfo(files[i]);
		if (!finfo.exists()) continue;

        QString text = QString("<a href='%1'  style='text-decoration:none; color:#666;'>%2 >></a>").arg(finfo.absoluteFilePath()).arg(finfo.fileName());
		m_recentList[ix]->setText(text);
		m_recentList[ix]->parentWidget()->setVisible(true);
        m_recentIconList[ix]->setText(QString("<a href='%1'><img src=':/resources/images/icons/WS-fzz-icon.png' /></a>").arg(finfo.absoluteFilePath()));
		if (++ix >= m_recentList.count()) {
			break;
		}
	}

	for (int j = ix; j < m_recentList.count(); ++j) {
		m_recentList[j]->parentWidget()->setVisible(false);
	}
}

void WelcomeView::clickRecent(const QString & url) {
	if (url == "open") {
		emit openSketch();
		return;
	}
	if (url == "new") {
		emit newSketch();
		return;
	}

	emit recentSketch(url, url);
}


void WelcomeView::gotBlogSnippet(QNetworkReply * networkReply) {

    foreach (QFrame * blogEntry, m_blogEntryList) blogEntry->setVisible(false);

    QNetworkAccessManager * manager = networkReply->manager();
	int responseCode = networkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	if (responseCode == 200) {
        QString data(networkReply->readAll());
        DebugDialog::debug("response data " + data);
		data = "<thing>" + data + "</thing>";		// make it one tree for xml parsing
		QDomDocument doc;
	    QString errorStr;
	    int errorLine;
	    int errorColumn;
	    if (doc.setContent(data, &errorStr, &errorLine, &errorColumn)) {
			readBlog(doc);
	    }
	}

    manager->deleteLater();
    networkReply->deleteLater();
}

void WelcomeView::clickBlog(const QString & url) {
	QDesktopServices::openUrl(url);
}

		/*

        // sample output from http://blog.fritzing.org/recent-posts-app/


<ul>
    <li>
        <img src=""/>
        <a class="title" href="http://blog.fritzing.org/2013/11/19/minimetalmaker/" title="MiniMetalMaker">MiniMetalMaker</a>
        <p class="date">Nov. 19, 2013</p>
        <p class="author">Nushin Isabelle</p>
        <p class="intro">We have heard a lot about synthetic 3D printers for home use - now we are entering the home use met...</p>
    </li> 
    <li>
        <img src="http://blog.fritzing.org/wp-content/uploads/doku-deckblatt.jpg"/>
        <a class="title" href="http://blog.fritzing.org/2013/11/19/the-little-black-midi/" title="The Little Black Midi">The Little Black Midi</a>
        <p class="date">Nov. 19, 2013</p>
        <p class="author">Nushin Isabelle</p>
        <p class="intro">Howdy!

We thought we should delight our readers a little by showing some dainties of creative ele...</p>
    </li> 
    <li>
        <img src="http://blog.fritzing.org/wp-content/uploads/charles1.jpg"/>
        <a class="title" href="http://blog.fritzing.org/2013/11/15/light-up-your-flat-with-charles-planetary-gear-system/" title="Light up your flat with Charles&#039; planetary gear system">Light up your flat with Charles' planetary gear system</a>
        <p class="date">Nov. 15, 2013</p>
        <p class="author">Nushin Isabelle</p>
        <p class="intro">Today, we got a visitor in the Fritzing Lab: Our neighbour, Charles Oleg, came by to show us his new...</p>
    </li> 
</ul>
		*/


void WelcomeView::readBlog(const QDomDocument & doc) {
	QDomNodeList nodeList = doc.elementsByTagName("li");
	int ix = 0;
	for (int i = 0; i < nodeList.count(); i++) {
		QDomElement element = nodeList.at(i).toElement();
        QDomElement child = element.firstChildElement();
        QHash<QString, QString> stuff;
        while (!child.isNull()) {
            if (child.tagName() == "img") {
                stuff.insert("img", child.attribute("src"));
            }
            else {
                QString clss = child.attribute("class");
                if (clss == "title") {
                    stuff.insert("title", child.attribute("title"));
                    stuff.insert("href", child.attribute("href"));
                }
                else {
                    stuff.insert(clss, child.text());
                }
            }
            child = child.nextSiblingElement();
        }
        if (stuff.value("title", "").isEmpty()) continue;
        if (stuff.value("href", "").isEmpty()) continue;
            
		m_blogEntryTitleList[ix]->setText(QString("<a href='%1' style='text-decoration:none; color:#666;'>%2</a>").arg(stuff.value("href")).arg(stuff.value("title")));
        QString text = stuff.value("intro", "");
        text.remove("\r");
        text.remove("\n");
        m_blogEntryTextList[ix]->setText(text);
        m_blogEntryPictureList[ix]->setText("");
        if (!stuff.value("img", "").isEmpty()) {
            QNetworkAccessManager * manager = new QNetworkAccessManager(this);
            manager->setProperty("index", ix);
            manager->setProperty("href", stuff.value("href"));
	        connect(manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(gotBlogImage(QNetworkReply *)));
	        manager->get(QNetworkRequest(QUrl(stuff.value("img"))));
        }

        QString dateStuff;
        if (!stuff.value("date", "").isEmpty()) {
            dateStuff.append(stuff.value("date"));
            dateStuff.append("    ");
        }
        dateStuff.append(stuff.value("author"));

        m_blogEntryDateList[ix]->setText(dateStuff);
        
        m_blogEntryList[ix]->setVisible(true);
		if (++ix >= m_blogEntryTextList.count()) {
			break;
		}

	}
}

void WelcomeView::gotBlogImage(QNetworkReply * networkReply) {

    QNetworkAccessManager * manager = networkReply->manager();
	int responseCode = networkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	if (responseCode == 200) {
        QByteArray data(networkReply->readAll());
        QString pic = QString("<a href='%1' style='text-decoration:none; color:#666;'><img src='data:image/jpg;base64,%2' /></a>").arg(manager->property("href").toString()).arg(QString(data.toBase64()));
		m_blogEntryPictureList[manager->property("index").toInt()]->setText(pic);
		
	}

    manager->deleteLater();
    networkReply->deleteLater();
}

QWidget * WelcomeView::initTip() {
	QFrame * tipFrame = new QFrame();
	tipFrame->setObjectName("tipFrame");
	QVBoxLayout * tipLayout = new QVBoxLayout();

    tipLayout->setMargin(0);
    tipLayout->setSpacing(0);

	QLabel * tipTitle = new QLabel(tr("Tip:"));
	tipTitle->setObjectName("tipTitle");
	tipLayout->addWidget(tipTitle);

	m_tip = new QLabel();
	m_tip->setObjectName("tip");
	connect(m_tip, SIGNAL(linkActivated(const QString &)), this->window(), SLOT(tipsAndTricks()));
	tipLayout->addWidget(m_tip);

	tipLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));

	tipFrame->setLayout(tipLayout);

	return tipFrame;
}

void WelcomeView::dragEnterEvent(QDragEnterEvent *event)
{
    event->ignore();
}
