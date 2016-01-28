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

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QDialogButtonBox>

#include "fixfontsdialog.h"
#include "../installedfonts.h"
#include "../debugdialog.h"

//////////////////////////////////////////////////////////////////////////////

class FixedFontComboBox : public QComboBox {
public:
	FixedFontComboBox(QWidget *parent, const QString &brokenFont)
		: QComboBox(parent)
	{
		m_brokenFont = brokenFont;
	}

	const QString &brokenFont() { return m_brokenFont; }
	void setBrokenFont(const QString &brokenFont) { m_brokenFont = brokenFont; }

protected:
	QString m_brokenFont;
};

//////////////////////////////////////////////////////////////////////////////


FixFontsDialog::FixFontsDialog(QWidget *parent, const QSet<QString> fontsTofix)
	: QDialog(parent)
{
	setWindowTitle(tr("Unavailable fonts"));

	QFrame *container = new QFrame(this);
	container->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Expanding);
	QVBoxLayout *layout = new QVBoxLayout(container);
	layout->setSpacing(1);
	layout->setMargin(1);

	foreach (QString f, fontsTofix) {
		DebugDialog::debug("incoming font " + f);
	}

	m_fontsToFix = fontsTofix - InstalledFonts::InstalledFontsList;
	foreach (QString fontFileName, InstalledFonts::InstalledFontsNameMapper.values()) {
		// SVG uses filename which may not match family name (e.g. "DroidSans" and "Droid Sans")
		m_fontsToFix.remove(fontFileName);
	}
	QStringList availFonts = InstalledFonts::InstalledFontsList.toList();
	qSort(availFonts);
	availFonts.insert(0,tr("-- ignore --"));
	int defaultIdx = availFonts.indexOf("Droid Sans");

	QStringList fontsToFixList = m_fontsToFix.toList();
	qSort(fontsToFixList);
	foreach(QString ftf, fontsToFixList) {
		DebugDialog::debug("font not found: "+ftf);
		createLine(layout,ftf,availFonts,defaultIdx);
	}
	layout->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::Minimum,QSizePolicy::Expanding));

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	QVBoxLayout *mainLO = new QVBoxLayout(this);
	mainLO->setMargin(8);
	mainLO->setSpacing(2);

	QLabel *msgLabel = new QLabel(tr(
			"One or more fonts used in this SVG file are not available in Fritzing.\n"
			"Please select one of the Fritzing fonts to replace them:"
			),this);
	msgLabel->setWordWrap(true);
	mainLO->addWidget(msgLabel);
	mainLO->addWidget(container);

	mainLO->addSpacerItem(new QSpacerItem(0,5));
	QLabel *infoLabel = new QLabel(
		"For more information, please refer to the "
		"<a href='http://fritzing.org/learning/tutorials/creating-custom-parts/'>part creation guidelines</a>."
	, this);
	infoLabel->setOpenExternalLinks(true);
	infoLabel->setWordWrap(true);
	mainLO->addWidget(infoLabel);
	mainLO->addSpacerItem(new QSpacerItem(0,5));

	QHBoxLayout *btnLayout = new QHBoxLayout();
	btnLayout->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::Expanding,QSizePolicy::Minimum));
	btnLayout->addWidget(buttonBox);
	mainLO->addLayout(btnLayout);


	setMinimumWidth(100);
}

FixFontsDialog::~FixFontsDialog() {
}

// assumes that the first item in items, is a default value
void FixFontsDialog::createLine(QLayout* layout, const QString &brokenFont, const QStringList &items, int defaultIdx) {
	FixedFontComboBox *cb = new FixedFontComboBox(this,brokenFont);

	int fontValue = -1;
	foreach(QString font, items) {
		cb->addItem(font, fontValue);
		fontValue++;
	}
	cb->setCurrentIndex(defaultIdx);
	m_fixedFonts << cb;

	QFrame *line = new QFrame(this);
	line->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
	QHBoxLayout *lineLO = new QHBoxLayout(line);
	lineLO->setSpacing(0);
	lineLO->setMargin(1);
	lineLO->addWidget(new QLabel(tr("Replace "),line));
	lineLO->addWidget(new QLabel(QString("<b>%1<b> ").arg(brokenFont),line));
	lineLO->addWidget(new QLabel(tr("with "),line));
	lineLO->addWidget(cb);

	layout->addWidget(line);
}

QSet<QString> FixFontsDialog::fontsToFix() {
	return m_fontsToFix;
}

FixedFontsHash FixFontsDialog::getFixedFontsHash() {
	FixedFontsHash retval;
	foreach(FixedFontComboBox* cb, m_fixedFonts) {
		int idx = cb->currentIndex();
		if( idx != -1) {
			int value = cb->itemData(idx).toInt();
			if(value > -1) {
				QString fixedFont = cb->itemText(idx);
				// at least for Droid Sans, family name is "Droid Sans" but SVGs seem to need filename ("DroidSans").
				fixedFont = InstalledFonts::InstalledFontsNameMapper.value(fixedFont);
				retval[cb->brokenFont()] = fixedFont;
			}
		}
	}
	return retval;
}

FixedFontsHash FixFontsDialog::fixFonts(QWidget *parent, const QSet<QString> fontsTofix, bool &cancelPressed) {
	FixedFontsHash retval;
	cancelPressed = false;
	FixFontsDialog *dlg = new FixFontsDialog(parent,fontsTofix);

	if(dlg->fontsToFix().size() > 0) {
		if(dlg->exec() == QDialog::Accepted) {
			retval = dlg->getFixedFontsHash();
		} else {
			cancelPressed = true;
		}
	}
	delete dlg;
	return retval;
}
