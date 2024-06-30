/*******************************************************************
skw
Part of the Fritzing project - http://fritzing.org
Copyright (c) 2023 Fritzing

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

#include "getspice.h"
#include "../debugdialog.h"
#include "../connectors/connectoritem.h"
#include "../items/propertydef.h"
#include "../model/modelpart.h"
#include "../utils/textutils.h"

#include <QRegularExpression>

QString GetSpice::getSpice(ItemBase * itemBase, const QList< QList<class ConnectorItem *>* >& netList) {
	static QRegularExpression curlies("\\{([^\\{\\}]*)\\}");
	QString spice = itemBase->spice();
	int pos = 0;
	while (true) {
		QRegularExpressionMatch match;
		pos = spice.indexOf(curlies, pos, &match);
		if (pos < 0) break;

        QString token = match.captured(1);
		QString replacement;
        if (token.toLower() == "instancetitle") {
			replacement = itemBase->instanceTitle();
			if (pos > 0 && replacement.at(0).toLower() == spice.at(pos - 1).toLower()) {
				// if the type letter is repeated
				replacement = replacement.mid(1);
			}
			replacement.replace(" ", "_");
		}
        else if (token.toLower().startsWith("net ")) {
			QString cname = token.mid(4).trimmed();
			Q_FOREACH (ConnectorItem * ci, itemBase->cachedConnectorItems()) {
				if (ci->connectorSharedID().toLower() == cname) {
					int ix = -1;
					Q_FOREACH (QList<ConnectorItem *> * net, netList) {
						ix++;
						if (net->contains(ci)) break;
					}

					replacement = QString::number(ix);
					break;
				}
			}
		}
		else {
			//Find the symbol of this property
			QString symbol;
			QHash<PropertyDef *, QString> propertyDefs;
			PropertyDefMaster::initPropertyDefs(itemBase->modelPart(), propertyDefs);
			foreach (PropertyDef * propertyDef, propertyDefs.keys()) {
				if (token.compare(propertyDef->name, Qt::CaseInsensitive) == 0) {
					symbol = propertyDef->symbol;
					break;
				}
			}
			//Find the value of the property
			QVariant variant = itemBase->modelPart()->localProp(token);

			if (variant.isNull()) {
				auto prop = itemBase->modelPart()->properties();
				replacement = prop.value(token, "");
				QList<QString> knownTokens;
				knownTokens << "inductance" << "resistance" << "current" << "tolerance" << "power" << "capacitance" << "voltage";
				if(replacement.isEmpty()) {
                    if (prop.contains(token) || knownTokens.contains(token.toLower())) {
						// Property exists but is empty. Or it is one of a few known tokens. Just assume zero.
						replacement = "0";
					} else {
						//Leave it, probably is a brace expresion for the spice simulator
						replacement = match.captured(0);
						continue;
					}
				}
			}
			else {
				replacement = variant.toString();
			}
			//Remove the symbol, if any. It is not mandatory:
			//(Ngspice ignores letters immediately following a number that are not scale factors)
			if (!symbol.isEmpty()) {
				replacement.replace(symbol, "");
			}
			//Ngspice does not differenciate from m and M prefixes, u shuld be used for micro
			replacement.replace("M", "Meg");
			replacement.replace(TextUtils::MicroSymbol, "u");
			replacement.replace(TextUtils::AltMicroSymbol, "u");
			replacement.replace("μ", "u");
			replacement.replace("Ω", "");
		}

		spice.replace(pos, match.captured(0).size(), replacement);
		DebugDialog::debug("spice " + spice);
	}
	return spice;
}
