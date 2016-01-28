/*********************************************************************

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
#include <QBuffer>
#include <QScrollArea>
#include <QStyleOption>
#include <QStyle>
#include <QApplication>

////////////////////////////////////////////////////////////

static const int TitleRole = Qt::UserRole;
static const int IntroRole = Qt::UserRole + 1;
static const int DateRole = Qt::UserRole + 2;
static const int AuthorRole = Qt::UserRole + 3;
static const int IconRole = Qt::UserRole + 4;
static const int RefRole = Qt::UserRole + 5;
static const int ImageSpace = 65;
static const int TopSpace = 5;

QString WelcomeView::m_activeHeaderLabelColor = "#333";
QString WelcomeView::m_inactiveHeaderLabelColor = "#b1b1b1";

///////////////////////////////////////////////////////////////////////////////

void zeroMargin(QLayout * layout) {
    layout->setMargin(0);
    layout->setSpacing(0);
}

QString makeUrlText(const QString & url, const QString & urlText, const QString & color) {
    return QString("<a href='%1' style='font-family:Droid Sans; text-decoration:none; font-weight:bold; color:%3;'>%2</a>").arg(url).arg(urlText).arg(color);
}

QString hackColor(QString oldText, const QString & color) {
    QRegExp colorFinder("color:(#[^;]*);");
    int ix = oldText.indexOf(colorFinder);
    if (ix >= 0) {
        oldText.replace(colorFinder.cap(1), color);
    }
    return oldText;
}

int pixelSize(const QString & sizeString) {
    // assume all sizes are of the form Npx otherwise return -1
    QString temp = sizeString;
    temp.remove(" ");
    if (temp.contains("px")) {
        temp.remove("px");
        bool ok;
        int ps = temp.toInt(&ok);
        if (ok) return ps;
    }

    return -1;
}


QString cleanData(const QString & data) {
    static QRegExp ListItemMatcher("<li>.*</li>");
    ListItemMatcher.setMinimal(true);           // equivalent of lazy matcher

    QDomDocument doc;
    QStringList listItems;
    int pos = 0;
    while (pos < data.count()) {
        int ix = data.indexOf(ListItemMatcher, pos);
        if (ix < 0) break;

        QString listItem = ListItemMatcher.cap(0);
        //DebugDialog::debug("ListItem " + listItem);
        if (doc.setContent(listItem)) {
            listItems << listItem;
        }
        pos += listItem.count();
    }
    return listItems.join("");
}

////////////////////////////////////////////////////////////////////////////////

BlogListWidget::BlogListWidget(QWidget * parent) : QListWidget(parent)
{
    connect(this, SIGNAL(itemEntered(QListWidgetItem *)), this, SLOT(itemEnteredSlot(QListWidgetItem *)));
}

BlogListWidget::~BlogListWidget()
{
}

QStringList & BlogListWidget::imageRequestList() {
    return m_imageRequestList;
}

/* blogEntry Title text properties color, fontfamily, fontsize*/

QColor BlogListWidget::titleTextColor() const {
    return m_titleTextColor;
}

void BlogListWidget::setTitleTextColor(QColor color) {
    m_titleTextColor = color;
}

QString BlogListWidget::titleTextFontFamily() const {
    return m_titleTextFontFamily;
}

void BlogListWidget::setTitleTextFontFamily(QString family) {
    m_titleTextFontFamily = family;
}

QString BlogListWidget::titleTextFontSize() const {
    return m_titleTextFontSize;
}

void BlogListWidget::setTitleTextFontSize(QString size) {
    m_titleTextFontSize = size;
}

QString BlogListWidget::titleTextExtraLeading() const {
    return m_titleTextExtraLeading;
}

void BlogListWidget::setTitleTextExtraLeading(QString leading) {
    m_titleTextExtraLeading = leading;
}

/* blogEntry intro text properties color, fontfamily, fontsize*/
QColor BlogListWidget::introTextColor() const {
    return m_introTextColor;
}

void BlogListWidget::setIntroTextColor(QColor color) {
    m_introTextColor = color;
}

QString BlogListWidget::introTextFontFamily() const {
    return m_introTextFontFamily;
}

void BlogListWidget::setIntroTextFontFamily(QString family) {
    m_introTextFontFamily = family;
}

QString BlogListWidget::introTextFontSize() const {
    return m_introTextFontSize;
}

void BlogListWidget::setIntroTextFontSize(QString size) {
    m_introTextFontSize = size;
}

QString BlogListWidget::introTextExtraLeading() const {
    return m_introTextExtraLeading;
}

void BlogListWidget::setIntroTextExtraLeading(QString leading) {
    m_introTextExtraLeading = leading;
}

/* blogEntry Date text properties color, fontfamily, fontsize*/

QColor BlogListWidget::dateTextColor() const {
    return m_dateTextColor;
}

void BlogListWidget::setDateTextColor(QColor color) {
    m_dateTextColor = color;
}

QString BlogListWidget::dateTextFontFamily() const {
    return m_dateTextFontFamily;
}

void BlogListWidget::setDateTextFontFamily(QString family) {
    m_dateTextFontFamily = family;
}

QString BlogListWidget::dateTextFontSize() const {
    return m_dateTextFontSize;
}

void BlogListWidget::setDateTextFontSize(QString size) {
    m_dateTextFontSize = size;
}

void BlogListWidget::itemEnteredSlot(QListWidgetItem * item) {
    QString url = item->data(RefRole).toString();
    bool arrow = (url.isEmpty()) || (url == "nop");

	setCursor(arrow ? Qt::ArrowCursor : Qt::PointingHandCursor);
}

////////////////////////////////////////////////////////////////////////////////
 
BlogListDelegate::BlogListDelegate(QObject *parent) : QAbstractItemDelegate(parent)
{
}
 
BlogListDelegate::~BlogListDelegate()
{
}

void BlogListDelegate::paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    BlogListWidget * listWidget = qobject_cast<BlogListWidget *>(this->parent());
    if (listWidget == NULL) return;

    QStyle * style = listWidget->style();
    if (style == NULL) return;

    painter->save();

    QFont itemFont(painter->font());

    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, listWidget);

    QPixmap pixmap = qvariant_cast<QPixmap>(index.data(IconRole));
	QString title = index.data(TitleRole).toString();
	QString date = index.data(DateRole).toString();
	QString author = index.data(AuthorRole).toString();
	QString intro = index.data(IntroRole).toString();

  //  QRect rect;
	int imageSpace = ImageSpace + 10;

    // TITLE
    painter->setPen(listWidget->titleTextColor());
    QFont titleFont(listWidget->titleTextFontFamily());
    titleFont.setPixelSize(pixelSize(listWidget->titleTextFontSize()));
    painter->setFont(titleFont);
    QRect rect = option.rect.adjusted(imageSpace, TopSpace, 0, 0);
    style->drawItemText(painter, rect, Qt::AlignLeft, option.palette, true, title);
    QFontMetrics titleFontMetrics(titleFont);

    // INTRO
    painter->setPen(listWidget->introTextColor());
    QFont introFont(listWidget->introTextFontFamily());
    introFont.setPixelSize(pixelSize(listWidget->introTextFontSize()));
    painter->setFont(introFont);
    rect = option.rect.adjusted(imageSpace, TopSpace + titleFontMetrics.lineSpacing() + pixelSize(listWidget->titleTextExtraLeading()) , 0, 0);
    style->drawItemText(painter, rect, Qt::AlignLeft, option.palette, true, intro);
    QFontMetrics introFontMetrics(introFont);

    // DATE
    painter->setPen(listWidget->dateTextColor());
    QFont font(listWidget->dateTextFontFamily());
    font.setPixelSize(pixelSize(listWidget->dateTextFontSize()));
    painter->setFont(font);
    rect = option.rect.adjusted(imageSpace, TopSpace + titleFontMetrics.lineSpacing() + introFontMetrics.lineSpacing() + pixelSize(listWidget->introTextExtraLeading()), 0, 0);
    style->drawItemText(painter, rect, Qt::AlignLeft, option.palette, true, date);
    QFontMetrics dateTextFontMetrics(font);

    // AUTHOR
    QRect textRect = style->itemTextRect(dateTextFontMetrics, option.rect, Qt::AlignLeft, true, date);
    rect = option.rect.adjusted(imageSpace + textRect.width() + 7, TopSpace + titleFontMetrics.lineSpacing() + introFontMetrics.lineSpacing() + pixelSize(listWidget->introTextExtraLeading()), 0, 0);
    style->drawItemText(painter, rect, Qt::AlignLeft, option.palette, true, author);

    if (!pixmap.isNull()) {
		//ic.paint(painter, option.rect, Qt::AlignVCenter|Qt::AlignLeft);
        style->drawItemPixmap(painter, option.rect.adjusted(0, TopSpace, 0, -TopSpace), Qt::AlignLeft, pixmap);
	}

    painter->restore();
}
 
QSize BlogListDelegate::sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    return QSize(100, ImageSpace); // very dumb value
}
 
//////////////////////////////////////

WelcomeView::WelcomeView(QWidget * parent) : QFrame(parent) 
{
    this->setObjectName("welcomeView");
        
    m_tip = NULL;
    setAcceptDrops(true);
    initLayout();

	connect(this, SIGNAL(newSketch()), this->window(), SLOT(createNewSketch()));
	connect(this, SIGNAL(openSketch()), this->window(), SLOT(mainLoad()));
	connect(this, SIGNAL(recentSketch(const QString &, const QString &)), this->window(), SLOT(openRecentOrExampleFile(const QString &, const QString &)));

    // TODO: blog network calls should only happen once, not for each window?
    QNetworkAccessManager * manager = new QNetworkAccessManager(this);
	connect(manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(gotBlogSnippet(QNetworkReply *)));
	manager->get(QNetworkRequest(QUrl("http://blog.fritzing.org/recent-posts-app/")));

    manager = new QNetworkAccessManager(this);
	connect(manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(gotBlogSnippet(QNetworkReply *)));
	manager->get(QNetworkRequest(QUrl("http://fritzing.org/projects/snippet/")));

	TipsAndTricks::initTipSets();
    nextTip();
}

WelcomeView::~WelcomeView() {
}

void WelcomeView::initLayout()
{
	QGridLayout * mainLayout = new QGridLayout();

   //mainLayout->setSpacing (0);
    //mainLayout->setContentsMargins (0, 0, 0, 0);
    mainLayout->setSizeConstraint (QLayout::SetMaximumSize);

	QWidget * recent = initRecent();
	mainLayout->addWidget(recent, 0, 0);

	QWidget * widget = initBlog();
	mainLayout->addWidget(widget, 0, 1);

	widget = initShop();
	mainLayout->addWidget(widget, 1, 1);

	widget = initTip();
	mainLayout->addWidget(widget, 1, 0);


	this->setLayout(mainLayout);
}


QWidget * WelcomeView::initRecent() {
	QFrame * frame = new QFrame;
    frame->setObjectName("recentFrame");
	QVBoxLayout * frameLayout = new QVBoxLayout;
    zeroMargin(frameLayout);

    QFrame * titleFrame = new QFrame;
    titleFrame-> setObjectName("recentTitleFrame");
    QHBoxLayout * titleFrameLayout = new QHBoxLayout;
    zeroMargin(titleFrameLayout);
    QLabel * label = new QLabel(tr("Recent Sketches"));

    label->setObjectName("recentTitle");
    titleFrameLayout->addWidget(label);
    titleFrame ->setLayout(titleFrameLayout);

    frameLayout->addWidget(titleFrame);

    m_recentListWidget = new QListWidget();
    m_recentListWidget->setObjectName("recentList");
    m_recentListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(m_recentListWidget, SIGNAL(itemClicked (QListWidgetItem *)), this, SLOT(recentItemClicked(QListWidgetItem *)));


    frameLayout->addWidget(m_recentListWidget);

    QStringList names;
    names << "recentSpace" << "recentNewSketch" << "recentOpenSketch";

	foreach (QString name, names) {
		QWidget * widget = NULL;
        QLayout * whichLayout = frameLayout;
        QLabel * icon = NULL;
        QLabel * text = NULL;
        if (name == "recentSpace") {
            widget = new QLabel();
		}
        else if (name == "recentTitleSpace") {
                widget = new QLabel();
        }
        else if (name == "recentNewSketch") {
			widget = makeRecentItem(name, 
                QString("<a href='new' style='text-decoration:none; color:#666; margin-right:5px;'><img src=':/resources/images/icons/WS-new-icon.png' /></a>"),
                QString("<a href='new' style='text-decoration:none; color:#666;'>%1</a>").arg(tr("New Sketch")),
                icon,
                text);

		}
		else if (name == "recentOpenSketch") {
			widget = makeRecentItem(name, 
                QString("<a href='open' style='text-decoration:none; color:#666; margin-right:5px;'><img src=':/resources/images/icons/WS-open-icon.png' /></a>"),
                QString("<a href='open' style='text-decoration:none; color:#666;'>%1</a>").arg(tr("Open Sketch")),
                icon,
                text);
		}

        if (widget) {
		    widget->setObjectName(name);
            whichLayout->addWidget(widget);
        }
	}

  //  frameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));

	frame->setLayout(frameLayout);
	return frame;
}

QWidget * WelcomeView::makeRecentItem(const QString & objectName, const QString & iconText, const QString & textText, QLabel * & icon, QLabel * & text) {
    QFrame * rFrame = new QFrame;
    QHBoxLayout * rFrameLayout = new QHBoxLayout;

    zeroMargin(rFrameLayout);

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

QWidget * WelcomeView::initShop() {

    QFrame * frame = new QFrame();
    frame->setObjectName("shopFrame");
    QVBoxLayout * frameLayout = new QVBoxLayout;
    zeroMargin(frameLayout);

    QWidget * headerFrame = createHeaderFrame( tr("Shop"), "Shop", tr("Fab"), "Fab", m_inactiveHeaderLabelColor,  m_activeHeaderLabelColor, m_shopLabel, m_fabLabel);
    frameLayout->addWidget(headerFrame);

    m_shopUberFrame = createShopContentFrame(":/resources/images/welcome_kit.png",
                                                tr("Fritzing CreatorKit"), 
                                                tr("The Fritzing Creator Kit is out of Stock. For Updates please visit the fritzing.blog"),
                                                "http://creatorkit.fritzing.org/",
                                                tr(""),
                                                tr(""),
                                                ":/resources/images/icons/WS-shopLogo.png",
                                                "#f5a400"
                                                );
    frameLayout->addWidget(m_shopUberFrame);

    m_fabUberFrame = createShopContentFrame(":/resources/images/pcbs_2013.png", 
                                                tr("Fritzing Fab"),
                                                tr("Fritzing Fab is an easy and affordable service for producing professional PCBs from your Fritzing sketches."),
                                                "http://fab.fritzing.org/",
                                                tr("produce your first pcb now >>"),
                                                tr("Order your PCB now."),
                                                ":/resources/images/icons/WS-fabLogo.png",
                                                "#5f4d4a"
                                                );
    frameLayout->addWidget(m_fabUberFrame);

    frame->setLayout(frameLayout);

    clickBlog("fab");

    return frame;
}

QWidget * WelcomeView::createShopContentFrame(const QString & imagePath, const QString & headline, const QString & description, 
                                              const QString & url, const QString & urlText, const QString & urlText2, const QString & logoPath, const QString & footerLabelColor )
{
    QFrame * uberFrame = new QFrame();
    uberFrame->setObjectName("shopUberFrame");
    QVBoxLayout * shopUberFrameLayout = new QVBoxLayout;
    zeroMargin(shopUberFrameLayout);

    QFrame* shopContentFrame = new QFrame();
    shopContentFrame->setObjectName("shopContentFrame");

    QHBoxLayout * contentFrameLayout = new QHBoxLayout;
    zeroMargin(contentFrameLayout);

    QLabel * label = new QLabel(QString("<img src='%1' />").arg(imagePath));
    label->setObjectName("shopContentImage");
    contentFrameLayout->addWidget(label);

    QFrame * contentTextFrame = new QFrame();
    contentTextFrame->setObjectName("shopContentTextFrame");

    QVBoxLayout * contentTextFrameLayout = new QVBoxLayout;
    zeroMargin(contentTextFrameLayout);

    contentTextFrameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding));

    label = new QLabel(headline);
    label->setObjectName("shopContentTextHeadline");
    //label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    contentTextFrameLayout->addWidget(label);

    label = new QLabel(description);
    label->setObjectName("shopContentTextDescription");
    //label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    contentTextFrameLayout->addWidget(label);

    label = new QLabel(QString("<a href='%1'  style='text-decoration:none; color:#802742;'>%2</a>").arg(url).arg(urlText));
    label->setObjectName("shopContentTextCaption");
    contentTextFrameLayout->addWidget(label);
    //label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(label, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));

    contentTextFrameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding));

    contentTextFrame->setLayout(contentTextFrameLayout);
    contentFrameLayout->addWidget(contentTextFrame);

    shopContentFrame->setLayout(contentFrameLayout);

    shopUberFrameLayout->addWidget(shopContentFrame);
    shopUberFrameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));

    QFrame * shopFooterFrame = new QFrame();
    shopFooterFrame->setObjectName("shopFooterFrame");

    QHBoxLayout * footerFrameLayout = new QHBoxLayout;
    zeroMargin(footerFrameLayout);
    footerFrameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));

    QLabel * footerLabel = new QLabel(QString("<a href='%1'  style='text-decoration:none; color:%3;'>%2</a>").arg(url).arg(urlText2).arg(footerLabelColor));
    footerLabel->setObjectName("shopLogoText");
    footerFrameLayout->addWidget(footerLabel);
    connect(footerLabel, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));

    QLabel * footerLogoLabel = new QLabel(tr("<a href='%1'><img src='%2'/></a>").arg(url).arg(logoPath));
    footerLogoLabel->setObjectName("shopLogo");
    footerFrameLayout->addWidget(footerLogoLabel);
    connect(footerLogoLabel, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));

    shopFooterFrame->setLayout(footerFrameLayout);

    shopUberFrameLayout->addWidget(shopFooterFrame);
    uberFrame->setLayout(shopUberFrameLayout);
    return uberFrame;
}

QWidget * WelcomeView::initBlog() {

    QFrame * frame = new QFrame();
	frame->setObjectName("blogFrame");
	QVBoxLayout * frameLayout = new QVBoxLayout;
    zeroMargin(frameLayout);

    QWidget * headerFrame = createHeaderFrame(tr("Projects"), "Projects", tr("Blog"), "Blog", m_inactiveHeaderLabelColor,  m_activeHeaderLabelColor, m_projectsLabel, m_blogLabel);
    frameLayout->addWidget(headerFrame);

    m_blogListWidget = createBlogContentFrame("http://blog.fritzing.org", tr("Fritzing News."), ":/resources/images/icons/WS-blogLogo.png", "#802742");
    m_blogUberFrame = m_blogListWidget;
    while (m_blogUberFrame->parentWidget()) m_blogUberFrame = m_blogUberFrame->parentWidget();
    frameLayout->addWidget(m_blogUberFrame);

    m_projectListWidget = createBlogContentFrame("http://fritzing.org/projects/", tr("Fritzing Projects."), ":/resources/images/icons/WS-galleryLogo.png", "#00a55b");
    m_projectsUberFrame = m_projectListWidget;
    while (m_projectsUberFrame->parentWidget()) m_projectsUberFrame = m_projectsUberFrame->parentWidget();
    frameLayout->addWidget(m_projectsUberFrame);

    frame->setLayout(frameLayout);

    //DebugDialog::debug("first click blog");

    clickBlog("Blog");

    return frame;
}

QFrame * WelcomeView::createHeaderFrame (const QString & url1, const QString & urlText1, const QString & url2, const QString & urlText2, const QString & inactiveColor, const QString & activeColor,
                                         QLabel * & label1, QLabel * & label2){
    QFrame * titleFrame = new QFrame();
    titleFrame->setObjectName("wsSwitchableFrameHeader");

    QHBoxLayout * titleFrameLayout = new QHBoxLayout;
    zeroMargin(titleFrameLayout);

    label1 = new QLabel(makeUrlText(url1, urlText1, inactiveColor));
    label1->setObjectName("headerTitle1");
    titleFrameLayout->addWidget(label1);
    connect(label1, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));

    QLabel * titleSpace = new QLabel("|");
    titleSpace->setObjectName("headerTitleSpace");
    titleFrameLayout->addWidget(titleSpace);

    label2 = new QLabel(makeUrlText(url2, urlText2, activeColor));
    label2->setObjectName("headerTitle2");
    titleFrameLayout->addWidget(label2);
    connect(label2, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));

    titleFrameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));
    titleFrame->setLayout(titleFrameLayout);

    return titleFrame;
}

BlogListWidget * WelcomeView::createBlogContentFrame(const QString & url, const QString & urlText, const QString & logoPath, const QString & footerLabelColor) {
    QFrame * uberFrame = new QFrame;
    QVBoxLayout * uberFrameLayout = new QVBoxLayout;
    zeroMargin(uberFrameLayout);

    BlogListWidget * listWidget = new BlogListWidget;
    listWidget->setObjectName("blogList");
    listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listWidget->setItemDelegate(new BlogListDelegate(listWidget));
    connect(listWidget, SIGNAL(itemClicked (QListWidgetItem *)), this, SLOT(blogItemClicked(QListWidgetItem *)));

    uberFrameLayout->addWidget(listWidget);

    QFrame * footerFrame = new QFrame();
    footerFrame->setObjectName("blogFooterFrame");

    QHBoxLayout * footerFrameLayout = new QHBoxLayout;
    zeroMargin(footerFrameLayout);
    footerFrameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));

    QLabel * footerLabel = new QLabel(QString("<a href='%1'  style='font-family:Droid Sans; text-decoration:none; color:%3;'>%2</a>").arg(url).arg(urlText).arg(footerLabelColor));
    footerLabel->setObjectName("blogLogoText");
    footerFrameLayout->addWidget(footerLabel);
    connect(footerLabel, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));

    footerLabel = new QLabel(tr("<a href='%1'><img src='%2' /></a>").arg(url).arg(logoPath));
    footerLabel->setObjectName("blogLogo");
    footerFrameLayout->addWidget(footerLabel);
    connect(footerLabel, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));

    footerFrame->setLayout(footerFrameLayout);

    uberFrameLayout->addWidget(footerFrame);
    uberFrame->setLayout(uberFrameLayout);

    return listWidget;
}

void WelcomeView::showEvent(QShowEvent * event) {
	QFrame::showEvent(event);
	updateRecent();
}

void WelcomeView::updateRecent() {
	if (m_recentListWidget == NULL) return;

	QSettings settings;
	QStringList files = settings.value("recentFileList").toStringList();
    m_recentListWidget->clear();

    bool gotOne = false;

    QIcon icon(":/resources/images/icons/WS-fzz-icon.png");
	for (int i = 0; i < files.size(); ++i) {
		QFileInfo finfo(files[i]);
		if (!finfo.exists()) continue;

        gotOne = true;
        QListWidgetItem * item = new QListWidgetItem(icon, finfo.fileName());
        item->setData(Qt::UserRole, finfo.absoluteFilePath());
        item->setToolTip(finfo.absoluteFilePath());
        m_recentListWidget->addItem(item);
	}

    if (!gotOne) {
        // put in a placeholder if there are no recent files
        QListWidgetItem * item = new QListWidgetItem(icon, tr("No recent sketches found"));
        item->setData(Qt::UserRole, "");
        m_recentListWidget->addItem(item);
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
}

void WelcomeView::gotBlogSnippet(QNetworkReply * networkReply) {
    bool blog = networkReply->url().toString().contains("blog");
    QString prefix = networkReply->url().scheme() + "://" + networkReply->url().authority();
    QNetworkAccessManager * manager = networkReply->manager();
	int responseCode = networkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    bool goodBlog = false;
    QDomDocument doc;
	QString errorStr;
	int errorLine;
	int errorColumn;	
    if (responseCode == 200) {
        QString data(networkReply->readAll());
        //DebugDialog::debug("response data " + data);
		data = "<thing>" + cleanData(data) + "</thing>";		// make it one tree for xml parsing
	    if (doc.setContent(data, &errorStr, &errorLine, &errorColumn)) {
		    readBlog(doc, true, blog, prefix);
            goodBlog = true;
        }
	}

    if (!goodBlog) {
        QString message = (blog) ? tr("Unable to reach blog.fritzing.org") : tr("Unable to reach friting.org/projects") ;
        QString placeHolder = QString("<li><a class='title' href='nop' title='%1'></a></li>").arg(message);
        if (doc.setContent(placeHolder, &errorStr, &errorLine, &errorColumn)) {
		    readBlog(doc, true, blog, "");
        }  
    }

    manager->deleteLater();
    networkReply->deleteLater();
}

void WelcomeView::clickBlog(const QString & url) {
    if (url.toLower() == "fab") {
        m_shopUberFrame->setVisible(false);
        m_fabUberFrame->setVisible(true);
        m_fabLabel->setText(hackColor(m_fabLabel->text(), m_activeHeaderLabelColor));
        m_shopLabel->setText(hackColor(m_shopLabel->text(), m_inactiveHeaderLabelColor));

        return;
    }

    if (url.toLower() == "shop") {
        m_shopUberFrame->setVisible(true);
        m_fabUberFrame->setVisible(false);
        m_fabLabel->setText(hackColor(m_fabLabel->text(), m_inactiveHeaderLabelColor));
        m_shopLabel->setText(hackColor(m_shopLabel->text(), m_activeHeaderLabelColor));
        return;
    }

    if (url.toLower() == "nexttip") {
        nextTip();
        return;
    }

    if (url.toLower() == "projects") {
        m_projectsUberFrame->setVisible(true);
        m_blogUberFrame->setVisible(false);
        m_projectsLabel->setText(hackColor(m_projectsLabel->text(), m_activeHeaderLabelColor));
        m_blogLabel->setText(hackColor(m_blogLabel->text(), m_inactiveHeaderLabelColor));
        return;
    }
 
    if (url.toLower() == "blog") {
        m_projectsUberFrame->setVisible(false);
        m_blogUberFrame->setVisible(true);
        m_projectsLabel->setText(hackColor(m_projectsLabel->text(), m_inactiveHeaderLabelColor));
        m_blogLabel->setText(hackColor(m_blogLabel->text(), m_activeHeaderLabelColor));
        return;
    }
 
	QDesktopServices::openUrl(url);
}

/*

// sample output from http://blog.fritzing.org/recent-posts-app/
<ul>
    <li>
        <img src="http://blog.fritzing.org/wp-content/uploads/charles1.jpg"/>
        <a class="title" href="http://blog.fritzing.org/2013/11/15/light-up-your-flat-with-charles-planetary-gear-system/" title="Light up your flat with Charles&#039; planetary gear system">Light up your flat with Charles' planetary gear system</a>
        <p class="date">Nov. 15, 2013</p>
        <p class="author">Nushin Isabelle</p>
        <p class="intro">Today, we got a visitor in the Fritzing Lab: Our neighbour, Charles Oleg, came by to show us his new...</p>
    </li> 
</ul>

// sample output from http://fritzing.org/projects/snippet/
<ul>
    <li>
        <a class="image" href="/projects/sensor-infrarrojos-para-nuestro-robot">
            <img src="" alt="Sensor Infrarrojos para nuestro Robot" />
        </a>
        <a class="title" href="/projects/sensor-infrarrojos-para-nuestro-robot" title="Sensor Infrarrojos para nuestro Robot">Sensor Infrarrojos para nuestro Robot</a>
        <p class="date">1 week ago</p>
        <p class="author">robotarduedu</p>
        <p class="difficulty">for kids</p>
        <p class="tags">in sensror infrarrojo, led infrarrojo, i.r., i.r., </p>
    </li>
</ul>
*/


void WelcomeView::readBlog(const QDomDocument & doc, bool doEmit, bool blog, const QString & prefix) {
    BlogListWidget * listWidget = (blog) ? m_blogListWidget : m_projectListWidget;
    listWidget->clear();
    listWidget->imageRequestList().clear();

	QDomNodeList nodeList = doc.elementsByTagName("li");
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
                    QString title = child.attribute("title");
                    QString href = child.attribute("href");
                    if (!blog) {
                        href.insert(0, prefix);
                    }
                    stuff.insert("title", title);
                    stuff.insert("href", href);
                }
                else if (clss == "image") {
                    QDomElement img = child.firstChildElement("img");
                    QString src = img.attribute("src");
                    if (!src.isEmpty()) src.insert(0, prefix);
                    stuff.insert("img", src);
                }
                else {
                    stuff.insert(clss, child.text());
                }
            }
            child = child.nextSiblingElement();
        }
        if (stuff.value("title", "").isEmpty()) continue;
        if (stuff.value("href", "").isEmpty()) continue;

        QListWidgetItem * item = new QListWidgetItem();
        item->setData(TitleRole, stuff.value("title"));
        item->setData(RefRole, stuff.value("href"));      
        QString text = stuff.value("intro", "");
        text.replace("\r", " ");
        text.replace("\n", " ");
        text.replace("\t", " ");
        item->setData(IntroRole, text);
        listWidget->addItem(item);

        listWidget->imageRequestList() << stuff.value("img", "");

        if (!stuff.value("date", "").isEmpty()) {
            item->setData(DateRole, stuff.value("date"));
        }
        if (!stuff.value("author", "").isEmpty()) {
            item->setData(AuthorRole, stuff.value("author"));
        }
	}

    if (listWidget->count() > 0) {
        listWidget->itemEnteredSlot(listWidget->item(0));
    }

    if (doEmit) {
        getNextBlogImage(0, blog);
        foreach (QWidget *widget, QApplication::topLevelWidgets()) {
            WelcomeView * other = widget->findChild<WelcomeView *>();
            if (other == NULL) continue;
            if (other == this) continue;

            other->readBlog(doc, false, blog, prefix);
        }     
    }
}

void WelcomeView::getNextBlogImage(int ix, bool blog) {
    BlogListWidget * listWidget = (blog) ? m_blogListWidget : m_projectListWidget;
    for (int i = ix; i < listWidget->imageRequestList().count(); i++) {
        QString image = listWidget->imageRequestList().at(i);
        if (image.isEmpty()) continue;

        QNetworkAccessManager * manager = new QNetworkAccessManager(this);
        manager->setProperty("index", i);
        manager->setProperty("blog", blog);
	    connect(manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(gotBlogImage(QNetworkReply *)));
	    manager->get(QNetworkRequest(QUrl(image)));
    }
}

void WelcomeView::gotBlogImage(QNetworkReply * networkReply) {
    QNetworkAccessManager * manager = networkReply->manager();
    if (manager == NULL) return;

    int index = manager->property("index").toInt();
    bool blog = manager->property("blog").toBool();

	int responseCode = networkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	if (responseCode == 200) {
        QByteArray data(networkReply->readAll());
        QPixmap pixmap;
        if (pixmap.loadFromData(data)) {
            QPixmap scaled = pixmap.scaled(QSize(ImageSpace, ImageSpace), Qt::KeepAspectRatio);
            setBlogItemImage(scaled, index, blog);
            foreach (QWidget *widget, QApplication::topLevelWidgets()) {
                WelcomeView * other = widget->findChild<WelcomeView *>();
                if (other == NULL) continue;
                if (other == this) continue;

                other->setBlogItemImage(scaled, index, blog);
            }
        }     
	}

    manager->deleteLater();
    networkReply->deleteLater();
    getNextBlogImage(index + 1, blog);
}

QWidget * WelcomeView::initTip() {
	QFrame * tipFrame = new QFrame();
	tipFrame->setObjectName("tipFrame");
	QVBoxLayout * tipLayout = new QVBoxLayout();
    zeroMargin(tipLayout);

    QLabel * tipTitle = new QLabel(tr("Tip of the Day:"));
	tipTitle->setObjectName("tipTitle");
	tipLayout->addWidget(tipTitle);

    QScrollArea * scrollArea = new QScrollArea;
    scrollArea->setObjectName("tipScrollArea");
	scrollArea->setWidgetResizable(true);
   // scrollArea->setAlignment(Qt::AlignTop | Qt::AlignLeft);
   // scrollArea->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);
	scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	m_tip = new QLabel();
	m_tip->setObjectName("tip");
    m_tip->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    //connect(m_tip, SIGNAL(linkActivated(const QString &)), this->window(), SLOT(tipsAndTricks()));

    scrollArea->setWidget(m_tip);
	tipLayout->addWidget(scrollArea);

    tipLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Fixed));
    QFrame * footerFrame = new QFrame();
    footerFrame->setObjectName("tipFooterFrame");

        QHBoxLayout * footerFrameLayout = new QHBoxLayout;
        zeroMargin(footerFrameLayout);


        QLabel * footerLabel = new QLabel(QString("<a href='http://blog.fritzing.org'  style='font-family:Droid Sans; text-decoration:none; color:#2e94af;'>%1</a>").arg(tr("All Tips")));
        footerLabel->setObjectName("allTips");
        footerFrameLayout->addWidget(footerLabel);
        connect(footerLabel, SIGNAL(linkActivated(const QString &)), this->window(), SLOT(tipsAndTricks()));

        footerFrameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));


        footerLabel = new QLabel(QString("<a href='http://blog.fritzing.org'  style='text-decoration:none; font-family:Droid Sans; color:#2e94af;'>%1</a>").arg(tr("Next Tip")));
        footerLabel->setObjectName("nextTip");
        footerFrameLayout->addWidget(footerLabel);
        connect(footerLabel, SIGNAL(linkActivated(const QString &)), this, SLOT(nextTip()));


        footerFrame->setLayout(footerFrameLayout);

    tipLayout->addWidget(footerFrame);

	tipFrame->setLayout(tipLayout);

	return tipFrame;
}

void WelcomeView::dragEnterEvent(QDragEnterEvent *event)
{
    DebugDialog::debug("ignoring drag enter");
    event->ignore();
}

void WelcomeView::nextTip() {
    if (m_tip == NULL) return;

    m_tip->setText(QString("<a href='tip' style='text-decoration:none; color:#2e94af;'>%1</a>").arg(TipsAndTricks::randomTip()));
}

void WelcomeView::recentItemClicked(QListWidgetItem * item) {
    QString data = item->data(Qt::UserRole).toString();
    if (data.isEmpty()) return;

	emit recentSketch(data, data);
}


void WelcomeView::blogItemClicked(QListWidgetItem * item) {
    QString url = item->data(RefRole).toString();
    if (url.isEmpty()) return;
    if (url == "nop") return;

	QDesktopServices::openUrl(url);
}

void WelcomeView::setBlogItemImage(QPixmap & pixmap, int index, bool blog) {
    // TODO: this is not totally thread-safe if there are multiple sketch widgets opened within a very short time
    BlogListWidget * listWidget = (blog) ? m_blogListWidget : m_projectListWidget;
    QListWidgetItem * item = listWidget->item(index);
	if (item) {
        item->setData(IconRole, pixmap);
    }
}
