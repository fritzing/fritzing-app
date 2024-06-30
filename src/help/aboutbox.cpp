/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2023 Fritzing

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

#include <QLabel>
#include <QFont>
#include <QChar>
#include <QTime>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QScrollBar>

#include "aboutbox.h"
#include "version/version.h"
#include "utils/expandinglabel.h"
#include "qboxlayout.h"

AboutBox* AboutBox::Singleton = nullptr;

static constexpr int AboutWidth = 390;
// static constexpr int AboutText = 220;

QString AboutBox::BuildType;

AboutBox::AboutBox(QWidget *parent)
	: QWidget(parent)
{
	Singleton = this;
	// To make the application not quit when the window closes
	this->setAttribute(Qt::WA_QuitOnClose, false);

	setFixedWidth(AboutWidth);

	setStyleSheet("background-color: #E8E8E8; color: #000");

	// the new Default Font
	QFont smallFont("Droid Sans", 11);
	QFont extraSmallFont("Droid Sans", 9);
	extraSmallFont.setLetterSpacing(QFont::PercentageSpacing, 92);

	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 10, 0, 10);

	// Big Icon
	auto *logoShield = new QLabel();
	logoShield->setPixmap(QPixmap(":/resources/images/AboutBoxLogoShield.png"));
	logoShield->setAlignment(Qt::AlignHCenter);
	mainLayout->addWidget(logoShield);

	// Version String
	auto *versionMain = new QLabel();
	versionMain->setText(tr("Version %1.%2.%3 <br><small>(%4%5 %6) %7 [Qt %8]</small>")
						 .arg(Version::majorVersion())
						 .arg(Version::minorVersion())
						 .arg(Version::minorSubVersion())
						 .arg(Version::modifier())
						 .arg(Version::gitVersion())
						 .arg(Version::date())
						 .arg(BuildType)
						 .arg(QT_VERSION_STR) );
	versionMain->setFont(smallFont);
	versionMain->setAlignment(Qt::AlignHCenter);
	versionMain->setTextInteractionFlags(Qt::TextSelectableByMouse);
	mainLayout->addWidget(versionMain);

	// Link to website
	auto *linkToFritzing = new QLabel();
	linkToFritzing->setText("<a href=\"https://fritzing.org\">fritzing.org</a>");
	linkToFritzing->setOpenExternalLinks(true);
	linkToFritzing->setFont(smallFont);
	linkToFritzing->setAlignment(Qt::AlignHCenter);
	mainLayout->addWidget(linkToFritzing);

	// Scrolling Credits Text
	QString data =
		QString("<br /><br /><br /><br /><br /><br />") +

		"<p>" +
		tr("Fritzing is made by: ") +
		tr("Prof. Reto Wettach, Andr&eacute; Kn&ouml;rig, Myriel Milicevic, ") +
		tr("Zach Eveland, Dirk van Oosterbosch, ") +
		tr("Jonathan Cohen, Marcus Paeschke, Omer Yosha, ") +
		tr("Travis Robertson, Stefan Hermann, Brendan Howell, ") +
		tr("Mariano Crowe, Johannes Landstorfer, ") +
		tr("Jenny Chowdhury, Lionel Michel, Fabian Althaus, Jannis Leidel, ") +
		tr("Bryant Mairs, Uleshka Asher, Daniel Tzschentke, and Kjell Morgenstern") +
		"</p>" +

		"<p>" +
		tr("Special thanks go out to: ") +
		tr("Jussi &Auml;ngeslev&auml;, Massimo Banzi, Ayah Bdeir, ") +
		tr("Durrell Bishop, David Cuartielles, Fabian Hemmert, ") +
		tr("Gero Herkenrath, Jeff Hoefs, Tom Hulbert, ") +
		tr("Tom Igoe, Hans-Peter Kadel, Till Savelkoul, ") +
		tr("Jan Sieber, Yaniv Steiner, Olaf Val, ") +
		tr("Michaela Vieser and Julia Werner.") +
		"</p>" +

		"<p>" +
		tr("Thanks for the translations go out to: ") + "<br/>" +
		tr("Bulgarian: ") + "Nikolay Stankov, Lyubomir Vasilev" + "<br/>" +
		tr("Chinese (Simplified): ") + tr("Yuelin and Ninjia") + "<br/>" +
		tr("Chinese (Traditional): ") + tr("Robert Lee") + "<br/>" +
		tr("Czech: ") + "Josef Dustira" + "<br/>" +
		tr("Dutch: ") + "Steven Noppe, Davy Uittenbogerd" + "<br/>" +
		tr("French: ") + "Lionel Michel, Yvan Kloster, Alexandre Dussart, Roald Baudoux" + "<br/>" +
		tr("Greek: ") + "Alexander Kaltsas" + "<br/>" +
		tr("Italian: ") + "Gianluca Urgese" + "<br/>" +
		tr("Japanese: ") + tr("Hiroshi Suzuki") + tr(", Siti Aishah Abdul Raouf") + "<br/>" +
		tr("Korean: ") + tr("Jinbuhm Kim") + "<br/>" +
		tr("Portuguese (European): ") + "Nuno Pessanha Santos, Bruno Ramalhete" + "<br/>" +
		tr("Portuguese (Brazilian): ") + " Arthur Zanona, Bruno Ramalhete, Nuno Pessanha Santos, Leandro Nunes, Gabriel Ferreira" + "<br/>" +
		tr("Russian: ") + "Vladimir Savinov" + "<br/>" +
		tr("Slovak: ") + " &#313;ubom&iacute;r Ducho&#328;" + "<br/>" +
		tr("Spanish: ") + "Kurt Badelt, Miguel Solis" + "<br/>" +
		tr("Turkish: ") + "Cihan Mete Bahad&#x0131;r" + "<br/>" +
		tr("Ukrainian: ") + tr("Yelyzaveta Chyhryna") + "<br/>" +

		"</p>" +

		"<p>" +
		tr("Fritzing is made possible with funding from the "
		   "MWFK Brandenburg, "
		   "the sponsorship of the Design Department of Bauhaus-University Weimar, "
		   "IxDS, an anonymous donor, Parallax, Picaxe, Sparkfun, "
		   "from the PCB Fab AISLER"
		   ", and each paid download.") +
		"</p>" +

		"<p>" +
		tr("Special thanks go out to all the students and alpha testers who were brave enough to give Fritzing a test spin.") +
		"</p>";

	QString br = "<br/>";
	QString lgplv3 = tr("LGPLv3");
	QString lgplv2 = tr("GPLv2 with linking exception");
	QString boost = tr("Boost License 1.0");
	QString modifiedbsd = tr("Modified BSD License");
	QString bsd = tr("BSD License");
	QString pnglicense = tr("PNG Reference Library License version 2");
	QString openssl = tr("Dual OpenSSL and SSLeay License");
	QString zlib = tr("zlib License");


	QString licensesTable = "<p>" + tr("The following libraries are used by Fritzing:") + br +
						QString(
							   "<table width='80%' border='0' cellspacing='0' cellpadding='0'>"
							   "<tr><td align='left'>Qt</td><td align='left'>%1</td></tr>"
							   "<tr><td align='left'>Boost</td><td align='left'>%2</td></tr>"
							   "<tr><td align='left'>svgpp</td><td align='left'>%2</td></tr>"
							   "<tr><td align='left'>libngspice</td><td align='left'>%3</td></tr>"
							   "<tr><td align='left'>libquazip</td><td align='left'>%1</td></tr>"
							   "<tr><td align='left'>libpng</td><td align='left'>%4</td></tr>"
							   "<tr><td align='left'>libcrypto</td><td align='left'>%5</td></tr>"
							   "<tr><td align='left'>libjpg</td><td align='left'>%6</td></tr>"
							   "<tr><td align='left'>zlib</td><td align='left'>%7</td></tr>"
							   "<tr><td align='left'>libgit2</td><td align='left'>%8</td></tr>"
							   "</table>"
							   "</p>"
								"<br /><br /><br /><br /><br /><br /><br /><br />"
								)
			 .arg(lgplv3)
			 .arg(boost)
			 .arg(modifiedbsd)
			 .arg(pnglicense)
			 .arg(openssl)
			 .arg(bsd)
			 .arg(zlib)
			 .arg(lgplv2);

	QPixmap fadepixmap(":/resources/images/aboutbox_scrollfade.png");

	data = "<div align='center'><table width='90%'><tr><td align='center'>" +
				data +
				"</td></td></table>" +
				licensesTable +
			"</div>";

	m_expandingLabel = new ExpandingLabel(nullptr);
	m_expandingLabel->setObjectName("aboutText");
	m_expandingLabel->setLabelText(data);
	m_expandingLabel->setFont(smallFont);
	m_expandingLabel->setFixedSize(AboutWidth, fadepixmap.height());
	mainLayout->addWidget(m_expandingLabel);

	QLabel *copyrightGNU = new QLabel();
	copyrightGNU->setText(tr("<b>GNU GPL v3 on the code and CreativeCommons:BY-SA on the rest"));
	copyrightGNU->setFont(extraSmallFont);
	copyrightGNU->setAlignment(Qt::AlignHCenter);
	mainLayout->addWidget(copyrightGNU);

	auto *CC = new QLabel();
	QPixmap cc(":/resources/images/aboutbox_CC.png");
	CC->setPixmap(cc);
	CC->setAlignment(Qt::AlignHCenter);
	mainLayout->addWidget(CC);

	auto *copyrightFritzing = new QLabel();
	copyrightFritzing->setText(tr("<b>Copyright %1 Fritzing GmbH</b>").arg(Version::year()));
	copyrightFritzing->setFont(extraSmallFont);
	copyrightFritzing->setAlignment(Qt::AlignHCenter);
	mainLayout->addWidget(copyrightFritzing);


	// auto scroll timer initialization
	m_restartAtTop = false;
	m_autoScrollTimer = new QTimer(this);
	m_autoScrollTimer->setTimerType(Qt::PreciseTimer);

	connect(m_autoScrollTimer, SIGNAL(timeout()), this, SLOT(scrollCredits()));
}

void AboutBox::resetScrollAnimation() {
	// Only called when the window is newly loaded
	m_autoScrollTimer->start(35);
	m_startTime.start();
}

void AboutBox::scrollCredits() {
	if (m_startTime.elapsed() >= 0 ) {
		//int max = m_scrollArea->verticalScrollBar()->maximum();
		//int v = m_scrollArea->widget()->sizeHint().height();
		if (m_restartAtTop) {
			// Reset at the top
			m_startTime.start();
			m_restartAtTop = false;
			m_expandingLabel->verticalScrollBar()->setValue(0);
			return;
		}
		if (m_expandingLabel->verticalScrollBar()->value() >= m_expandingLabel->verticalScrollBar()->maximum()) {
			// go and reset
			// m_startTime.start();
			m_restartAtTop = true;
		} else {
			m_expandingLabel->verticalScrollBar()->setValue(m_expandingLabel->verticalScrollBar()->value() + 1);
		}
	}
}

void AboutBox::initBuildType(const QString & buildType) {
	BuildType = buildType;
}

void AboutBox::hideAbout() {
	//DebugDialog::debug("the AboutBox gets a hide action triggered");
	if (Singleton != nullptr) {
		Singleton->hide();
	}
}

void AboutBox::showAbout() {
	//DebugDialog::debug("the AboutBox gets a show action triggered");
	if (Singleton == nullptr) {
		new AboutBox();
	}

	// scroll text now to prevent a flash of text if text was visible the last time the about box was open
	Singleton->m_expandingLabel->verticalScrollBar()->setValue(0);

	Singleton->show();
}

void AboutBox::closeAbout() {
	//DebugDialog::debug("the AboutBox gets a close action triggered");
	// Note: not every close triggers this function. we better listen to closeEvent
	if (Singleton != nullptr) {
		Singleton->close();
	}
}

void AboutBox::closeEvent(QCloseEvent *event) {
	// called when the window is about to close
	//DebugDialog::debug("the AboutBox gets a closeEvent");
	m_autoScrollTimer->stop();
	event->accept();
}

void AboutBox::keyPressEvent ( QKeyEvent * event ) {
	if ((event->key() == Qt::Key_W) && (event->modifiers() & Qt::ControlModifier) ) {
		// We get the ctrl + W / command + W key event
		//DebugDialog::debug("W key!");
		this->closeAbout();
	}
}

void AboutBox::show() {
	QWidget::show();
	m_restartAtTop = true;
	resetScrollAnimation();
}
