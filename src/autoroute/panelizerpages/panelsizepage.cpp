/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2026 Fritzing

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

#include "panelsizepage.h"
#include "../panelpresets.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>

PanelSizePage::PanelSizePage(QWidget *parent)
	: QWizardPage(parent)
	, m_presetCombo(nullptr)
	, m_widthSpin(nullptr)
	, m_heightSpin(nullptr)
	, m_mmCheckbox(nullptr)
	, m_railsCheckbox(nullptr)
	, m_autoFitCheckbox(nullptr)
	, m_gutterSpin(nullptr)
	, m_borderSpin(nullptr)
	, m_fitsLabel(nullptr)
	, m_autosizeButton(nullptr)
	, m_currentPreset("100x100")
{
	setTitle(tr("Panel Size"));
	setSubTitle(tr("Choose panel dimensions and border configuration"));

	m_presetCombo = new QComboBox(this);
	// NOTE(landracer): preset strings must start with "<W>x<H> mm" or
	// "<W>x<H> in" - onPresetChanged() parses the leading WxH. Anything
	// after the size is descriptive (vendor / tier) and ignored by the
	// parser. "Custom..." disables the preset, leaving spinners alone.
	//
	// The mm presets come from PanelPresets::list() so this combo and
	// PanelCandidatePage stay in lockstep. Inch-only presets stay
	// inline below — the candidate page intentionally only offers mm
	// sizes (matches fab catalog ordering).
	QStringList items;
	for (const auto &p : PanelPresets::list()) items << p.label;
	items << "5x5 in"
	      << "5x10 in"
	      << "10x15 in"
	      << "15x20 in"
	      << tr("Custom...");
	m_presetCombo->addItems(items);

	m_widthSpin = new QDoubleSpinBox(this);
	m_widthSpin->setRange(10.0, 600.0);
	m_widthSpin->setValue(100.0);
	m_widthSpin->setSuffix(" mm");

	m_heightSpin = new QDoubleSpinBox(this);
	m_heightSpin->setRange(10.0, 600.0);
	m_heightSpin->setValue(100.0);
	m_heightSpin->setSuffix(" mm");

	m_mmCheckbox = new QCheckBox(tr("Use millimeters"), this);
	m_mmCheckbox->setChecked(true);

	m_gutterSpin = new QDoubleSpinBox(this);
	m_gutterSpin->setRange(0.0, 20.0);
	m_gutterSpin->setValue(2.0);
	m_gutterSpin->setSuffix(" mm");

	m_borderSpin = new QDoubleSpinBox(this);
	m_borderSpin->setRange(0.0, 20.0);
	m_borderSpin->setValue(5.0);
	m_borderSpin->setSuffix(" mm");

	m_railsCheckbox = new QCheckBox(tr("Add top + bottom rails (recommended for V-cut)"), this);
	m_railsCheckbox->setChecked(true);

	// Auto-fit mode: when on, the user-entered W/H are ignored and the
	// wizard walks the preset list to pick the smallest panel that fits
	// the requested copies. Lets timid users say 'I just want 10 of
	// these' without having to know fab-house tier sizes up front. The
	// chosen panel is reported in the preview page status row.
	m_autoFitCheckbox = new QCheckBox(tr("Auto-fit panel to copies (pick smallest preset that fits)"), this);
	m_autoFitCheckbox->setChecked(false);

	m_autosizeButton = new QPushButton(tr("Autosize from current PCB"), this);

	m_fitsLabel = new QLabel(tr("Layout will be computed on Finish."), this);

	auto *form = new QFormLayout;
	form->addRow(tr("Preset:"),  m_presetCombo);
	form->addRow(tr("Width:"),   m_widthSpin);
	form->addRow(tr("Height:"),  m_heightSpin);
	form->addRow(tr("Gutter:"),  m_gutterSpin);
	form->addRow(tr("Border:"),  m_borderSpin);
	form->addRow(QString(),      m_mmCheckbox);
	form->addRow(QString(),      m_railsCheckbox);
	form->addRow(QString(),      m_autoFitCheckbox);
	form->addRow(m_autosizeButton);
	form->addRow(QString(),      m_fitsLabel);
	setLayout(form);

	registerField("panel.width",    m_widthSpin,  "value", SIGNAL(valueChanged(double)));
	registerField("panel.height",   m_heightSpin, "value", SIGNAL(valueChanged(double)));
	registerField("panel.gutter",   m_gutterSpin, "value", SIGNAL(valueChanged(double)));
	registerField("panel.border",   m_borderSpin, "value", SIGNAL(valueChanged(double)));
	registerField("panel.addRails", m_railsCheckbox);
	registerField("panel.useMm",    m_mmCheckbox);
	registerField("panel.autoFit",  m_autoFitCheckbox);

	connect(m_presetCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onPresetChanged(int)));
	connect(m_widthSpin,   SIGNAL(valueChanged(double)),    this, SLOT(onSizeChanged()));
	connect(m_heightSpin,  SIGNAL(valueChanged(double)),    this, SLOT(onSizeChanged()));
	connect(m_mmCheckbox,  SIGNAL(toggled(bool)),           this, SLOT(onUnitToggled(bool)));
	connect(m_autoFitCheckbox, SIGNAL(toggled(bool)),       this, SLOT(onAutoFitToggled(bool)));
}

PanelSizePage::~PanelSizePage()
{
}

bool PanelSizePage::isComplete() const
{
	return m_widthSpin->value() > 0.0 && m_heightSpin->value() > 0.0;
}

void PanelSizePage::setSketchPath(const QString &sketchPath)
{
	m_sketchPath = sketchPath;
}

void PanelSizePage::onPresetChanged(int index)
{
	// Parse the leading WxH from the preset string; "Custom..." leaves
	// the spinners untouched. Detect a trailing "in" suffix and convert
	// to mm so inch-based presets work even though the spinners use mm
	// (the mmCheckbox just changes the suffix text, not the underlying
	// stored value).
	const QString text = m_presetCombo->itemText(index);
	const int xPos = text.indexOf('x');
	if (xPos <= 0) return;
	bool wOk = false, hOk = false;
	const double w = text.left(xPos).trimmed().toDouble(&wOk);
	const QString rest = text.mid(xPos + 1).trimmed();
	const int spacePos = rest.indexOf(' ');
	const double h = (spacePos > 0 ? rest.left(spacePos) : rest).toDouble(&hOk);
	if (!wOk || !hOk) return;
	// Units detection: look for " in" anywhere in the trailing slice.
	const bool isInches = text.contains(QStringLiteral(" in"));
	const double scale = isInches ? 25.4 : 1.0;
	m_widthSpin->setValue(w * scale);
	m_heightSpin->setValue(h * scale);
}

void PanelSizePage::onSizeChanged()
{
	emit completeChanged();
}

void PanelSizePage::onFitsCountReady()
{
}

void PanelSizePage::onUnitToggled(bool checked)
{
	const QString suffix = checked ? " mm" : " in";
	m_widthSpin->setSuffix(suffix);
	m_heightSpin->setSuffix(suffix);
	m_gutterSpin->setSuffix(suffix);
	m_borderSpin->setSuffix(suffix);
}

void PanelSizePage::onAutosizeFromCurrent()
{
	// This is a placeholder implementation - in a real implementation,
	// we would need to access the actual board size from the sketch
	// and compute the panel size needed to fit the requested number of copies.
	// For now, we'll just show a message.
	QMessageBox::information(this, tr("Autosize"),
		tr("In a real implementation, this would compute the panel size needed to fit the current sketch.\n\n"
		   "For now, it just shows this message."));
}

void PanelSizePage::onAutoFitToggled(bool on)
{
	// Disable the W/H spinners + preset combo while auto-fit is active:
	// the wizard substitutes a chosen preset at panelize time, so the
	// user-entered values are irrelevant. Keep them visible (greyed) so
	// the user can see what would be used after switching back to manual.
	if (m_presetCombo) m_presetCombo->setEnabled(!on);
	if (m_widthSpin)   m_widthSpin->setEnabled(!on);
	if (m_heightSpin)  m_heightSpin->setEnabled(!on);
	if (m_fitsLabel) {
		m_fitsLabel->setText(on
			? tr("Auto-fit ON: panel will be chosen on Next.")
			: tr("Layout will be computed on Finish."));
	}
}

