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

#ifndef MISC_H
#define MISC_H

#include <QHash>
#include <QVector>

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
// TODO: this debugging hack seems to break in Qt 5; needs investigation
#ifdef Q_OS_WIN
#ifndef QT_NO_DEBUG

#ifdef _MSC_VER // just for the MS compiler
// windows hack for finding memory leaks
// the 'new' redefinition breaks QHash and QVector so they are included beforehand.
#define _CRTDBG_MAP_ALLOC
#include <iostream>
#include <crtdbg.h>
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif

#endif
#endif
#endif

#ifdef Q_OS_WIN
#define getenvUser() getenv("USERNAME")
#else
#define getenvUser() getenv("USER")
#endif

#include <QString>
#include <QDir>
#include <QDomElement>
#include <QStringList>
#include <QPair>
#include <QList>
#include <QPointF>

#define ALLMOUSEBUTTONS (Qt::LeftButton | Qt::MidButton | Qt::RightButton | Qt::XButton1 | Qt::XButton2)

typedef QPair<double, double> RealPair;

static QString ___emptyString___;
static QDomElement ___emptyElement___;
static QStringList ___emptyStringList___;
static QHash<QString, QString> ___emptyStringHash___;
static QDir ___emptyDir___;
static QByteArray ___emptyByteArray___;

static const QString OCRAFontName("OCRA"); 

static const QString ResourcePath(":/resources/");

bool isParent(QObject * candidateParent, QObject * candidateChild);

static const QString FritzingSketchExtension(".fz");
static const QString FritzingBundleExtension(".fzz");
static const QString FritzingBinExtension(".fzb");
static const QString FritzingBundledBinExtension(".fzbz");
static const QString FritzingPartExtension(".fzp");
static const QString FritzingBundledPartExtension(".fzpz");

inline double qMin(float f, double d) {
    return qMin((double) f, d);
}

inline double qMin(double d, float f) {
    return qMin((double) f, d);
}

inline double qMax(float f, double d) {
    return qMax((double) f, d);
}

inline double qMax(double d, float f) {
    return qMax((double) f, d);
}

const QStringList & fritzingExtensions();
const QStringList & fritzingBundleExtensions();

static const QString FemaleSymbolString = QString("%1").arg(QChar(0x2640));
static const QString MaleSymbolString = QString("%1").arg(QChar(0x2642));

Qt::KeyboardModifier altOrMetaModifier();

static QRegExp IntegerFinder("\\d+");

static const int PartsBinHeightDefault = 240;
static const int InfoViewHeightDefault = 220;
static const int InfoViewMinHeight = 50;
static const int DockWidthDefault = 300;
static const int DockHeightDefault = 50;

#endif
