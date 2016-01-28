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

$Revision: 6926 $:
$Author: irascibl@gmail.com $:
$Date: 2013-03-10 21:27:34 +0100 (So, 10. Mrz 2013) $

********************************************************************/

#include "layerattributes.h"
#include "debugdialog.h"

LayerAttributes::LayerAttributes()
{
    orientation = Qt::Vertical;
    createShape = true;
}

const QString & LayerAttributes::filename() {
	return m_filename;
}

void LayerAttributes::setFilename(const QString & filename) {
	m_filename = filename;
}

const QByteArray & LayerAttributes::loaded() const {
	return m_loaded;
}

void LayerAttributes::clearLoaded() {
	m_loaded.clear();
}

void LayerAttributes::setLoaded(const QByteArray & loaded) {
	m_loaded = loaded;
}

