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


#ifndef PREFSDIALOG_H
#define PREFSDIALOG_H

#include <QDialog>
#include <QFileInfoList>
#include <QHash>
#include <QLabel>
#include <QTabWidget>
#include <QRadioButton>
#include <QLineEdit>
#include <QDoubleValidator>
#include <QPointer>
#include <QProcess>

#include "../program/platform.h"
#include "../mainwindow/mainwindow.h"
#include "../project_properties.h"

struct ViewInfoThing
{
	QString viewName;
	QString shortName;
	int index;
	bool curvy;
};


class PrefsDialog : public QDialog
{
	Q_OBJECT

public:
	PrefsDialog(const QString & language, QWidget *parent = 0);
	~PrefsDialog();

	bool cleared();
	QHash<QString, QString> & settings();
	QHash<QString, QString> & tempSettings();
	void initLayout(QFileInfoList & languages, QList<Platform *> platforms, MainWindow * mainWindow);
	void initViewInfo(int index, const QString & viewName, const QString & shortName, bool curvy);

protected:
	QWidget * createLanguageForm(QFileInfoList & languages);
	QWidget* createOtherForm();
	QWidget* createColorForm();
	QWidget * createZoomerForm();
	QWidget * createAutosaveForm();
	QWidget *createProgrammerForm(QList<Platform *> platforms);
	QWidget *createGerberBetaFeaturesForm();
	QWidget *createProjectPropertiesForm();
	QWidget *createBlocklyForm();
	void updateWheelText();
	void initGeneral(QWidget * general, QFileInfoList & languages);
	void initBreadboard(QWidget *, ViewInfoThing *);
	void initSchematic(QWidget *, ViewInfoThing *);
	void initPCB(QWidget *, ViewInfoThing *);
	void initCode(QWidget *widget, QList<Platform *> platforms);
	void initBlockly(QWidget *);
	void initBetaFeatures(QWidget *widget);
	QWidget * createCurvyForm(ViewInfoThing *);

protected Q_SLOTS:
	void changeLanguage(int);
	void clear();
	void setConnectedColor();
	void setUnconnectedColor();
	void changeWheelBehavior();
	void toggleAutosave(bool);
	void changeAutosavePeriod(int);
	void curvyChanged();
	void chooseProgrammer();
	void installBlockly();
	void blocklyPackageJsonDownloaded();
	void blocklyReleaseDownloaded();
	QString getLocalBlocklyVersion(const QString & blocklyPath);
	QString parseVersionFromPackageJson(const QByteArray & jsonData);
	bool compareVersions(const QString & version1, const QString & version2);
	void copyDirectoryRecursive(const QDir & srcDir, const QDir & dstDir);
    void setSimulationTimeStepMode(const bool &timeStepMode);
    void setSimulationNumberOfSteps(const QString &numberOfSteps);
    void setSimulationTimeStep(const QString &timeStep);
    void setSimulationAnimationTime(const QString &animationTime);

Q_SIGNALS:
	void blocklyInstalled();

protected:
	QPointer<QTabWidget> m_tabWidget;
	QPointer<QWidget> m_general;
	QPointer<QWidget> m_breadboard;
	QPointer<QWidget> m_schematic;
	QPointer<QWidget> m_pcb;
	QPointer<QWidget> m_code;
	QPointer<QWidget> m_blockly;
	QPointer<QWidget> m_beta_features;
	QPointer<QNetworkAccessManager> m_blocklyNetworkManager;
	QPointer<QNetworkReply> m_blocklyPackageJsonReply;
	QPointer<QNetworkReply> m_blocklyDownloadReply;
	QPointer<QMessageBox> m_blocklyDownloadProgressDialog;
	QString m_blocklyTargetPath;
	QString m_blocklyDownloadPath;
	QPointer<QLabel> m_wheelLabel;
	QPointer<QLabel> m_connectedColorLabel;
	QPointer<QLabel> m_unconnectedColorLabel;
	QList<Platform *> m_platforms;
	QHash<QString, QLineEdit *> m_programmerLEs;
	QHash<QString, QString> m_settings;
	QHash<QString, QString> m_tempSettings;
	QString m_name;
	class TranslatorListModel * m_translatorListModel = nullptr;
	bool m_cleared = false;
	int m_wheelMapping = 0;
	ViewInfoThing m_viewInfoThings[6];
	MainWindow * m_mainWindow;
	QSharedPointer<ProjectProperties> m_projectProperties;
};

#endif
