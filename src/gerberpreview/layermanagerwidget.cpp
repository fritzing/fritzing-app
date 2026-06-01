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

#include "layermanagerwidget.h"

#include <QAction>
#include <QCheckBox>
#include <QColorDialog>
#include <QGridLayout>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QSlider>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
constexpr int kSwatchSize = 18;
}

LayerManagerWidget::LayerManagerWidget(QWidget *parent)
	: QWidget(parent)
	, m_grid(nullptr)
{
	// Vertical container holding the grid of rows. Caller is expected
	// to drop this widget into a QSplitter so the user can resize.
	auto * outer = new QVBoxLayout(this);
	outer->setContentsMargins(6, 6, 6, 6);
	outer->setSpacing(4);

	auto * header = new QLabel(tr("Layers"), this);
	QFont f = header->font();
	f.setBold(true);
	header->setFont(f);
	outer->addWidget(header);

	auto * gridHost = new QWidget(this);
	m_grid = new QGridLayout(gridHost);
	m_grid->setContentsMargins(0, 0, 0, 0);
	m_grid->setHorizontalSpacing(6);
	m_grid->setVerticalSpacing(3);
	outer->addWidget(gridHost);

	outer->addStretch(1);

	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, &QWidget::customContextMenuRequested,
	        this, &LayerManagerWidget::onContextMenu);
}

LayerManagerWidget::~LayerManagerWidget() = default;

QString LayerManagerWidget::labelFor(GerberPreviewWidget::LayerKind kind)
{
	switch (kind) {
		case GerberPreviewWidget::TopCopper:    return tr("Top Copper");
		case GerberPreviewWidget::BottomCopper: return tr("Bottom Copper");
		case GerberPreviewWidget::TopMask:      return tr("Top Solder Mask");
		case GerberPreviewWidget::BottomMask:   return tr("Bottom Solder Mask");
		case GerberPreviewWidget::TopSilk:      return tr("Top Silkscreen");
		case GerberPreviewWidget::BottomSilk:   return tr("Bottom Silkscreen");
		case GerberPreviewWidget::TopPaste:     return tr("Top Paste");
		case GerberPreviewWidget::BottomPaste:  return tr("Bottom Paste");
		case GerberPreviewWidget::EdgeCuts:     return tr("Edge Cuts");
		case GerberPreviewWidget::Drill:        return tr("Drill");
		case GerberPreviewWidget::Unknown:
		default:                                return tr("Unknown");
	}
}

void LayerManagerWidget::clearRows()
{
	// Wipe the grid; QObject ownership cleans up the child widgets.
	for (const Row &r : m_rows) {
		delete r.visible;
		delete r.swatch;
		delete r.name;
		delete r.opacity;
	}
	m_rows.clear();
}

void LayerManagerWidget::setLayers(
	const QVector<GerberPreviewWidget::LayerKind> &kinds,
	const QHash<GerberPreviewWidget::LayerKind, QColor> &colors)
{
	clearRows();
	for (const auto kind : kinds) {
		const QColor c = colors.value(kind, QColor(128, 128, 128, 200));
		appendRow(kind, c);
	}
}

void LayerManagerWidget::paintSwatch(QToolButton *btn, const QColor &color)
{
	// Draw a checkerboard backdrop so the alpha channel of the
	// current color is visible at a glance. Then overpaint with the
	// color itself. Matches the common Qt color-picker convention.
	QPixmap pm(kSwatchSize, kSwatchSize);
	pm.fill(Qt::transparent);
	{
		QPainter p(&pm);
		p.setPen(Qt::NoPen);
		// Two-tone checker.
		const QColor c1(220, 220, 220);
		const QColor c2(160, 160, 160);
		const int cell = kSwatchSize / 2;
		p.fillRect(0,    0,    cell, cell, c1);
		p.fillRect(cell, 0,    cell, cell, c2);
		p.fillRect(0,    cell, cell, cell, c2);
		p.fillRect(cell, cell, cell, cell, c1);
		// Color overlay using full alpha for visibility.
		p.fillRect(pm.rect(), color);
		// Frame.
		p.setPen(QColor(60, 60, 60));
		p.setBrush(Qt::NoBrush);
		p.drawRect(pm.rect().adjusted(0, 0, -1, -1));
	}
	btn->setIcon(QIcon(pm));
	btn->setIconSize(pm.size());
	// Store the live color on the button so click handlers can read
	// it without going back through the rows table.
	btn->setProperty("layerColor", color);
}

void LayerManagerWidget::appendRow(GerberPreviewWidget::LayerKind kind,
                                   const QColor &color)
{
	Row r;
	r.kind    = kind;
	r.visible = new QCheckBox(this);
	r.visible->setChecked(true);
	r.visible->setToolTip(tr("Show/hide this layer"));
	r.visible->setProperty("layerKind", static_cast<int>(kind));

	r.swatch  = new QToolButton(this);
	r.swatch->setAutoRaise(true);
	r.swatch->setToolTip(tr("Click to change color"));
	r.swatch->setProperty("layerKind", static_cast<int>(kind));
	paintSwatch(r.swatch, color);

	r.name    = new QLabel(labelFor(kind), this);
	r.name->setMinimumWidth(140);

	r.opacity = new QSlider(Qt::Horizontal, this);
	r.opacity->setMinimum(0);
	r.opacity->setMaximum(255);
	r.opacity->setValue(color.alpha());
	r.opacity->setToolTip(tr("Opacity: %1%").arg(int(color.alpha() * 100.0 / 255.0)));
	r.opacity->setMinimumWidth(80);
	r.opacity->setProperty("layerKind", static_cast<int>(kind));

	const int row = m_rows.size();
	m_grid->addWidget(r.visible, row, 0);
	m_grid->addWidget(r.swatch,  row, 1);
	m_grid->addWidget(r.name,    row, 2);
	m_grid->addWidget(r.opacity, row, 3);

	connect(r.visible, &QCheckBox::toggled, this, &LayerManagerWidget::onVisibilityToggled);
	connect(r.swatch,  &QToolButton::clicked, this, &LayerManagerWidget::onSwatchClicked);
	connect(r.opacity, &QSlider::valueChanged, this, &LayerManagerWidget::onOpacityChanged);

	m_rows.append(r);
}

void LayerManagerWidget::onVisibilityToggled(bool on)
{
	auto * src = qobject_cast<QCheckBox *>(sender());
	if (src == nullptr) return;
	const auto kind = static_cast<GerberPreviewWidget::LayerKind>(
		src->property("layerKind").toInt());
	emit visibilityChanged(kind, on);
}

void LayerManagerWidget::onSwatchClicked()
{
	auto * btn = qobject_cast<QToolButton *>(sender());
	if (btn == nullptr) return;
	const auto kind = static_cast<GerberPreviewWidget::LayerKind>(
		btn->property("layerKind").toInt());
	const QColor current = btn->property("layerColor").value<QColor>();

	// ShowAlphaChannel so the user can also tune opacity from the
	// dialog (matches the slider behavior).
	const QColor picked = QColorDialog::getColor(current, this,
		tr("Choose layer color"), QColorDialog::ShowAlphaChannel);
	if (!picked.isValid()) return;

	paintSwatch(btn, picked);
	// Keep the row's opacity slider in sync without re-emitting.
	for (const Row &r : m_rows) {
		if (r.kind != kind) continue;
		r.opacity->blockSignals(true);
		r.opacity->setValue(picked.alpha());
		r.opacity->setToolTip(tr("Opacity: %1%").arg(int(picked.alpha() * 100.0 / 255.0)));
		r.opacity->blockSignals(false);
		break;
	}
	emit colorChanged(kind, picked);
}

void LayerManagerWidget::onOpacityChanged(int alpha)
{
	auto * src = qobject_cast<QSlider *>(sender());
	if (src == nullptr) return;
	const auto kind = static_cast<GerberPreviewWidget::LayerKind>(
		src->property("layerKind").toInt());

	for (const Row &r : m_rows) {
		if (r.kind != kind) continue;
		QColor c = r.swatch->property("layerColor").value<QColor>();
		c.setAlpha(alpha);
		paintSwatch(r.swatch, c);
		r.opacity->setToolTip(tr("Opacity: %1%").arg(int(alpha * 100.0 / 255.0)));
		emit colorChanged(kind, c);
		return;
	}
}

void LayerManagerWidget::emitShowOnly(GerberPreviewWidget::LayerKind only)
{
	for (const Row &r : m_rows) {
		const bool target = (r.kind == only);
		if (r.visible->isChecked() != target) {
			r.visible->setChecked(target); // fires onVisibilityToggled
		}
	}
}

void LayerManagerWidget::emitShowAll(bool on)
{
	for (const Row &r : m_rows) {
		if (r.visible->isChecked() != on) {
			r.visible->setChecked(on);
		}
	}
}

void LayerManagerWidget::onContextMenu(const QPoint &pos)
{
	// Locate which row the click landed in (if any) so the
	// "Show only this" entry has a target.
	GerberPreviewWidget::LayerKind under = GerberPreviewWidget::Unknown;
	for (const Row &r : m_rows) {
		const QRect rowRect = r.name->geometry().united(r.swatch->geometry())
		                           .united(r.opacity->geometry())
		                           .united(r.visible->geometry());
		if (rowRect.contains(pos)) { under = r.kind; break; }
	}

	QMenu menu(this);
	if (under != GerberPreviewWidget::Unknown) {
		QAction * only = menu.addAction(tr("Show only %1").arg(labelFor(under)));
		connect(only, &QAction::triggered, this, [this, under]() { emitShowOnly(under); });
		menu.addSeparator();
	}
	QAction * showAll = menu.addAction(tr("Show all"));
	QAction * hideAll = menu.addAction(tr("Hide all"));
	connect(showAll, &QAction::triggered, this, [this]() { emitShowAll(true); });
	connect(hideAll, &QAction::triggered, this, [this]() { emitShowAll(false); });
	menu.exec(mapToGlobal(pos));
}

void LayerManagerWidget::setInitialVisibility(
	GerberPreviewWidget::LayerKind kind, bool on)
{
	for (const Row &r : m_rows) {
		if (r.kind != kind) continue;
		r.visible->blockSignals(true);
		r.visible->setChecked(on);
		r.visible->blockSignals(false);
		return;
	}
}

void LayerManagerWidget::setRowColor(
	GerberPreviewWidget::LayerKind kind, const QColor &color)
{
	// Repaint the swatch and refresh the stored "layerColor" property so
	// colorMap() (used by QSettings persistence) reports the new value.
	for (const Row &r : m_rows) {
		if (r.kind != kind) continue;
		paintSwatch(r.swatch, color);
		return;
	}
}

QHash<GerberPreviewWidget::LayerKind, bool>
LayerManagerWidget::visibilityMap() const
{
	QHash<GerberPreviewWidget::LayerKind, bool> out;
	for (const Row &r : m_rows) out.insert(r.kind, r.visible->isChecked());
	return out;
}

QHash<GerberPreviewWidget::LayerKind, QColor>
LayerManagerWidget::colorMap() const
{
	QHash<GerberPreviewWidget::LayerKind, QColor> out;
	for (const Row &r : m_rows) {
		out.insert(r.kind, r.swatch->property("layerColor").value<QColor>());
	}
	return out;
}
