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

$Revision: 6181 $:
$Author: cohen@irascible.com $:
$Date: 2012-07-18 15:52:11 +0200 (Mi, 18. Jul 2012) $

********************************************************************/

// misc Fritzing utility functions
#include "misc.h"
#include <QDesktopServices>
#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QTextStream>
#include <QSet>

static QStringList p_fritzingExtensions;
static QStringList p_fritzingBundleExtensions;

static inline void initializeExtensionList() {
	if (p_fritzingExtensions.count() == 0) {
		p_fritzingExtensions
			<< FritzingSketchExtension << FritzingBinExtension
			<< FritzingPartExtension
			<< FritzingBundleExtension << FritzingBundledPartExtension
			<< FritzingBundledBinExtension;
                p_fritzingBundleExtensions
			<< FritzingBundleExtension 
                        << FritzingBundledPartExtension
			<< FritzingBundledBinExtension;
	}
        return;
}

const QStringList & fritzingExtensions() {
	initializeExtensionList();
	return p_fritzingExtensions;
}

const QStringList & fritzingBundleExtensions() {
	initializeExtensionList();
	return p_fritzingBundleExtensions;
}

bool isParent(QObject * candidateParent, QObject * candidateChild) {
	QObject * parent = candidateChild->parent();
	while (parent) {
		if (parent == candidateParent) return true;

		parent = parent->parent();
	}

	return false;
}

Qt::KeyboardModifier altOrMetaModifier() {
#ifdef LINUX_32
	return Qt::MetaModifier;
#endif
#ifdef LINUX_64
	return Qt::MetaModifier;
#endif
	return Qt::AltModifier;
}
