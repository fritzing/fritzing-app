/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-08 Fritzing

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

#include "gedaelementlexer.h"
#include "gedaelementgrammar_p.h"
#include <qdebug.h>
#include <QStringList>

static QRegularExpression findWhitespace("[\\s]+");

GedaElementLexer::GedaElementLexer(const QString &source) :
    m_nonWhitespaceMatcher("[^\\s]"),
	m_commentMatcher("(^\\s*\\#)"),
	m_elementMatcher("Element\\s*([\\(\\[])"),
	m_stringMatcher("\"([^\"\\\\]*(\\\\.[^\"\\\\]*)*)\""),
	m_integerMatcher("[-+]?\\d+"),
	m_hexMatcher("0[xX][0-9a-fA-F]+"),
	m_source(),
	m_chars(nullptr),
	m_size(0),
	m_pos(0),
	m_current(0),
	m_currentCommand(),
	m_currentNumber(0l),
	m_currentHexString(0l),
	m_currentString(),
	m_comments()
{
	m_source = clean(source);
	m_chars = m_source.unicode();
	m_size = m_source.size();
	//qDebug() << m_source;
	m_current = next();
}

QString GedaElementLexer::clean(const QString & source) {
	// clean it up to make it easier to parse

	QStringList s = source.split("\n");
	for (int i = s.length() - 1; i >= 0; i--) {
		QString str = s[i];
		QRegularExpressionMatch match;
		if (str.indexOf(m_commentMatcher, 0, &match) == 0) {
			s.removeAt(i);
			str = str.remove(0, match.capturedLength());
			if (str.contains(m_nonWhitespaceMatcher)) {
				m_comments.push_front(str.trimmed());
			}
		}
	}
	QString s1 = s.join("\n");
	QString s2 = s1.replace(findWhitespace, " ");
	QRegularExpressionMatch match;
	int ix = s2.indexOf(m_elementMatcher, 0, &match);
	if (ix < 0) {
		return s2;
	}

	// now figure out how many params the Element has and fill in the missing ones
	// this is because Element grammar requires a longer lookahead than provided by an LALR(1) parser

	QStringList params;
	GedaElementLexer lexer(s2.right(s2.length() - match.capturedLength() - ix));
	int nix = ix;
	bool done = false;
	while (!done) {
		int result = lexer.lex();
		if (result <= 0) break;

		switch (result) {
		case GedaElementGrammar::HEXNUMBER:
		case GedaElementGrammar::NUMBER:
		case GedaElementGrammar::STRING:
			params.append(lexer.m_currentString);
			break;
		case GedaElementGrammar::RIGHTBRACKET:
			nix = lexer.m_pos - 2;
			done = true;
			break;
		case GedaElementGrammar::RIGHTPAREN:
			nix = lexer.m_pos - 2;
			done = true;
			break;
		default:
			return s2;
		}
	}
	if (params.count() == 11) {
		// no changes required
		return s2;
	}

	bool swap = false;
	if (params.count() == 9) {
		// insert MX and MY
		params.insert(4, "0");
		params.insert(4, "0");
		swap = true;
	}

	if (params.count() == 8) {
		// insert value
		params.insert(3, "\"\"");
		// insert MX and MY
		params.insert(4, "0");
		params.insert(4, "0");
		swap = true;
	}

	if (params.count() == 7) {
		// insert SFlags
		params.insert(0, "\"\"");
		// insert value
		params.insert(3, "\"\"");
		// insert MX and MY
		params.insert(4, "0");
		params.insert(4, "0");
		swap = true;
	}

	if (!swap) {
		return s2;
	}

	QString j = params.join(" ");
	return s2.replace(ix + match.capturedLength(), nix, j);
}

int GedaElementLexer::lex()
{
	while (true) {
		QRegularExpressionMatch match;
		if (m_source.indexOf(m_hexMatcher, m_pos - 1, &match) == m_pos - 1) {
			bool ok;
			m_currentString = match.captured(0);
			m_currentNumber = m_currentString.toLong(&ok, 16);
			m_pos += match.capturedLength() - 1;
			if (m_source.at(m_pos).isSpace()) {
				m_pos++;
			}
			next();
			return GedaElementGrammar::HEXNUMBER;
		}
		else if (m_source.indexOf(m_integerMatcher, m_pos - 1, &match) == m_pos - 1) {
			m_currentString = match.captured(0);
			m_currentNumber = m_currentString.toLong();
			m_pos += match.capturedLength() - 1;
			if (m_source.at(m_pos).isSpace()) {
				m_pos++;
			}
			next();
			return GedaElementGrammar::NUMBER;
		}
		else if (m_source.indexOf(m_stringMatcher, m_pos - 1, &match) == m_pos - 1) {
			m_currentString = match.captured(0);
			m_pos += match.capturedLength() - 1;
			next();
			return GedaElementGrammar::STRING;
		}
		else if (m_current.isSpace()) {
			next();
			continue;
		}
		else if (m_current.isNull()) {
			return GedaElementGrammar::EOF_SYMBOL;
		}
		else if (m_current == QLatin1Char('(')) {
			next();
			return GedaElementGrammar::LEFTPAREN;
		}
		else if (m_current == QLatin1Char(')')) {
			next();
			return GedaElementGrammar::RIGHTPAREN;
		}
		else if (m_current == QLatin1Char('[')) {
			next();
			return GedaElementGrammar::LEFTBRACKET;
		}
		else if (m_current == QLatin1Char(']')) {
			next();
			return GedaElementGrammar::RIGHTBRACKET;
		}
		else if (m_source.indexOf("elementline", m_pos - 1, Qt::CaseInsensitive) == m_pos - 1) {
			m_currentCommand = "elementline";
			m_pos += m_currentCommand.length() - 1;
			next();
			return GedaElementGrammar::ELEMENTLINE;
		}
		else if (m_source.indexOf("elementarc", m_pos - 1, Qt::CaseInsensitive) == m_pos - 1) {
			m_currentCommand = "elementarc";
			m_pos += m_currentCommand.length() - 1;
			next();
			return GedaElementGrammar::ELEMENTARC;
		}
		else if (m_source.indexOf("attribute", m_pos - 1, Qt::CaseInsensitive) == m_pos - 1) {
			m_currentCommand = "attribute";
			m_pos += m_currentCommand.length() - 1;
			next();
			return GedaElementGrammar::ATTRIBUTE;
		}
		else if (m_source.indexOf("element", m_pos - 1, Qt::CaseInsensitive) == m_pos - 1) {
			m_currentCommand = "element";
			m_pos += m_currentCommand.length() - 1;
			next();
			return GedaElementGrammar::ELEMENT;
		}
		else if (m_source.indexOf("pad", m_pos - 1, Qt::CaseInsensitive) == m_pos - 1) {
			m_currentCommand = "pad";
			m_pos += m_currentCommand.length() - 1;
			next();
			return GedaElementGrammar::PAD;
		}
		else if (m_source.indexOf("pin", m_pos - 1, Qt::CaseInsensitive) == m_pos - 1) {
			m_currentCommand = "pin";
			m_pos += m_currentCommand.length() - 1;
			next();
			return GedaElementGrammar::PIN;
		}
		else if (m_source.indexOf("mark", m_pos - 1, Qt::CaseInsensitive) == m_pos - 1) {
			m_currentCommand = "mark";
			m_pos += m_currentCommand.length() - 1;
			next();
			return GedaElementGrammar::MARK;
		}


		return -1;
	}
}

QChar GedaElementLexer::next()
{
	if (m_pos < m_size)
		m_current = m_chars[m_pos++];
	else
		m_current = QChar();
	return m_current;
}

QString GedaElementLexer::currentCommand() {
	return m_currentCommand;
}

double GedaElementLexer::currentNumber() {
	return m_currentNumber;
}

QString GedaElementLexer::currentString() {
	return m_currentString;
}

const QStringList & GedaElementLexer::comments() {
	return m_comments;
}
