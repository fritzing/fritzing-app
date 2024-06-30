#include "ftooltip.h"
#include "connectors/connectoritem.h"
#include <QStringBuilder>

namespace FToolTip {

QString createConnectionHtmlList(const QList<ConnectorItem*>& connectors)
{
	QString connections = "<ul style='margin-left:0;padding-left:0;'>";
	for (const auto& connectorItem : connectors) {
		connections += "<li style='margin-left:0;padding-left:0;'><b>"
					   % connectorItem->attachedTo()->label()
					   % "</b> "
					   % connectorItem->connectorSharedName()
					   % "</li>";
	}
	connections += "</ul>";
	return connections;
}

QString createTooltipHtml(const QString& text, const QString& title) {
    QString html = QStringLiteral("<b>") % text % QStringLiteral("</b>");
	if (!title.isEmpty()) {
        html += QStringLiteral("<br></br><font size='2'>")
				% title
				% QStringLiteral("</font>");
	}
	return html;
}

QString createNonWireItemTooltipHtml(const QString& name, const QString& descr, const QString& title) {
    QString html = QStringLiteral("<span><b>")
				   % name
				   % QStringLiteral("</b>");

	if (!descr.isEmpty()) {
		html += QStringLiteral(":") % descr;
	}

	html += QStringLiteral("<br /><span style='font-size:small;'>")
			% title
			% QStringLiteral("</span></span>");

	return html;
}

} // namespace FToolTip
