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

#ifndef GEDAELEMENTLEXER_H
#define GEDAELEMENTLEXER_H

#include <QString>
#include <QStringList>
#include <QHash>
#include <QRegExp>

class GedaElementLexer
{
public:
	GedaElementLexer(const QString &source);
	~GedaElementLexer() = default;
	int lex();
	QString currentCommand();
	double currentNumber();
	QString currentString();
	const QStringList & comments();

protected:
	QChar next();
	QString clean(const QString & source);

protected:
	QRegExp m_nonWhitespaceMatcher;
	QRegExp m_commentMatcher;
	QRegExp m_elementMatcher;
	QRegExp m_stringMatcher;
	QRegExp m_integerMatcher;
	QRegExp m_hexMatcher;
	QString m_source;
	const QChar *m_chars = nullptr;
	int m_size = 0;
	int m_pos = 0;
	QChar m_current;
	QString m_currentCommand;
	long m_currentNumber = 0l;
	long m_currentHexString = 0l;
	QString m_currentString;
	QStringList m_comments;
};

#endif
