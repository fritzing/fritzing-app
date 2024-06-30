#ifndef FPROBEACTIONS_H
#define FPROBEACTIONS_H

#include "testing/FProbe.h"

#include <QObject>
#include <QVariant>
#include <QMenuBar>

class FProbeActions : public QObject, FProbe {
	Q_OBJECT
public:
	explicit FProbeActions(QString name, QWidget* actionContainer, QObject *parent = nullptr);
	~FProbeActions();

	QVariant read() override;
	void write(QVariant var) override;

signals:
	void triggerAction(QAction* action);

private:
	QString serializeVariantListToJsonString(const QVariantList& list) const;
	QVariantList listActionsInMenu(const QMenu* menu);
	bool searchActionsInMenu(const QMenu* menu, const QString& searchText);

	QWidget* actionContainer;
};

#endif // FPROBEACTIONS_H
