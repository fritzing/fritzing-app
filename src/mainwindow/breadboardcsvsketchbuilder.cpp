#include "breadboardcsvsketchbuilder.h"

#include "../commands.h"
#include "../items/itembase.h"
#include "../model/modelpart.h"
#include "../referencemodel/referencemodel.h"
#include "../sketch/sketchwidget.h"

#include <QGraphicsItem>
#include <QPointF>
#include <QSet>
#include <QUndoCommand>

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
