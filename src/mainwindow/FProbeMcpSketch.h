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

#ifndef FPROBEMCPSKETCH_H
#define FPROBEMCPSKETCH_H

#include "testing/FProbe.h"

#include <QObject>
#include <QPointer>
#include <QJsonObject>
#include <QVariant>

class MainWindow;
class ReferenceModel;
class SketchWidget;
class ItemBase;

// Automation endpoint for the MCP (Model Context Protocol) bridge in
// tools/mcp/fritzing_mcp.py. Exposes sketch editing operations through the
// FTesting server's "call" verb:
//   GET /McpSketch/call/<percent-encoded {"tool": ..., "args": {...}}>
// All tool code runs on the GUI thread; call() marshals from the server
// thread with a blocking queued invocation.
class FProbeMcpSketch : public QObject, public FProbe {
	Q_OBJECT
public:
	FProbeMcpSketch(MainWindow * mainWindow, ReferenceModel * referenceModel);
	~FProbeMcpSketch() {};

	QVariant read() override;
	void write(QVariant) override {};
	QVariant call(QVariant params) override;

private:
	QJsonObject dispatch(const QJsonObject & request);
	QJsonObject toolSearchParts(const QJsonObject & args);
	QJsonObject toolGetPartInfo(const QJsonObject & args);
	QJsonObject toolAddPart(const QJsonObject & args);
	QJsonObject toolConnect(const QJsonObject & args);
	QJsonObject toolMove(const QJsonObject & args);
	QJsonObject toolRotate(const QJsonObject & args);
	QJsonObject toolDelete(const QJsonObject & args);
	QJsonObject toolGetSketchState(const QJsonObject & args);
	QJsonObject toolGetConnectors(const QJsonObject & args);
	QJsonObject toolExportImage(const QJsonObject & args);
	QJsonObject toolSaveSketch(const QJsonObject & args);
	QJsonObject toolOpenSketch(const QJsonObject & args);
	QJsonObject toolImportPart(const QJsonObject & args);

	SketchWidget * breadboardView();
	ItemBase * findItemOrError(long id, QJsonObject & error);
	void selectOnly(SketchWidget * view, ItemBase * itemBase);
	void settle(SketchWidget * view);

	QPointer<MainWindow> m_mainWindow;
	QPointer<ReferenceModel> m_referenceModel;
};

#endif
