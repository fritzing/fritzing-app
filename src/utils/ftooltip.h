#ifndef FTOOLTIP_H
#define FTOOLTIP_H

#include "connectors/connectoritem.h"

namespace FToolTip {

QString createConnectionHtmlList(const QList<ConnectorItem*>& connectors);
QString createTooltipHtml(const QString& text, const QString& title);
QString createNonWireItemTooltipHtml(const QString& name,
									 const QString& descr,
									 const QString& title);

}

#endif // FTOOLTIP_H
