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

#include <QFile>

#include "quazip.h"

QuaZip::QuaZip():
  fileNameCodec(QTextCodec::codecForLocale()),
  commentCodec(QTextCodec::codecForLocale()),
  mode(mdNotOpen), hasCurrentFile_f(false), zipError(UNZ_OK)
{
}

QuaZip::QuaZip(const QString& zipName):
  fileNameCodec(QTextCodec::codecForLocale()),
  commentCodec(QTextCodec::codecForLocale()),
  zipName(zipName),
  mode(mdNotOpen), hasCurrentFile_f(false), zipError(UNZ_OK)
{
}

QuaZip::~QuaZip()
{
  if(isOpen()) close();
}

bool QuaZip::open(Mode mode, zlib_filefunc_def* ioApi)
{
  zipError=UNZ_OK;
  if(isOpen()) {
    qWarning("QuaZip::open(): ZIP already opened");
    return false;
  }
  switch(mode) {
    case mdUnzip:
      unzFile_f=unzOpen2(QFile::encodeName(zipName).constData(), ioApi);
      if(unzFile_f!=NULL) {
        this->mode=mode;
        return true;
      } else {
        zipError=UNZ_OPENERROR;
        return false;
      }
    case mdCreate:
    case mdAppend:
    case mdAdd:
      zipFile_f=zipOpen2(QFile::encodeName(zipName).constData(),
          mode==mdCreate?APPEND_STATUS_CREATE:
          mode==mdAppend?APPEND_STATUS_CREATEAFTER:
          APPEND_STATUS_ADDINZIP,
          NULL,
          ioApi);
      if(zipFile_f!=NULL) {
        this->mode=mode;
        return true;
      } else {
        zipError=UNZ_OPENERROR;
        return false;
      }
    default:
      qWarning("QuaZip::open(): unknown mode: %d", (int)mode);
      return false;
      break;
  }
}

void QuaZip::close()
{
  zipError=UNZ_OK;
  switch(mode) {
    case mdNotOpen:
      qWarning("QuaZip::close(): ZIP is not open");
      return;
    case mdUnzip:
      zipError=unzClose(unzFile_f);
      break;
    case mdCreate:
    case mdAppend:
    case mdAdd:
      zipError=zipClose(zipFile_f, commentCodec->fromUnicode(comment).constData());
      break;
    default:
      qWarning("QuaZip::close(): unknown mode: %d", (int)mode);
      return;
  }
  if(zipError==UNZ_OK) mode=mdNotOpen;
}

void QuaZip::setZipName(const QString& zipName)
{
  if(isOpen()) {
    qWarning("QuaZip::setZipName(): ZIP is already open!");
    return;
  }
  this->zipName=zipName;
}

int QuaZip::getEntriesCount()const
{
  QuaZip *fakeThis=(QuaZip*)this; // non-const
  fakeThis->zipError=UNZ_OK;
  if(mode!=mdUnzip) {
    qWarning("QuaZip::getEntriesCount(): ZIP is not open in mdUnzip mode");
    return -1;
  }
  unz_global_info globalInfo;
  if((fakeThis->zipError=unzGetGlobalInfo(unzFile_f, &globalInfo))!=UNZ_OK)
    return zipError;
  return (int)globalInfo.number_entry;
}

QString QuaZip::getComment()const
{
  QuaZip *fakeThis=(QuaZip*)this; // non-const
  fakeThis->zipError=UNZ_OK;
  if(mode!=mdUnzip) {
    qWarning("QuaZip::getComment(): ZIP is not open in mdUnzip mode");
    return QString();
  }
  unz_global_info globalInfo;
  QByteArray comment;
  if((fakeThis->zipError=unzGetGlobalInfo(unzFile_f, &globalInfo))!=UNZ_OK)
    return QString();
  comment.resize(globalInfo.size_comment);
  if((fakeThis->zipError=unzGetGlobalComment(unzFile_f, comment.data(), comment.size()))!=UNZ_OK)
    return QString();
  return commentCodec->toUnicode(comment);
}

bool QuaZip::setCurrentFile(const QString& fileName, CaseSensitivity cs)
{
  zipError=UNZ_OK;
  if(mode!=mdUnzip) {
    qWarning("QuaZip::setCurrentFile(): ZIP is not open in mdUnzip mode");
    return false;
  }
  if(fileName.isNull()) {
    hasCurrentFile_f=false;
    return true;
  }
  // Unicode-aware reimplementation of the unzLocateFile function
  if(unzFile_f==NULL) {
    zipError=UNZ_PARAMERROR;
    return false;
  }
  if(fileName.length()>MAX_FILE_NAME_LENGTH) {
    zipError=UNZ_PARAMERROR;
    return false;
  }
  bool sens;
  if(cs==csDefault) {
#ifdef Q_WS_WIN
    sens=false;
#else
    sens=true;
#endif
  } else sens=cs==csSensitive;
  QString lower, current;
  if(!sens) lower=fileName.toLower();
  hasCurrentFile_f=false;
  for(bool more=goToFirstFile(); more; more=goToNextFile()) {
    current=getCurrentFileName();
    if(current.isNull()) return false;
    if(sens) {
      if(current==fileName) break;
    } else {
      if(current.toLower()==lower) break;
    }
  }
  return hasCurrentFile_f;
}

bool QuaZip::goToFirstFile()
{
  zipError=UNZ_OK;
  if(mode!=mdUnzip) {
    qWarning("QuaZip::goToFirstFile(): ZIP is not open in mdUnzip mode");
    return false;
  }
  zipError=unzGoToFirstFile(unzFile_f);
  hasCurrentFile_f=zipError==UNZ_OK;
  return hasCurrentFile_f;
}

bool QuaZip::goToNextFile()
{
  zipError=UNZ_OK;
  if(mode!=mdUnzip) {
    qWarning("QuaZip::goToFirstFile(): ZIP is not open in mdUnzip mode");
    return false;
  }
  zipError=unzGoToNextFile(unzFile_f);
  hasCurrentFile_f=zipError==UNZ_OK;
  if(zipError==UNZ_END_OF_LIST_OF_FILE) zipError=UNZ_OK;
  return hasCurrentFile_f;
}

bool QuaZip::getCurrentFileInfo(QuaZipFileInfo *info)const
{
  QuaZip *fakeThis=(QuaZip*)this; // non-const
  fakeThis->zipError=UNZ_OK;
  if(mode!=mdUnzip) {
    qWarning("QuaZip::getCurrentFileInfo(): ZIP is not open in mdUnzip mode");
    return false;
  }
  unz_file_info info_z;
  QByteArray fileName;
  QByteArray extra;
  QByteArray comment;
  if(info==NULL) return false;
  if(!isOpen()||!hasCurrentFile()) return false;
  if((fakeThis->zipError=unzGetCurrentFileInfo(unzFile_f, &info_z, NULL, 0, NULL, 0, NULL, 0))!=UNZ_OK)
    return false;
  fileName.resize(info_z.size_filename);
  extra.resize(info_z.size_file_extra);
  comment.resize(info_z.size_file_comment);
  if((fakeThis->zipError=unzGetCurrentFileInfo(unzFile_f, NULL,
      fileName.data(), fileName.size(),
      extra.data(), extra.size(),
      comment.data(), comment.size()))!=UNZ_OK)
    return false;
  info->versionCreated=info_z.version;
  info->versionNeeded=info_z.version_needed;
  info->flags=info_z.flag;
  info->method=info_z.compression_method;
  info->crc=info_z.crc;
  info->compressedSize=info_z.compressed_size;
  info->uncompressedSize=info_z.uncompressed_size;
  info->diskNumberStart=info_z.disk_num_start;
  info->internalAttr=info_z.internal_fa;
  info->externalAttr=info_z.external_fa;
  info->name=fileNameCodec->toUnicode(fileName);
  info->comment=commentCodec->toUnicode(comment);
  info->extra=extra;
  info->dateTime=QDateTime(
      QDate(info_z.tmu_date.tm_year, info_z.tmu_date.tm_mon+1, info_z.tmu_date.tm_mday),
      QTime(info_z.tmu_date.tm_hour, info_z.tmu_date.tm_min, info_z.tmu_date.tm_sec));
  return true;
}

QString QuaZip::getCurrentFileName()const
{
  QuaZip *fakeThis=(QuaZip*)this; // non-const
  fakeThis->zipError=UNZ_OK;
  if(mode!=mdUnzip) {
    qWarning("QuaZip::getCurrentFileName(): ZIP is not open in mdUnzip mode");
    return QString();
  }
  if(!isOpen()||!hasCurrentFile()) return QString();
  QByteArray fileName(MAX_FILE_NAME_LENGTH, 0);
  if((fakeThis->zipError=unzGetCurrentFileInfo(unzFile_f, NULL, fileName.data(), fileName.size(),
      NULL, 0, NULL, 0))!=UNZ_OK)
    return QString();
  return fileNameCodec->toUnicode(fileName.constData());
}
