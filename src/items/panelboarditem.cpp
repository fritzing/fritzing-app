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

#include "panelboarditem.h"
#include "../items/resizableboard.h"
#include "../items/moduleidnames.h"
#include "../referencemodel/referencemodel.h"
#include "../model/modelpart.h"
#include "../debugdialog.h"
#include "../utils/graphicsutils.h"

namespace {
/**
 * @brief Resolve a real ModelPart for the synthetic panel container.
 *
 * Board's ctor unconditionally dereferences the ModelPart pointer
 * (PaletteItem -> ItemBase initialization), so passing nullptr here
 * crashes the application immediately after the wizard completes.
 *
 * We try the dedicated PanelBoardModuleID first (loaded if the
 * resources/templates/panel_board.fzp template has been registered),
 * then fall back to the always-present RectanglePCB core part. The
 * fallback is acceptable because the panel's actual outline geometry
 * is driven by m_panelWidth/m_panelHeight + the separation SVG, not
 * by the underlying ModelPart's silkscreen.
 */
static ModelPart * resolvePanelModelPart(ReferenceModel * referenceModel) {
	if (referenceModel == nullptr) return nullptr;
	ModelPart * mp = referenceModel->retrieveModelPart(ModuleIDNames::PanelBoardModuleIDName);
	if (mp != nullptr) return mp;
	// NOTE: RectanglePCB is a core part loaded at FApplication startup -
	// if even this is missing the parts library is broken and the app
	// would not have made it this far.
	return referenceModel->retrieveModelPart(ModuleIDNames::RectanglePCBModuleIDName);
}
}

/**
 * @brief Constructs a PanelBoardItem with the given panel dimensions.
 *
 * @param viewGeometry ViewGeometry with location and size for the panel,
 *                     in inches * GraphicsUtils::StandardFritzingDPI.
 * @param referenceModel ReferenceModel used to look up the synthetic
 *                       panel ModelPart; must not be null.
 * @param parent Parent QGraphicsItem (if any).
 *
 * @note PanelBoardItem inherits from Board (resizableboard.h) to reuse
 *       the standard board rendering / save / load infrastructure
 *       (spec docs/design/panelizer-v2.md §7 step 3).
 */
PanelBoardItem::PanelBoardItem(const ViewGeometry &viewGeometry,
                               ReferenceModel *referenceModel,
                               QGraphicsItem *parent)
	: Board(resolvePanelModelPart(referenceModel), ViewLayer::PCBView, viewGeometry, ItemBase::getNextID(), nullptr, false)
{
	Q_UNUSED(parent)

	// Cache rect dimensions for boundingRect()/paint() so we never
	// have to round-trip through the (potentially shared) ModelPart's SVG.
	m_panelWidth = viewGeometry.rect().width();
	m_panelHeight = viewGeometry.rect().height();

	if (referenceModel == nullptr || resolvePanelModelPart(referenceModel) == nullptr) {
		DebugDialog::debug("PanelBoardItem: no ModelPart resolved - panel will not render", DebugDialog::Warning);
	}
}

PanelBoardItem::~PanelBoardItem()
{
}

QRectF PanelBoardItem::boundingRect() const
{
    // NOTE: boundingRect includes the panel outline plus any
    // fiducial/tooling hole markers that extend beyond the edge.
    return QRectF(0, 0, m_panelWidth, m_panelHeight);
}

void PanelBoardItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                           QWidget *widget)
{
    // Let the Board base class draw the substrate / silkscreen / etc.
    // using the loaded ModelPart's SVG.
    PaletteItem::paint(painter, option, widget);
    
    // TODO(landracer): render separation SVG (V-cut/mouse-bite lines) on top
    // using FSvgRenderer if m_separationSvg is non-empty.
    Q_UNUSED(option)
    Q_UNUSED(widget)
}

void PanelBoardItem::setSeparationSvg(const QString &svg)
{
    m_separationSvg = svg;
    emit panelChanged();
    update();
}
