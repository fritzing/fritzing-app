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

////////////////////////////////////////////////////////////

static const int TitleRole = Qt::UserRole;
static const int IntroRole = Qt::UserRole + 1;
static const int DateRole = Qt::UserRole + 2;
static const int AuthorRole = Qt::UserRole + 3;
static const int IconRole = Qt::UserRole + 4;
static const int RefRole = Qt::UserRole + 5;
static const int ImageSpace = 60;

///////////////////////////////////////////////////////////////////////////////

void zeroMargin(QLayout * layout) {
    layout->setMargin(0);
    layout->setSpacing(0);
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
        DebugDialog::debug("ListItem " + listItem);
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
}

BlogListWidget::~BlogListWidget()
{
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

    QRect ret;
	int imageSpace = ImageSpace + 10;

    // TITLE
    painter->setPen(listWidget->titleTextColor());
    QFont titleFont(listWidget->titleTextFontFamily());
    titleFont.setPixelSize(pixelSize(listWidget->titleTextFontSize()));
 //   titleFont.setStyleStrategy(QFont::PreferAntialias);
    painter->setFont(titleFont);
    QRect rect = option.rect.adjusted(imageSpace, 2, 0, 0);
    style->drawItemText(painter, rect, Qt::AlignLeft, option.palette, true, title);
    QFontMetrics titleFontMetrics(titleFont);

    // INTRO
    painter->setPen(listWidget->introTextColor());
    QFont introFont(listWidget->introTextFontFamily());


    introFont.setPixelSize(pixelSize(listWidget->introTextFontSize()));
    painter->setFont(introFont);
    rect = option.rect.adjusted(imageSpace, titleFontMetrics.lineSpacing() + pixelSize(listWidget->titleTextExtraLeading()) , 0, 0);
    style->drawItemText(painter, rect, Qt::AlignLeft, option.palette, true, intro);
    QFontMetrics introFontMetrics(introFont);

    // DATE
    painter->setPen(listWidget->dateTextColor());
    QFont font(listWidget->dateTextFontFamily());
    font.setPixelSize(pixelSize(listWidget->dateTextFontSize()));
    painter->setFont(font);
    rect = option.rect.adjusted(imageSpace, titleFontMetrics.lineSpacing() + introFontMetrics.lineSpacing() + pixelSize(listWidget->introTextExtraLeading()), 0, 0);
    style->drawItemText(painter, rect, Qt::AlignLeft, option.palette, true, date);
    QFontMetrics dateTextFontMetrics(font);

    // AUTHOR
    painter->setFont(itemFont);
    QRect textRect = style->itemTextRect(dateTextFontMetrics, option.rect, Qt::AlignLeft, true, date);
    rect = option.rect.adjusted(imageSpace + textRect.width() + 7, titleFontMetrics.lineSpacing() + introFontMetrics.lineSpacing() + pixelSize(listWidget->introTextExtraLeading()), 0, 0);
    style->drawItemText(painter, rect, Qt::AlignLeft, option.palette, true, author);

    if (!pixmap.isNull()) {
		//ic.paint(painter, option.rect, Qt::AlignVCenter|Qt::AlignLeft);
        style->drawItemPixmap(painter, option.rect, Qt::AlignLeft, pixmap);
	}

    painter->restore();
}
 
QSize BlogListDelegate::sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    return QSize(100, 60); // very dumb value
}
 
//////////////////////////////////////

WelcomeView::WelcomeView(QWidget * parent) : QFrame(parent) 
{
    m_tip = NULL;
    setAcceptDrops(false);
    initLayout();

	connect(this, SIGNAL(newSketch()), this->window(), SLOT(createNewSketch()));
	connect(this, SIGNAL(openSketch()), this->window(), SLOT(mainLoad()));
	connect(this, SIGNAL(recentSketch(const QString &, const QString &)), this->window(), SLOT(openRecentOrExampleFile(const QString &, const QString &)));

    QNetworkAccessManager * manager = new QNetworkAccessManager(this);
	connect(manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(gotBlogSnippet(QNetworkReply *)));
	manager->get(QNetworkRequest(QUrl("http://blog.fritzing.org/recent-posts-app/")));

	TipsAndTricks::initTipSets();
    nextTip();
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
                QString("<a href='new' style='text-decoration:none; color:#666;'>%1</a>").arg(tr("New Sketch >>")),
                icon,
                text);

		}
		else if (name == "recentOpenSketch") {
			widget = makeRecentItem(name, 
                QString("<a href='open' style='text-decoration:none; color:#666; margin-right:5px;'><img src=':/resources/images/icons/WS-open-icon.png' /></a>"),
                QString("<a href='open' style='text-decoration:none; color:#666;'>%1</a>").arg(tr("Open Sketch >>")),
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

    QFrame * titleFrame = new QFrame();
    titleFrame->setObjectName("shopTitleFrame");
    QHBoxLayout * titleFrameLayout = new QHBoxLayout;
    zeroMargin(titleFrameLayout);

    QLabel * shopTitle = new QLabel(QString("<a href='fab' style='font-family:Droid Sans; text-decoration:none; display:block; font-weight:bold; color:#323232;'>%1</a>").arg(tr("FAB")));
    shopTitle->setObjectName("shopTitle");
    connect(shopTitle, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));
    titleFrameLayout->addWidget(shopTitle);

    QLabel * space = new QLabel(QString("|"));
    space->setObjectName("fabshopTitleSpace");
    titleFrameLayout->addWidget(space);

    QLabel * fabTitle = new QLabel(QString("<a href='shop' style='text-decoration:none; font-family:Droid Sans; display:block; font-weight:bold; color:#323232;'>%1</a>").arg(tr("Shop")));
    fabTitle->setObjectName("fabTitle");
    connect(fabTitle, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));
    titleFrameLayout->addWidget(fabTitle);

    titleFrameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));
    titleFrame->setLayout(titleFrameLayout);

    frameLayout->addWidget(titleFrame);

    {
        m_shopUberFrame = new QFrame();
        m_shopUberFrame->setObjectName("shopUberFrame");
        QVBoxLayout * shopUberFrameLayout = new QVBoxLayout;
        zeroMargin(shopUberFrameLayout);

        QFrame* shopContentFrame = new QFrame();
        shopContentFrame->setObjectName("shopContentFrame");

        QHBoxLayout * contentFrameLayout = new QHBoxLayout;
        zeroMargin(contentFrameLayout);

        QLabel * label = new QLabel("<img src=':/resources/images/welcome_kit.png' />");
        label->setObjectName("shopContentImage");
        contentFrameLayout->addWidget(label);


            QFrame * contentTextFrame = new QFrame();
            contentTextFrame->setObjectName("shopContentTextFrame");

            QVBoxLayout * contentTextFrameLayout = new QVBoxLayout;
            zeroMargin(contentTextFrameLayout);

            contentTextFrameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::MinimumExpanding));

            label = new QLabel(QString("Fritzing CreatorKit"));
            label->setObjectName("shopContentTextHeadline");
            label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            contentTextFrameLayout->addWidget(label);
            connect(label, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));

            label = new QLabel(QString("Get Your own Fritzing Creator Kit and become an electronical inventor. It's as easy as bicycling."));
            label->setObjectName("shopContentTextDescription");
            label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
            contentTextFrameLayout->addWidget(label);
            connect(label, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));

            label = new QLabel(QString("<a href='http://fritzing.org/creatorkit/'  style='text-decoration:none; color:#802742;'>%1</a>").arg(tr("order now >>")));
            label->setObjectName("shopContentTextCaption");
            contentTextFrameLayout->addWidget(label);
            label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            connect(label, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));

            contentTextFrameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::MinimumExpanding));

            contentTextFrame->setLayout(contentTextFrameLayout);

            contentFrameLayout->addWidget(contentTextFrame);

        shopContentFrame->setLayout(contentFrameLayout);

        shopUberFrameLayout->addWidget(shopContentFrame);
        shopUberFrameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));

        QFrame * shopFooterFrame = new QFrame();
        shopFooterFrame->setObjectName("shopFooterFrame");

            QHBoxLayout * footerFrameLayout = new QHBoxLayout;
            zeroMargin(footerFrameLayout);
            footerFrameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::MinimumExpanding));

            QLabel * footerLabel = new QLabel(QString("<a href='http://creatorkit.fritzing.org/'  style='text-decoration:none; color:#f5a400;'>%1</a>").arg(tr("Get your Creator Kit now.   ")));
            footerLabel->setObjectName("shopLogoText");
            footerFrameLayout->addWidget(footerLabel);
            connect(footerLabel, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));

            QLabel * footerLogoLabel = new QLabel(tr("<a href='http://creatorkit.fritzing.org/'><img src=':/resources/images/icons/WS-shopLogo.png'/></a>"));
            footerLogoLabel->setObjectName("shopLogo");
            footerFrameLayout->addWidget(footerLogoLabel);
            connect(footerLogoLabel, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));

            shopFooterFrame->setLayout(footerFrameLayout);

       shopUberFrameLayout->addWidget(shopFooterFrame);
       m_shopUberFrame->setLayout(shopUberFrameLayout);
       frameLayout->addWidget(m_shopUberFrame);

    }

    {
        m_fabUberFrame = new QFrame();
        m_fabUberFrame->setObjectName("fabUberFrame");
        QVBoxLayout * fabUberFrameLayout = new QVBoxLayout;
        zeroMargin(fabUberFrameLayout);

        QFrame * fabContentFrame = new QFrame();
        fabContentFrame->setObjectName("fabContentFrame");

        QHBoxLayout * contentFrameLayout = new QHBoxLayout;
        zeroMargin(contentFrameLayout);

        QLabel * label = new QLabel("<img src=':/resources/images/pcbs_2013.png' />");
        label->setObjectName("fabContentImage");
        contentFrameLayout->addWidget(label);


            QFrame * contentTextFrame = new QFrame();
            contentTextFrame->setObjectName("fabContentTextFrame");

            QVBoxLayout * contentTextFrameLayout = new QVBoxLayout;
            zeroMargin(contentTextFrameLayout);

            contentTextFrameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::MinimumExpanding));

            label = new QLabel(QString("Fritzing FAB"));
            label->setObjectName("fabContentTextHeadline");
            label->setSizePolicy(QSizePolicy::Fixed ,QSizePolicy::Fixed);
            contentTextFrameLayout->addWidget(label);
            connect(label, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));

            label = new QLabel(QString("Fritzing FAB brings your Hardware easily on your desk with your own design and high quality."));
            label->setObjectName("fabContentTextDescription");
            label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
            contentTextFrameLayout->addWidget(label);
            connect(label, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));

            label = new QLabel(QString("<a href='http://fab.fritzing.org/'  style='text-decoration:none; color:#802742;'>%1</a>").arg(tr("produce your first pcb now >>")));
            label->setObjectName("fabContentTextCaption");
            label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            contentTextFrameLayout->addWidget(label);
            connect(label, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));

            contentTextFrameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::MinimumExpanding));

            contentTextFrame->setLayout(contentTextFrameLayout);

            contentFrameLayout->addWidget(contentTextFrame);

        fabContentFrame->setLayout(contentFrameLayout);

        fabUberFrameLayout->addWidget(fabContentFrame);
        fabUberFrameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));

        QFrame * fabFooterFrame = new QFrame();
        fabFooterFrame->setObjectName("fabFooterFrame");

            QHBoxLayout * footerFrameLayout = new QHBoxLayout;
            zeroMargin(footerFrameLayout);
            footerFrameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));

            QLabel * footerLabel = new QLabel(QString("<a href='http://fab.fritzing.org/'  style='text-decoration:none; color:#5f4d4a;'>%1</a>").arg(tr("Order your PCB now.   ")));
            footerLabel->setObjectName("fabLogoText");
            footerFrameLayout->addWidget(footerLabel);
            connect(footerLabel, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));

            QLabel * footerLogoLabel = new QLabel(tr("<a href='http://fab.fritzing.org/'><img src=':/resources/images/icons/WS-fabLogo.png'/></a>"));
            footerLogoLabel->setObjectName("fabLogo");
            footerFrameLayout->addWidget(footerLogoLabel);
            connect(footerLogoLabel, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));

            fabFooterFrame->setLayout(footerFrameLayout);

        fabUberFrameLayout->addWidget(fabFooterFrame);
        m_fabUberFrame->setLayout(fabUberFrameLayout);
        frameLayout->addWidget(m_fabUberFrame);

    }



    frame->setLayout(frameLayout);

    clickBlog("shop");

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

    QLabel * label = new QLabel(QString("<a href='http://blog.fritzing.org'  style='font-family:Droid Sans; text-decoration:none; font-weight:bold; color:#323232;'>%1</a>").arg(tr("| Blog >>")));
	label->setObjectName("blogTitleGoto");
	titleFrameLayout->addWidget(label);
	connect(label, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));

	titleFrameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));

    titleFrame->setLayout(titleFrameLayout);
	frameLayout->addWidget(titleFrame);

    m_blogListWidget = new BlogListWidget;
    m_blogListWidget->setObjectName("blogList");
    m_blogListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_blogListWidget->setItemDelegate(new BlogListDelegate(m_blogListWidget));
    connect(m_blogListWidget, SIGNAL(itemClicked (QListWidgetItem *)), this, SLOT(blogItemClicked(QListWidgetItem *)));

    frameLayout->addWidget(m_blogListWidget);

    QFrame * footerFrame = new QFrame();
    footerFrame->setObjectName("blogFooterFrame");

        QHBoxLayout * footerFrameLayout = new QHBoxLayout;
        zeroMargin(footerFrameLayout);

        footerFrameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));

        QLabel * footerLabel = new QLabel(QString("<a href='http://blog.fritzing.org'  style='font-family:Droid Sans; text-decoration:none; color:#802742;'>%1</a>").arg(tr("Fritzing Project News.   ")));
        footerLabel->setObjectName("blogLogoText");
        footerFrameLayout->addWidget(footerLabel);
        connect(footerLabel, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));

        footerLabel = new QLabel(tr("<a href='http://blog.fritzing.org'><img src=':/resources/images/icons/WS-blogLogo.png' /></a>"));
        footerLabel->setObjectName("blogLogo");
        footerFrameLayout->addWidget(footerLabel);
        connect(footerLabel, SIGNAL(linkActivated(const QString &)), this, SLOT(clickBlog(const QString &)));

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
	if (m_recentListWidget == NULL) return;

	QSettings settings;
	QStringList files = settings.value("recentFileList").toStringList();
    m_recentListWidget->clear();

    QIcon icon(":/resources/images/icons/WS-fzz-icon.png");
	for (int i = 0; i < files.size(); ++i) {
		QFileInfo finfo(files[i]);
		if (!finfo.exists()) continue;

        QListWidgetItem * item = new QListWidgetItem(icon, finfo.fileName());
        item->setData(Qt::UserRole, finfo.absoluteFilePath());
        m_recentListWidget->addItem(item);
	}

    if (files.size() == 0) {
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

    QNetworkAccessManager * manager = networkReply->manager();
	int responseCode = networkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	if (responseCode == 200) {
        QString data(networkReply->readAll());
        DebugDialog::debug("response data " + data);
		data = "<thing>" + cleanData(data) + "</thing>";		// make it one tree for xml parsing
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
    if (url == "fab") {
        m_shopUberFrame->setVisible(false);
        m_fabUberFrame->setVisible(true);
        return;
    }

    if (url == "shop") {
        m_shopUberFrame->setVisible(true);
        m_fabUberFrame->setVisible(false);
        return;
    }

    if (url == "nextTip") {
        nextTip();
        return;
    }
 
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
    m_blogListWidget->clear();

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

        QListWidgetItem * item = new QListWidgetItem();
        item->setData(TitleRole, stuff.value("title"));
        item->setData(RefRole, stuff.value("href"));      
        QString text = stuff.value("intro", "");
        text.remove("\r");
        text.remove("\n");
        item->setData(IntroRole, text);
        m_blogListWidget->addItem(item);

        if (!stuff.value("img", "").isEmpty()) {
            QNetworkAccessManager * manager = new QNetworkAccessManager(this);
            manager->setProperty("index", ix);
	        connect(manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(gotBlogImage(QNetworkReply *)));
	        manager->get(QNetworkRequest(QUrl(stuff.value("img"))));
        }


        if (!stuff.value("date", "").isEmpty()) {
            item->setData(DateRole, stuff.value("date"));
        }
        if (!stuff.value("author", "").isEmpty()) {
            item->setData(AuthorRole, stuff.value("author"));
        }
	}
}

void WelcomeView::gotBlogImage(QNetworkReply * networkReply) {

    QNetworkAccessManager * manager = networkReply->manager();
	int responseCode = networkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	if (responseCode == 200) {
        QByteArray data(networkReply->readAll());
        QPixmap pixmap;
        if (pixmap.loadFromData(data)) {
            QPixmap scaled = pixmap.scaled(QSize(ImageSpace, ImageSpace), Qt::KeepAspectRatio);
            int index = manager->property("index").toInt();
            QListWidgetItem * item = m_blogListWidget->item(index);
		    if (item) {
                item->setData(IconRole, scaled);
            }
        }
	}

    manager->deleteLater();
    networkReply->deleteLater();
}

QWidget * WelcomeView::initTip() {
	QFrame * tipFrame = new QFrame();
	tipFrame->setObjectName("tipFrame");
	QVBoxLayout * tipLayout = new QVBoxLayout();
    zeroMargin(tipLayout);

	QLabel * tipTitle = new QLabel(tr("Tip:"));
	tipTitle->setObjectName("tipTitle");
	tipLayout->addWidget(tipTitle);

    QScrollArea * scrollArea = new QScrollArea;
    scrollArea->setObjectName("tipScrollArea");
	scrollArea->setWidgetResizable(true);
	scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	m_tip = new QLabel();
	m_tip->setObjectName("tip");
    //connect(m_tip, SIGNAL(linkActivated(const QString &)), this->window(), SLOT(tipsAndTricks()));

    scrollArea->setWidget(m_tip);
	tipLayout->addWidget(scrollArea);

/*	tipLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));*/
    QFrame * footerFrame = new QFrame();
    footerFrame->setObjectName("tipFooterFrame");

        QHBoxLayout * footerFrameLayout = new QHBoxLayout;
        zeroMargin(footerFrameLayout);


        QLabel * footerLabel = new QLabel(QString("<a href='http://blog.fritzing.org'  style='font-family:Droid Sans; text-decoration:none; color:#2e94af;'>%1</a>").arg(tr("All Tips >>")));
        footerLabel->setObjectName("allTips");
        footerFrameLayout->addWidget(footerLabel);
        connect(footerLabel, SIGNAL(linkActivated(const QString &)), this->window(), SLOT(tipsAndTricks()));

        footerFrameLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));


        footerLabel = new QLabel(QString("<a href='http://blog.fritzing.org'  style='text-decoration:none; font-family:Droid Sans; color:#2e94af;'>%1</a>").arg(tr("Next Tip >>")));
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

	QDesktopServices::openUrl(url);
}

