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

$Revision: 6954 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-05 10:22:00 +0200 (Fr, 05. Apr 2013) $

********************************************************************/

#ifndef PALETTEMODEL_H
#define PALETTEMODEL_H

#include "modelpart.h"
#include "modelbase.h"

#include <QDomDocument>
#include <QList>
#include <QDir>
#include <QStringList>
#include <QHash>

class PaletteModel : public ModelBase
{
Q_OBJECT
public:
	PaletteModel();
	PaletteModel(bool makeRoot, bool doInit);
    ~PaletteModel();
	ModelPart * retrieveModelPart(const QString & moduleID);
	virtual bool containsModelPart(const QString & moduleID);
	virtual ModelPart * loadPart(const QString & path, bool update);
	void clear();
	bool loadedFromFile();
	QString loadedFrom();
	bool loadFromFile(const QString & fileName, ModelBase* referenceModel, bool checkViews);
	ModelPart * addPart(QString newPartPath, bool addToReference, bool updateIdAlreadyExists);
	void removePart(const QString &moduleID);
    void removeParts();
    QList<ModelPart *> search(const QString & searchText, bool allowObsolete);

	void clearPartHash();
	void setOrdererChildren(QList<QObject*> children);
    void search(ModelPart * modelPart, const QStringList & searchStrings, QList<ModelPart *> & modelParts, bool allowObsolete);
    QList<ModelPart *> findContribNoBin();
    QList<ModelPart *> allParts();

protected:
	QHash<QString, ModelPart *> m_partHash;
	bool m_loadedFromFile;
	QString m_loadedFrom; // The file this was loaded from, only if m_loadedFromFile == true

	bool m_loadingContrib;
    bool m_fullLoad;

signals:
	void loadedPart(int i, int total);
    void incSearch();
    void addSearchMaximum(int);
    void partsToLoad(int total);

protected:
	virtual void initParts(bool dbExists);
	void loadParts(bool dbExists);
	void loadPartsAux(QDir & dir, QStringList & nameFilters, int & loadedPart, int totalParts);
	void countParts(QDir & dir, QStringList & nameFilters, int & partCount);
    ModelPart * makeSubpart(ModelPart * originalModelPart, const QDomElement & originalSubparth);

public:
	static void initNames();
    static void setFzpOverrideFolder(const QString &);

protected:
    static QString s_fzpOverrideFolder;

};
#endif
