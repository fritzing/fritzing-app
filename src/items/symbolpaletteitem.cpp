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

#include "symbolpaletteitem.h"
#include "../debugdialog.h"
#include "../connectors/connectoritem.h"
#include "../connectors/bus.h"
#include "moduleidnames.h"
#include "../fsvgrenderer.h"
#include "../utils/textutils.h"
#include "../utils/focusoutcombobox.h"
#include "../utils/graphicsutils.h"
#include "../sketch/infographicsview.h"
#include "partlabel.h"
#include "partfactory.h"
#include "layerkinpaletteitem.h"
#include "../svg/svgfilesplitter.h"

#include <QLineEdit>
#include <QMultiHash>
#include <QMessageBox>
#include <QComboBox>
#include <QSettings>
#include <QPixmap>
#include <QPainter>
#include <QSvgRenderer>
#include <QFontMetricsF>
#include <QIcon>

#include <cmath>

#define VOLTAGE_HASH_CONVERSION 1000000
#define FROMVOLTAGE(v) ((long) (v * VOLTAGE_HASH_CONVERSION))

static QMultiHash<long, QPointer<ConnectorItem> > LocalVoltages;			// Qt doesn't do Hash keys with double
static QMultiHash<QString, QPointer<ConnectorItem> > LocalNetLabels;
static QList< QPointer<ConnectorItem> > LocalGrounds;
static QList<double> Voltages;
double SymbolPaletteItem::DefaultVoltage = 5;

// A net label's "style" localProp stores the actual text alignment in the label's own
// (unflipped) frame: which box edge the text hugs. Flipping the label mirrors it on screen.
static const QString NetLabelAlignLeft("left");
static const QString NetLabelAlignRight("right");
static const QString NetLabelStyleLegacy("legacy");
// The Schematic preferences default uses an orientation-independent vocabulary instead
// (outside = away from the connector, connector = at the connector); it only picks the
// initial alignment for a new (right-pointing) net label.
static const QString NetLabelStyleOutside("outside");
static const QString NetLabelStyleConnector("connector");

// Cached default net label style, read lazily from QSettings (the "schemNetLabelStyle"
// key, set in the Schematic preferences tab). makeSvg/effectiveAlign run on every render,
// so we avoid constructing a QSettings object each time.
static QString NetLabelDefaultStyle;
static bool NetLabelDefaultStyleLoaded = false;

// Leading and trailing whitespace in a net label is purely cosmetic (it is used to
// align the box sizes of several net labels), so it must be ignored when grouping
// net labels into sub-nets. Inner whitespace is kept significant.
static QString netLabelKey(const QString & label) {
	return label.trimmed();
}

/////////////////////////////////////////////////////

/*
FocusBugLineEdit::FocusBugLineEdit(QWidget * parent) : QLineEdit(parent)
{
	connect(this, SIGNAL(editingFinished()), this, SLOT(editingFinishedSlot()));
    m_lastEditingFinishedEmit = QTime::currentTime();
}

FocusBugLineEdit::~FocusBugLineEdit()
{
}

void FocusBugLineEdit::editingFinishedSlot() {
    QTime now = QTime::currentTime();
    int d = m_lastEditingFinishedEmit.msecsTo(now);
    DebugDialog::debug(QString("dtime %1").arg(d));
    if (d < 1000) {
        return;
    }

    m_lastEditingFinishedEmit = now;
    emit safeEditingFinished();
}
*/

////////////////////////////////////////

SymbolPaletteItem::SymbolPaletteItem( ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: PaletteItem(modelPart, viewID, viewGeometry, id, itemMenu, doLabel)
{
	if (Voltages.count() == 0) {
		Voltages.append(0.0);
		Voltages.append(3.3);
		Voltages.append(5.0);
		Voltages.append(12.0);
	}

	m_connector0 = m_connector1 = nullptr;
	m_voltage = 0;
	m_voltageReference = (modelPart->properties().value("type").compare("voltage reference") == 0);

	if (modelPart->moduleID().endsWith(ModuleIDNames::NetLabelModuleIDName)) {
		m_isNetLabel = true;
	}
	else {
		m_isNetLabel = modelPart->moduleID().endsWith(ModuleIDNames::PowerLabelModuleIDName);

		bool ok;
		double temp = modelPart->localProp("voltage").toDouble(&ok);
		if (ok) {
			m_voltage = temp;
		}
		else {
			if (ok) {
				m_voltage = temp;
			}
			else {
				m_voltage = SymbolPaletteItem::DefaultVoltage;
			}
			modelPart->setLocalProp("voltage", m_voltage);
		}
		if (!Voltages.contains(m_voltage)) {
			Voltages.append(m_voltage);
		}
	}
}

SymbolPaletteItem::~SymbolPaletteItem() {
	if (m_isNetLabel) {
		Q_FOREACH (QString key, LocalNetLabels.uniqueKeys()) {
			if (m_connector0 != nullptr) {
				LocalNetLabels.remove(key, m_connector0);
			}
			if (m_connector1 != nullptr) {
				LocalNetLabels.remove(key, m_connector1);
			}
			LocalNetLabels.remove(key, nullptr);		// cleans null QPointers
		}
	}
	else {
		if (m_connector0 != nullptr) LocalGrounds.removeOne(m_connector0);
		if (m_connector1 != nullptr) LocalGrounds.removeOne(m_connector1);
		LocalGrounds.removeOne(QPointer<ConnectorItem>(nullptr));   // cleans null QPointers

		Q_FOREACH (long key, LocalVoltages.uniqueKeys()) {
			if (m_connector0 != nullptr) {
				LocalVoltages.remove(key, m_connector0);
			}
			if (m_connector1 != nullptr) {
				LocalVoltages.remove(key, m_connector1);
			}
			LocalVoltages.remove(key, nullptr);		// cleans null QPointers
		}
	}
}

void SymbolPaletteItem::removeMeFromBus(double v) {
	Q_FOREACH (ConnectorItem * connectorItem, cachedConnectorItems()) {
		if (m_isNetLabel) {
			if (m_voltageReference) {
				LocalNetLabels.remove(netLabelKey(getLabel()), connectorItem);
			} else {
				LocalNetLabels.remove(netLabelKey(m_label), connectorItem);
			}
		}
		else {
			double nv = useVoltage(connectorItem);
			if (std::fabs(nv - v) < 0.00001 ) {
				//connectorItem->debugInfo(QString("remove %1").arg(useVoltage(connectorItem)));

				bool gotOne = LocalGrounds.removeOne(connectorItem);
				int count = LocalVoltages.remove(FROMVOLTAGE(v), connectorItem);
				LocalVoltages.remove(FROMVOLTAGE(v), nullptr);


				if (count == 0 && !gotOne) {
					DebugDialog::debug(QString("removeMeFromBus failed %1 %2 %3 %4")
					                   .arg(this->id())
					                   .arg(connectorItem->connectorSharedID())
					                   .arg(v).arg(nv));
				}
			}
		}
	}
	LocalGrounds.removeOne(QPointer<ConnectorItem>(nullptr));  // keep cleaning these out
}

void SymbolPaletteItem::swapEntry(int index)
{
	// Before swapping the item, remove it from the bus,
	// so item-to-be-deleted (this one) doesn't count against
	// connections when calling restoreColor on the replacement
	removeMeFromBus(0);
	ItemBase::swapEntry(index);
}


ConnectorItem* SymbolPaletteItem::newConnectorItem(Connector *connector)
{
	ConnectorItem * connectorItem = PaletteItemBase::newConnectorItem(connector);

	if (connector->connectorSharedID().compare("connector0") == 0) {
		m_connector0 = connectorItem;
	}
	else if (connector->connectorSharedID().compare("connector1") == 0) {
		m_connector1 = connectorItem;
	}
	else {
		return connectorItem;
	}

	if (m_isNetLabel) {
		LocalNetLabels.insert(netLabelKey(getLabel()), connectorItem);
	}
	else if (connectorItem->isGrounded()) {
		LocalGrounds.append(connectorItem);
		//connectorItem->debugInfo("new ground insert");
	}
	else {
		LocalVoltages.insert(FROMVOLTAGE(useVoltage(connectorItem)), connectorItem);
		//connectorItem->debugInfo(QString("new voltage insert %1").arg(useVoltage(connectorItem)));
	}
	return connectorItem;
}

bool SymbolPaletteItem::busConnectorItems(ConnectorItem * fromConnectorItem, QList<class ConnectorItem *> & items) {
	auto * bus = fromConnectorItem->bus();
	if (bus == nullptr) return false;

	PaletteItem::busConnectorItems(fromConnectorItem, items);

	//foreach (ConnectorItem * bc, items) {
	//bc->debugInfo(QString("bc %1").arg(bus->id()));
	//}

	QList< QPointer<ConnectorItem> > mitems;
	if (m_isNetLabel) {
		mitems.append(LocalNetLabels.values(netLabelKey(getLabel())));
	}
	else if (bus->id().compare("groundbus", Qt::CaseInsensitive) == 0) {
		mitems.append(LocalGrounds);
	}
	else {
		mitems.append(LocalVoltages.values(FROMVOLTAGE(m_voltage)));
	}
	Q_FOREACH (ConnectorItem * connectorItem, mitems) {
		if (connectorItem == nullptr) continue;

		if (connectorItem->scene() == this->scene()) {
			items.append(connectorItem);
			//connectorItem->debugInfo(QString("symbol bus %1").arg(bus->id()));
		}
	}
	return true;
}

double SymbolPaletteItem::voltage() {
	return m_voltage;
}

void SymbolPaletteItem::setProp(const QString & prop, const QString & value) {
	if (prop.compare("voltage", Qt::CaseInsensitive) == 0) {
		setVoltage(value.toDouble());
		return;
	}
	if (prop.compare("label", Qt::CaseInsensitive) == 0 && m_isNetLabel) {
		setLabel(value);
		return;
	}
	if (prop.compare("style", Qt::CaseInsensitive) == 0 && m_isNetLabel) {
		setStyle(value);
		return;
	}

	PaletteItem::setProp(prop, value);
}

void SymbolPaletteItem::setLabel(const QString & label) {
	removeMeFromBus(0); //Remove this specific item (bb, sch or pcb) from the previous net label
	m_label = label; // Update the label for this specific item (bb, sch or pcb)
	m_modelPart->setLocalProp("label", label); //This line modifies the property label in the bb, sch and pcb items

	//Add the conectors of the item to the new net label
	Q_FOREACH (ConnectorItem * connectorItem, cachedConnectorItems()) {
		LocalNetLabels.insert(netLabelKey(label), connectorItem);
	}

	QTransform  transform = untransform();

	QString svg = makeSvg(this->viewLayerID());
	resetRenderer(svg);
	resetLayerKin();
	resetConnectors(nullptr, nullptr);

	retransform(transform);
}

QString SymbolPaletteItem::defaultNetLabelStyle() {
	if (!NetLabelDefaultStyleLoaded) {
		QSettings settings;
		QString value = settings.value("schemNetLabelStyle", NetLabelStyleOutside).toString();
		// Legacy is only ever the initial state of old parts, never a configurable default.
		NetLabelDefaultStyle = (value == NetLabelStyleConnector) ? NetLabelStyleConnector : NetLabelStyleOutside;
		NetLabelDefaultStyleLoaded = true;
	}
	return NetLabelDefaultStyle;
}

void SymbolPaletteItem::refreshDefaultNetLabelStyle() {
	NetLabelDefaultStyleLoaded = false;
}

QString SymbolPaletteItem::effectiveAlign() {
	// Old (v4 and earlier) net labels render in the legacy Droid Sans style. Switching a
	// label between legacy and the modern aligned styles is a part swap (v4 <-> v5), so the
	// part version -- not a local prop -- is the source of truth for legacy-ness.
	if (modelPart()->modelPartShared()->version().toInt() <= 4) {
		return NetLabelStyleLegacy;
	}
	QString s = modelPart()->localProp("style").toString();
	// Normalize the orientation-independent policy values to a concrete alignment. New
	// net labels point right (connector on the right), so outside = left edge / connector
	// = right edge. This also covers sketches saved with the earlier outside/connector
	// values before storage switched to left/right.
	if (s == NetLabelStyleOutside) return NetLabelAlignLeft;
	if (s == NetLabelStyleConnector) return NetLabelAlignRight;
	if (s == NetLabelAlignLeft || s == NetLabelAlignRight || s == NetLabelStyleLegacy) {
		return s;
	}
	// Empty or unknown: fall back to the configured default policy.
	return (defaultNetLabelStyle() == NetLabelStyleOutside) ? NetLabelAlignLeft : NetLabelAlignRight;
}

QString SymbolPaletteItem::alignForPolicy(const QString & policy, bool goLeft) {
	// The connector is on the local-left edge when goLeft. "outside" puts the text on the
	// non-connector edge, "connector" on the connector edge.
	if (policy == NetLabelStyleConnector) {
		return goLeft ? NetLabelAlignLeft : NetLabelAlignRight;
	}
	return goLeft ? NetLabelAlignRight : NetLabelAlignLeft;   // outside
}

QString SymbolPaletteItem::policyForAlign(const QString & align, bool goLeft) {
	// Inverse of alignForPolicy: the text is at the connector when its edge matches the
	// connector's (local-left when goLeft).
	bool textLeft = (align == NetLabelAlignLeft);
	return (textLeft == goLeft) ? NetLabelStyleConnector : NetLabelStyleOutside;
}

void SymbolPaletteItem::setStyle(const QString & style) {
	m_modelPart->setLocalProp("style", style);

	// Only the text alignment within the box changes (the connector/box geometry is
	// identical across styles), but re-render via the same proven path as setLabel().
	QTransform transform = untransform();

	QString svg = makeSvg(this->viewLayerID());
	resetRenderer(svg);
	resetLayerKin();
	resetConnectors(nullptr, nullptr);

	retransform(transform);
}

void SymbolPaletteItem::refreshNetLabelStyleFromDefault() {
	// Re-render a net label that follows the configurable default style (used when the
	// default changes in the preferences). Labels with an explicit style, and legacy
	// (v4) labels, are unaffected.
	if (!m_isNetLabel) return;
	if (modelPart()->modelPartShared()->version().toInt() <= 4) return;
	if (!modelPart()->localProp("style").toString().isEmpty()) return;

	QTransform transform = untransform();

	QString svg = makeSvg(this->viewLayerID());
	resetRenderer(svg);
	resetLayerKin();
	resetConnectors(nullptr, nullptr);

	retransform(transform);
}

void SymbolPaletteItem::setVoltage(double v) {
	removeMeFromBus(m_voltage);

	m_voltage = v;
	m_modelPart->setLocalProp("voltage", v);
	if (!Voltages.contains(v)) {
		Voltages.append(v);
	}

	if (m_isNetLabel) {
		Q_FOREACH (ConnectorItem * connectorItem, cachedConnectorItems()) {
			LocalNetLabels.insert(QString::number(m_voltage), connectorItem);
		}
	}
	else {
		Q_FOREACH (ConnectorItem * connectorItem, cachedConnectorItems()) {
			if (connectorItem->isGrounded()) {
				LocalGrounds.append(connectorItem);
				//connectorItem->debugInfo("ground insert");

			}
			else {
				LocalVoltages.insert(FROMVOLTAGE(v), connectorItem);
				//connectorItem->debugInfo(QString("voltage insert %1").arg(useVoltage(connectorItem)));
			}
		}
	}

	if (m_viewID == ViewLayer::SchematicView) {
		if (m_voltageReference || m_isNetLabel) {
			QTransform transform = untransform();
			QString svg = makeSvg(viewLayerID());
			reloadRenderer(svg, false);

			resetLayerKin();

			retransform(transform);

			if (m_partLabel != nullptr) m_partLabel->displayTextsIf();
		}
	}
}

QString SymbolPaletteItem::makeSvg(ViewLayer::ViewLayerID viewLayerID) {
	QString path = filename();
	QFile file(filename());
	QString svg;
	if (file.open(QFile::ReadOnly)) {
		svg = file.readAll();
		file.close();
		svg = replaceTextElement(svg);
		if (viewLayerID == ViewLayer::SchematicText) {
			bool hasText;
			return SvgFileSplitter::showText3(svg, hasText);
		}
		else {
			return SvgFileSplitter::hideText3(svg);
		}
	}

	return "";
}

QString SymbolPaletteItem::replaceTextElement(QString svg) {
	double v = ((int) (m_voltage * 1000)) / 1000.0;
	return TextUtils::replaceTextElement(svg, "label", QString::number(v) + "V");
}

QString SymbolPaletteItem::getProperty(const QString & key) {
	if (key.compare("voltage", Qt::CaseInsensitive) == 0) {
		return QString::number(m_voltage);
	}
	if (key.compare("style", Qt::CaseInsensitive) == 0 && m_isNetLabel) {
		return effectiveAlign();
	}

	return PaletteItem::getProperty(key);
}

double SymbolPaletteItem::useVoltage(ConnectorItem * connectorItem) {
	return (connectorItem->connectorSharedName().compare("GND", Qt::CaseInsensitive) == 0) ? 0 : m_voltage;
}

ConnectorItem * SymbolPaletteItem::connector0() {
	return m_connector0;
}

ConnectorItem * SymbolPaletteItem::connector1() {
	return m_connector1;
}

void SymbolPaletteItem::addedToScene(bool temporary)
{
	if (this->scene() != nullptr) {
		setVoltage(m_voltage);
	}

	return PaletteItem::addedToScene(temporary);
}

QString SymbolPaletteItem::retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor)
{
	QString svg = PaletteItem::retrieveSvg(viewLayerID, svgHash, blackOnly, dpi, factor);
	if (m_voltageReference) {
		switch (viewLayerID) {
		case ViewLayer::Schematic:
			svg = replaceTextElement(svg); // So even the hidden text is consistent as is done for the netlabel.
			return SvgFileSplitter::hideText3(svg);
		case ViewLayer::SchematicText:
			svg = replaceTextElement(svg);
			bool hasText;
			svg = SvgFileSplitter::showText3(svg, hasText);
			return transformTextSvg(svg);
		default:
			break;
		}
	}

	return svg;
}

bool SymbolPaletteItem::collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide)
{
	if ((prop.compare("voltage", Qt::CaseInsensitive) == 0) &&
	        (moduleID().compare(ModuleIDNames::GroundModuleIDName) != 0))
	{
		auto * edit = new FocusOutComboBox(parent);
		edit->setEnabled(swappingEnabled);
		int ix = 0;
		Q_FOREACH (double v, Voltages) {
			edit->addItem(QString::number(v));
			if (v == m_voltage) {
				edit->setCurrentIndex(ix);
			}
			ix++;
		}

		auto * validator = new QDoubleValidator(edit);
		validator->setRange(-9999.99, 9999.99, 2);
		validator->setLocale(QLocale::C);
		validator->setNotation(QDoubleValidator::StandardNotation);
		validator->setLocale(QLocale::C);
		edit->setValidator(validator);

		edit->setObjectName("infoViewComboBox");


		connect(edit, SIGNAL(currentIndexChanged(int)), this, SLOT(voltageEntry(int)));
		returnWidget = edit;

		returnValue = QString("%1").arg(m_voltage);
		returnProp = tr("voltage");
		return true;
	}

	// When several net labels are selected, the inspector switches to a group mode: the
	// per-label text field is hidden, and the alignment is expressed with the orientation-
	// independent outside/connector policy applied to the whole selection.
	bool groupMode = false;
	QList<SymbolPaletteItem *> selectedNetLabels;
	if (isOnlyNetLabel()) {
		InfoGraphicsView * igv = InfoGraphicsView::getInfoGraphicsView(this);
		if ((igv != nullptr) && igv->collectSelectedNetLabels(selectedNetLabels) > 1
		        && selectedNetLabels.contains(this)) {
			groupMode = true;
		}
	}

	if (prop.compare("label", Qt::CaseInsensitive) == 0 && m_isNetLabel)
	{
		// The net label name is edited via the inspector's title field, so this duplicate
		// property row is always hidden (single and group).
		hide = true;
		return true;
	}

	if (prop.compare("style", Qt::CaseInsensitive) == 0 && m_isNetLabel)
	{
		auto * edit = new FocusOutComboBox(parent);
		edit->setEnabled(swappingEnabled);
		edit->setObjectName("infoViewComboBox");
		returnProp = tr("style");

		if (groupMode) {
			// Orientation-independent policy applied to the whole selection.
			edit->addItem(tr("Outside aligned"), NetLabelStyleOutside);
			edit->addItem(tr("Connector aligned"), NetLabelStyleConnector);

			QString common;
			bool first = true, mixed = false;
			Q_FOREACH (SymbolPaletteItem * netLabel, selectedNetLabels) {
				QString a = netLabel->effectiveAlign();
				if (a == NetLabelStyleLegacy) { mixed = true; break; }
				QString p = policyForAlign(a, netLabel->getDirection() == "left");
				if (first) { common = p; first = false; }
				else if (p != common) { mixed = true; break; }
			}
			edit->setCurrentIndex(mixed ? -1 : edit->findData(common));

			connect(edit, SIGNAL(currentIndexChanged(int)), this, SLOT(groupStyleEntry(int)));
			returnWidget = edit;
			returnValue = mixed ? QString() : common;
			return true;
		}

		// Single item: show the ACTUAL on-screen text alignment with standard left/right
		// align icons (+ legacy). The stored value is in the label's local frame, so the
		// actual side differs from it whenever the transform mirrors the x-axis (horizontal
		// flip OR 180° rotation, i.e. m11 < 0).
		QString align = effectiveAlign();
		edit->addItem(QIcon(":/resources/images/icons/align_left.svg"), tr("Left aligned"), NetLabelAlignLeft);
		edit->addItem(QIcon(":/resources/images/icons/align_right.svg"), tr("Right aligned"), NetLabelAlignRight);
		edit->addItem(tr("Legacy"), NetLabelStyleLegacy);

		QString shown = align;
		if (align != NetLabelStyleLegacy) {
			bool reversed = (this->transform().m11() < -0.5);   // m11 ≈ -1: horizontal flip or 180°
			bool actualLeft = (align == NetLabelAlignLeft) != reversed;   // XOR
			shown = actualLeft ? NetLabelAlignLeft : NetLabelAlignRight;
		}

		int ix = edit->findData(shown);
		edit->setCurrentIndex(ix >= 0 ? ix : 0);

		connect(edit, SIGNAL(currentIndexChanged(int)), this, SLOT(styleEntry(int)));
		returnWidget = edit;
		returnValue = shown;
		return true;
	}

	return PaletteItem::collectExtraInfo(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget, hide);
}

void SymbolPaletteItem::voltageEntry(int index) {
	auto * comboBox = qobject_cast<QComboBox *>(sender());
	if (comboBox == nullptr) return;
	QString text = comboBox->itemText(index);

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != nullptr) {
		infoGraphicsView->setVoltage(text.toDouble(), true);
	}
}

void SymbolPaletteItem::labelEntry() {
	auto * edit = qobject_cast<QLineEdit *>(sender());
	if (edit == nullptr) return;

	QString current = getLabel();
	if (edit->text().compare(current) == 0) return;

	if (edit->text().isEmpty()) {
		QMessageBox::warning(nullptr, tr("Net labels", "dialog title"), tr("Net labels cannot be blank"));
		return;
	}

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != nullptr) {
		infoGraphicsView->setProp(this, "label", ItemBase::TranslatedPropertyNames.value("label"), current, edit->text(), true);
	}
}

void SymbolPaletteItem::styleEntry(int index) {
	auto * comboBox = qobject_cast<QComboBox *>(sender());
	if (comboBox == nullptr) return;
	// Single-item combo values: NetLabelAlignLeft / NetLabelAlignRight / NetLabelStyleLegacy.
	requestNetLabelStyle(comboBox->itemData(index).toString());
}

void SymbolPaletteItem::groupStyleEntry(int index) {
	auto * comboBox = qobject_cast<QComboBox *>(sender());
	if (comboBox == nullptr) return;
	// Group combo values: NetLabelStyleOutside / NetLabelStyleConnector.
	requestNetLabelStyle(comboBox->itemData(index).toString());
}

void SymbolPaletteItem::requestNetLabelStyle(const QString & picked) {
	if (picked.isEmpty()) return;

	// Every net-label style change routes through the swap machinery. Crossing the legacy
	// boundary swaps the part (v4 <-> v5); staying within the modern styles only changes the
	// alignment. MainWindow::swapSelectedMap dispatches "net label"/"style" to
	// swapNetLabelStyleForSelection, which applies the choice to every selected net label in
	// one undo (resolving each item's target via resolveStyleSwap).
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView == nullptr) return;
	QMap<QString, QString> propsMap;
	propsMap.insert("style", picked);
	infoGraphicsView->swap(modelPart()->family(), "style", propsMap, this);
}

QString SymbolPaletteItem::resolveStyleSwap(ItemBase * item, const QString & picked, QString & newStyle) {
	// Decide what a net label should become when `picked` is chosen in the Inspector. Returns
	// the moduleID to swap to (empty => no swap, just apply newStyle to the item) and sets
	// newStyle to the alignment local-prop for the result.
	newStyle.clear();
	if ((item == nullptr) || (item->modelPart() == nullptr) || (item->modelPart()->modelPartShared() == nullptr)) {
		return QString();
	}

	bool isV4 = item->modelPart()->modelPartShared()->version().toInt() <= 4;

	if (picked == NetLabelStyleLegacy) {
		// Modern -> legacy: swap back to the v4 part (which renders legacy). Orientation is
		// carried over by the swap (preserved transform + the transferred direction prop).
		return isV4 ? QString() : ModuleIDNames::NetLabelModuleIDName;
	}

	// Modern target: resolve the concrete left/right alignment in the label's own frame.
	if (picked == NetLabelStyleOutside || picked == NetLabelStyleConnector) {
		// Group policy: depends on which way this label faces.
		bool goLeft = (item->modelPart()->localProp("direction").toString() == "left");
		newStyle = alignForPolicy(picked, goLeft);
	}
	else {
		// Single item: `picked` is the actual on-screen side; translate to the local frame,
		// accounting for an x-axis mirror (horizontal flip or 180° rotation, m11 < 0).
		bool reversed = (item->transform().m11() < -0.5);
		bool wantLeft = (picked == NetLabelAlignLeft);
		newStyle = (wantLeft != reversed) ? NetLabelAlignLeft : NetLabelAlignRight;
	}
	// Legacy (v4) -> modern is a swap to v5; an existing v5 just changes its alignment.
	return isV4 ? ModuleIDNames::V5NetLabelModuleIDName : QString();
}

ItemBase::PluralType SymbolPaletteItem::isPlural() {
	return Singular;
}

bool SymbolPaletteItem::hasPartNumberProperty()
{
	return false;
}

ViewLayer::ViewID SymbolPaletteItem::useViewIDForPixmap(ViewLayer::ViewID vid, bool)
{
	if (vid == ViewLayer::SchematicView) {
		return ViewLayer::IconView;
	}

	return ViewLayer::UnknownView;
}

bool SymbolPaletteItem::hasPartLabel() {
	return !m_isNetLabel;
}

bool SymbolPaletteItem::isOnlyNetLabel() {
	return false;
}

bool SymbolPaletteItem::inspectorRefreshOnTransform() {
	// A net label's inspector shows its actual on-screen text alignment, which depends on
	// the flip/rotation, so the inspector must rebuild when the transform changes.
	return m_isNetLabel;
}

QString SymbolPaletteItem::getLabel() {
	if (m_voltageReference) {
		return QString::number(m_voltage);
	}
	return  modelPart()->localProp("label").toString();
}

QString SymbolPaletteItem::getDirection() {
	return  modelPart()->localProp("direction").toString();
}

void SymbolPaletteItem::setAutoroutable(bool ar) {
	m_viewGeometry.setAutoroutable(ar);
}

bool SymbolPaletteItem::getAutoroutable() {
	return m_viewGeometry.getAutoroutable();
}

void SymbolPaletteItem::resetLayerKin() {

	Q_FOREACH (ItemBase * lkpi, layerKin()) {
		if (lkpi->viewLayerID() == ViewLayer::SchematicText) {
			QString svg = makeSvg(lkpi->viewLayerID());
			lkpi->resetRenderer(svg);
			lkpi->setProperty("textSvg", svg);
			qobject_cast<SchematicTextLayerKinPaletteItem *>(lkpi)->clearTextThings();
			break;
		}
	}
}

///////////////////////////////////////////////////////////////

NetLabel::NetLabel( ModelPart * modelPart, ViewLayer::ViewID viewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: SymbolPaletteItem(modelPart, viewID, viewGeometry, id, itemMenu, doLabel)
{
	QString label = getLabel();
	if (label.isEmpty()) {
		label = modelPart->properties().value("label");
		if (label.isEmpty()) {
			label = tr("net label");
		}
		modelPart->setLocalProp("label", label);
	}
	m_label = label;
	setInstanceTitle(label, true);

	// direction is now obsolete, new netlabels use flip
	QString direction = getDirection();
	if (direction.isEmpty()) {
		direction = modelPart->properties().value("direction");
		if (direction.isEmpty()) {
			direction = modelPart->moduleID().contains("left", Qt::CaseInsensitive) ? "left" : "right";
		}
		modelPart->setLocalProp("direction", direction);
	}
}

NetLabel::~NetLabel() {
}

QString NetLabel::getVersion()
{
	return modelPart()->modelPartShared()->version();
}

QString NetLabel::makeSvg(ViewLayer::ViewLayerID viewLayerID)
{
	QString align = effectiveAlign();
	bool useOldVersion = (align == "legacy");  // "legacy" == old Droid Sans rendering (v4 and earlier)
	double divisor = moduleID().contains(PartFactory::OldSchematicPrefix) ? 1 : 3;  // Fritzing before ~0.7.0

	double labelFontSize = 200 / divisor;
	double totalHeight = 300 / divisor;
	double arrowWidth = totalHeight / 2;
	double strokeWidth = 10 / divisor;
	double halfStrokeWidth = strokeWidth / 2;
	double labelBaseLine = (useOldVersion ? 220 : 228) / divisor;

	QString fontName = useOldVersion ? "Droid Sans" : "Noto Sans";
#ifdef Q_OS_MAC
	QFont font(fontName, labelFontSize, QFont::Normal);
#else
	QFont font(useOldVersion ?
				   QFont("Droid Sans", labelFontSize * 72 / GraphicsUtils::StandardFritzingDPI, QFont::Normal) :
				   QFont("Noto Sans", labelFontSize, QFont::Normal));
#endif
	QFontMetricsF fm(font);

#ifdef Q_OS_MAC
	static const double TextWidthScalingFactor = 1.0;
#elif defined(Q_OS_WIN)
	const double TextWidthScalingFactor = useOldVersion ? 0.77 : 0.755;
#else
	static const double TextWidthScalingFactor = 0.77;
#endif

	double textWidth;
#ifdef Q_OS_MAC
	// On macOS, always use the new calculation approach with scaling factor. The past one was bugged.
	textWidth = fm.horizontalAdvance(getLabel()) * TextWidthScalingFactor;
#else
	// On other platforms, use version-dependent approach
	if (useOldVersion) {
		textWidth = fm.horizontalAdvance(getLabel()) * GraphicsUtils::StandardFritzingDPI / 72;
	} else {
		textWidth = fm.horizontalAdvance(getLabel()) * TextWidthScalingFactor;
	}
#endif

	double totalWidth;

	if (useOldVersion) {
		double labelOffset = 20 / divisor;
		totalWidth = textWidth + arrowWidth + labelOffset;

	} else {
		double labelPadding = 50 / divisor;
		double widthStep = 50;
		double adjustedWidth = textWidth - labelPadding;
		double roundedWidth = ceil(adjustedWidth / widthStep) * widthStep;
		totalWidth = roundedWidth + arrowWidth + labelPadding * 2;
	}

	QString header("<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
				   "<svg xmlns:svg='http://www.w3.org/2000/svg' xmlns='http://www.w3.org/2000/svg' "
				   "version='1.2' baseProfile='tiny' \n"
				   "width='%1in' height='%2in' viewBox='0 0 %3 %4' >\n"
				   "<g id='%5' >\n");

	bool goLeft = (getDirection() == "left");
	double offset = goLeft ? arrowWidth : 0;

	QString svg = header.arg(totalWidth / 1000)
					  .arg(totalHeight / 1000)
					  .arg(totalWidth)
					  .arg(totalHeight)
					  .arg(ViewLayer::viewLayerXmlNameFromID(viewLayerID));

	if (viewLayerID == ViewLayer::SchematicText) {
		double xPosition;
		QString textAnchor = "start";
		if (useOldVersion) {
			double labelOffset = 20 / divisor;
			xPosition = labelOffset + offset;
		} else {
			// The text hugs the left or right edge of the box per the stored alignment.
			// The arrow/connector occupies arrowWidth on the left when goLeft, otherwise on
			// the right; the rest of the box (minus the two labelPaddings) is the text band,
			// and the 50-unit width-rounding slack falls on whichever side the text does not
			// hug. Flipping the label mirrors this on screen.
			double labelPadding = 50 / divisor;
			double bandLeft = labelPadding + (goLeft ? arrowWidth : 0);
			double bandRight = totalWidth - labelPadding - (goLeft ? 0 : arrowWidth);
			bool startAtLeft = (align == "left");
			xPosition = startAtLeft ? bandLeft : bandRight;
			textAnchor = startAtLeft ? "start" : "end";
		}

		svg += QString("<text id='label' x='%1' y='%2' fill='#000000' font-family='%5' font-weight='400' "
					   "text-anchor='%6' font-size='%3'>%4</text>\n")
				   .arg(xPosition)
				   .arg(labelBaseLine)
				   .arg(labelFontSize)
				   .arg(getLabel(),
						fontName)
				   .arg(textAnchor);
	} else {
		QString pin = QString("<rect id='connector0pin' x='%1' y='%2' width='%3' height='%4' "
							  "fill='none' stroke='none' stroke-width='0' />\n");
		QString terminal = QString("<rect id='connector0terminal' x='%1' y='%2' width='0.1' "
								   "height='0.1' fill='none' stroke='none' stroke-width='0' />\n");

		QString points = QString("%1,%2 %3,%4 %5,%4 %5,%6 %3,%6");
		if (goLeft) {
			points = points.arg(halfStrokeWidth)
			.arg(totalHeight / 2)
				.arg(arrowWidth)
				.arg(halfStrokeWidth)
				.arg(totalWidth - halfStrokeWidth)
				.arg(totalHeight - halfStrokeWidth);
			terminal = terminal.arg(0).arg(totalHeight / 2);
			pin = pin.arg(0).arg(0).arg(arrowWidth).arg(totalHeight);
		} else {
			points = points.arg(totalWidth - halfStrokeWidth)
			.arg(totalHeight / 2)
				.arg(totalWidth - arrowWidth)
				.arg(halfStrokeWidth)
				.arg(halfStrokeWidth)
				.arg(totalHeight - halfStrokeWidth);
			terminal = terminal.arg(totalWidth).arg(totalHeight / 2);
			pin = pin.arg(totalWidth - arrowWidth - 0.1).arg(0).arg(arrowWidth).arg(totalHeight);
		}

		svg += pin;
		svg += terminal;
		svg += QString("<polygon fill='white' stroke='#000000' stroke-width='%1' points='%2' />\n")
				   .arg(strokeWidth)
				   .arg(points);
	}

	svg += "</g>\n</svg>\n";

	if (viewLayerID == ViewLayer::SchematicText) {
		svg = transformTextSvg(svg);
	}

	return svg;
}

QPixmap NetLabel::stylePreviewPixmap(const QString & policy, bool goLeft, const QSize & size)
{
	// A faithful miniature of a net label (arrow + a short sample label) rendered for the
	// given settings policy and orientation. Mirrors the non-legacy geometry of makeSvg,
	// but combines the arrow and text into one SVG so the whole symbol shows in one image.
	const double divisor = 3;
	const double labelFontSize = 200 / divisor;
	const double totalHeight = 300 / divisor;
	const double arrowWidth = totalHeight / 2;
	const double strokeWidth = 10 / divisor;
	const double halfStrokeWidth = strokeWidth / 2;
	const double labelBaseLine = 228 / divisor;
	const double labelPadding = 50 / divisor;

	const QString sample("A1");

#if defined(Q_OS_WIN)
	const double TextWidthScalingFactor = 0.755;
#else
	const double TextWidthScalingFactor = 0.77;
#endif
	QFont font("Noto Sans", labelFontSize, QFont::Normal);
	QFontMetricsF fm(font);
	double textWidth = fm.horizontalAdvance(sample) * TextWidthScalingFactor;

	double widthStep = 50;
	double roundedWidth = ceil((textWidth - labelPadding) / widthStep) * widthStep;
	double totalWidth = roundedWidth + arrowWidth + labelPadding * 2;

	QString points = QString("%1,%2 %3,%4 %5,%4 %5,%6 %3,%6");
	if (goLeft) {
		points = points.arg(halfStrokeWidth).arg(totalHeight / 2).arg(arrowWidth)
		             .arg(halfStrokeWidth).arg(totalWidth - halfStrokeWidth).arg(totalHeight - halfStrokeWidth);
	} else {
		points = points.arg(totalWidth - halfStrokeWidth).arg(totalHeight / 2).arg(totalWidth - arrowWidth)
		             .arg(halfStrokeWidth).arg(halfStrokeWidth).arg(totalHeight - halfStrokeWidth);
	}

	// "outside" = text away from the connector; "connector" = text at the connector. The
	// connector sits on the left when goLeft, otherwise on the right.
	bool outside = (policy == NetLabelStyleOutside);
	bool hugLeft = goLeft ? !outside : outside;
	double bandLeft = labelPadding + (goLeft ? arrowWidth : 0);
	double bandRight = totalWidth - labelPadding - (goLeft ? 0 : arrowWidth);
	double xPosition = hugLeft ? bandLeft : bandRight;
	QString anchor = hugLeft ? "start" : "end";

	QString svg = QString(
	        "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 %1 %2' >\n"
	        "<polygon fill='white' stroke='#000000' stroke-width='%3' points='%4' />\n"
	        "<text x='%5' y='%6' fill='#000000' font-family='Noto Sans' font-weight='400' "
	        "text-anchor='%7' font-size='%8'>%9</text>\n"
	        "</svg>\n")
	        .arg(totalWidth).arg(totalHeight).arg(strokeWidth).arg(points)
	        .arg(xPosition).arg(labelBaseLine).arg(anchor).arg(labelFontSize).arg(sample);

	QPixmap pixmap(size);
	pixmap.fill(Qt::transparent);
	QSvgRenderer renderer(svg.toUtf8());
	QPainter painter(&pixmap);
	painter.setRenderHint(QPainter::Antialiasing, true);
	// Fit while preserving aspect ratio, centered.
	QSizeF def = renderer.defaultSize();
	double newW = size.width();
	double newH = (def.width() > 0) ? newW * def.height() / def.width() : size.height();
	if (newH > size.height()) {
		newH = size.height();
		newW = (def.height() > 0) ? newH * def.width() / def.height() : size.width();
	}
	QRectF bounds((size.width() - newW) / 2.0, (size.height() - newH) / 2.0, newW, newH);
	renderer.render(&painter, bounds);
	painter.end();
	return pixmap;
}

void NetLabel::addedToScene(bool temporary)
{
	if ((this->scene() != nullptr) && m_viewID == ViewLayer::SchematicView) {
		// do not understand why plan setLabel() doesn't work the same as the Mystery Part setChipLabel() in addedToScene()

		if (!this->transform().isIdentity()) {
			// need to establish correct bounding rect here or setLabel will screw up alignment
			// this seems to solve half of the cases
			QString svg = makeSvg(this->viewLayerID());
			resetRenderer(svg);
			resetLayerKin();
			QRectF r = boundingRect();
			//debugInfo(QString("added to scene %1,%2").arg(r.width()).arg(r.height()));


			QTransform chiefTransform = this->transform();
			QTransform local;
			local.setMatrix(chiefTransform.m11(), chiefTransform.m12(), chiefTransform.m13(), chiefTransform.m21(), chiefTransform.m22(), chiefTransform.m23(), 0, 0, chiefTransform.m33());
			if (!local.isIdentity()) {
				double x = r.width() / 2.0;
				double y = r.height() / 2.0;
				QTransform transf = QTransform().translate(-x, -y) * local * QTransform().translate(x, y);
				if (qAbs(chiefTransform.dx() - transf.dx()) > .01 || qAbs(chiefTransform.dy() - transf.dy()) > .01) {
					DebugDialog::debug("got the translation bug here");
					this->getViewGeometry().setTransform(transf);
					this->setTransform2(transf);
				}
			}
		}

		setLabel(getLabel());

	}

	// deliberately skip SymbolPaletteItem::addedToScene()
	return PaletteItem::addedToScene(temporary);
}

QString NetLabel::retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor) {
	Q_UNUSED(svgHash);
	QString svg = makeSvg(viewLayerID);
	return PaletteItemBase::normalizeSvg(svg, viewLayerID, blackOnly, dpi, factor);
}

ItemBase::PluralType NetLabel::isPlural() {
	return Plural;
}

bool NetLabel::isOnlyNetLabel() {
	return true;
}

QString NetLabel::getInspectorTitle() {
	return getLabel();

}

void NetLabel::setInspectorTitle(const QString & oldText, const QString & newText) {
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView == nullptr) return;

	infoGraphicsView->setProp(this, "label", ItemBase::TranslatedPropertyNames.value("label"), oldText, newText, true);
}
