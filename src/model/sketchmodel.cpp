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

#include "sketchmodel.h"

#include <QDir>
#include <QMessageBox>
#include <QTextStream>
#include <QFile>
#include <QXmlStreamWriter>

SketchModel::SketchModel(bool makeRoot) : ModelBase(makeRoot)
{
}

SketchModel::SketchModel(ModelPart * root) : ModelBase(false) {
	m_root = root;
}

void SketchModel::removeModelPart(ModelPart * modelPart) {
	modelPart->setParent(nullptr);
	//DebugDialog::debug(QString("model count %1").arg(root()->children().size()));
}

ModelPart * SketchModel::findModelPart(const QString & moduleID, long id) {
	if (m_root == nullptr) return nullptr;

	return findModelPartAux(m_root, moduleID, id);
}

ModelPart * SketchModel::findModelPartAux(ModelPart * modelPart, const QString & moduleID, long id)
{
	if (modelPart->moduleID().compare(moduleID) == 0) {
		if (modelPart->hasViewID(id)) {
			return modelPart;
		}
		if (modelPart->modelIndex() * ModelPart::indexMultiplier == id) {
			return modelPart;
		}
	}

	Q_FOREACH (QObject * child, modelPart->children()) {
		auto * mp = qobject_cast<ModelPart *>(child);
		if (mp == nullptr) continue;

		mp = findModelPartAux(mp, moduleID, id);
		if (mp != nullptr) {
			return mp;
		}
	}

	return nullptr;
}
