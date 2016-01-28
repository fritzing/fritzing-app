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



#ifndef PARTINFOWIDGET_H_
#define PARTINFOWIDGET_H_

#include <QWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QDateEdit>

#include "addremovelistwidget.h"
#include "../model/modelpart.h"
#include "../model/modelpartshared.h"

class PartInfoWidget : public QWidget {
	Q_OBJECT
	public:
		PartInfoWidget(QWidget *parent = 0);
		ModelPartShared* modelPartShared();

	public slots:
		void updateInfo(ModelPart *);

	protected:
		QLineEdit *m_version;
		QLineEdit *m_author;
		QLineEdit *m_title;
		//QLineEdit *m_taxonomy;
		QLineEdit *m_label;
		QTextEdit *m_description;

		QDateEdit *m_date;

		AddRemoveListWidget * m_tagsList;
		AddRemoveListWidget * m_propertyList;
};

#endif /* PARTINFOWIDGET_H_ */
