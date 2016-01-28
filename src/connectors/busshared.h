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

$Revision: 6309 $:
$Author: cohen@irascible.com $:
$Date: 2012-08-17 11:16:43 +0200 (Fr, 17. Aug 2012) $

********************************************************************/

#ifndef BUSSHARED_H
#define BUSSHARED_H

#include <QString>
#include <QDomElement>
#include <QHash>
#include <QList>
#include <QXmlStreamWriter>
#include <QPointer>

class BusShared {
	
public:
	BusShared(const QDomElement & busElement, const QHash<QString, QPointer<class ConnectorShared> > & connectorHash);
	BusShared(const QString & id);

	const QString & id() const;
	const QList<class ConnectorShared *> & connectors();
	void addConnectorShared(class ConnectorShared *);

protected:
	void initConnector(QDomElement & connector, const QHash<QString, QPointer<class ConnectorShared> > & connectorHash);

protected:
	QString m_id;
	QList<class ConnectorShared *> m_connectors;
};

#endif
