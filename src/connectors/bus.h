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

#ifndef BUS_H
#define BUS_H

#include <QString>
#include <QDomElement>
#include <QHash>
#include <QList>
#include <QXmlStreamWriter>
#include <QGraphicsScene>
#include <QPointer>

class BusShared;
class ModelPart;
class Connector;
class Bus : public QObject
{
	Q_OBJECT

public:
	Bus(BusShared *, ModelPart *);

	const QString & id() const noexcept;
	void addConnector(Connector *);
	ModelPart * modelPart() const noexcept;
	const QList<Connector *> & connectors() const noexcept;
	Connector * subConnector() const noexcept;
	void addSubConnector(Connector *);

protected:
	QList<Connector *> m_connectors;
	Connector * m_subConnector;
	BusShared * m_busShared;
	QPointer<ModelPart> m_modelPart;
};


#endif
