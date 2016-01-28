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

$Revision: 6385 $:
$Author: cohen@irascible.com $:
$Date: 2012-09-08 21:21:20 +0200 (Sa, 08. Sep 2012) $

********************************************************************/


#ifndef KICADMODULEDIALOG_H
#define KICADMODULEDIALOG_H

#include <QDialog>
#include <QComboBox>

class KicadModuleDialog : public QDialog
{
Q_OBJECT

public:
	KicadModuleDialog(const QString & partType, const QString & filename, const QStringList & moduleNames, QWidget *parent = 0);
	~KicadModuleDialog();

	const QString selectedModule();

protected:
	QComboBox * m_comboBox;
};

#endif 
