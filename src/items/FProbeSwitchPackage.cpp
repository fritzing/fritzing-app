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

#include "FProbeSwitchPackage.h"


FProbeSwitchPackage::FProbeSwitchPackage(FamilyPropertyComboBox * familyPropertyComboBox) :
	FProbe("SwitchPackage"),
	m_familyPropertyComboBox(familyPropertyComboBox)
{
}

void FProbeSwitchPackage::write(QVariant data) {
	int index = m_familyPropertyComboBox->findText(data.toString());
	DebugDialog::debug(QString("FProbeSwitchPackage index old: %1 index new: %2").arg(m_familyPropertyComboBox->currentIndex()).arg(index));
	DebugDialog::debug(QString("FProbeSwitchPackage text old: %1 text new: %2").arg(m_familyPropertyComboBox->itemText(m_familyPropertyComboBox->currentIndex())).arg(m_familyPropertyComboBox->itemText(index)));
	if (index != -1) {
		m_familyPropertyComboBox->setCurrentIndex(index);
	}
}
