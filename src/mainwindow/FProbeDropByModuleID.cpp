/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2022 Fritzing GmbH

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

#include "FProbeDropByModuleID.h"

FProbeDropByModuleID::FProbeDropByModuleID() :
	FProbe("DropByModuleID")
{
}

void FProbeDropByModuleID::write(QVariant data) {
	QString moduleID = data.toString();
	Q_EMIT putItemByModuleID(moduleID);

	/*
	// find modulepart by moduleID
	// from MainWindow::loadBundledSketch

	ModelPart * mp = m_referenceModel->retrieveModelPart(moduleID);
	// Add new item with undo command
	AddItemCommand * addItemCommand = newAddItemCommand(crossViewType, modelPart, modelPart->moduleID(), viewLayerPlacement, viewGeometry, fromID, true, -1, true, parentCommand);
	addItemCommand->setDropOrigin(this);
	*/
}
