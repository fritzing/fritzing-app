/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2026 Fritzing GmbH

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

#include "items/partlabelcontextmenu.h"
#include "items/partlabel.h"
#include "items/itembase.h"
#include "sketch/sketchwidget.h"
#include "model/modelpart.h"
#include "mainwindow/fprobeactions.h"
#include "viewlayer.h"

#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QGraphicsScene>
#include <QSet>

// The Flip/Rotate entries: the single source of truth shared by this menu and
// the SelectedPartLabel probe. Names/status tips stay in the "PartLabel"
// translation context (QT_TRANSLATE_NOOP) so existing translations carry over
// from when this menu lived in PartLabel.
namespace {

struct RotateFlipEntry {
	const char * name;          // untranslated; the action's lookup key
	const char * statusTip;
	double degrees;             // 0 for a flip
	Qt::Orientations orientation;
	bool pcbOnly;               // the 45/135 degree steps exist only in PCB view
};

const RotateFlipEntry RotateFlipTable[] = {
	{ QT_TRANSLATE_NOOP("PartLabel", "Rotate 45° Clockwise"),
	  QT_TRANSLATE_NOOP("PartLabel", "Rotate the label by 45 degrees clockwise"), 45, {}, true },
	{ QT_TRANSLATE_NOOP("PartLabel", "Rotate 90° Clockwise"),
	  QT_TRANSLATE_NOOP("PartLabel", "Rotate the label by 90 degrees clockwise"), 90, {}, false },
	{ QT_TRANSLATE_NOOP("PartLabel", "Rotate 135° Clockwise"),
	  QT_TRANSLATE_NOOP("PartLabel", "Rotate the label by 135 degrees clockwise"), 135, {}, true },
	{ QT_TRANSLATE_NOOP("PartLabel", "Rotate 180°"),
	  QT_TRANSLATE_NOOP("PartLabel", "Rotate the label by 180 degrees"), 180, {}, false },
	{ QT_TRANSLATE_NOOP("PartLabel", "Rotate 135° Counter Clockwise"),
	  QT_TRANSLATE_NOOP("PartLabel", "Rotate the label by 135 degrees counter clockwise"), 225, {}, true },
	{ QT_TRANSLATE_NOOP("PartLabel", "Rotate 90° Counter Clockwise"),
	  QT_TRANSLATE_NOOP("PartLabel", "Rotate current selection 90 degrees counter clockwise"), 270, {}, false },
	{ QT_TRANSLATE_NOOP("PartLabel", "Rotate 45° Counter Clockwise"),
	  QT_TRANSLATE_NOOP("PartLabel", "Rotate the label by 45 degrees counter clockwise"), 315, {}, true },
	{ QT_TRANSLATE_NOOP("PartLabel", "Flip Horizontal"),
	  QT_TRANSLATE_NOOP("PartLabel", "Flip label horizontally"), 0, Qt::Horizontal, false },
	{ QT_TRANSLATE_NOOP("PartLabel", "Flip Vertical"),
	  QT_TRANSLATE_NOOP("PartLabel", "Flip label vertically"), 0, Qt::Vertical, false },
};

// Translate in PartLabel's context, so strings carry the translations they had
// when this menu was built inside PartLabel.
QString trLabel(const char * s) {
	return QApplication::translate("PartLabel", s);
}

}

PartLabelContextMenu::PartLabelContextMenu(SketchWidget * view)
	: QObject(view)
	, m_view(view)
{
	buildMenu();
}

void PartLabelContextMenu::buildMenu()
{
	m_menu = new QMenu(QStringLiteral("PartLabel"), m_view);

	QAction * editAct = m_menu->addAction(trLabel("Edit"));
	editAct->setStatusTip(trLabel("Edit label text"));
	connect(editAct, &QAction::triggered, this, [this]() {
		if (PartLabel * label = selectedLabel()) label->partLabelEdit();
	});

	QAction * hideAct = m_menu->addAction(trLabel("Hide"));
	hideAct->setStatusTip(trLabel("Hide part label"));
	connect(hideAct, &QAction::triggered, this, [this]() {
		PartLabel * label = selectedLabel();
		if (label != nullptr && label->owner() != nullptr) label->owner()->hidePartLabel();
	});

	m_menu->addSeparator();

	m_displayValuesMenu = m_menu->addMenu(trLabel("Display Values"));
	m_flipRotateMenu = m_menu->addMenu(trLabel("Flip/Rotate"));
	QMenu * fontSizeMenu = m_menu->addMenu(trLabel("Font Size"));

	const bool include45 = (m_view != nullptr) && (m_view->viewID() == ViewLayer::PCBView);
	for (const RotateFlipEntry & entry : RotateFlipTable) {
		if (entry.pcbOnly && !include45) continue;
		QAction * act = m_flipRotateMenu->addAction(trLabel(entry.name));
		act->setStatusTip(trLabel(entry.statusTip));
		act->setData(QString::fromUtf8(entry.name));        // untranslated lookup key
		const double degrees = entry.degrees;
		const Qt::Orientations orientation = entry.orientation;
		connect(act, &QAction::triggered, this, [this, degrees, orientation]() {
			PartLabel * label = selectedLabel();
			if (label != nullptr && label->owner() != nullptr)
				label->owner()->rotateFlipPartLabel(degrees, orientation);
		});
	}

	// Display Values: the static "Label text" toggle; the per-part property rows
	// are (re)built in prepare().
	m_labelTextAct = m_displayValuesMenu->addAction(trLabel("Label text"));
	m_labelTextAct->setStatusTip(trLabel("Display the text of the label"));
	m_labelTextAct->setCheckable(true);
	connect(m_labelTextAct, &QAction::triggered, this, [this]() {
		if (PartLabel * label = selectedLabel()) label->toggleDisplayKey(QString());   // LabelTextKey
	});
	m_displayValuesMenu->addSeparator();

	// Font Size: checkable radio, refreshed in prepare().
	m_tinyAct = fontSizeMenu->addAction(trLabel("Tiny"));
	m_tinyAct->setStatusTip(trLabel("Set font size to tiny"));
	m_tinyAct->setCheckable(true);
	connect(m_tinyAct, &QAction::triggered, this, [this]() {
		if (PartLabel * label = selectedLabel()) label->setFontPointSize(m_view->getLabelFontSizeTiny());
	});

	m_smallAct = fontSizeMenu->addAction(trLabel("Small"));
	m_smallAct->setStatusTip(trLabel("Set font size to small"));
	m_smallAct->setCheckable(true);
	connect(m_smallAct, &QAction::triggered, this, [this]() {
		if (PartLabel * label = selectedLabel()) label->setFontPointSize(m_view->getLabelFontSizeSmall());
	});

	m_mediumAct = fontSizeMenu->addAction(trLabel("Medium"));
	m_mediumAct->setStatusTip(trLabel("Set font size to medium"));
	m_mediumAct->setCheckable(true);
	connect(m_mediumAct, &QAction::triggered, this, [this]() {
		if (PartLabel * label = selectedLabel()) label->setFontPointSize(m_view->getLabelFontSizeMedium());
	});

	m_largeAct = fontSizeMenu->addAction(trLabel("Large"));
	m_largeAct->setStatusTip(trLabel("Set font size to large"));
	m_largeAct->setCheckable(true);
	connect(m_largeAct, &QAction::triggered, this, [this]() {
		if (PartLabel * label = selectedLabel()) label->setFontPointSize(m_view->getLabelFontSizeLarge());
	});

	connect(m_menu, &QMenu::aboutToShow, this, [this]() { prepare(); });

	// Make the real menu testable like the part item menus (e.g. PCBItemActions).
	QString viewName;
	switch (m_view->viewID()) {
	case ViewLayer::BreadboardView: viewName = "Breadboard"; break;
	case ViewLayer::SchematicView: viewName = "Schematic"; break;
	case ViewLayer::PCBView: viewName = "PCB"; break;
	default: viewName = "Unknown"; break;
	}
	new FProbeActions(viewName + "Label", m_menu, this);
}

void PartLabelContextMenu::prepare()
{
	PartLabel * label = selectedLabel();
	if (label == nullptr) return;

	const QStringList & displayKeys = label->displayKeys();

	// Rebuild the per-part Display Values property rows.
	for (QAction * act : m_propertyActs) {
		m_displayValuesMenu->removeAction(act);
		act->deleteLater();
	}
	m_propertyActs.clear();

	m_labelTextAct->setChecked(displayKeys.contains(QString()));    // LabelTextKey

	ItemBase * owner = label->owner();
	if (owner != nullptr && owner->modelPart() != nullptr) {
		const QHash<QString, QString> & properties = owner->modelPart()->properties();
		const QList<QString> keys = properties.keys();
		for (const QString & key : keys) {
			QString translatedName = ItemBase::translatePropertyName(key);
			QAction * act = m_displayValuesMenu->addAction(translatedName);
			act->setCheckable(true);
			act->setChecked(displayKeys.contains(key));
			act->setStatusTip(trLabel("Display the value of property %1").arg(translatedName));
			connect(act, &QAction::triggered, this, [this, key]() {
				if (PartLabel * l = selectedLabel()) l->toggleDisplayKey(key);
			});
			m_propertyActs.append(act);
		}
	}

	// Font size radio: check the preset that matches the label's current size.
	const int fs = label->labelFont().pointSize();
	m_tinyAct->setChecked(fs == m_view->getLabelFontSizeTiny());
	m_smallAct->setChecked(fs == m_view->getLabelFontSizeSmall());
	m_mediumAct->setChecked(fs == m_view->getLabelFontSizeMedium());
	m_largeAct->setChecked(fs == m_view->getLabelFontSizeLarge());
}

void PartLabelContextMenu::popup(const QPoint & screenPos)
{
	if (selectedLabel() == nullptr) return;
#ifdef Q_OS_WASM
	// In the browser only one event loop can run nested in the main one (Asyncify),
	// so a modal dialog opened from a menu running its own loop (exec()) gets no input
	m_menu->popup(screenPos);    // aboutToShow -> prepare()
#else
	m_menu->exec(screenPos);    // aboutToShow -> prepare()
#endif
}

QStringList PartLabelContextMenu::rotateFlipActionNames() const
{
	QStringList names;
	if (m_flipRotateMenu == nullptr) return names;
	const QList<QAction *> actions = m_flipRotateMenu->actions();
	for (QAction * act : actions) {
		const QString name = act->data().toString();
		if (!name.isEmpty()) names.append(name);   // skip separators
	}
	return names;
}

bool PartLabelContextMenu::triggerRotateFlip(const QString & untranslatedName)
{
	if (m_flipRotateMenu == nullptr) return false;
	const QList<QAction *> actions = m_flipRotateMenu->actions();
	for (QAction * act : actions) {
		if (act->data().toString() == untranslatedName) {
			act->trigger();
			return true;
		}
	}
	return false;
}

PartLabel * PartLabelContextMenu::selectedLabel() const
{
	if (m_view == nullptr || m_view->scene() == nullptr) return nullptr;

	// A PCB part is split into one ItemBase per layer, all selected together;
	// collapse onto the layer kin chief so one selected part counts once.
	QSet<ItemBase *> parts;
	const QList<QGraphicsItem *> selected = m_view->scene()->selectedItems();
	for (QGraphicsItem * item : selected) {
		auto * base = dynamic_cast<ItemBase *>(item);
		if (base != nullptr) parts.insert(base->layerKinChief());
	}
	if (parts.count() != 1) return nullptr;

	ItemBase * part = *parts.constBegin();
	PartLabel * label = part->partLabel();
	if (label == nullptr || !label->initialized()) return nullptr;
	return label;
}
