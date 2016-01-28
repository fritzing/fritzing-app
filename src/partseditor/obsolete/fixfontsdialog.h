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

#ifndef FIXFONTSDIALOG_H_
#define FIXFONTSDIALOG_H_

#include <QDialog>
#include <QSet>

#define FixedFontsHash QHash<QString /*borkenFont*/, QString /*replacementFont*/>

class FixFontsDialog : public QDialog {
	Q_OBJECT
public:
	FixFontsDialog(QWidget *parent, const QSet<QString> fontsTofix);
	virtual ~FixFontsDialog();
	FixedFontsHash getFixedFontsHash();
	QSet<QString> fontsToFix();

	static FixedFontsHash fixFonts(QWidget *parent, const QSet<QString> fontsTofix, bool &cancelPressed);

protected:
	void createLine(QLayout* layout, const QString &brokenFont, const QStringList &items, int defaultIdx);

	QSet<QString> m_fontsToFix;
	QList<class FixedFontComboBox *> m_fixedFonts;
};

#endif /* FIXFONTSDIALOG_H_ */
