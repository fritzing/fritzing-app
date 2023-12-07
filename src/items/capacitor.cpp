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
	Q_FOREACH (PropertyDef * propertyDef, m_propertyDefs.keys()) {
		if (prop.compare(propertyDef->name, Qt::CaseInsensitive) == 0) {
			returnProp = TranslatedPropertyNames.value(prop);
			if (returnProp.isEmpty()) {
				returnProp = propertyDef->name;
			}

			auto * focusOutComboBox = new FocusOutComboBox();
			focusOutComboBox->setEnabled(swappingEnabled);
			focusOutComboBox->setEditable(propertyDef->editable);
			focusOutComboBox->setObjectName("infoViewComboBox");
			QString current = m_propertyDefs.value(propertyDef);
			if (current.isEmpty() && !propertyDef->defaultValue.isEmpty()) {
				current = propertyDef->defaultValue + propertyDef->symbol;
				setProp(propertyDef->name, propertyDef->defaultValue + propertyDef->symbol);
			}
			if (propertyDef->editable) {
				focusOutComboBox->setToolTip(tr("Select from the dropdown, or type in a %1 value").arg(returnProp));
			}

			if (propertyDef->numeric) {
				focusOutComboBox->setToolTip(tr("Select from the dropdown, or type in a %1 value\n"
												"Range: [%2 - %3] %4\n"
												"Background: Green = ok, Red = incorrect value, Grey = current value").
											 arg(returnProp).
											 arg(TextUtils::convertToPowerPrefix(propertyDef->minValue)).
											 arg(TextUtils::convertToPowerPrefix(propertyDef->maxValue)).
											 arg(propertyDef->symbol));
				if (!current.isEmpty()) {
					double val = TextUtils::convertFromPowerPrefixU(current, propertyDef->symbol);
					if (!propertyDef->menuItems.contains(val)) {
						propertyDef->menuItems.append(val);
					}
				}
				Q_FOREACH(double q, propertyDef->menuItems) {
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
				auto * validator = new BoundedRegExpValidator(focusOutComboBox);
				validator->setSymbol(propertyDef->symbol);
				validator->setConverter(TextUtils::convertFromPowerPrefix);
				if (propertyDef->maxValue > propertyDef->minValue) {
					validator->setBounds(propertyDef->minValue, propertyDef->maxValue);
				}
                QString symbolRegExp = propertyDef->symbol.isEmpty() ? "" : QString("[%1]{0,1}").arg(propertyDef->symbol);

    //			QString pattern = QString("((\\d{0,10})|(\\d{0,10}\\.)|(\\d{0,10}\\.\\d{1,10}))[%1]{0,1}%2")
                QString pattern = QString("((-?\\d{1,3})|(-?\\d{1,3}\\.)|(-?\\d{1,3}\\.\\d{1,2}))[%1]{0,1}%2").arg(
    //			QString pattern = QString("((\\d{0,3})|(\\d{0,3}\\.)|(\\d{0,3}\\.\\d{1,3}))[%1]{0,1}%2")
					TextUtils::PowerPrefixesString, 
                    symbolRegExp
                );
				validator->setRegularExpression(QRegularExpression(pattern));
				focusOutComboBox->setValidator(validator);
				connect(focusOutComboBox->validator(), SIGNAL(sendState(QValidator::State)), this, SLOT(textModified(QValidator::State)));
				connect(focusOutComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(propertyEntry(int)));
			}
			else {
				connect(focusOutComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(simplePropertyEntry(int)));
			}

			this->m_comboBoxes.insert(propertyDef, focusOutComboBox);

			returnValue = focusOutComboBox->currentText();
			returnWidget = focusOutComboBox;

			return true;
		}
	}

	return PaletteItem::collectExtraInfo(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget, hide);
}

/**
 * Sets the appropriate background colour in the combo box when the text is modified. When a user changes the text of an editable
 * property in a combo box, this function is called. The function checks the validator of the combo box and sets a red background if the
 * state is Intermediate (the value does is not within the limits of the property) or green if the state is Acceptable. Invalid states
 * are not possible as the characters that make the string invalid are deleted if introduced.
 * not changed in this function.
 * @param[in] text The new string in the combo box.
 */
void Capacitor::textModified(QValidator::State state) {
	BoundedRegExpValidator * validator = qobject_cast<BoundedRegExpValidator *>(sender());
	if (validator == NULL) return;
	FocusOutComboBox * focusOutComboBox = qobject_cast<FocusOutComboBox *>(validator->parent());
	if (focusOutComboBox == NULL) return;

	if (state == QValidator::Acceptable) {
		QColor backColor = QColor(210, 246, 210);
		QLineEdit *lineEditor = focusOutComboBox->lineEdit();
		QPalette pal = lineEditor->palette();
		pal.setColor(QPalette::Base, backColor);
		lineEditor->setPalette(pal);
	} else if (state == QValidator::Intermediate) {
		QColor backColor = QColor(246, 210, 210);
		QLineEdit *lineEditor = focusOutComboBox->lineEdit();
		QPalette pal = lineEditor->palette();
		pal.setColor(QPalette::Base, backColor);
		lineEditor->setPalette(pal);
	}
}

void Capacitor::propertyEntry(int index) {
	auto * focusOutComboBox = qobject_cast<FocusOutComboBox *>(sender());
	if (focusOutComboBox == nullptr) return;
	QString text = focusOutComboBox->itemText(index);

	Q_FOREACH (PropertyDef * propertyDef, m_comboBoxes.keys()) {
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
			if (infoGraphicsView != nullptr) {
				infoGraphicsView->setProp(this, propertyDef->name, "", m_propertyDefs.value(propertyDef, ""), utext, true);
			}
			break;
		}
	}
}

void Capacitor::setProp(const QString & prop, const QString & value) {
	Q_FOREACH (PropertyDef * propertyDef, m_propertyDefs.keys()) {
		if (prop.compare(propertyDef->name, Qt::CaseInsensitive) == 0) {
			m_propertyDefs.insert(propertyDef, value);
			modelPart()->setLocalProp(propertyDef->name, value);
			if (m_partLabel != nullptr) m_partLabel->displayTextsIf();
			return;
		}
	}

	PaletteItem::setProp(prop, value);
}

void Capacitor::simplePropertyEntry(int index) {

	auto * focusOutComboBox = qobject_cast<FocusOutComboBox *>(sender());
	if (focusOutComboBox == nullptr) return;
	QString text = focusOutComboBox->itemText(index);

	Q_FOREACH (PropertyDef * propertyDef, m_comboBoxes.keys()) {
		if (m_comboBoxes.value(propertyDef) == focusOutComboBox) {
			InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
			if (infoGraphicsView != nullptr) {
				infoGraphicsView->setProp(this, propertyDef->name, "", m_propertyDefs.value(propertyDef, ""), text, true);
			}
			break;
		}
	}
}

void Capacitor::getProperties(QHash<QString, QString> & hash) {
	Q_FOREACH (PropertyDef * propertyDef, m_propertyDefs.keys()) {
		hash.insert(propertyDef->name, m_propertyDefs.value(propertyDef));
	}
}

QHash<QString, QString> Capacitor::prepareProps(ModelPart * modelPart, bool wantDebug, QStringList & keys)
{
	QHash<QString, QString> props = ItemBase::prepareProps(modelPart, wantDebug, keys);

	// ensure capacitance and other properties are after family, if it is a capacitor;
	if (keys.removeOne("capacitance")) {
		keys.insert(1, "capacitance");
		if (keys.removeOne("voltage")) keys.insert(2, "voltage");
	}

	return props;
}
