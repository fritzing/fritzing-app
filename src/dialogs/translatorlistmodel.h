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


#ifndef TRANSLATORLISTMODEL_H
#define TRANSLATORLISTMODEL_H

#include <QAbstractListModel>
#include <QFileInfoList>
#include <QLocale>
#include <QHash>

class TranslatorListModel : public  QAbstractListModel
{
	Q_OBJECT

public:
	TranslatorListModel(QFileInfoList &, QObject* parent = 0);
	~TranslatorListModel();

	QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
	int rowCount ( const QModelIndex & parent = QModelIndex() ) const ;

	const QLocale * locale(int index);
	int findIndex(const QString & language);

	static void cleanup();

protected:
	static QList<QLocale *> m_localeList;

	static QHash<QString, QString> m_languages;
};

#endif 
