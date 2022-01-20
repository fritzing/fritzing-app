# /*******************************************************************
# Part of the Fritzing project - https://fritzing.org
# Copyright (c) 2021 Fritzing GmbH
# ********************************************************************/

#include "src/items/itembase.h"
#include "src/model/modelpart.h"
#include "src/utils/textutils.h"
#include "src/utils/graphicsutils.h"
#include "src/connectors/nonconnectoritem.h"
#include "src/connectors/connectoritem.h"
#include "src/connectors/ercdata.h"

#include <QString>
#include <QMessageBox>
#include <qmath.h>


// Forward declaration, currently unused
// QString electricalTestRecord(QString netLabel, QString partLabel, QString connectorName, QString pin, bool isTHT, bool isMiddle, bool isPlated, bool isDrilled, int r, int x, int y, int w, int h, int layer, int ccw_angle);
QString electricalTestRecord(QString netLabel, QString partLabel, QString connectorName, QString connectorId, bool isTHT, bool isMiddle, bool isPlated, bool isDrilled, int r, int x, int y, int w, int h, int layer, int ccw_angle) {
// Standard electrical test record (setr)
//		Column      Data            Description      Number      Meaning
//			1 P,C,3  C=comment, P=parameter, 3=test record
//			2 1,2,5,6 1=through hole, 2=SMT feature, 3=tooling feature/hole, 4=tooling hole only
//			3 7 Always a 7 for a test record 999      =      end      of      file
//			4-17 Net Name Alphanumeric string (yes, nets name are limited to 14 characters)
//			18-20  --- These fields are left blank for finished PCBs.
//			21-32  Ref Des This is where the reference designator goes if it is known.
//				21-26 ID,VIA RefDes (U or IC) or “VIA” if it is a via
//				27  - Always a dash. ie U-17 or IC-12
//				28-31 Alpha# Component pin number
//				32  M M means a point in the middle of a net. Blank means the end of a net.
//			33-38  Hole type Hole definition field
//				33-37 D##### D=diameter, #=size in .0001 inches or .001 mm.
//				38  P or U P=plated, U=unplated
//			39-41  A## A=access side of PCB.  ##=00 if point is available from both sides. 01 if Primary side only. >01 means internal layers.
//			42-57  Coords X, Y coordinates of test location.
//				42  X Start of X coordinate
//				43  +- or blank X coordinate polarity
//				44-49 value  6 digits to .0001 inches. (No decimal points.) Leading zeros surpressed.
//				50  Y Start of Y coordinate
//				51  +- or blank Y coordinate polarity
//				52-57 value  6 digits to .0001 inches. (No decimal points.) Leading zeros surpressed.
//			58-71  Rect Data Dimensions for rectangular test feature.
//				58-62 X####  X dimension of feature in .0001 inches
//				63-67 Y####  Y dimension of feature in .0001 inches (Fields 68 - 80 are often left blank.)
//				68-71 R### Rotation of feature in whole degrees.
//			72 Not used. Must be left blank.
//			73-74  S# Optional solder mask information.
//			75-80  Optional test record. Commonly left blank.

	int access = 0;
	if (layer == ViewLayer::Copper0) {
		access = 1;
	}
	if (layer == ViewLayer::Copper1) {
		access = 16;
	}

	int cmd = 317;
	if (!isTHT) {
		cmd = 327;
	}
	int soldermask = 0;

	QString pin = QString::number(TextUtils::getPositiveIntegers(connectorId).constFirst());
	if (connectorName.contains("anode", Qt::CaseInsensitive)) pin = "A";
	if (connectorName.contains("catho", Qt::CaseInsensitive)) pin = "C";
	if (connectorName.contains("gnd", Qt::CaseInsensitive)) pin = "GND";
	if (connectorName == "-") pin = "-";
	if (connectorName == "+") pin = "+";

	const char setr[]{"%3d%-14.14s   %-6.6s-%4.4s%1.1s%1.1s%04d%1sA%02dX%+07dY%+07dX%04dY%04dR%03d S%1d     \n"};
	QString s = QString::asprintf(setr,
								  cmd,
								  netLabel.toStdString().c_str(),
								  partLabel.toStdString().c_str(),
								  pin.toStdString().c_str(),
								  isMiddle ? "M" : " ",
								  isDrilled ? "D" : " ",
								  r,
								  isPlated ? "P" : "U",
								  access,
								  x,
								  y,
								  w,
								  h,
								  ccw_angle,
								  soldermask
								  );
	return s;
}



QString getExportIPC_D_356A(ItemBase * board, QString basename, QList< QList<ConnectorItem *>* > netList) {

	QPointF origin = board->sceneBoundingRect().bottomLeft();

	QString ipc; // IPC D 356A

	const char comment[]{"C  %.66s\n"};
	const char header[]{"P  %.4s  %.62s\n"};
	const char ende[]{"999\n"};

	ipc += QString::asprintf(comment, TextUtils::CreatedWithFritzingString.toStdString().c_str());
//	ipc += QString::asprintf(header, "JOB", "TEST");
	ipc += QString::asprintf(header, "CODE", "00");

	//	 SI Metric
	//	 CUST 0 or CUST Inches and degrees
	//	 CUST 1 Millimeters and degrees
	//	 CUST 2 Inches and radians
	ipc += QString::asprintf(header, "UNITS", "CUST 1");
	ipc += QString::asprintf(header, "TITLE", basename.toStdString().c_str());
	ipc += QString::asprintf(header, "VER", "IPC-D-356");

	QList< QList<ConnectorItem *>* > deleteNets;
	Q_FOREACH (QList<ConnectorItem *> * net, netList) {
		// Sorting so we get consistend export data, avoid random order
		std::sort(net->begin(), net->end(), [](ConnectorItem * a, ConnectorItem * b){
			return a->rect().center().x() + a->rect().center().y() > b->rect().center().x() + b->rect().center().y();
		});
		QList<ConnectorItem *> deleteItems;
		Q_FOREACH (ConnectorItem * connectorItem, *net) {
			ErcData * ercData = connectorItem->connectorSharedErcData();
			if (ercData == nullptr) continue;

			if (ercData->ignore() == ErcData::Always) {
				deleteItems.append(connectorItem);
			}
			else if ((ercData->ignore() == ErcData::IfUnconnected) && (net->count() == 1)) {
				deleteItems.append(connectorItem);
			}
		}

		Q_FOREACH (ConnectorItem * connectorItem, deleteItems) {
			net->removeOne(connectorItem);
		}
		if (net->count() < 2) {
			deleteNets.append(net);
		}
	}

	Q_FOREACH (QList<ConnectorItem *> * net, deleteNets) {
		netList.removeOne(net);
	}

	constexpr double MM2IPC = 1000.0;
	int countNets = 0;

	// Sorting so we get consistend export data, avoid random order
	std::sort(netList.begin(), netList.end(), [](QList<ConnectorItem *> * a, QList<ConnectorItem *> * b){
		if (a->constFirst()->attachedToInstanceTitle() == b->constFirst()->attachedToInstanceTitle()) {
			return a->constFirst()->rect().center().x() > b->constFirst()->rect().center().x();
		}
		return a->constFirst()->attachedToInstanceTitle() > b->constFirst()->attachedToInstanceTitle();
	});

	Q_FOREACH (QList<ConnectorItem *> * net, netList) {

		auto * i1 = net->constFirst();
		auto * i2 = net->constLast();
		if (net->size() == 2) {
			// Skip 'nets' that consist only of top and bottom layer of one THT.
			if (i1->isCrossLayerConnectorItem(i2)) continue;
		}
		QString netLabel = "NET" + QString::number(++countNets) + i1->attachedToInstanceTitle() + i2->attachedToInstanceTitle();
		Q_FOREACH (ConnectorItem * connectorItem, *net) {
			if (connectorItem->connectorSharedName().contains("gnd", Qt::CaseInsensitive)) {
				netLabel = "NET" + QString::number(++countNets) + "-GND";
			}
		}

		Q_FOREACH (ConnectorItem * connectorItem, *net) {
			QString connectorId = connectorItem->connectorSharedID();
			QString connectorName = connectorItem->connectorSharedName();

			ItemBase * itemBase = connectorItem->attachedTo();
			QString title = itemBase->instanceTitle();

//			Q_FOREACH (QGraphicsItem * childItem, itemBase->childItems()) {
//				NonConnectorItem * nonConnectorItem = dynamic_cast<NonConnectorItem *>(childItem);
//				if (nonConnectorItem) {
//					QString s = nonConnectorItem->attachedToTitle();
//					s += QString(" ") + (nonConnectorItem->isEffectivelyCircular() ? "circular" : "rectangular");
//					DebugDialog::debug(s);
//				}
//			}

			QPointF loc = connectorItem->mapToScene(connectorItem->rect().center());
			double width = connectorItem->rect().width();
			double height = connectorItem->rect().height();
			QTransform transform = itemBase->transform();

			bool isMiddle = connectorItem->connectionsCount() > 1;
			bool isPlated = false;
			// Using stroke width for isPlated is a wild guess based on svg2gerber line 336 and variable m_platedApertures.
			if (connectorItem->strokeWidth() > 0.0000001) {
				isPlated = true;
			}
			QString package = itemBase->modelPart()->properties().value("package");
			bool isTHT = package.contains("THT");
			int holeDiameter = round(MM2IPC  * GraphicsUtils::pixels2mm(2 * connectorItem->radius() - connectorItem->strokeWidth(), GraphicsUtils::SVGDPI));
			int diameter = round(MM2IPC  * GraphicsUtils::pixels2mm(2 * connectorItem->radius() + connectorItem->strokeWidth(), GraphicsUtils::SVGDPI));
			bool isDrilled = (isTHT || (holeDiameter > 0));
			int x = round(MM2IPC  * GraphicsUtils::pixels2mm(loc.x() - origin.x(), GraphicsUtils::SVGDPI)); // /from GerberGenerator::exportPickAndPlace
			int y = round(MM2IPC  * GraphicsUtils::pixels2mm(origin.y() - loc.y() , GraphicsUtils::SVGDPI));
			int widthOrDiameter = isDrilled ? diameter : round(MM2IPC  * GraphicsUtils::pixels2mm(width, GraphicsUtils::SVGDPI));
			int heightOrZero = connectorItem->isEffectivelyCircular() ? 0 : round(MM2IPC  * GraphicsUtils::pixels2mm(height, GraphicsUtils::SVGDPI));
			int layer = connectorItem->attachedToViewLayerID();
			int ccw_angle = round(atan2(transform.m12(), transform.m11()) * 180.0 / M_PI); // doesn't account for scaling. from GerberGenerator::exportPickAndPlace.

			ipc += electricalTestRecord(netLabel, title, connectorName, connectorId, isTHT, isMiddle, isPlated, isDrilled, holeDiameter, x, y, widthOrDiameter, heightOrZero, layer, ccw_angle);
		}
	}

	Q_FOREACH (QList<ConnectorItem *> * net, netList) {
		delete net;
	}

	ipc += ende;
	return ipc;
}
