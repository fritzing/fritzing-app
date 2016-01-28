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

$Revision: 6955 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-06 23:14:37 +0200 (Sa, 06. Apr 2013) $

********************************************************************/

#ifndef BUS_H
#define BUS_H

#include <QString>
#include <QDomElement>
#include <QHash>
#include <QList>
#include <QXmlStreamWriter>
#include <QGraphicsScene>
#include <QPointer>

class Bus : public QObject 
{
	Q_OBJECT
	
public:
	Bus(class BusShared *, class ModelPart *);
	
	const QString & id() const;
	void addConnector(class Connector *);
	class ModelPart * modelPart();
	const QList<Connector *> & connectors() const;
	Connector * subConnector() const;
    void addSubConnector(Connector *);
	
protected:
	QList<class Connector *> m_connectors;
	class Connector * m_subConnector;
	BusShared * m_busShared;
	QPointer<class ModelPart> m_modelPart;
};


#endif
