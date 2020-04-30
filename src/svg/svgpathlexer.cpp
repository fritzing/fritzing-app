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

#include "svgpathlexer.h"
#include "svgpathgrammar_p.h"
#include "../utils/textutils.h"

static const QRegExp findWhitespaceBefore(" ([AaCcMmVvTtQqSsLlVvHhZzx,])");
static const QRegExp findWhitespaceAfter("([AaCcMmVvTtQqSsLlVvHhZz,]) ");
static const QRegExp findWhitespaceAtEnd(" $");
static const QRegExp findMinus("-");

SVGPathLexer::SVGPathLexer(const QString &source)
{
	m_source = clean(source);
	m_chars = m_source.unicode();
	m_size = m_source.size();
	m_pos = 0;
	m_current = next();
}

SVGPathLexer::~SVGPathLexer()
{
}

/**
 * Clean up svg path element data to make parsing simpler
 *
 * Mostly this removes extra whitespace, but it also adds a space before any minus (-)
 * character. The attribute specification allows a negative number (leading minus sign
 * to follow another number without any intervening separator. The minus sign acts as
 * the separator. However (later) grammer processing does not recognized that sequence,
 * at least for the "a" command. Inserting a leading space avoids the failing parse
 * sequence, and does not change anything else. The extra space will get removed again
 * by the existing (here) preprocessing, for all cases except
 *   `number (space) negative number`
 * @brief remove unneeded whitespaces from `d` attribute data
 * @param source original svg path data
 * @return sanitized svg path element data string
 */
QString SVGPathLexer::clean(const QString & source) {
	QString s1 = source;
	s1.replace(findMinus, " -");  // Patch (not fix) for #3647 bug: add space before "-" characters
	s1.replace(TextUtils::FindWhitespace, " "); // collapse multiple whitespace to single space
	s1.replace(findWhitespaceBefore, "\\1");  // remove space before command or comma
	s1.replace(findWhitespaceAfter, "\\1");   // remove space after command or comma
	s1.replace(findWhitespaceAtEnd, "");      // remove trailing space
	return s1;
}

int SVGPathLexer::lex()
{
	if (m_current.isNull()) {
		// Do this first, to prevent infinite loop when last content of the path data is a number
		return SVGPathGrammar::EOF_SYMBOL;
	}
	if (TextUtils::floatingPointMatcher.indexIn(m_source, m_pos - 1) == m_pos - 1) {
		// sitting at the start of a number: collect and advance past it
		m_currentNumber = m_source.mid(m_pos - 1, TextUtils::floatingPointMatcher.matchedLength()).toDouble();
		m_pos += TextUtils::floatingPointMatcher.matchedLength() - 1;
		next();
		return SVGPathGrammar::NUMBER;
	}

	if (m_current.isSpace()) {
		next();
		return SVGPathGrammar::WHITESPACE;
	}
	else if (m_current == QLatin1Char('V')) {
		m_currentCommand = m_current;
		next();
		return SVGPathGrammar::VEE;
	}
	else if (m_current == QLatin1Char('v')) {
		m_currentCommand = m_current;
		next();
		return SVGPathGrammar::VEE;
	}
	else if (m_current == QLatin1Char('T')) {
		m_currentCommand = m_current;
		next();
		return SVGPathGrammar::TEE;
	}
	else if (m_current == QLatin1Char('t')) {
		m_currentCommand = m_current;
		next();
		return SVGPathGrammar::TEE;
	}
	else if (m_current == QLatin1Char(FakeClosePathChar)) {
		m_currentCommand = m_current;
		next();
		return SVGPathGrammar::EKS;
	}
	else if (m_current == QLatin1Char('C')) {
		m_currentCommand = m_current;
		next();
		return SVGPathGrammar::CEE;
	}
	else if (m_current == QLatin1Char('c')) {
		m_currentCommand = m_current;
		next();
		return SVGPathGrammar::CEE;
	}
	else if (m_current == QLatin1Char('Q')) {
		m_currentCommand = m_current;
		next();
		return SVGPathGrammar::KYU;
	}
	else if (m_current == QLatin1Char('q')) {
		m_currentCommand = m_current;
		next();
		return SVGPathGrammar::KYU;
	}
	else if (m_current == QLatin1Char('S')) {
		m_currentCommand = m_current;
		next();
		return SVGPathGrammar::ESS;
	}
	else if (m_current == QLatin1Char('s')) {
		m_currentCommand = m_current;
		next();
		return SVGPathGrammar::ESS;
	}
	else if (m_current == QLatin1Char('H')) {
		m_currentCommand = m_current;
		next();
		return SVGPathGrammar::AITCH;
	}
	else if (m_current == QLatin1Char('h')) {
		m_currentCommand = m_current;
		next();
		return SVGPathGrammar::AITCH;
	}
	else if (m_current == QLatin1Char('L')) {
		m_currentCommand = m_current;
		next();
		return SVGPathGrammar::EL;
	}
	else if (m_current == QLatin1Char('l')) {
		m_currentCommand = m_current;
		next();
		return SVGPathGrammar::EL;
	}
	else if (m_current == QLatin1Char('M')) {
		m_currentCommand = m_current;
		next();
		return SVGPathGrammar::EM;
	}
	else if (m_current == QLatin1Char('m')) {
		m_currentCommand = m_current;
		next();
		return SVGPathGrammar::EM;
	}
	else if (m_current == QLatin1Char('A')) {
		m_currentCommand = m_current;
		next();
		return SVGPathGrammar::AE;
	}
	else if (m_current == QLatin1Char('a')) {
		m_currentCommand = m_current;
		next();
		return SVGPathGrammar::AE;
	}
	else if (m_current == QLatin1Char('Z')) {
		m_currentCommand = m_current;
		next();
		return SVGPathGrammar::ZEE;
	}
	else if (m_current == QLatin1Char('z')) {
		m_currentCommand = m_current;
		next();
		return SVGPathGrammar::ZEE;
	}
	else if (m_current == QLatin1Char(',')) {
		next();
		return SVGPathGrammar::COMMA;
	}

	return -1; // the command character does not match any known path command
}

QChar SVGPathLexer::next()
{
	if (m_pos < m_size)
		m_current = m_chars[m_pos++];
	else
		m_current = QChar();
	return m_current;
}

QChar SVGPathLexer::currentCommand() {
	return m_currentCommand;
}

double SVGPathLexer::currentNumber() {
	return m_currentNumber;
}
