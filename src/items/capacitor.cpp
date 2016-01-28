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

$Revision: 6984 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-22 23:44:56 +0200 (Mo, 22. Apr 2013) $

********************************************************************/

#include "capacitor.h"


#include "../utils/textutils.h"
#include "../utils/focusoutcombobox.h"
#include "../utils/boundedregexpvalidator.h"
#include "../sketch/infographicsview.h"
#include "partlabel.h"

// TODO
//	save into parts bin

Capacitor::Capacitor( ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: PaletteItem(modelPart, viewID, viewGeometry, id, itemMenu, doLabel)
{
	PropertyDefMaster::initPropertyDefs(modelPart, m_propertyDefs);
}

Capacitor::~Capacitor() {
}

ItemBase::PluralType Capacitor::isPlural() {
	return Plural;
}

bool Capacitor::collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide)
{
	foreach (PropertyDef * propertyDef, m_propertyDefs.keys()) {
		if (prop.compare(propertyDef->name, Qt::CaseInsensitive) == 0) {
			returnProp = TranslatedPropertyNames.value(prop);
			if (returnProp.isEmpty()) {
				returnProp = propertyDef->name;
			}

			FocusOutComboBox * focusOutComboBox = new FocusOutComboBox();
			focusOutComboBox->setEnabled(swappingEnabled);
			focusOutComboBox->setEditable(propertyDef->editable);
			focusOutComboBox->setObjectName("infoViewComboBox");
			QString current = m_propertyDefs.value(propertyDef);
            if (propertyDef->editable) {
                focusOutComboBox->setToolTip(tr("Select from the dropdown, or type in a %1 value").arg(returnProp));
            }
		
			if (propertyDef->numeric) {
				if (!current.isEmpty()) {
					double val = TextUtils::convertFromPowerPrefixU(current, propertyDef->symbol);
					if (!propertyDef->menuItems.contains(val)) {
						propertyDef->menuItems.append(val);
					}
				}
				foreach(double q, propertyDef->menuItems) {
					QString s = TextUtils::convertToPowerPrefix(q) + propertyDef->symbol;
					focusOutComboBox->addItem(s);
				}
			}
			else {
				if (!current.isEmpty()) {
					if (!propertyDef->sMenuItems.contains(current)) {
						propertyDef->sMenuItems.append(current);
					}
				}
				focusOutComboBox->addItems(propertyDef->sMenuItems);
			}
			if (!current.isEmpty()) {
				int ix = focusOutComboBox->findText(current);
				if (ix < 0) {
					focusOutComboBox->addItem(current);
					ix = focusOutComboBox->findText(current);
				}
				focusOutComboBox->setCurrentIndex(ix);
			}

			if (propertyDef->editable) {
				BoundedRegExpValidator * validator = new BoundedRegExpValidator(focusOutComboBox);
				validator->setSymbol(propertyDef->symbol);
				validator->setConverter(TextUtils::convertFromPowerPrefix);
				if (propertyDef->maxValue > propertyDef->minValue) {
					validator->setBounds(propertyDef->minValue, propertyDef->maxValue);
				}
				QString pattern = QString("((\\d{1,3})|(\\d{1,3}\\.)|(\\d{1,3}\\.\\d{1,2}))[%1]{0,1}[%2]{0,1}")
												.arg(TextUtils::PowerPrefixesString)
												.arg(propertyDef->symbol);
				validator->setRegExp(QRegExp(pattern));
				focusOutComboBox->setValidator(validator);
				connect(focusOutComboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(propertyEntry(const QString &)));
			}
			else {
				connect(focusOutComboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(simplePropertyEntry(const QString &)));
			}

			this->m_comboBoxes.insert(propertyDef, focusOutComboBox);
				
			returnValue = focusOutComboBox->currentText();
			returnWidget = focusOutComboBox;	

			return true;
		}
	}

	return PaletteItem::collectExtraInfo(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget, hide);
}

void Capacitor::propertyEntry(const QString & text) {
	FocusOutComboBox * focusOutComboBox = qobject_cast<FocusOutComboBox *>(sender());
	if (focusOutComboBox == NULL) return;

	foreach (PropertyDef * propertyDef, m_comboBoxes.keys()) {
		if (m_comboBoxes.value(propertyDef) == focusOutComboBox) {
			QString utext = text;
			if (propertyDef->numeric) {
				double val = TextUtils::convertFromPowerPrefixU(utext, propertyDef->symbol);
				if (!propertyDef->menuItems.contains(val)) {
					// info view is redrawn, so combobox is recreated, so the new item is added to the combo box menu
					propertyDef->menuItems.append(val);
				}
			}
			else {
				if (!propertyDef->sMenuItems.contains(text)) {
					// info view is redrawn, so combobox is recreated, so the new item is added to the combo box menu
					propertyDef->sMenuItems.append(text);
				}
			}

			InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
			if (infoGraphicsView != NULL) {
				infoGraphicsView->setProp(this, propertyDef->name, "", m_propertyDefs.value(propertyDef, ""), utext, true);
			}
			break;
		}
	}
}

void Capacitor::setProp(const QString & prop, const QString & value) {
	foreach (PropertyDef * propertyDef, m_propertyDefs.keys()) {
		if (prop.compare(propertyDef->name, Qt::CaseInsensitive) == 0) {
			m_propertyDefs.insert(propertyDef, value);
			modelPart()->setLocalProp(propertyDef->name, value);
			if (m_partLabel) m_partLabel->displayTextsIf();
			return;
		}
	}

	PaletteItem::setProp(prop, value);
}

void Capacitor::simplePropertyEntry(const QString & text) {
	
	FocusOutComboBox * focusOutComboBox = qobject_cast<FocusOutComboBox *>(sender());
	if (focusOutComboBox == NULL) return;

	foreach (PropertyDef * propertyDef, m_comboBoxes.keys()) {
		if (m_comboBoxes.value(propertyDef) == focusOutComboBox) {
			InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
			if (infoGraphicsView != NULL) {
				infoGraphicsView->setProp(this, propertyDef->name, "", m_propertyDefs.value(propertyDef, ""), text, true);
			}
			break;
		}
	}
}

void Capacitor::getProperties(QHash<QString, QString> & hash) {
	foreach (PropertyDef * propertyDef, m_propertyDefs.keys()) {
		hash.insert(propertyDef->name, m_propertyDefs.value(propertyDef));
	}
}
