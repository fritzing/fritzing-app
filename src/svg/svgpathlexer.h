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

#ifndef SVGPATHLEXER_H
#define SVGPATHLEXER_H

#include <QtCore/QString>
#include <QtCore/QHash>
#include <QRegExp>
#include <QDomElement>
#include <QMatrix>

class SVGPathLexer
{
public:
	SVGPathLexer(const QString &source);
	~SVGPathLexer();
	int lex();
	QChar currentCommand();
	double currentNumber();

public:
	static constexpr char FakeClosePathChar = 'x';

protected:
	QChar next();
	QString clean(const QString & source);

protected:
	QString m_source;
	const QChar *m_chars = nullptr;
	int m_size = 0;
	int m_pos = 0;
	QChar m_current = 0;
	QChar m_currentCommand = 0;
	double m_currentNumber = 0.0;
};

#endif
