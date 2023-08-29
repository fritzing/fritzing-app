/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2023 Fritzing GmbH

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

#ifndef UPLOADPAIR_H
#define UPLOADPAIR_H

#include <QVariant>

struct UploadPair {
	QString fab_name;
	QString link;
};

QDataStream &operator<<(QDataStream &out, const UploadPair &data);
QDataStream &operator>>(QDataStream &in, UploadPair &data);

Q_DECLARE_METATYPE(UploadPair)

#endif // UPLOADPAIR_H
