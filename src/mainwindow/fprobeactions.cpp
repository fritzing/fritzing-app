#include "fprobeactions.h"
#include "debugdialog.h"

#include <QMenu>
#include <QAction>
#include <QDebug>
#include <QVariantList>
#include <QVariantMap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

FProbeActions::FProbeActions(QString name, QWidget* actionContainer, QObject* parent)
	: QObject(parent)
	, FProbe(QString("%1Actions").arg(name).toStdString())
	, actionContainer(actionContainer)
{}

FProbeActions::~FProbeActions() {}

QVariant FProbeActions::read()
{
	QVariantList actionsList;
	if (!actionContainer)
		return QVariant();

	QMenuBar* menuBar = dynamic_cast<QMenuBar*>(actionContainer);
	if (menuBar) {
		for (QAction* action : menuBar->actions()) {
			QMenu* menu = action->menu();
			if (menu) {
				QVariantList menuActions = listActionsInMenu(menu);
				actionsList.append(menuActions);
			}
		}
		return QVariant(serializeVariantListToJsonString(actionsList));
	}

	QMenu* menu = dynamic_cast<QMenu*>(actionContainer);
	if (menu) {
		return serializeVariantListToJsonString(listActionsInMenu(menu));
	}

	return QVariant();
}

void FProbeActions::write(QVariant var)
{
	QString searchText = var.toString();
	if (!actionContainer)
		return;

	QMenuBar* menuBar = dynamic_cast<QMenuBar*>(actionContainer);
	if (menuBar) {
		for (QAction* action : menuBar->actions()) {
			QMenu* menu = action->menu();
			if (menu && searchActionsInMenu(menu, searchText)) {
				break; // If a match is found, stop searching
			}
		}
		return;
	}

	QMenu* menu = dynamic_cast<QMenu*>(actionContainer);
	if (menu) {
		searchActionsInMenu(menu, searchText);
	}
}

QString FProbeActions::serializeVariantListToJsonString(const QVariantList& list) const {
	QJsonArray jsonArray = QJsonArray::fromVariantList(list);
	QJsonDocument doc(jsonArray);
	QString jsonString = QString(doc.toJson(QJsonDocument::Compact));
	return jsonString;
}

QVariantList FProbeActions::listActionsInMenu(const QMenu* menu)
{
	QVariantList actionDetails;
	for (QAction* action : menu->actions()) {
		if (!action || action->isSeparator()) {
			continue;
		}
		if (action->menu()) {
			QVariantList submenuActions = listActionsInMenu(action->menu());
			actionDetails.append(submenuActions);
		} else {
			QVariantMap actionDetail;
			QString text = action->text();
			bool isEnabled = action->isEnabled();
			actionDetail["name"] = text ;
			actionDetail["enabled"] = isEnabled;
			DebugDialog::debug(QString("%1: %2").arg(isEnabled? "enabled":"disabled", text));
			actionDetails.append(actionDetail);
		}
	}
	return actionDetails;
}

bool FProbeActions::searchActionsInMenu(const QMenu* menu, const QString& searchText)
{
	// Determine if the search is for start or end of string, or general 'contains'
	// We can use \A or ^ to match the start
	// and \Z or $ to mathc the end
	bool matchStart = searchText.startsWith("\\A") || searchText.startsWith("^");
	bool matchEnd = searchText.endsWith("\\Z") || searchText.endsWith("$");

	QString actualSearchText = searchText;
	if(matchStart) {
		actualSearchText.remove(0, 2);
	} else if(searchText.startsWith("^")) {
		actualSearchText.remove(0, 1);
	}

	if(matchEnd) {
		actualSearchText.chop(2);
	} else if(searchText.endsWith("$")) {
		actualSearchText.chop(1);
	}

	for (QAction* action : menu->actions()) {
		if (!action || action->isSeparator()) {
			continue;
		}

		if (action->menu()) {
			if (searchActionsInMenu(action->menu(), searchText)) {
				return true;
			}
			continue;
		}

		const QString& actionText = action->text();

		if ((matchStart && actionText.startsWith(actualSearchText, Qt::CaseInsensitive)) ||
			(matchEnd && actionText.endsWith(actualSearchText, Qt::CaseInsensitive)) ||
			(!matchStart && !matchEnd && actionText.contains(actualSearchText, Qt::CaseInsensitive))) {
			qDebug() << "Triggering action:" << action->text();
			emit action->triggered();
			return true; // Match found
		}
	}
	return false; // No match found in this menu
}

