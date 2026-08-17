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


BreadboardCsvDipPlacementResult
BreadboardCsvSketchBuilder::placeDipFootprints(
	SketchWidget *breadboardView,
	const QList<long> &boardIds,
	const QList<BreadboardCsvDipFootprint> &footprints
)
{
	BreadboardCsvDipPlacementResult result;

	if (breadboardView == nullptr) {
		result.error =
			"Breadboard view is not available.";

		return result;
	}

	if (boardIds.size() < 3) {
		result.error =
			QString(
				"Expected three breadboard item IDs, "
				"but received %1."
			)
				.arg(boardIds.size());

		return result;
	}

	if (footprints.isEmpty()) {
		result.error =
			"No resolved DIP footprints were supplied.";

		return result;
	}

	ReferenceModel *referenceModel =
		breadboardView->referenceModel();

	if (referenceModel == nullptr) {
		result.error =
			"Fritzing reference model is not available.";

		return result;
	}

	QUndoStack *stack =
		breadboardView->undoStack();

	if (stack == nullptr) {
		result.error =
			"Fritzing undo stack is not available.";

		return result;
	}

	struct PreparedDip
	{
		BreadboardCsvDipFootprint footprint;

		ItemBase *board = nullptr;
		ModelPart *modelPart = nullptr;

		ConnectorItem *targetPin1 = nullptr;
		ConnectorItem *targetPinHalf = nullptr;
		ConnectorItem *targetPinHalfPlus1 = nullptr;
		ConnectorItem *targetPinLast = nullptr;

		QString moduleId;
	};

	QList<PreparedDip> preparedDips;

	// --------------------------------------------------------
	// PRE-FLIGHT EVERYTHING BEFORE MUTATING THE SKETCH.
	// --------------------------------------------------------

	for (
		const BreadboardCsvDipFootprint &footprint :
		footprints
	) {
		if (!footprint.ok) {
			result.error =
				QString(
					"%1 does not contain a valid "
					"resolved DIP footprint."
				)
					.arg(footprint.component);

			return result;
		}

		if (
			footprint.board < 1 ||
			footprint.board > boardIds.size()
		) {
			result.error =
				QString(
					"%1 resolved to invalid breadboard B%2."
				)
					.arg(footprint.component)
					.arg(footprint.board);

			return result;
		}

		if (
			footprint.pinCount < 2 ||
			(footprint.pinCount % 2) != 0
		) {
			result.error =
				QString(
					"%1 has invalid DIP pin count %2."
				)
					.arg(footprint.component)
					.arg(footprint.pinCount);

			return result;
		}

		if (
			footprint.spacingMil != 300 &&
			footprint.spacingMil != 600
		) {
			result.error =
				QString(
					"%1 has unsupported DIP spacing %2 mil."
				)
					.arg(footprint.component)
					.arg(footprint.spacingMil);

			return result;
		}

		const long boardId =
			boardIds.at(footprint.board - 1);

		ItemBase *board =
			breadboardView->findItem(boardId);

		if (board == nullptr) {
			result.error =
				QString(
					"%1 requires B%2, but breadboard "
					"item %3 was not found."
				)
					.arg(footprint.component)
					.arg(footprint.board)
					.arg(boardId);

			return result;
		}

		if (
			board->moduleID() !=
			"Breadboard-RSR03MB102-ModuleID"
		) {
			result.error =
				QString(
					"%1 requires B%2, but item %3 is "
					"not the expected RSR 03MB102 breadboard."
				)
					.arg(footprint.component)
					.arg(footprint.board)
					.arg(boardId);

			return result;
		}

		const QString moduleId =
			QString(
				"generic_ic_dip_v2_%1_%2mil"
			)
				.arg(footprint.pinCount)
				.arg(footprint.spacingMil);

		ModelPart *modelPart =
			referenceModel->retrieveModelPart(
				moduleId
			);

		if (modelPart == nullptr) {
			const QString generatedFzp =
				PartFactory::getFzpFilename(
					moduleId
				);

			if (generatedFzp.isEmpty()) {
				result.error =
					QString(
						"Fritzing could not generate "
						"%1 for %2."
					)
						.arg(moduleId)
						.arg(footprint.component);

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
					"Fritzing generated but could not "
					"load %1 for %2."
				)
					.arg(moduleId)
					.arg(footprint.component);

			return result;
		}

		ConnectorItem *targetPin1 =
			board->findConnectorItemWithSharedID(
				footprint.pin1ConnectorId
			);

		ConnectorItem *targetPinHalf =
			board->findConnectorItemWithSharedID(
				footprint.pinHalfConnectorId
			);

		ConnectorItem *targetPinHalfPlus1 =
			board->findConnectorItemWithSharedID(
				footprint.pinHalfPlus1ConnectorId
			);

		ConnectorItem *targetPinLast =
			board->findConnectorItemWithSharedID(
				footprint.pinLastConnectorId
			);

		if (
			targetPin1 == nullptr ||
			targetPinHalf == nullptr ||
			targetPinHalfPlus1 == nullptr ||
			targetPinLast == nullptr
		) {
			result.error =
				QString(
					"Could not resolve one or more "
					"breadboard corner connectors for %1."
				)
					.arg(footprint.component);

			return result;
		}

		PreparedDip prepared;

		prepared.footprint =
			footprint;

		prepared.board =
			board;

		prepared.modelPart =
			modelPart;

		prepared.targetPin1 =
			targetPin1;

		prepared.targetPinHalf =
			targetPinHalf;

		prepared.targetPinHalfPlus1 =
			targetPinHalfPlus1;

		prepared.targetPinLast =
			targetPinLast;

		prepared.moduleId =
			moduleId;

		preparedDips.append(
			prepared
		);
	}

	// --------------------------------------------------------
	// ALL PREFLIGHT CHECKS PASSED.
	// PLACE EVERY DIP AS ONE UNDO TRANSACTION.
	// --------------------------------------------------------

	stack->beginMacro(
		QObject::tr(
			"Place DIP parts from wiring CSV"
		)
	);

	for (const PreparedDip &prepared : preparedDips) {
		const BreadboardCsvDipFootprint &footprint =
			prepared.footprint;

		const ViewLayer::ViewLayerPlacement placement =
			breadboardView->defaultViewLayerPlacement(
				prepared.modelPart
			);

		ViewGeometry initialGeometry;

		initialGeometry.setLoc(
			prepared.board->getViewGeometry().loc()
		);

		const long itemId =
			ItemBase::getNextID();

		stack->push(
			new AddItemCommand(
				breadboardView,
				BaseCommand::CrossView,
				prepared.moduleId,
				placement,
				initialGeometry,
				itemId,
				false,
				-1,
				nullptr
			)
		);

		ItemBase *dip =
			breadboardView->findItem(itemId);

		if (dip == nullptr) {
			stack->endMacro();
			stack->undo();

			result.error =
				QString(
					"%1 was added but could not be "
					"found in the breadboard view."
				)
					.arg(footprint.component);

			return result;
		}

		const int halfPins =
			footprint.pinCount / 2;

		const QString dipPin1Id =
			"connector0";

		const QString dipPinHalfId =
			QString("connector%1")
				.arg(halfPins - 1);

		const QString dipPinHalfPlus1Id =
			QString("connector%1")
				.arg(halfPins);

		const QString dipPinLastId =
			QString("connector%1")
				.arg(footprint.pinCount - 1);

		ConnectorItem *dipPin1 =
			dip->findConnectorItemWithSharedID(
				dipPin1Id
			);

		ConnectorItem *dipPinHalf =
			dip->findConnectorItemWithSharedID(
				dipPinHalfId
			);

		ConnectorItem *dipPinHalfPlus1 =
			dip->findConnectorItemWithSharedID(
				dipPinHalfPlus1Id
			);

		ConnectorItem *dipPinLast =
			dip->findConnectorItemWithSharedID(
				dipPinLastId
			);

		if (
			dipPin1 == nullptr ||
			dipPinHalf == nullptr ||
			dipPinHalfPlus1 == nullptr ||
			dipPinLast == nullptr
		) {
			stack->endMacro();
			stack->undo();

			result.error =
				QString(
					"%1 does not expose the expected "
					"DIP connector IDs."
				)
					.arg(footprint.component);

			return result;
		}

		const QPointF pin1Delta =
			prepared.targetPin1->scenePinPoint() -
			dipPin1->scenePinPoint();

		const QPointF pinHalfDelta =
			prepared.targetPinHalf->scenePinPoint() -
			dipPinHalf->scenePinPoint();

		const QPointF pinHalfPlus1Delta =
			prepared.targetPinHalfPlus1->scenePinPoint() -
			dipPinHalfPlus1->scenePinPoint();

		const QPointF pinLastDelta =
			prepared.targetPinLast->scenePinPoint() -
			dipPinLast->scenePinPoint();

		const QPointF delta(
			(
				pin1Delta.x() +
				pinHalfDelta.x() +
				pinHalfPlus1Delta.x() +
				pinLastDelta.x()
			) / 4.0,
			(
				pin1Delta.y() +
				pinHalfDelta.y() +
				pinHalfPlus1Delta.y() +
				pinLastDelta.y()
			) / 4.0
		);

		ViewGeometry oldGeometry =
			dip->getViewGeometry();

		ViewGeometry newGeometry =
			oldGeometry;

		newGeometry.setLoc(
			oldGeometry.loc() + delta
		);

		stack->push(
			new MoveItemCommand(
				breadboardView,
				itemId,
				oldGeometry,
				newGeometry,
				false,
				nullptr
			)
		);

		const double pin1Error =
			QLineF(
				dipPin1->scenePinPoint(),
				prepared.targetPin1->scenePinPoint()
			).length();

		const double pinHalfError =
			QLineF(
				dipPinHalf->scenePinPoint(),
				prepared.targetPinHalf->scenePinPoint()
			).length();

		const double pinHalfPlus1Error =
			QLineF(
				dipPinHalfPlus1->scenePinPoint(),
				prepared.targetPinHalfPlus1->scenePinPoint()
			).length();

		const double pinLastError =
			QLineF(
				dipPinLast->scenePinPoint(),
				prepared.targetPinLast->scenePinPoint()
			).length();

		const double errors[] = {
			pin1Error,
			pinHalfError,
			pinHalfPlus1Error,
			pinLastError
		};

		for (double error : errors) {
			if (error > result.maxCornerError) {
				result.maxCornerError =
					error;

				result.worstComponent =
					footprint.component;
			}
		}

		result.itemIds.append(
			itemId
		);
	}

	stack->endMacro();

	result.partsPlaced =
		result.itemIds.size();

	const double tolerance = 1.0;

	result.aligned =
		result.maxCornerError <= tolerance;

	result.ok = true;

	return result;
}
