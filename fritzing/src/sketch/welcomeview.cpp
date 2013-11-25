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
	manager->get(QNetworkRequest(QUrl("http://blog.fritzing.org/recent-posts/")));

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

    QFrame * frame2 = new QFrame();
    frame2->setObjectName("shopFrame");
    QVBoxLayout * frameLayout2 = new QVBoxLayout;

	// use parent/child relation to manage overlapping widgets
        QFrame * overlapFrame = new QFrame();
        overlapFrame->setObjectName("shopTitelFrame");
        QHBoxLayout * overlapFrameLayout = new QHBoxLayout;
   /* overlapFrame->setGeometry(0, 0, pixmap.width(), 30);*/

        QLabel * title = new QLabel(tr("Fritzing Creator Kit"));
        title->setObjectName("shopTitle");
        overlapFrameLayout->addWidget(title);

        QLabel * url = new QLabel(QString("<a href='http://fritzing.org/shop/'  style='text-decoration:none; display:block; font-weight:bold; color:#323232;'>%1</a>").arg(tr("| Shop >>")));
        url->setObjectName("shopTitleGoto");
        connect(url, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));
        overlapFrameLayout->addWidget(url);


        overlapFrame->setLayout(overlapFrameLayout);


    frameLayout2->addWidget(overlapFrame);

    QLabel * label2 = new QLabel(QString("<img src:'/resources/images/welcome_kit.png' />"));
    label2->setObjectName("shopImage");
    frameLayout2->addWidget(label2);


    frameLayout2->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));


    QFrame * footerFrame = new QFrame();
        footerFrame->setObjectName("shopFooterFrame");

        QHBoxLayout * footerFrameLayout = new QHBoxLayout;

        footerFrameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));

        QLabel * footerLabel = new QLabel(QString("<a href='http://fritzing.org/shop/'  style='text-decoration:none; color:#802742;'>%1</a>").arg(tr("Get your CreatorKit now.   ")));
        footerLabel->setObjectName("shopLogoText");
        footerFrameLayout->addWidget(footerLabel);

       QLabel * footerLogoLabel = new QLabel(tr("<a href='http://fritzing.org/shop/'><img src=':/resources/images/icons/WS-shopLogo.png'/></a>"));
        footerLogoLabel->setObjectName("shopLogo");
        footerFrameLayout->addWidget(footerLogoLabel);

        footerFrame->setLayout(footerFrameLayout);
     frameLayout2->addWidget(footerFrame);

      frame2->setLayout(frameLayout2);
      return frame2;

}


QWidget * WelcomeView::initBlog() {
    QFrame * frame = new QFrame();
	frame->setObjectName("blogFrame");
	QVBoxLayout * frameLayout = new QVBoxLayout;

    QFrame * titleFrame = new QFrame();
	titleFrame->setObjectName("blogTitleFrame");

	QHBoxLayout * titleFrameLayout = new QHBoxLayout;

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
		QLabel * label = new QLabel();
		label->setObjectName("blogLineTitle");
		frameLayout->addWidget(label);
		m_blogTitleList << label;

		label = new QLabel();
		label->setObjectName("blogLineText");
		frameLayout->addWidget(label);
		m_blogTextList << label;
		connect(label, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));

        label = new QLabel();
        label->setObjectName("blogArtikelSpacer");
        frameLayout->addWidget(label);
        m_blogArtikelSpacer << label;
	}


 /*
    label = new QLabel();
    label->setObjectName("blogLogo");
    frameLayout->addWidget(label);
    m_blogLogo << label;
*/
    frameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));



	foreach (QLabel * label, m_blogTextList) label->setVisible(false);
	foreach (QLabel * label, m_blogTitleList) label->setVisible(false);
    foreach (QLabel * label, m_blogArtikelSpacer) label->setVisible(true);



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

	foreach (QLabel * label, m_blogTextList) label->setVisible(false);
	foreach (QLabel * label, m_blogTitleList) label->setVisible(false);

    QNetworkAccessManager * manager = networkReply->manager();
	int responseCode = networkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	if (responseCode == 200) {
        QString data(networkReply->readAll());
        //DebugDialog::debug("response data " + data);
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

void WelcomeView::readBlog(const QDomDocument & doc) {
	QDomNodeList nodeList = doc.elementsByTagName("a");
	int ix = 0;
	for (int i = 0; i < nodeList.count(); i++) {
		QDomElement element = nodeList.at(i).toElement();
		QString title = element.attribute("title");
		QString href = element.attribute("href");
		if (title.isEmpty() || href.isEmpty()) continue;

		m_blogTitleList[ix]->setText(title);
        m_blogTextList[ix]->setText(QString("<a href='%1' style='text-decoration:none; color:#666;'>%2</a>").arg(href).arg(tr("read more >>")));
		m_blogTitleList[ix]->setVisible(true);
		m_blogTextList[ix]->setVisible(true);
		if (++ix >= m_blogTextList.count()) {
			break;
		}

	}

		/*

		<ul>
<li>
                <a href="http://blog.fritzing.org/2013/11/19/minimetalmaker/" title="MiniMetalMaker">MiniMetalMaker</a>
                <a href="http://blog.fritzing.org/2013/11/19/minimetalmaker/"><small>Nov. 19, 2013</small></a> 
            </li> <li>
                <a href="http://blog.fritzing.org/2013/11/19/the-little-black-midi/" title="The Little Black Midi">The Little Black Midi</a>
                <a href="http://blog.fritzing.org/2013/11/19/the-little-black-midi/"><small>Nov. 19, 2013</small></a> 
            </li> <li>
                <a href="http://blog.fritzing.org/2013/11/15/light-up-your-flat-with-charles-planetary-gear-system/" title="Light up your flat with Charles&#039; planetary gear system">Light up your flat with Charles' planetary gear system</a>
                <a href="http://blog.fritzing.org/2013/11/15/light-up-your-flat-with-charles-planetary-gear-system/"><small>Nov. 15, 2013</small></a> 
            </li> </ul>
<a href="http://blog.fritzing.org">More posts&hellip;</a>"

		*/

}

QWidget * WelcomeView::initTip() {
	QFrame * tipFrame = new QFrame();
	tipFrame->setObjectName("tipFrame");
	QVBoxLayout * tipLayout = new QVBoxLayout();

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

/*

	QTextEdit * textEdit = new QTextEdit();

	QString breadboardHelpText = tr(
                "<br/>"
        "The <b>Breadboard View</b> is meant to look like a <i>real-life</i> breadboard prototype."
	"<br/><br/>"
        "Begin by dragging a part from the Parts Bin, which is over at the top right. "
        "Then pull in more parts, connecting them by clicking on the connectors and dragging wires. "
        "The process is similar to how you would arrange things in the physical world. "
	"<br/><br/>"
        "After you're finished creating your sketch in the breadboard view, try the other views. "
        "You can switch by clicking the other views in either the View Switcher or the Navigator on the lower right. "
        "Because different views have different purposes, parts will look different in the other views.");

	QString schematicHelpText = tr(
        "Welcome to the <b>Schematic View</b>"
	"<br/><br/>"
        "This is a more abstract way to look at components and connections than the Breadboard View. "
        "You have the same elements as you have on your breadboard, "
        "they just look different. This representation is closer to the traditional diagrams used by engineers."
        "<br/><br/>"
        "You can press &lt;Shift&gt;-click with the mouse to create bend points and tidy up your connections. "
        "The Schematic View can help you check that you have made the right connections between components. "
        "You can also print out your schematic for documentation.");

	QString pcbHelpText = tr(
        "The <b>PCB View</b> is where you design how the components will appear on a physical PCB (Printed Circuit Board)."
	"<br/><br/>"
        "PCBs can be made at home or in a small lab using DIY etching processes. "
        "They also can be sent to professional PCB manufacturing services for more precise fabrication. "
        "<br/>"
		"<table><tr><td>"
		"The first thing you will need is a board to place your parts on. "
		"There should already be one beneath this widget, but if not, "
		"drag in the board icon from the parts bin (image at right). "
		"</td><td>"
		"<img src=\":resources/parts/svg/core/icon/rectangle_pcb.svg\" />"
		"</td></tr></table>"
        "<br/><br/>"
	    "To lay out your PCB, rearrange all the components so they fit nicely on the board. "
        "Then try to shift them around to minimize the length and confusion of connections. "
        "You can also resize rectangular boards. "
		"<br/>"
		"<table><tr><td>"
        "Once the parts are sorted out, you connect them with copper traces. "
		"You can drag out a trace from individual connections or use "
        "the autorouter to generate them. "
		"The Autoroute button is at the bottom of the window (image at right)."
		"</td><td>"
		"<img src=\":resources/images/icons/toolbarAutorouteEnabled_icon.png\" />"
		"</td></tr></table>");


    textEdit->setText(breadboardHelpText + "<br /><br />" + schematicHelpText + "<br /><br />" + pcbHelpText);


	mainLayout->addWidget(textEdit);


	*/
