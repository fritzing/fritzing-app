#include "breadboardcsvsketchbuilder.h"

#include "../commands.h"
#include "../connectors/connectoritem.h"
#include "../items/itembase.h"
#include "../items/partfactory.h"
#include "../model/modelpart.h"
#include "../referencemodel/referencemodel.h"
#include "../sketch/sketchwidget.h"

#include <QGraphicsItem>
#include <QLineF>
#include <QPointF>
#include <QSet>
#include <QUndoCommand>
#include <QUndoStack>

BreadboardCsvPlacementResult
BreadboardCsvSketchBuilder::placeThreeBreadboards(
	SketchWidget *breadboardView
)
{
	BreadboardCsvPlacementResult result;

	if (breadboardView == nullptr) {
		result.error = "Breadboard view is not available.";
		return result;
	}

	ReferenceModel *referenceModel =
		breadboardView->referenceModel();

	if (referenceModel == nullptr) {
		result.error = "Fritzing reference model is not available.";
		return result;
	}

	const QString moduleId =
		"Breadboard-RSR03MB102-ModuleID";

	ModelPart *modelPart =
		referenceModel->retrieveModelPart(moduleId);

	if (modelPart == nullptr) {
		result.error =
			QString("Required breadboard part was not found: %1")
				.arg(moduleId);

		return result;
	}

	QSet<long> existingIds;
	QList<ItemBase *> existingBoards;

	for (QGraphicsItem *graphicsItem :
	     breadboardView->scene()->items()) {

		auto *itemBase =
			dynamic_cast<ItemBase *>(graphicsItem);

		if (itemBase == nullptr) {
			continue;
		}

		ItemBase *chief =
			itemBase->layerKinChief();

		if (chief == nullptr) {
			continue;
		}

		if (chief->moduleID() != moduleId) {
			continue;
		}

		if (existingIds.contains(chief->id())) {
			continue;
		}

		existingIds.insert(chief->id());
		existingBoards.append(chief);
	}

	if (existingBoards.size() >= 3) {
		result.error =
			QString(
				"Sketch already contains %1 RSR 03MB102 "
				"breadboard(s). No boards were added."
			)
				.arg(existingBoards.size());

		return result;
	}

	if (existingBoards.size() == 2) {
		result.error =
			"Sketch already contains 2 RSR 03MB102 breadboards. "
			"Automatic B1/B2 identity would be ambiguous, so no "
			"boards were added.";

		return result;
	}

	QPointF origin(0.0, 0.0);
	double boardWidth = 0.0;
	const double boardGap = 30.0;
	int firstBoardToCreate = 0;

	if (existingBoards.size() == 1) {
		ItemBase *existingBoard =
			existingBoards.first();

		origin =
			existingBoard->getViewGeometry().loc();

		boardWidth =
			existingBoard->boundingRectWithoutLegs().width();

		if (boardWidth <= 0.0) {
			result.error =
				"Could not determine the width of the existing "
				"RSR 03MB102 breadboard.";

			return result;
		}

		result.boardIds.append(
			existingBoard->id()
		);

		result.reusedExistingBoard = true;
		firstBoardToCreate = 1;
	}
	else {
		boardWidth = 936.0;
	}

	const ViewLayer::ViewLayerPlacement placement =
		breadboardView->defaultViewLayerPlacement(modelPart);

	auto *parentCommand =
		new QUndoCommand(
			result.reusedExistingBoard
				? QObject::tr("Place CSV breadboards B2 and B3")
				: QObject::tr("Place three CSV breadboards")
		);

	for (
		int index = firstBoardToCreate;
		index < 3;
		++index
	) {
		ViewGeometry geometry;

		const double xOffset =
			(boardWidth + boardGap) * index;

		geometry.setLoc(
			origin + QPointF(xOffset, 0.0)
		);

		const long id =
			ItemBase::getNextID();

		new AddItemCommand(
			breadboardView,
			BaseCommand::CrossView,
			moduleId,
			placement,
			geometry,
			id,
			false,
			-1,
			parentCommand
		);

		result.boardIds.append(id);
		++result.boardsAdded;
	}

	if (result.boardsAdded == 0) {
		delete parentCommand;

		result.error =
			"No breadboards needed to be added.";

		return result;
	}

	breadboardView->undoStack()->push(
		parentCommand
	);

	result.ok =
		result.boardIds.size() == 3;

	if (!result.ok) {
		result.error =
			QString(
				"Expected 3 total breadboards after placement, "
				"but tracked %1."
			)
				.arg(result.boardIds.size());
	}

	return result;
}


BreadboardCsvCpuProbeResult
BreadboardCsvSketchBuilder::placeCpuAlignmentProbe(
	SketchWidget *breadboardView,
	long boardId
)
{
	BreadboardCsvCpuProbeResult result;

	if (breadboardView == nullptr) {
		result.error =
			"Breadboard view is not available.";

		return result;
	}

	ItemBase *board =
		breadboardView->findItem(boardId);

	if (board == nullptr) {
		result.error =
			QString(
				"B1 breadboard item %1 was not found."
			)
				.arg(boardId);

		return result;
	}

	if (
		board->moduleID() !=
		"Breadboard-RSR03MB102-ModuleID"
	) {
		result.error =
			QString(
				"Item %1 is not the expected "
				"RSR 03MB102 breadboard."
			)
				.arg(boardId);

		return result;
	}

	const QString moduleId =
		"generic_ic_dip_v2_40_600mil";

	ReferenceModel *referenceModel =
		breadboardView->referenceModel();

	if (referenceModel == nullptr) {
		result.error =
			"Fritzing reference model is not available.";

		return result;
	}

	ModelPart *modelPart =
		referenceModel->retrieveModelPart(moduleId);

	if (modelPart == nullptr) {
		const QString generatedFzp =
			PartFactory::getFzpFilename(moduleId);

		if (generatedFzp.isEmpty()) {
			result.error =
				QString(
					"Fritzing could not generate the dynamic "
					"DIP FZP: %1"
				)
					.arg(moduleId);

			return result;
		}

		modelPart =
			referenceModel->loadPart(
				generatedFzp,
				false
			);
	}

	if (modelPart == nullptr) {
		result.error =
			QString(
				"The dynamic DIP FZP was generated, but "
				"Fritzing could not load it: %1"
			)
				.arg(moduleId);

		return result;
	}

	ConnectorItem *targetPin1 =
		board->findConnectorItemWithSharedID(
			"pin1C"
		);

	ConnectorItem *targetPin20 =
		board->findConnectorItemWithSharedID(
			"pin20C"
		);

	ConnectorItem *targetPin21 =
		board->findConnectorItemWithSharedID(
			"pin20G"
		);

	ConnectorItem *targetPin40 =
		board->findConnectorItemWithSharedID(
			"pin1G"
		);

	if (
		targetPin1 == nullptr ||
		targetPin20 == nullptr ||
		targetPin21 == nullptr ||
		targetPin40 == nullptr
	) {
		result.error =
			"Could not resolve one or more B1 CPU "
			"corner coordinates.";

		return result;
	}

	const ViewLayer::ViewLayerPlacement placement =
		breadboardView->defaultViewLayerPlacement(
			modelPart
		);

	ViewGeometry initialGeometry;

	initialGeometry.setLoc(
		board->getViewGeometry().loc()
	);

	const long cpuId =
		ItemBase::getNextID();

	QUndoStack *stack =
		breadboardView->undoStack();

	stack->beginMacro(
		QObject::tr(
			"Place W65C02 CPU alignment probe"
		)
	);

	stack->push(
		new AddItemCommand(
			breadboardView,
			BaseCommand::CrossView,
			moduleId,
			placement,
			initialGeometry,
			cpuId,
			false,
			-1,
			nullptr
		)
	);

	ItemBase *cpu =
		breadboardView->findItem(cpuId);

	if (cpu == nullptr) {
		stack->endMacro();
		stack->undo();

		result.error =
			"Dynamic 40-pin DIP was generated but "
			"could not be found in the breadboard view.";

		return result;
	}

	ConnectorItem *cpuPin1 =
		cpu->findConnectorItemWithSharedID(
			"connector0"
		);

	ConnectorItem *cpuPin20 =
		cpu->findConnectorItemWithSharedID(
			"connector19"
		);

	ConnectorItem *cpuPin21 =
		cpu->findConnectorItemWithSharedID(
			"connector20"
		);

	ConnectorItem *cpuPin40 =
		cpu->findConnectorItemWithSharedID(
			"connector39"
		);

	if (
		cpuPin1 == nullptr ||
		cpuPin20 == nullptr ||
		cpuPin21 == nullptr ||
		cpuPin40 == nullptr
	) {
		stack->endMacro();
		stack->undo();

		result.error =
			"Generated 40-pin DIP does not expose "
			"the expected connector0..connector39 IDs.";

		return result;
	}

	const QPointF pin1Delta =
		targetPin1->scenePinPoint() -
		cpuPin1->scenePinPoint();

	const QPointF pin20Delta =
		targetPin20->scenePinPoint() -
		cpuPin20->scenePinPoint();

	const QPointF pin21Delta =
		targetPin21->scenePinPoint() -
		cpuPin21->scenePinPoint();

	const QPointF pin40Delta =
		targetPin40->scenePinPoint() -
		cpuPin40->scenePinPoint();

	const QPointF delta(
		(
			pin1Delta.x() +
			pin20Delta.x() +
			pin21Delta.x() +
			pin40Delta.x()
		) / 4.0,
		(
			pin1Delta.y() +
			pin20Delta.y() +
			pin21Delta.y() +
			pin40Delta.y()
		) / 4.0
	);

	ViewGeometry oldGeometry =
		cpu->getViewGeometry();

	ViewGeometry newGeometry =
		oldGeometry;

	newGeometry.setLoc(
		oldGeometry.loc() + delta
	);

	stack->push(
		new MoveItemCommand(
			breadboardView,
			cpuId,
			oldGeometry,
			newGeometry,
			false,
			nullptr
		)
	);

	result.cpuId = cpuId;

	result.pin1Error =
		QLineF(
			cpuPin1->scenePinPoint(),
			targetPin1->scenePinPoint()
		).length();

	result.pin20Error =
		QLineF(
			cpuPin20->scenePinPoint(),
			targetPin20->scenePinPoint()
		).length();

	result.pin21Error =
		QLineF(
			cpuPin21->scenePinPoint(),
			targetPin21->scenePinPoint()
		).length();

	result.pin40Error =
		QLineF(
			cpuPin40->scenePinPoint(),
			targetPin40->scenePinPoint()
		).length();

	stack->endMacro();

	const double tolerance = 1.0;

	result.aligned =
		result.pin1Error <= tolerance &&
		result.pin20Error <= tolerance &&
		result.pin21Error <= tolerance &&
		result.pin40Error <= tolerance;

	result.ok = true;

	return result;
}
