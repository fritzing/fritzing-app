#ifndef QUA_ZIPFILEINFO_H
#define QUA_ZIPFILEINFO_H

/*
-- A kind of "standard" GPL license statement --
QuaZIP - a Qt/C++ wrapper for the ZIP/UNZIP package
Copyright (C) 2005-2007 Sergey A. Tachenov

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

-- A kind of "standard" GPL license statement ends here --

See COPYING file for GPL.

You are also permitted to use QuaZIP under the terms of LGPL (see
COPYING.LGPL). You are free to choose either license, but please note
that QuaZIP makes use of Qt, which is not licensed under LGPL. So if
you are using Open Source edition of Qt, you therefore MUST use GPL for
your code based on QuaZIP, since it would be also based on Qt in this
case. If you are Qt commercial license owner, then you are free to use
QuaZIP as long as you respect either GPL or LGPL for QuaZIP code.
 **/

#include <QByteArray>
#include <QDateTime>

/// Information about a file inside archive.
/** Call QuaZip::getCurrentFileInfo() or QuaZipFile::getFileInfo() to
 * fill this structure. */
struct QuaZipFileInfo {
  /// File name.
  QString name;
  /// Version created by.
  quint16 versionCreated;
  /// Version needed to extract.
  quint16 versionNeeded;
  /// General purpose flags.
  quint16 flags;
  /// Compression method.
  quint16 method;
  /// Last modification date and time.
  QDateTime dateTime;
  /// CRC.
  quint32 crc;
  /// Compressed file size.
  quint32 compressedSize;
  /// Uncompressed file size.
  quint32 uncompressedSize;
  /// Disk number start.
  quint16 diskNumberStart;
  /// Internal file attributes.
  quint16 internalAttr;
  /// External file attributes.
  quint32 externalAttr;
  /// Comment.
  QString comment;
  /// Extra field.
  QByteArray extra;
};

#endif
