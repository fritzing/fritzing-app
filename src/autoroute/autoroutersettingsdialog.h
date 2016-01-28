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

$Revision: 6904 $:
$Author: irascibl@gmail.com $:
$Date: 2013-02-26 16:26:03 +0100 (Di, 26. Feb 2013) $

********************************************************************/

#ifndef AUTOROUTERSETTINGSDIALOG_H
#define AUTOROUTERSETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QDoubleValidator>
#include <QRadioButton>
#include <QGroupBox>
#include <QDoubleSpinBox>

#include "../items/via.h"

class AutorouterSettingsDialog : public QDialog
{
Q_OBJECT

public:
	AutorouterSettingsDialog(QHash<QString, QString> & settings, QWidget *parent = 0);
	~AutorouterSettingsDialog();

    QHash<QString, QString> getSettings();

protected slots:
	void production(bool);
	void widthEntry(const QString &);
	void changeUnits(bool);
	void changeHoleSize(const QString &);
	void changeDiameter();
	void changeThickness();
    void toInches();
    void toMM();
    void keepoutEntry();

protected:
	void enableCustom(bool enable);
	bool initProductionType();
	void setTraceWidth(int newWidth);
    QWidget * createViaWidget();
    QWidget * createTraceWidget();
    QWidget * createKeepoutWidget(const QString & keepoutString);
    QString getKeepoutString();
    void setDefaultKeepout();

protected:
	QRadioButton * m_homebrewButton;
	QRadioButton * m_professionalButton;
	QRadioButton * m_customButton;
	HoleSettings m_holeSettings;
	QFrame * m_customFrame;
	QComboBox * m_traceWidthComboBox;
	int m_traceWidth;
    double m_keepoutMils;
    bool m_inches;
    QDoubleSpinBox * m_keepoutSpinBox;
    QRadioButton * m_inRadio;
    QRadioButton * m_mmRadio;

public:
	static const QString AutorouteTraceWidth;

};


#endif // AUTOROUTERSETTINGSDIALOG_H
