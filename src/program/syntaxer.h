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


#ifndef SYNTAXER_H_
#define SYNTAXER_H_

#include <QDomDocument>
#include <QObject>
#include <QHash>
#include <QStringList>

#include "trienode.h"

class CommentInfo
{
public:
	CommentInfo(const QString & start, const QString & end, Qt::CaseSensitivity);

public:
	bool m_multiLine = false;
	QString m_start;
	QString m_end;
	int m_index = 0;
	Qt::CaseSensitivity m_caseSensitive;
};

class Syntaxer : public QObject
{
	Q_OBJECT


public:
	Syntaxer();
	virtual ~Syntaxer();

	bool loadSyntax(const QString & filename);
	bool matches(const QString & string, TrieLeaf * & leaf);
	const CommentInfo * getCommentInfo(int ix);
	bool matchCommentStart(const QString & text, int offset, int & result, const CommentInfo * & resultCommentInfo);
	int matchStringStart(const QString & text, int offset);
	int matchStringEnd(const QString & text, int offset);
	const QString & extensionString();
	const QStringList & extensions();
	bool hlCStringChar();
	bool canProgram();

public:
	static QString parseForName(const QString & filename);
	static QString formatFromList(const QString & list);

protected:
	void loadList(QDomElement & list);
	QChar getStringDelimiter(QDomElement & context);
	void initListsToFormats(QDomElement & context);

protected:
	static QHash<QString, QString> m_listsToFormats;

protected:
	TrieNode * m_trieRoot = nullptr;
	QString m_name;
	QString m_extensionString;
	QStringList m_extensions;
	QList<CommentInfo *> m_commentInfo;
	QChar m_stringDelimiter = 0;
	bool m_hlCStringChar = false;
	bool m_canProgram = false;
};

class SyntaxerTrieLeaf : public TrieLeaf
{
public:
	SyntaxerTrieLeaf(QString name);
	~SyntaxerTrieLeaf();

	const QString & name();

protected:
	QString m_name;
};

#endif /* SYNTAXER_H_ */
