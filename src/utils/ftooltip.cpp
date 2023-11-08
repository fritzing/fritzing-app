#include "ftooltip.h"
#include "connectors/connectoritem.h"
#include <QStringBuilder>

namespace FToolTip {

const QString HOVER_TEXT_COLOR = "#363636";
const QString HOVER_ID_TEXT_COLOR = "#909090";

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
	QString html = QStringLiteral("<b><font color='") % HOVER_TEXT_COLOR % "'>" % text % QStringLiteral("</font></b>");
	if (!title.isEmpty()) {
		html += QStringLiteral("<br></br><font size='2' color='")
				% HOVER_TEXT_COLOR % "'>"
				% title
				% QStringLiteral("</font>");
	}
	return html;
}

QString createNonWireItemTooltipHtml(const QString& name, const QString& descr, const QString& id, const QString& title) {
	QString html = QStringLiteral("<span style='color:")
				   % HOVER_TEXT_COLOR
				   % QStringLiteral(";'><b>")
				   % name
				   % QStringLiteral("</b>");

	if (!descr.isEmpty()) {
		html += QStringLiteral(":") % descr;
	}

	if (!id.isEmpty()) {
		html += QStringLiteral(" <span style='color:")
				% HOVER_ID_TEXT_COLOR
				% QStringLiteral(";'>(")
				% id
				% QStringLiteral(")</span>");
	}

	html += QStringLiteral("<br /><span style='font-size:small;'>")
			% title
			% QStringLiteral("</span></span>");

	return html;
}

} // namespace FToolTip
