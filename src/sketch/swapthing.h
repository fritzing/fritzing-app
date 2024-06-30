#ifndef SWAPTHING_H
#define SWAPTHING_H

#include <QString>
#include <QHash>
#include <QUndoCommand>

#include "viewlayer.h"

class ConnectorItem;
class Connector;
class ChangeConnectionCommand;
class ItemBase;
class SketchWidget;
class SubpartSwapManager;
class Wire;

class SwapThing
{
public:
	SwapThing();

	bool firstTime;
	long newID;
	ItemBase * itemBase;
	long newModelIndex;
	QString newModuleID;
	QSharedPointer<SubpartSwapManager> subpartSwapManager;
	ViewLayer::ViewLayerPlacement viewLayerPlacement;
	QList<Wire *> wiresToDelete;
	QUndoCommand * parentCommand;
	QHash<ConnectorItem *, ChangeConnectionCommand *> reconnections;
	QHash<ConnectorItem *, Connector *> byWire;
	QHash<ConnectorItem *, ConnectorItem *> toConnectorItems;
	QHash<ConnectorItem *, Connector *> swappedGender;
	SketchWidget * bbView;
	QMap<QString, QString> propsMap;
};

#endif // SWAPTHING_H
