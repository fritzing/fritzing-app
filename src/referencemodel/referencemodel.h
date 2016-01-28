/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2016 Fritzing

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

********************************************************************

$Revision: 6904 $:
$Author: irascibl@gmail.com $:
$Date: 2013-02-26 16:26:03 +0100 (Di, 26. Feb 2013) $

********************************************************************/


#ifndef REFERENCEMODEL_H_
#define REFERENCEMODEL_H_

#include "../model/palettemodel.h"

class ReferenceModel : public PaletteModel {
	Q_OBJECT
	public:
		virtual bool loadAll(const QString & databaseName, bool fullLoad, bool dbExists) = 0;
		virtual bool loadFromDB(const QString & databaseName) = 0;
		virtual ModelPart *loadPart(const QString & path, bool update) = 0;
		virtual ModelPart *reloadPart(const QString & path, const QString & moduleID) = 0;

		virtual ModelPart *retrieveModelPart(const QString &moduleID) = 0;

		virtual bool addPart(ModelPart * newModel, bool update) = 0;
		virtual ModelPart * addPart(QString newPartPath, bool addToReference, bool updateIdAlreadyExists) = 0;
		virtual bool updatePart(ModelPart * newModel) = 0;

        virtual bool swapEnabled() const = 0;
		virtual QString partTitle(const QString & moduleID) = 0;
		virtual void recordProperty(const QString &name, const QString &value) = 0;
		virtual QString retrieveModuleIdWith(const QString &family, const QString &propertyName, bool closestMatch) = 0;
		virtual QString retrieveModuleId(const QString &family, const QMultiHash<QString /*name*/, QString /*value*/> &properties, const QString &propertyName, bool closestMatch) = 0;
		virtual QStringList propValues(const QString &family, const QString &propName, bool distinct) = 0;
		virtual QMultiHash<QString, QString> allPropValues(const QString &family, const QString &propName) = 0;
		virtual bool lastWasExactMatch() = 0;
        virtual void setSha(const QString & sha) = 0;
        virtual const QString & sha() const = 0;

};

#endif /* REFERENCEMODEL_H_ */
