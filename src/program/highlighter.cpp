/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2019 Fritzing

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

#include <QRegularExpression>

#include "highlighter.h"
#include "syntaxer.h"

#include "../utils/textutils.h"
#include "../debugdialog.h"

#include <stdlib.h>

#define STRINGOFFSET 10
#define COMMENTOFFSET 100
static const QChar CEscapeChar('\\');

QHash <QString, QTextCharFormat *> Highlighter::m_styleFormats;

Highlighter::Highlighter(QTextEdit * textEdit) : QSyntaxHighlighter(textEdit)
{
	m_syntaxer = nullptr;
}

Highlighter::~Highlighter()
{
}

void Highlighter::loadStyles(const QString & filename) {
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly)) {
		DebugDialog::debug(QString("Unable to open :%1").arg(filename));
	}

	QString errorStr;
	int errorLine;
	int errorColumn;

	QDomDocument domDocument;
	if (!domDocument.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
		return;
	}

	QDomElement root = domDocument.documentElement();
	if (root.isNull()) return;
	if (root.tagName() != "styles") return;

	QDomElement style = root.firstChildElement("style");
	while (!style.isNull()) {
		auto * tcf = new QTextCharFormat();
		QColor color(Qt::black);
		QString colorString = style.attribute("color");
		if (!colorString.isEmpty()) {
			color.setNamedColor(colorString);
			tcf->setForeground(QBrush(color));
		}
		QString italicString = style.attribute("italic");
		if (italicString.compare("1") == 0) {
			tcf->setFontItalic(true);
		}
		QString boldString = style.attribute("bold");
		if (boldString.compare("1") == 0) {
			tcf->setFontWeight(QFont::Bold);
		}
		QString underlineString = style.attribute("underline");
		if (underlineString.compare("1") == 0) {
			tcf->setFontUnderline(true);
		}

		m_styleFormats.insert(style.attribute("name"), tcf);

		style = style.nextSiblingElement("style");
	}
}

void Highlighter::setSyntaxer(Syntaxer * syntaxer) {
	m_syntaxer = syntaxer;
}

Syntaxer * Highlighter::syntaxer() {
	return m_syntaxer;
}

void Highlighter::highlightBlock(const QString &text)
{
	if (m_syntaxer == nullptr) return;

	if (text.isEmpty()) {
		setCurrentBlockState(previousBlockState());
		return;
	}

	setCurrentBlockState(0);
	int startCommentIndex = -1;
	int startStringIndex = -1;
	const CommentInfo * currentCommentInfo = nullptr;
	int pbs = previousBlockState();
	if (pbs <= 0) {
		m_syntaxer->matchCommentStart(text, 0, startCommentIndex, currentCommentInfo);
	}
	else if (pbs >= COMMENTOFFSET) {
		currentCommentInfo = m_syntaxer->getCommentInfo(previousBlockState() - COMMENTOFFSET);
		startCommentIndex = 0;
	}
	else if (pbs == STRINGOFFSET) {
		startStringIndex = 0;
	}

	QString noComment = text;

	while (startCommentIndex >= 0) {
		int endIndex = currentCommentInfo->m_multiLine ? text.indexOf(currentCommentInfo->m_end, startCommentIndex, currentCommentInfo->m_caseSensitive) : text.length();
		int commentLength;
		if (endIndex == -1) {
			setCurrentBlockState(currentCommentInfo->m_index + COMMENTOFFSET);
			commentLength = text.length() - startCommentIndex;
		}
		else {
			commentLength = endIndex - startCommentIndex + currentCommentInfo->m_end.length();
		}
		noComment.replace(startCommentIndex, commentLength, QString(commentLength, ' '));
		QTextCharFormat * cf = m_styleFormats.value("Comment", nullptr);
		if (cf != nullptr) {
			setFormat(startCommentIndex, commentLength, *cf);
		}
		m_syntaxer->matchCommentStart(text, startCommentIndex + commentLength, startCommentIndex, currentCommentInfo);
	}

	highlightStrings(startStringIndex, noComment);
	highlightTerms(noComment);
	highlightNumbers(noComment);

	// highlight single chars
	applyRule(QRegularExpression(R"RX('.?')RX"), m_styleFormats.value("Constant", nullptr), text);

}

void Highlighter::highlightStrings(int startStringIndex, QString & text) {
	if (startStringIndex < 0) {
		startStringIndex = m_syntaxer->matchStringStart(text, 0);
	}

	// TODO: not handling "" as a way to escape-quote
	while (startStringIndex >= 0) {
		int endIndex = -1;
		int ssi = startStringIndex;
		while (true) {
			endIndex = m_syntaxer->matchStringEnd(text, ssi + 1);
			if (!m_syntaxer->hlCStringChar()) {
				// only some languages use \ to escape
				break;
			}

			if (endIndex == -1) {
				break;
			}

			// TODO: escape char is backslash only; are there others in other compilers?
			if (text.at(endIndex - 1) != CEscapeChar) {
				break;
			}
			ssi = endIndex;
		}
		int stringLength;
		if (endIndex == -1) {
			setCurrentBlockState(STRINGOFFSET);
			stringLength = text.length() - startStringIndex;
		}
		else {
			stringLength = endIndex - startStringIndex + 1;
		}
		text.replace(startStringIndex, stringLength, QString(stringLength, ' '));
		QTextCharFormat * sf = m_styleFormats.value("String", NULL);
		if (sf != nullptr) {
			setFormat(startStringIndex, stringLength, *sf);
		}
		startStringIndex = m_syntaxer->matchStringStart(text, startStringIndex + stringLength);
	}
}

void Highlighter::highlightTerms(const QString & text) {
	int lastWordBreak = 0;
	int textLength = text.length();
	int b;
	while (lastWordBreak < textLength) {
		for (b = lastWordBreak; b < textLength; b++) {
			if (!isWordChar(text.at(b))) break;
		}

		if (b > lastWordBreak) {
			TrieLeaf * leaf = nullptr;
			if (m_syntaxer->matches(text.mid(lastWordBreak, b - lastWordBreak), leaf)) {
				auto * stl = dynamic_cast<SyntaxerTrieLeaf *>(leaf);
				if (stl != nullptr) {
					QString format = Syntaxer::formatFromList(stl->name());
					QTextCharFormat * tcf = m_styleFormats.value(format, NULL);
					if (tcf != nullptr) {
						setFormat(lastWordBreak, b - lastWordBreak, *tcf);
					}
				}
			}
		}

		lastWordBreak = b + 1;
	}
}

void Highlighter::highlightNumbers(const QString &text) {

	// Floats , but also Integers
	auto * floatStyle(m_styleFormats.value("Float", nullptr));
	applyRule(TextUtils::floatingPointMatcher, floatStyle, text);

	// Hex
	auto * hexStyle(m_styleFormats.value("Hex", nullptr));
	QRegularExpression hexRule(R"RX((\b0x[\dA-F]+\b))RX", QRegularExpression::CaseInsensitiveOption);
	applyRule(hexRule, hexStyle, text);

	// Octal (use the hex style)
	QRegularExpression octalRule(R"RX((\b0[0-7]+\b))RX");
	applyRule(octalRule, hexStyle, text);

	// Binary (use the hex style)
	QRegularExpression binaryRule(R"RX((\b0b[01]+\b))RX", QRegularExpression::CaseInsensitiveOption);
	applyRule(binaryRule, hexStyle, text);
}

void Highlighter::applyRule(const QRegularExpression & rule, QTextCharFormat * format, const QString & text) {
	if (format) { // only apply style if it is present in the styles.xml
		QRegularExpressionMatchIterator matchIterator = rule.globalMatch(text);
		while (matchIterator.hasNext()) {
			QRegularExpressionMatch match = matchIterator.next();
			setFormat(match.capturedStart(), match.capturedLength(), *format);
		}
	}
}

bool Highlighter::isWordChar(QChar c) {
	return c.isLetterOrNumber() || c == '#' || c == '_';
}
