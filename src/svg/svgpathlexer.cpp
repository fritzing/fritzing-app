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

#include "svgpathlexer.h"
#include "svgpathgrammar_p.h"
#include "../utils/textutils.h"
#include <qdebug.h>

static const QRegExp findWhitespaceBefore(" ([AaCcMmVvTtQqSsLlVvHhZz,])");
static const QRegExp findWhitespaceAfter("([AaCcMmVvTtQqSsLlVvHhZz,]) ");
static const QRegExp findWhitespaceAtEnd(" $");

SVGPathLexer::SVGPathLexer(const QString &source)
{
    m_source = clean(source);
    m_chars = m_source.unicode();
    m_size = m_source.size();
	//qDebug() << m_source;
    m_pos = 0;
    m_current = next();
}

SVGPathLexer::~SVGPathLexer()
{
}

QString SVGPathLexer::clean(const QString & source) {
	// SVG path grammar is ambiguous, so clean it up to make it easier to parse
	// the rules are WSP* , WSP* ==> ,
	// WSP* command WSP* ==> command
	// WSP+ ==> WSP
	// where WSP is whitespace, command is one of the path commands such as M C V ...

	
	QString s1 = source;
	s1.replace(TextUtils::FindWhitespace, " ");
	s1.replace(findWhitespaceBefore, "\\1");    // replace with the captured character
	s1.replace(findWhitespaceAfter, "\\1");    // replace with the captured character
	s1.replace(findWhitespaceAtEnd, "");
	return s1;

}

int SVGPathLexer::lex()
{
	if (TextUtils::floatingPointMatcher.indexIn(m_source, m_pos - 1) == m_pos - 1) {
		m_currentNumber = m_source.mid(m_pos - 1, TextUtils::floatingPointMatcher.matchedLength()).toDouble();
		m_pos += TextUtils::floatingPointMatcher.matchedLength() - 1;
		next();
		return SVGPathGrammar::NUMBER;
	}

	if (m_current.isSpace()) {
		next();
		return SVGPathGrammar::WHITESPACE;
	}
	else if (m_current.isNull()) {
		return SVGPathGrammar::EOF_SYMBOL;
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

	
	return -1;
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

