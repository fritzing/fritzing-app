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

$Revision: 6112 $:
$Author: cohen@irascible.com $:
$Date: 2012-06-28 00:18:10 +0200 (Do, 28. Jun 2012) $

********************************************************************/

#include "syntaxer.h"
#include "../debugdialog.h"
#include "../utils/textutils.h"

#include <QRegExp>
#include <QXmlStreamReader>

QHash<QString, QString> Syntaxer::m_listsToFormats;

Syntaxer::Syntaxer() : QObject() {
	m_trieRoot = NULL;
}

Syntaxer::~Syntaxer() {
	if (m_trieRoot) {
		delete m_trieRoot;
	}
}

bool Syntaxer::loadSyntax(const QString &filename)
 {
	QFile file(filename);

	QString errorStr;
	int errorLine;
	int errorColumn;

	QDomDocument domDocument;
	if (!domDocument.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
		return false;
	}

	QDomElement root = domDocument.documentElement();
	if (root.isNull()) return false;
	if (root.tagName() != "language") return false;

	QDomElement highlighting = root.firstChildElement("highlighting");
	if (highlighting.isNull()) return false;

	QDomElement general = root.firstChildElement("general");
	if (general.isNull()) return false;

	QDomElement contexts = highlighting.firstChildElement("contexts");
	if (contexts.isNull()) return false;

	m_hlCStringChar = false;
	QDomElement context = contexts.firstChildElement("context");
	while (!context.isNull()) {
		if (context.attribute("attribute").compare("Normal Text") == 0) {
			m_stringDelimiter = getStringDelimiter(context);
			initListsToFormats(context);
		}
		else if (context.attribute("attribute").compare("String") == 0) {
			QDomElement HlCStringChar = context.firstChildElement("HlCStringChar");
			if (!HlCStringChar.isNull()) {
				m_hlCStringChar = true;
			}
		}
		context = context.nextSiblingElement("context");
	}

    m_canProgram = root.attribute("canProgram", "").compare("true", Qt::CaseInsensitive) == 0;
	m_name = root.attribute("name");
	QStringList extensions = root.attribute("extensions").split(";", QString::SkipEmptyParts);
	if (extensions.count() > 0) {
		m_extensionString = m_name + " " + QObject::tr("files") + " (";
		foreach (QString ext, extensions) {
			m_extensionString += ext + " ";
			int ix = ext.indexOf(".");
			if (ix > 0) {
				ext.remove(0, ix);
			}
			m_extensions.append(ext);
		}
		m_extensionString.chop(1);
		m_extensionString += ")";
	}
	
	m_trieRoot = new TrieNode('\0');

	QDomElement list = highlighting.firstChildElement("list");
	while (!list.isNull()) {
		loadList(list);
		list = list.nextSiblingElement("list");
	}

	QDomElement comments = general.firstChildElement("comments");
	if (!comments.isNull()) {
		Qt::CaseSensitivity caseSensitivity = comments.attribute("casesensitive").compare("1") == 0 ? Qt::CaseSensitive : Qt::CaseInsensitive;
		QDomElement comment = comments.firstChildElement("comment");
		while (!comment.isNull()) {
			CommentInfo * commentInfo = new CommentInfo(comment.attribute("start"), comment.attribute("end"), caseSensitivity);
			commentInfo->m_index = m_commentInfo.count();
			m_commentInfo.append(commentInfo);
			comment = comment.nextSiblingElement("comment");
		}
	}

	return true;
}

QString Syntaxer::parseForName(const QString & filename)
{
	QFile file(filename);
	file.open(QFile::ReadOnly);
	QXmlStreamReader xml(&file);
    xml.setNamespaceProcessing(false);

	while (!xml.atEnd()) {
        switch (xml.readNext()) {
			case QXmlStreamReader::StartElement:
				if (xml.name().toString().compare("language") == 0) {
					return xml.attributes().value("name").toString();
				}
				break;
			default:
				break;
		}
	}

	return "";
}

void Syntaxer::loadList(QDomElement & list) {
	QString name = list.attribute("name");
	SyntaxerTrieLeaf * stf = new SyntaxerTrieLeaf(name);
	QDomElement item = list.firstChildElement("item");
	while (!item.isNull()) {
		QString text;
		if (TextUtils::findText(item, text)) {
            QString s = text.trimmed();
            m_trieRoot->addString(s, false, stf);
		}
		item = item.nextSiblingElement("item");
	}
}

bool Syntaxer::matches(const QString & string, TrieLeaf * & leaf) {
	if (m_trieRoot == NULL) return false;

	QString temp = string;
	return m_trieRoot->matches(temp, leaf);
}

const CommentInfo * Syntaxer::getCommentInfo(int ix) {
	return m_commentInfo.at(ix);
}

bool Syntaxer::matchCommentStart(const QString & text, int offset, int & result, const CommentInfo * & resultCommentInfo) {
	result = -1;
	foreach (CommentInfo * commentInfo, m_commentInfo) {
		int si = text.indexOf(commentInfo->m_start, offset, commentInfo->m_caseSensitive);
		if (si >= 0 && (result < 0 || si < result)) {
			result = si;
			resultCommentInfo = commentInfo;
		}
	}

	return (result >= offset);
}

int Syntaxer::matchStringStart(const QString & text, int offset) {
	if (m_stringDelimiter.isNull()) return -1;

	return text.indexOf(m_stringDelimiter, offset);
}

int Syntaxer::matchStringEnd(const QString & text, int offset) {
	return matchStringStart(text, offset);
}

const QStringList & Syntaxer::extensions() {
	return m_extensions;
}

const QString & Syntaxer::extensionString() {
	return m_extensionString;
}

QChar Syntaxer::getStringDelimiter(QDomElement & context) {
	QDomElement detectChar = context.firstChildElement("DetectChar");
	while (!detectChar.isNull()) {
		if (detectChar.attribute("attribute").compare("String") == 0) {
			QString c = detectChar.attribute("char");
			if (c.length() > 0) {
				return c.at(0);
			}
			return QChar();
		}
		detectChar = detectChar.nextSiblingElement("DetectChar");
	}

	return QChar();
}

void Syntaxer::initListsToFormats(QDomElement & context) {
	QDomElement keyword = context.firstChildElement("keyword");
	while (!keyword.isNull()) {
		QString format = keyword.attribute("attribute");
		QString list = keyword.attribute("String");
		if (!format.isEmpty() && !list.isEmpty()) {
			m_listsToFormats.insert(list, format);
		}
		keyword = keyword.nextSiblingElement("keyword");
	}
}

QString Syntaxer::formatFromList(const QString & list) {
	return m_listsToFormats.value(list, ___emptyString___);
}

bool Syntaxer::hlCStringChar() {
	return m_hlCStringChar;
}


bool Syntaxer::canProgram() {
    return m_canProgram;
}


//////////////////////////////////////////////

SyntaxerTrieLeaf::SyntaxerTrieLeaf(QString name) {
	m_name = name;
}

SyntaxerTrieLeaf::~SyntaxerTrieLeaf()
{
}

const QString & SyntaxerTrieLeaf::name()
{
	return m_name;
}

//////////////////////////////////////////////

CommentInfo::CommentInfo(const QString & start, const QString & end, Qt::CaseSensitivity caseSensitive) {
	m_start = start;
	m_end = end;
	m_multiLine = !end.isEmpty();
	m_caseSensitive = caseSensitive;
}
