/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2026 Fritzing GmbH

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

#include "FProbeMcpSketch.h"
#include "mainwindow.h"

#include "commands.h"
#include "waitpushundostack.h"
#include "viewgeometry.h"
#include "sketch/sketchwidget.h"
#include "model/modelpart.h"
#include "referencemodel/referencemodel.h"
#include "connectors/connector.h"
#include "connectors/connectoritem.h"
#include "items/itembase.h"
#include "items/wire.h"
#include "items/virtualwire.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QMetaObject>
#include <QThread>
#include <QDir>
#include <QTemporaryFile>
#include <QImageReader>
#include <QGraphicsScene>
#include <QTextDocumentFragment>

#include <algorithm>

// Part descriptions are rich HTML; MCP clients want short plain text.
static QString plainDescription(const QString & html, int maxLength = 200) {
	QString text = QTextDocumentFragment::fromHtml(html).toPlainText().simplified();
	if (text.length() > maxLength) {
		text = text.left(maxLength - 1) + QChar(0x2026);
	}
	return text;
}

static QJsonObject mcpError(const QString & code, const QString & detail) {
	QJsonObject obj;
	obj.insert("ok", false);
	obj.insert("error", code);
	obj.insert("detail", detail);
	return obj;
}

FProbeMcpSketch::FProbeMcpSketch(MainWindow * mainWindow, ReferenceModel * referenceModel) :
	FProbe("McpSketch"),
	m_mainWindow(mainWindow),
	m_referenceModel(referenceModel)
{
}

QVariant FProbeMcpSketch::read() {
	QJsonObject obj;
	obj.insert("application", "Fritzing");
	obj.insert("probe", "McpSketch");
	QJsonArray tools;
	for (const QString & tool : { "search_parts", "get_part_info", "add_part", "connect",
	                              "move", "rotate", "delete", "get_sketch_state",
	                              "get_connectors", "export_image", "save_sketch", "open_sketch" }) {
		tools.append(tool);
	}
	obj.insert("tools", tools);
	return QVariant(QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)));
}

QVariant FProbeMcpSketch::call(QVariant params) {
	QJsonParseError parseError;
	QJsonDocument doc = QJsonDocument::fromJson(params.toString().toUtf8(), &parseError);
	QJsonObject response;
	if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
		response = mcpError("bad_args", QString("request is not a JSON object: %1").arg(parseError.errorString()));
	}
	else if (!m_mainWindow) {
		response = mcpError("no_sketch", "main window is gone");
	}
	else if (QThread::currentThread() == m_mainWindow->thread()) {
		response = dispatch(doc.object());
	}
	else {
		// Runs on the FTestingServerThread; all sketch access must happen on
		// the GUI thread.
		QJsonObject request = doc.object();
		QMetaObject::invokeMethod(m_mainWindow, [this, request]() {
			return dispatch(request);
		}, Qt::BlockingQueuedConnection, &response);
	}
	return QVariant(QString::fromUtf8(QJsonDocument(response).toJson(QJsonDocument::Compact)));
}

QJsonObject FProbeMcpSketch::dispatch(const QJsonObject & request) {
	QString tool = request.value("tool").toString();
	QJsonObject args = request.value("args").toObject();

	if (tool == "search_parts") return toolSearchParts(args);
	if (tool == "get_part_info") return toolGetPartInfo(args);
	if (tool == "add_part") return toolAddPart(args);
	if (tool == "connect") return toolConnect(args);
	if (tool == "move") return toolMove(args);
	if (tool == "rotate") return toolRotate(args);
	if (tool == "delete") return toolDelete(args);
	if (tool == "get_sketch_state") return toolGetSketchState(args);
	if (tool == "get_connectors") return toolGetConnectors(args);
	if (tool == "export_image") return toolExportImage(args);
	if (tool == "save_sketch") return toolSaveSketch(args);
	if (tool == "open_sketch") return toolOpenSketch(args);

	return mcpError("bad_args", QString("unknown tool '%1'").arg(tool));
}

SketchWidget * FProbeMcpSketch::breadboardView() {
	if (!m_mainWindow) return nullptr;
	Q_FOREACH (SketchWidget * sketchWidget, m_mainWindow->sketchWidgets()) {
		if (sketchWidget && sketchWidget->viewID() == ViewLayer::BreadboardView) return sketchWidget;
	}
	return nullptr;
}

ItemBase * FProbeMcpSketch::findItemOrError(long id, QJsonObject & error) {
	SketchWidget * bb = breadboardView();
	if (!bb) {
		error = mcpError("no_sketch", "no breadboard view");
		return nullptr;
	}
	ItemBase * itemBase = bb->findItem(id);
	if (!itemBase) {
		error = mcpError("item_not_found", QString("no item with id %1").arg(id));
	}
	return itemBase;
}

void FProbeMcpSketch::selectOnly(SketchWidget * view, ItemBase * itemBase) {
	view->scene()->clearSelection();
	itemBase->setSelected(true);
}

void FProbeMcpSketch::settle(SketchWidget * view) {
	WaitPushUndoStack * undoStack = view->undoStack();
	if (undoStack) undoStack->waitForTimers();
}

QJsonObject FProbeMcpSketch::toolSearchParts(const QJsonObject & args) {
	QString query = args.value("query").toString();
	if (query.isEmpty()) return mcpError("bad_args", "missing 'query'");
	int maxResults = args.value("max_results").toInt(10);
	if (!m_referenceModel) return mcpError("no_sketch", "no parts library");

	// Rank matches so that "LED" surfaces the core LED part before boards
	// that merely mention LEDs in their description.
	QList<ModelPart *> matches = m_referenceModel->search(query, false);
	auto rank = [&query](ModelPart * modelPart) {
		QString title = modelPart->title();
		if (title.compare(query, Qt::CaseInsensitive) == 0) return 0;
		if (title.startsWith(query, Qt::CaseInsensitive)) return 1;
		if (title.contains(query, Qt::CaseInsensitive)) return 2;
		if (modelPart->family().contains(query, Qt::CaseInsensitive)) return 3;
		return 4;
	};
	std::stable_sort(matches.begin(), matches.end(), [&rank](ModelPart * a, ModelPart * b) {
		return rank(a) < rank(b);
	});

	QJsonArray results;
	Q_FOREACH (ModelPart * modelPart, matches) {
		if (results.count() >= maxResults) break;
		if (!modelPart) continue;
		QJsonObject part;
		part.insert("module_id", modelPart->moduleID());
		part.insert("title", modelPart->title());
		part.insert("description", plainDescription(modelPart->description()));
		part.insert("family", modelPart->family());
		results.append(part);
	}

	QJsonObject obj;
	obj.insert("ok", true);
	obj.insert("results", results);
	return obj;
}

QJsonObject FProbeMcpSketch::toolGetPartInfo(const QJsonObject & args) {
	QString moduleID = args.value("module_id").toString();
	if (moduleID.isEmpty()) return mcpError("bad_args", "missing 'module_id'");
	if (!m_referenceModel) return mcpError("no_sketch", "no parts library");

	ModelPart * modelPart = m_referenceModel->retrieveModelPart(moduleID);
	if (!modelPart) return mcpError("part_not_found", QString("no part with module id '%1'").arg(moduleID));

	QJsonObject obj;
	obj.insert("ok", true);
	obj.insert("module_id", modelPart->moduleID());
	obj.insert("title", modelPart->title());
	obj.insert("description", plainDescription(modelPart->description(), 500));
	obj.insert("family", modelPart->family());

	QJsonObject properties;
	QHashIterator<QString, QString> it(modelPart->properties());
	while (it.hasNext()) {
		it.next();
		properties.insert(it.key(), it.value());
	}
	obj.insert("properties", properties);

	QJsonArray connectors;
	QStringList connectorIDs = modelPart->connectors().keys();
	connectorIDs.sort();
	Q_FOREACH (const QString & connectorID, connectorIDs) {
		Connector * connector = modelPart->connectors().value(connectorID);
		if (!connector) continue;
		QJsonObject connectorObj;
		connectorObj.insert("id", connectorID);
		connectorObj.insert("name", connector->connectorSharedName());
		connectorObj.insert("description", connector->connectorSharedDescription());
		connectors.append(connectorObj);
	}
	obj.insert("connectors", connectors);
	return obj;
}

QJsonObject FProbeMcpSketch::toolAddPart(const QJsonObject & args) {
	QString moduleID = args.value("module_id").toString();
	if (moduleID.isEmpty()) return mcpError("bad_args", "missing 'module_id'");
	if (!args.contains("x") || !args.contains("y")) return mcpError("bad_args", "missing 'x' or 'y'");
	double x = args.value("x").toDouble();
	double y = args.value("y").toDouble();
	double rotation = args.value("rotation").toDouble(0);

	SketchWidget * bb = breadboardView();
	if (!bb) return mcpError("no_sketch", "no breadboard view");
	if (!m_referenceModel || !m_referenceModel->retrieveModelPart(moduleID)) {
		return mcpError("part_not_found", QString("no part with module id '%1'").arg(moduleID));
	}

	long id = bb->putItemByModuleID(moduleID, QPointF(x, y));
	if (id < 0) {
		return mcpError("bad_args", QString("part '%1' cannot be dropped in the breadboard view").arg(moduleID));
	}
	settle(bb);

	ItemBase * itemBase = bb->findItem(id);
	if (!itemBase) return mcpError("item_not_found", "part was dropped but cannot be found afterwards");

	if (rotation != 0) {
		selectOnly(bb, itemBase);
		bb->rotateX(rotation, false, nullptr);
		settle(bb);
	}

	QPointF center = itemBase->sceneBoundingRect().center();
	QJsonObject obj;
	obj.insert("ok", true);
	obj.insert("item_id", (double) id);
	obj.insert("title", itemBase->title());
	obj.insert("refdes", itemBase->instanceTitle());
	obj.insert("x", center.x());
	obj.insert("y", center.y());
	return obj;
}

QJsonObject FProbeMcpSketch::toolConnect(const QJsonObject & args) {
	SketchWidget * bb = breadboardView();
	if (!bb) return mcpError("no_sketch", "no breadboard view");

	QJsonObject error;
	ItemBase * fromItem = findItemOrError((long) args.value("from_item_id").toDouble(-1), error);
	if (!fromItem) return error;
	ItemBase * toItem = findItemOrError((long) args.value("to_item_id").toDouble(-1), error);
	if (!toItem) return error;

	auto findConnector = [](ItemBase * itemBase, const QString & connectorID, QJsonObject & error) -> ConnectorItem * {
		ConnectorItem * connectorItem = itemBase->findConnectorItemWithSharedID(connectorID);
		if (!connectorItem) {
			QJsonArray available;
			Q_FOREACH (ConnectorItem * ci, itemBase->cachedConnectorItems()) {
				if (available.count() >= 100) break;
				available.append(ci->connectorSharedID());
			}
			error = mcpError("connector_not_found",
				QString("item %1 ('%2') has no connector '%3'").arg(itemBase->id()).arg(itemBase->title()).arg(connectorID));
			error.insert("available", available);
		}
		return connectorItem;
	};

	ConnectorItem * from = findConnector(fromItem, args.value("from_connector").toString(), error);
	if (!from) return error;
	ConnectorItem * to = findConnector(toItem, args.value("to_connector").toString(), error);
	if (!to) return error;

	auto * parentCommand = new QUndoCommand(QObject::tr("MCP: create wire"));
	new CleanUpWiresCommand(bb, CleanUpWiresCommand::UndoOnly, parentCommand);
	new CleanUpRatsnestsCommand(bb, CleanUpWiresCommand::UndoOnly, parentCommand);
	long wireID = bb->createWire(from, to, ViewGeometry::NormalFlag, false, BaseCommand::CrossView, parentCommand);
	if (wireID < 0) {
		delete parentCommand;
		return mcpError("bad_args", "could not create wire");
	}
	QString color = args.value("color").toString();
	if (!color.isEmpty()) {
		new WireColorChangeCommand(bb, wireID, color, color, 1.0, 1.0, parentCommand);
	}
	new CleanUpRatsnestsCommand(bb, CleanUpWiresCommand::RedoOnly, parentCommand);
	new CleanUpWiresCommand(bb, CleanUpWiresCommand::RedoOnly, parentCommand);
	bb->undoStack()->push(parentCommand);
	settle(bb);

	QJsonObject obj;
	obj.insert("ok", true);
	obj.insert("wire_id", (double) wireID);
	return obj;
}

QJsonObject FProbeMcpSketch::toolMove(const QJsonObject & args) {
	SketchWidget * bb = breadboardView();
	if (!bb) return mcpError("no_sketch", "no breadboard view");
	if (!args.contains("x") || !args.contains("y")) return mcpError("bad_args", "missing 'x' or 'y'");

	QJsonObject error;
	ItemBase * itemBase = findItemOrError((long) args.value("item_id").toDouble(-1), error);
	if (!itemBase) return error;

	itemBase->saveGeometry();
	ViewGeometry oldGeometry = itemBase->getViewGeometry();
	ViewGeometry newGeometry = oldGeometry;
	QPointF delta = QPointF(args.value("x").toDouble(), args.value("y").toDouble()) - itemBase->sceneBoundingRect().center();
	newGeometry.setLoc(oldGeometry.loc() + delta);

	auto * parentCommand = new QUndoCommand(QObject::tr("MCP: move item"));
	new CleanUpWiresCommand(bb, CleanUpWiresCommand::UndoOnly, parentCommand);
	new CleanUpRatsnestsCommand(bb, CleanUpWiresCommand::UndoOnly, parentCommand);
	new MoveItemCommand(bb, itemBase->id(), oldGeometry, newGeometry, true, parentCommand);
	new CleanUpRatsnestsCommand(bb, CleanUpWiresCommand::RedoOnly, parentCommand);
	new CleanUpWiresCommand(bb, CleanUpWiresCommand::RedoOnly, parentCommand);
	bb->undoStack()->push(parentCommand);
	settle(bb);

	QPointF center = itemBase->sceneBoundingRect().center();
	QJsonObject obj;
	obj.insert("ok", true);
	obj.insert("x", center.x());
	obj.insert("y", center.y());
	return obj;
}

QJsonObject FProbeMcpSketch::toolRotate(const QJsonObject & args) {
	SketchWidget * bb = breadboardView();
	if (!bb) return mcpError("no_sketch", "no breadboard view");

	QJsonObject error;
	ItemBase * itemBase = findItemOrError((long) args.value("item_id").toDouble(-1), error);
	if (!itemBase) return error;

	double degrees = args.value("degrees").toDouble();
	selectOnly(bb, itemBase);
	bb->rotateX(degrees, false, nullptr);
	settle(bb);

	QJsonObject obj;
	obj.insert("ok", true);
	return obj;
}

QJsonObject FProbeMcpSketch::toolDelete(const QJsonObject & args) {
	SketchWidget * bb = breadboardView();
	if (!bb) return mcpError("no_sketch", "no breadboard view");

	QJsonObject error;
	ItemBase * itemBase = findItemOrError((long) args.value("item_id").toDouble(-1), error);
	if (!itemBase) return error;

	selectOnly(bb, itemBase);
	bb->deleteSelected(nullptr, false);
	settle(bb);

	QJsonObject obj;
	obj.insert("ok", true);
	return obj;
}

QJsonObject FProbeMcpSketch::toolGetSketchState(const QJsonObject & args) {
	Q_UNUSED(args);
	SketchWidget * bb = breadboardView();
	if (!bb) return mcpError("no_sketch", "no breadboard view");

	QJsonArray parts;
	QJsonArray wires;
	int ratsnestCount = 0;

	Q_FOREACH (QGraphicsItem * graphicsItem, bb->scene()->items()) {
		auto * itemBase = dynamic_cast<ItemBase *>(graphicsItem);
		if (!itemBase) continue;
		if (itemBase->layerKinChief() != itemBase) continue;

		auto * wire = dynamic_cast<Wire *>(itemBase);
		if (wire) {
			if (dynamic_cast<VirtualWire *>(wire)) {
				ratsnestCount++;
				continue;
			}
			QJsonObject wireObj;
			wireObj.insert("wire_id", (double) wire->id());
			wireObj.insert("color", wire->colorString());
			const char * ends[] = { "from", "to" };
			ConnectorItem * wireConnectors[] = { wire->connector0(), wire->connector1() };
			for (int i = 0; i < 2; i++) {
				QJsonObject endObj;
				if (wireConnectors[i] && !wireConnectors[i]->connectedToItems().isEmpty()) {
					ConnectorItem * other = wireConnectors[i]->connectedToItems().first();
					if (other && other->attachedTo()) {
						endObj.insert("item_id", (double) other->attachedTo()->id());
						endObj.insert("connector", other->connectorSharedID());
					}
				}
				wireObj.insert(ends[i], endObj);
			}
			wires.append(wireObj);
			continue;
		}

		QJsonObject partObj;
		partObj.insert("item_id", (double) itemBase->id());
		if (itemBase->modelPart()) {
			partObj.insert("module_id", itemBase->modelPart()->moduleID());
		}
		partObj.insert("title", itemBase->title());
		partObj.insert("refdes", itemBase->instanceTitle());
		QPointF center = itemBase->sceneBoundingRect().center();
		partObj.insert("x", center.x());
		partObj.insert("y", center.y());
		partObj.insert("connector_count", itemBase->cachedConnectorItems().count());

		QJsonArray connections;
		Q_FOREACH (ConnectorItem * connectorItem, itemBase->cachedConnectorItems()) {
			if (connectorItem->connectedToItems().isEmpty()) continue;
			QJsonObject connectionObj;
			connectionObj.insert("connector", connectorItem->connectorSharedID());
			QJsonArray connectedTo;
			Q_FOREACH (ConnectorItem * other, connectorItem->connectedToItems()) {
				if (!other || !other->attachedTo()) continue;
				if (dynamic_cast<VirtualWire *>(other->attachedTo())) continue;
				QJsonObject otherObj;
				otherObj.insert("item_id", (double) other->attachedTo()->id());
				otherObj.insert("connector", other->connectorSharedID());
				connectedTo.append(otherObj);
			}
			if (connectedTo.isEmpty()) continue;
			connectionObj.insert("connected_to", connectedTo);
			connections.append(connectionObj);
		}
		partObj.insert("connections", connections);
		parts.append(partObj);
	}

	QJsonObject obj;
	obj.insert("ok", true);
	obj.insert("parts", parts);
	obj.insert("wires", wires);
	obj.insert("ratsnest_wires", ratsnestCount);
	return obj;
}

QJsonObject FProbeMcpSketch::toolGetConnectors(const QJsonObject & args) {
	QJsonObject error;
	ItemBase * itemBase = findItemOrError((long) args.value("item_id").toDouble(-1), error);
	if (!itemBase) return error;

	QString idPrefix = args.value("id_prefix").toString();
	QJsonArray connectors;
	Q_FOREACH (ConnectorItem * connectorItem, itemBase->cachedConnectorItems()) {
		if (!idPrefix.isEmpty() && !connectorItem->connectorSharedID().startsWith(idPrefix)) continue;
		QJsonObject connectorObj;
		connectorObj.insert("id", connectorItem->connectorSharedID());
		connectorObj.insert("name", connectorItem->connectorSharedName());
		QPointF p = connectorItem->sceneAdjustedTerminalPoint(nullptr);
		connectorObj.insert("x", p.x());
		connectorObj.insert("y", p.y());
		connectors.append(connectorObj);
	}

	QJsonObject obj;
	obj.insert("ok", true);
	obj.insert("connectors", connectors);
	return obj;
}

QJsonObject FProbeMcpSketch::toolExportImage(const QJsonObject & args) {
	if (!m_mainWindow) return mcpError("no_sketch", "main window is gone");

	QString view = args.value("view").toString("breadboard");
	ViewLayer::ViewID viewID;
	if (view == "breadboard") viewID = ViewLayer::BreadboardView;
	else if (view == "schematic") viewID = ViewLayer::SchematicView;
	else if (view == "pcb") viewID = ViewLayer::PCBView;
	else return mcpError("bad_args", QString("unknown view '%1', expected breadboard|schematic|pcb").arg(view));

	QString format = args.value("format").toString("png");
	if (format != "png" && format != "svg") {
		return mcpError("bad_args", QString("unknown format '%1', expected png|svg").arg(format));
	}
	int dpi = args.value("dpi").toInt(300);

	QString path = args.value("path").toString();
	if (path.isEmpty()) {
		QTemporaryFile tempFile(QDir::temp().filePath(QString("fritzing-mcp-XXXXXX.%1").arg(format)));
		tempFile.setAutoRemove(false);
		if (!tempFile.open()) return mcpError("export_failed", "cannot create temporary file");
		path = tempFile.fileName();
	}

	m_mainWindow->setCurrentView(viewID);

	if (format == "svg") {
		m_mainWindow->exportSvg(dpi, false, false, path);
	}
	else {
		QString exportError;
		if (!m_mainWindow->exportImageHeadless(path, dpi, QImage::Format_ARGB32, 1, true, &exportError)) {
			return mcpError("export_failed", exportError);
		}
	}

	QJsonObject obj;
	obj.insert("ok", true);
	obj.insert("path", path);
	QImageReader reader(path);
	if (reader.canRead()) {
		QSize size = reader.size();
		obj.insert("width", size.width());
		obj.insert("height", size.height());
	}
	return obj;
}

QJsonObject FProbeMcpSketch::toolSaveSketch(const QJsonObject & args) {
	if (!m_mainWindow) return mcpError("no_sketch", "main window is gone");
	QString path = args.value("path").toString();
	if (path.isEmpty()) return mcpError("bad_args", "missing 'path'");

	if (!m_mainWindow->saveAsAux(path)) {
		return mcpError("export_failed", QString("could not save to %1").arg(path));
	}
	QJsonObject obj;
	obj.insert("ok", true);
	obj.insert("path", path);
	return obj;
}

QJsonObject FProbeMcpSketch::toolOpenSketch(const QJsonObject & args) {
	if (!m_mainWindow) return mcpError("no_sketch", "main window is gone");
	QString path = args.value("path").toString();
	if (path.isEmpty()) return mcpError("bad_args", "missing 'path'");
	if (!QFile::exists(path)) return mcpError("bad_args", QString("no such file: %1").arg(path));

	if (!m_mainWindow->loadWhich(path, false, false, false, path)) {
		return mcpError("bad_args", QString("could not load %1").arg(path));
	}
	QJsonObject obj;
	obj.insert("ok", true);
	return obj;
}
