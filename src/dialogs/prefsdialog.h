/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2016 Fritzing

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

********************************************************************

$Revision: 6732 $:
$Author: irascibl@gmail.com $:
$Date: 2012-12-27 08:26:48 +0100 (Do, 27. Dez 2012) $

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

#include "../program/platform.h"

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
    void initLayout(QFileInfoList & languages, QList<Platform *> platforms);
	void initViewInfo(int index, const QString & viewName, const QString & shortName, bool curvy);

protected:
    QWidget * createLanguageForm(QFileInfoList & languages);
	QWidget* createOtherForm();
	QWidget* createColorForm();
	QWidget * createZoomerForm();
	QWidget * createAutosaveForm();
    QWidget *createProgrammerForm(QList<Platform *> platforms);
	void updateWheelText();
    void initGeneral(QWidget * general, QFileInfoList & languages);
	void initBreadboard(QWidget *, ViewInfoThing *);
	void initSchematic(QWidget *, ViewInfoThing *);
	void initPCB(QWidget *, ViewInfoThing *);
    void initCode(QWidget *widget, QList<Platform *> platforms);
    QWidget * createCurvyForm(ViewInfoThing *);

protected slots:
	void changeLanguage(int);
	void clear();
	void setConnectedColor();
	void setUnconnectedColor();
	void changeWheelBehavior();
	void toggleAutosave(bool);
	void changeAutosavePeriod(int);
	void curvyChanged();
    void chooseProgrammer();

protected:
	QPointer<QTabWidget> m_tabWidget;
	QPointer<QWidget> m_general;
	QPointer<QWidget> m_breadboard;
	QPointer<QWidget> m_schematic;
	QPointer<QWidget> m_pcb;
    QPointer<QWidget> m_code;
	QPointer<QLabel> m_wheelLabel[3];
    QList<Platform *> m_platforms;
    QHash<QString, QLineEdit *> m_programmerLEs;
	QHash<QString, QString> m_settings;
	QHash<QString, QString> m_tempSettings;
	QString m_name;
	class TranslatorListModel * m_translatorListModel;
	bool m_cleared;
	int m_wheelMapping;
    ViewInfoThing m_viewInfoThings[3];
};

#endif 
