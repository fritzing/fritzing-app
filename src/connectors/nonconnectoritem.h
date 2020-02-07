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

#ifndef NONCONNECTORITEM_H
#define NONCONNECTORITEM_H

#include <QGraphicsRectItem>
#include <QGraphicsSceneHoverEvent>
#include <QPen>
#include <QBrush>
#include <QXmlStreamWriter>
#include <QPointer>

#include "../items/itembase.h"

class NonConnectorItem : public QObject, public QGraphicsRectItem
{
	Q_OBJECT

public:
	NonConnectorItem(ItemBase* attachedTo);
	~NonConnectorItem() = default;

	ItemBase * attachedTo() const noexcept { return m_attachedTo; }
	virtual void setHidden(bool hidden);
	constexpr bool hidden() const noexcept { return m_hidden; }
	virtual void setInactive(bool inactivate);
	constexpr bool inactive() const noexcept { return m_inactive; }
	virtual void setLayerHidden(bool hidden);
	constexpr bool layerHidden() const noexcept { return m_layerHidden; }
	long attachedToID();
	const QString & attachedToTitle();
	const QString & attachedToInstanceTitle();
	void setCircular(bool);
	void setRadius(double radius, double strokeWidth);
	constexpr double radius() const noexcept { return m_radius; }
	constexpr double strokeWidth() const noexcept { return m_strokeWidth; }
	void setShape(QPainterPath &);
	void paint( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );
	QPainterPath shape() const;
	void setIsPath(bool);
	constexpr bool isPath() const noexcept { return m_isPath; }
	int attachedToItemType();

protected:
	constexpr bool doNotPaint() const noexcept { 
        return (m_hidden || m_inactive || !m_paint || m_layerHidden);
    }

	enum Effectively {
		EffectivelyCircular = 1,
		EffectivelyRectangular,
		EffectivelyPolygonal,
		EffectivelyUnknown
	};

protected:
	QPointer<ItemBase> m_attachedTo;
	bool m_hidden = false;
	bool m_layerHidden = false;
	bool m_inactive = false;
	bool m_paint = false;
	double m_opacity = 0.0;
	bool m_circular = false;
	Effectively m_effectively = Effectively::EffectivelyUnknown;
	double m_radius = 0.0;
	double m_strokeWidth = 0.0;
	double m_negativePenWidth = 0.0;
	bool m_negativeOffsetRect = false;
	QPainterPath m_shape;
	bool m_isPath = false;

};

#endif
