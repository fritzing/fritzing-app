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

#ifndef FPPROBESWITCHPROPERTY_H
#define FPPROBESWITCHPROPERTY_H

#include "testing/FProbe.h"

#include "utils/familypropertycombobox.h"

#include <QObject>
#include <QVariant>

class FProbeSwitchProperty : public FProbe {
public:
	FProbeSwitchProperty(FamilyPropertyComboBox * familyPropertyComboBox, std::string property);
	~FProbeSwitchProperty() {};

	QVariant read() { return QVariant(); };
	void write(QVariant);

	static void insertIf(QString property,
						 FamilyPropertyComboBox *comboBox,
						 std::string propertyList);


protected:
	FamilyPropertyComboBox * m_familyPropertyComboBox;
};

#endif
