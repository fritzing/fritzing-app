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

#include "translatorlistmodel.h"
#include "../debugdialog.h"

static QVariant emptyVariant;
QHash<QString, QString> TranslatorListModel::m_languages;
QList<QLocale *> TranslatorListModel::m_localeList;

// More languages written in their own language can be found
// at http://www.mozilla.com/en-US/firefox/all.html 

// recipe for translating from mozilla strings into source code via windows:
//		1. copy the string from the mozilla page into wordpad and save it as a unicode text file
//		2. open that unicode text file as a binary file (e.g. in msvc)
//		3. ignore the first two bytes (these are a signal that says "I'm unicode")
//		4. initialize an array of ushort using the rest of the byte pairs
//		5. don't forget to reverse the order of the bytes in each pair.

// to add a new language to fritzing, find its two-letter language code: http://en.wikipedia.org/wiki/List_of_ISO_639-1_codes
// then in the translations.pri file, add a new line translations/fritzing_XX.ts where you substitute the two letters for "XX".
// If the language has multiple dialects, then you will need to add _YY to distinguish


TranslatorListModel::TranslatorListModel(QFileInfoList & fileInfoList, QObject* parent)
: QAbstractListModel(parent) 
{

    if (m_languages.count() == 0) {
        m_languages.insert("english", tr("English - %1").arg("English"));
        m_languages.insert("german", tr("German - %1").arg("Deutsch"));
        m_languages.insert("hungarian", tr("Hungarian - %1").arg("Magyar"));
        m_languages.insert("estonian", tr("Estonian - %1").arg("Eesti keel"));
        m_languages.insert("dutch", tr("Dutch - %1").arg("Nederlands"));
        m_languages.insert("italian", tr("Italian - %1").arg("Italiano"));
        m_languages.insert("polish", tr("Polish - %1").arg("Polski"));
		m_languages.insert("swedish", tr("Swedish - %1").arg("Svenska"));
		m_languages.insert("galician", tr("Galician - %1").arg("Galego"));
        m_languages.insert("indonesian", tr("Indonesian - %1").arg("Bahasa Indonesia"));
        m_languages.insert("danish", tr("Danish - %1").arg("Dansk"));

        ushort t1[] = { 0x65e5, 0x672c, 0x8a9e, 0 };
		m_languages.insert("japanese", tr("Japanese - %1").arg(QString::fromUtf16(t1)));

		ushort t2[] = { 0x0420, 0x0443, 0x0441, 0x0441, 0x043a, 0x0438, 0x0439, 0 };
        m_languages.insert("russian", tr("Russian - %1").arg(QString::fromUtf16(t2)));

 		ushort t3[] = { 0x05e2, 0x05d1, 0x05e8, 0x05d9, 0x05ea, 0 };
        m_languages.insert("hebrew", tr("Hebrew - %1").arg(QString::fromUtf16(t3)));

		ushort t4[] = { 0x0639, 0x0631, 0x0628, 0x064a, 0 };
        m_languages.insert("arabic", tr("Arabic - %1").arg(QString::fromUtf16(t4)));

		ushort t5[] = { 0x0939, 0x093f, 0x0928, 0x094d, 0x0926, 0x0940, 0x0020, 0x0028, 0x092d, 0x093e, 0x0930, 0x0924, 0x0029, 0 };
        m_languages.insert("hindi", tr("Hindi - %1").arg(QString::fromUtf16(t5)));

		ushort t6[] = { 0x4e2d, 0x6587, 0x0020, 0x0028, 0x7b80, 0x4f53, 0x0029, 0 };
        m_languages.insert("chinese_china", tr("Chinese (Simplified) - %1").arg(QString::fromUtf16(t6)));

        ushort t7[] = { 0x6b63, 0x9ad4, 0x4e2d, 0x6587, 0x0020, 0x0028, 0x7e41, 0x9ad4, 0x0029, 0 };
        m_languages.insert("chinese_taiwan", tr("Chinese (Traditional) - %1").arg(QString::fromUtf16(t7)));

		ushort t8[] = { 0x010C, 0x65, 0x0161, 0x74, 0x69, 0x6E, 0x61, 0 };
        m_languages.insert("czech", tr("Czech - %1").arg(QString::fromUtf16(t8)));

		ushort t9[] = { 0x52,  0x6F, 0x6D, 0xE2, 0x6E, 0x0103, 0 };
		m_languages.insert("romanian", tr("Romanian - %1").arg(QString::fromUtf16(t9)));

		ushort t10[] = { 0x0E44, 0x0E17, 0x0E22, 0 };
		m_languages.insert("thai", tr("Thai - %1").arg(QString::fromUtf16(t10)));

		ushort t11[] = { 0x0395, 0x03BB, 0x03BB, 0x03B7, 0x03BD, 0x03B9, 0x03BA, 0x03AC, 0};
		m_languages.insert("greek", tr("Greek - %1").arg(QString::fromUtf16(t11)));

		ushort t12[] = { 0x0411, 0x044A, 0x043B, 0x0433, 0x0430, 0x0440, 0x0441, 0x043A, 0x0438, 0 };
		m_languages.insert("bulgarian", tr("Bulgarian - %1").arg(QString::fromUtf16(t12)));

		ushort t13[] = { 0xD55C,  0xAD6D,  0xC5B4, 0};
		m_languages.insert("korean", tr("Korean - %1").arg(QString::fromUtf16(t13)));

		ushort t14[] = { 0x53, 0x6C, 0x6F, 0x76, 0x65, 0x6E, 0x010D, 0x69, 0x6E, 0x61, 0};
		m_languages.insert("slovak", tr("Slovak - %1").arg(QString::fromUtf16(t14)));

        ushort t15[] = { 0x09AC,  0x09BE,   0x0982,   0x09B2,   0x09BE, 0 };
        m_languages.insert("bengali", tr("Bengali - %1").arg(QString::fromUtf16(t15)));

        ushort t16[] = { 0x0641, 0x0627, 0x0631, 0x0633, 0x06CC, 0 };
        m_languages.insert("persian", tr("Persian - %1").arg(QString::fromUtf16(t16)));

        ushort t17[] = {0x53, 0x6C, 0x6F, 0x76, 0x65, 0x6E, 0x0161, 0x010D, 0x69, 0x6E, 0x61, 0 };
        m_languages.insert("slovenian", tr("Slovenian - %1").arg(QString::fromUtf16(t17)));

        ushort t18[] = { 0x092E,  0x0930,  0x093E,  0x0920,  0x0940, 0 };
        m_languages.insert("marathi", tr("Marathi - %1").arg(QString::fromUtf16(t18)));

        ushort t19[] = {0x0443,  0x043A,  0x0440,  0x0430,  0x0457,  0x043D,  0x0441,  0x044C,  0x043A,  0x0430,  0x0020,  0x043C,  0x043E,  0x0432,  0x0430, 0};
        m_languages.insert("ukrainian", tr("Ukrainian - %1").arg(QString::fromUtf16(t19)));

        ushort t20[] = { 0x46, 0x72, 0x61, 0x6E, 0xE7, 0x61, 0x69, 0x73, 0 };
        m_languages.insert("french", tr("French - %1").arg(QString::fromUtf16(t20)));

        ushort t21[] = { 0x45, 0x73, 0x70, 0x61, 0xF1, 0x6F, 0x6C, 0 };
        m_languages.insert("spanish", tr("Spanish - %1").arg(QString::fromUtf16(t21)));

        ushort t22[] = { 0x50, 0x6F, 0x72, 0x74, 0x75, 0x67, 0x75, 0xEA, 0x73, 0x20, 0x28, 0x45, 0x75, 0x72, 0x6F, 0x70, 0x65, 0x75, 0x29, 0 };
        m_languages.insert("portuguese_portugal", tr("Portuguese (European)- %1").arg(QString::fromUtf16(t22)));

        ushort t23[] = { 0x50, 0x6F, 0x72, 0x74, 0x75, 0x67, 0x75, 0xEA, 0x73, 0x20, 0x28, 0x64, 0x6F, 0x20, 0x42, 0x72, 0x61, 0x73, 0x69, 0x6C, 0x29, 0 };
        m_languages.insert("portuguese_brazil", tr("Portuguese (Brazilian) - %1").arg(QString::fromUtf16(t23)));

        ushort t24[] = { 0x54, 0xFC, 0x72, 0x6B, 0xE7, 0x65, 0 };
        m_languages.insert("turkish", tr("Turkish - %1").arg(QString::fromUtf16(t24)));

        ushort t25[] = { 0x043C, 0x0430,  0x043A,  0x0435,  0x0434, 0x043E, 0x043D, 0x0441, 0x043A, 0x0438, 0x20, 0x0458, 0x0430, 0x0437, 0x0438, 0x043A, 0 };
        m_languages.insert("macedonian", tr("Macedonian - %1").arg(QString::fromUtf16(t25)));

        ushort t26[] = { 0x0441, 0x0440, 0x043F, 0x0441, 0x043a, 0x0438, 0x0020, 0x0458, 0x20, 0x0435, 0x0437, 0x0438, 0x043A, 0 };
        m_languages.insert("serbian", tr("Serbian - %1").arg(QString::fromUtf16(t26)));

        ushort t27[] = { 0x0627, 0x0631, 0x062f, 0x0648, 0 };
        m_languages.insert("urdu", tr("Urdu - %1").arg(QString::fromUtf16(t27)));

	}

	if (m_localeList.count() == 0) {
		foreach (QFileInfo fileinfo, fileInfoList) {
			QString name = fileinfo.completeBaseName();
			name.replace("fritzing_", "");
			QStringList names = name.split("_");
			if (names.count() > 1) {
				name = names[0] + "_" + names[1].toUpper();
			}
			QLocale * locale = new QLocale(name);
			m_localeList.append(locale);
		}
	}
}


TranslatorListModel::~TranslatorListModel() {
}

void TranslatorListModel::cleanup() {
	foreach (QLocale * locale, m_localeList) {
		delete locale;
	}
	m_localeList.clear();
}

QVariant TranslatorListModel::data ( const QModelIndex & index, int role) const 
{
	if (role == Qt::DisplayRole && index.row() >= 0 && index.row() < m_localeList.count()) {
		QString languageString = QLocale::languageToString(m_localeList.at(index.row())->language());
		QString countryString = QLocale::countryToString(m_localeList.at(index.row())->country());

        //DebugDialog::debug(QString("language %1 %2").arg(languageString).arg(countryString));

		// QLocale::languageToString() only returns an English string, 
		// so put it through a language-dependent hash table.
		QString trLanguageString = m_languages.value(languageString.toLower(), "");
		if (trLanguageString.isEmpty()) {
			trLanguageString = m_languages.value(languageString.toLower() + "_" + countryString.toLower(), "");
		}

		if (trLanguageString.isEmpty()) {
			foreach (QString key, m_languages.keys()) {
				if (key.startsWith(languageString.toLower())) {
					return m_languages.value(key);
				}
			}

			return languageString;
		}

		return trLanguageString;
	}

	return emptyVariant;
	
}

int TranslatorListModel::rowCount ( const QModelIndex & parent) const 
{
	Q_UNUSED(parent);

	return m_localeList.count();
}

const QLocale * TranslatorListModel::locale( int index) 
{
	if (index < 0 || index >= m_localeList.count()) return NULL;

	return m_localeList.at(index);	
}

int TranslatorListModel::findIndex(const QString & language) {
	int ix = 0;
	foreach (QLocale * locale, m_localeList) {
		//DebugDialog::debug(QString("find index %1 %2").arg(language).arg(locale->name()));
		if (language.compare(locale->name()) == 0) return ix;
		ix++;
	}

	ix = 0;
	foreach (QLocale * locale, m_localeList) {
		if (locale->name().startsWith("en")) return ix;
		ix++;
	}

	return 0;

}

