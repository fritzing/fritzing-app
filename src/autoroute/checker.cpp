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

#include "checker.h"
#include "../debugdialog.h"
#include "../sketch/pcbsketchwidget.h"
#include "../utils/graphicsutils.h"
#include "../connectors/connectoritem.h"

#include <QFile>
#include <QDomDocument>
#include <QDomElement>
#include <QDir>
#include <qmath.h>

static QString CheckerOutputPath;
static QSet<QString> CheckerFileNames;

int Checker::checkText(MainWindow * mainWindow, bool displayMessage) {
	QHash<QString, QString> svgHash;
	QList<ItemBase *> missing;

	Q_FOREACH (QGraphicsItem * item, mainWindow->pcbView()->scene()->items()) {
		ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase == NULL) continue;
		if (!itemBase->isEverVisible()) continue;

		double factor;
		QString itemSvg = itemBase->retrieveSvg(itemBase->viewLayerID(), svgHash, false, GraphicsUtils::StandardFritzingDPI, factor);
		if (itemSvg.isEmpty()) continue;

		QDomDocument doc;
		QString errorStr;
		int errorLine;
		int errorColumn;
		if (!doc.setContent(itemSvg, &errorStr, &errorLine, &errorColumn)) {
			DebugDialog::debug(QString("itembase svg failure %1").arg(itemBase->id()));
			continue;
		}

		QDomElement root = doc.documentElement();
		QDomNodeList domNodeList = root.elementsByTagName("path");
		for (int i = 0; i < domNodeList.count(); i++) {
			QDomElement textElement = domNodeList.at(i).toElement();
			if (textElement.attribute("fill").isEmpty() && textElement.attribute("stroke").isEmpty() && textElement.attribute("stroke-width").isEmpty()) {
				missing.append(itemBase);
				break;
			}
		}
	}

	if (displayMessage && missing.count() > 0) {
		mainWindow->pcbView()->selectAllItems(false, false);
		mainWindow->pcbView()->selectItems(missing);
		QMessageBox::warning(NULL, "Text", QString("There are %1 possible instances of parts with <path> elements missing stroke/fill/stroke-width attributes").arg(missing.count()));
	}

	if (missing.count() > 0) {
		QFileInfo info(mainWindow->fileName());
		writeCheckerOutput(QString("%2 ... There are %1 possible instances of parts with <path> elements missing stroke/fill/stroke-width attributes")
		                     .arg(missing.count()).arg(info.fileName())
		                    );
		collectFilenames(info.fileName());
	}

	return  missing.count();
}

int Checker::checkDonuts(MainWindow * mainWindow, bool displayMessage) {
	QList<ConnectorItem *> donuts;
	Q_FOREACH (QGraphicsItem * item, mainWindow->pcbView()->scene()->items()) {
		ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(item);
		if (connectorItem == NULL) continue;
		if (!connectorItem->attachedTo()->isEverVisible()) continue;

		if (connectorItem->isPath() && (connectorItem->getCrossLayerConnectorItem() != nullptr)) {  // && connectorItem->radius() == 0
			connectorItem->debugInfo("possible donut");
			connectorItem->attachedTo()->debugInfo("\t");
			donuts << connectorItem;
		}
	}

	if (displayMessage && donuts.count() > 0) {
		mainWindow->pcbView()->selectAllItems(false, false);
		QSet<ItemBase *> itemBases;
		Q_FOREACH (ConnectorItem * connectorItem, donuts) {
			itemBases.insert(connectorItem->attachedTo());
		}
		mainWindow->pcbView()->selectItems(itemBases.values());
		QMessageBox::warning(NULL, "Donuts", QString("There are %1 possible donut connectors").arg(donuts.count() / 2));
	}

	if (donuts.count() > 0) {
		QFileInfo info(mainWindow->fileName());
		writeCheckerOutput(QString("%2 ... %1 possible donuts").arg(donuts.count() / 2).arg(info.fileName()));
		collectFilenames(mainWindow->fileName());
	}

	return donuts.count() / 2;
}

void Checker::writeCheckerOutput(const QString & message) {
	DebugDialog::debug(message);
	if (CheckerOutputPath.length() > 0) {
		QFile file(CheckerOutputPath);
		if (file.open(QFile::Append)) {
			QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
			out.setCodec("UTF-8");
#endif
			out << message << "\n";
			file.close();
		}
	}
}

void Checker::collectFilenames(const QString & filename) {
	if (filename.endsWith(".fz")) {
		CheckerFileNames.insert(filename + "z");
	}
	else CheckerFileNames.insert(filename);
}
