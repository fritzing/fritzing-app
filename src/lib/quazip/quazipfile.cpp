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

#include "quazipfile.h"

using namespace std;

QuaZipFile::QuaZipFile():
  zip(NULL), internal(true), zipError(UNZ_OK)
{
}

QuaZipFile::QuaZipFile(QObject *parent):
  QIODevice(parent), zip(NULL), internal(true), zipError(UNZ_OK)
{
}

QuaZipFile::QuaZipFile(const QString& zipName, QObject *parent):
  QIODevice(parent), internal(true), zipError(UNZ_OK)
{
  zip=new QuaZip(zipName);
  Q_CHECK_PTR(zip);
}

QuaZipFile::QuaZipFile(const QString& zipName, const QString& fileName,
    QuaZip::CaseSensitivity cs, QObject *parent):
  QIODevice(parent), internal(true), zipError(UNZ_OK)
{
  zip=new QuaZip(zipName);
  Q_CHECK_PTR(zip);
  this->fileName=fileName;
  this->caseSensitivity=cs;
}

QuaZipFile::QuaZipFile(QuaZip *zip, QObject *parent):
  QIODevice(parent),
  zip(zip), internal(false),
  zipError(UNZ_OK)
{
}

QuaZipFile::~QuaZipFile()
{
  if(isOpen()) close();
  if(internal) delete zip;
}

QString QuaZipFile::getZipName()const
{
  return zip==NULL?QString():zip->getZipName();
}

QString QuaZipFile::getActualFileName()const
{
  setZipError(UNZ_OK);
  if(zip==NULL||(openMode()&WriteOnly)) return QString();
  QString name=zip->getCurrentFileName();
  if(name.isNull())
    setZipError(zip->getZipError());
  return name;
}

void QuaZipFile::setZipName(const QString& zipName)
{
  if(isOpen()) {
    qWarning("QuaZipFile::setZipName(): file is already open - can not set ZIP name");
    return;
  }
  if(zip!=NULL&&internal) delete zip;
  zip=new QuaZip(zipName);
  Q_CHECK_PTR(zip);
  internal=true;
}

void QuaZipFile::setZip(QuaZip *zip)
{
  if(isOpen()) {
    qWarning("QuaZipFile::setZip(): file is already open - can not set ZIP");
    return;
  }
  if(this->zip!=NULL&&internal) delete this->zip;
  this->zip=zip;
  this->fileName=QString();
  internal=false;
}

void QuaZipFile::setFileName(const QString& fileName, QuaZip::CaseSensitivity cs)
{
  if(zip==NULL) {
    qWarning("QuaZipFile::setFileName(): call setZipName() first");
    return;
  }
  if(!internal) {
    qWarning("QuaZipFile::setFileName(): should not be used when not using internal QuaZip");
    return;
  }
  if(isOpen()) {
    qWarning("QuaZipFile::setFileName(): can not set file name for already opened file");
    return;
  }
  this->fileName=fileName;
  this->caseSensitivity=cs;
}

void QuaZipFile::setZipError(int zipError)const
{
  QuaZipFile *fakeThis=(QuaZipFile*)this; // non-const
  fakeThis->zipError=zipError;
  if(zipError==UNZ_OK)
    fakeThis->setErrorString(QString());
  else
    fakeThis->setErrorString(QString("ZIP/UNZIP API error %1").arg(zipError));
}

bool QuaZipFile::open(OpenMode mode)
{
  return open(mode, NULL);
}

bool QuaZipFile::open(OpenMode mode, int *method, int *level, bool raw, const char *password)
{
  resetZipError();
  if(isOpen()) {
    qWarning("QuaZipFile::open(): already opened");
    return false;
  }
  if(mode&Unbuffered) {
    qWarning("QuaZipFile::open(): Unbuffered mode is not supported");
    return false;
  }
  if((mode&ReadOnly)&&!(mode&WriteOnly)) {
    if(internal) {
      if(!zip->open(QuaZip::mdUnzip)) {
        setZipError(zip->getZipError());
        return false;
      }
      if(!zip->setCurrentFile(fileName, caseSensitivity)) {
        setZipError(zip->getZipError());
        zip->close();
        return false;
      }
    } else {
      if(zip==NULL) {
        qWarning("QuaZipFile::open(): zip is NULL");
        return false;
      }
      if(zip->getMode()!=QuaZip::mdUnzip) {
        qWarning("QuaZipFile::open(): file open mode %d incompatible with ZIP open mode %d",
            (int)mode, (int)zip->getMode());
        return false;
      }
      if(!zip->hasCurrentFile()) {
        qWarning("QuaZipFile::open(): zip does not have current file");
        return false;
      }
    }
    setZipError(unzOpenCurrentFile3(zip->getUnzFile(), method, level, (int)raw, password));
    if(zipError==UNZ_OK) {
      setOpenMode(mode);
      this->raw=raw;
      return true;
    } else
      return false;
  }
  qWarning("QuaZipFile::open(): open mode %d not supported by this function", (int)mode);
  return false;
}

bool QuaZipFile::open(OpenMode mode, const QuaZipNewInfo& info,
    const char *password, quint32 crc,
    int method, int level, bool raw,
    int windowBits, int memLevel, int strategy)
{
  zip_fileinfo info_z;
  resetZipError();
  if(isOpen()) {
    qWarning("QuaZipFile::open(): already opened");
    return false;
  }
  if((mode&WriteOnly)&&!(mode&ReadOnly)) {
    if(internal) {
      qWarning("QuaZipFile::open(): write mode is incompatible with internal QuaZip approach");
      return false;
    }
    if(zip==NULL) {
      qWarning("QuaZipFile::open(): zip is NULL");
      return false;
    }
    if(zip->getMode()!=QuaZip::mdCreate&&zip->getMode()!=QuaZip::mdAppend&&zip->getMode()!=QuaZip::mdAdd) {
      qWarning("QuaZipFile::open(): file open mode %d incompatible with ZIP open mode %d",
          (int)mode, (int)zip->getMode());
      return false;
    }
    info_z.tmz_date.tm_year=info.dateTime.date().year();
    info_z.tmz_date.tm_mon=info.dateTime.date().month() - 1;
    info_z.tmz_date.tm_mday=info.dateTime.date().day();
    info_z.tmz_date.tm_hour=info.dateTime.time().hour();
    info_z.tmz_date.tm_min=info.dateTime.time().minute();
    info_z.tmz_date.tm_sec=info.dateTime.time().second();
    info_z.dosDate = 0;
    info_z.internal_fa=(uLong)info.internalAttr;
    info_z.external_fa=(uLong)info.externalAttr;
    setZipError(zipOpenNewFileInZip3(zip->getZipFile(),
          zip->getFileNameCodec()->fromUnicode(info.name).constData(), &info_z,
          info.extraLocal.constData(), info.extraLocal.length(),
          info.extraGlobal.constData(), info.extraGlobal.length(),
          zip->getCommentCodec()->fromUnicode(info.comment).constData(),
          method, level, (int)raw,
          windowBits, memLevel, strategy,
          password, (uLong)crc));
    if(zipError==UNZ_OK) {
      writePos=0;
      setOpenMode(mode);
      this->raw=raw;
      if(raw) {
        this->crc=crc;
        this->uncompressedSize=info.uncompressedSize;
      }
      return true;
    } else
      return false;
  }
  qWarning("QuaZipFile::open(): open mode %d not supported by this function", (int)mode);
  return false;
}

bool QuaZipFile::isSequential()const
{
  return true;
}

qint64 QuaZipFile::pos()const
{
  if(zip==NULL) {
    qWarning("QuaZipFile::pos(): call setZipName() or setZip() first");
    return -1;
  }
  if(!isOpen()) {
    qWarning("QuaZipFile::pos(): file is not open");
    return -1;
  }
  if(openMode()&ReadOnly)
    return unztell(zip->getUnzFile());
  else
    return writePos;
}

bool QuaZipFile::atEnd()const
{
  if(zip==NULL) {
    qWarning("QuaZipFile::atEnd(): call setZipName() or setZip() first");
    return false;
  }
  if(!isOpen()) {
    qWarning("QuaZipFile::atEnd(): file is not open");
    return false;
  }
  if(openMode()&ReadOnly)
    return unzeof(zip->getUnzFile())==1;
  else
    return true;
}

qint64 QuaZipFile::size()const
{
  if(!isOpen()) {
    qWarning("QuaZipFile::atEnd(): file is not open");
    return -1;
  }
  if(openMode()&ReadOnly)
    return raw?csize():usize();
  else
    return writePos;
}

qint64 QuaZipFile::csize()const
{
  unz_file_info info_z;
  setZipError(UNZ_OK);
  if(zip==NULL||zip->getMode()!=QuaZip::mdUnzip) return -1;
  setZipError(unzGetCurrentFileInfo(zip->getUnzFile(), &info_z, NULL, 0, NULL, 0, NULL, 0));
  if(zipError!=UNZ_OK)
    return -1;
  return info_z.compressed_size;
}

qint64 QuaZipFile::usize()const
{
  unz_file_info info_z;
  setZipError(UNZ_OK);
  if(zip==NULL||zip->getMode()!=QuaZip::mdUnzip) return -1;
  setZipError(unzGetCurrentFileInfo(zip->getUnzFile(), &info_z, NULL, 0, NULL, 0, NULL, 0));
  if(zipError!=UNZ_OK)
    return -1;
  return info_z.uncompressed_size;
}

bool QuaZipFile::getFileInfo(QuaZipFileInfo *info)
{
  if(zip==NULL||zip->getMode()!=QuaZip::mdUnzip) return false;
  zip->getCurrentFileInfo(info);
  setZipError(zip->getZipError());
  return zipError==UNZ_OK;
}

void QuaZipFile::close()
{
  resetZipError();
  if(zip==NULL||!zip->isOpen()) return;
  if(!isOpen()) {
    qWarning("QuaZipFile::close(): file isn't open");
    return;
  }
  if(openMode()&ReadOnly)
    setZipError(unzCloseCurrentFile(zip->getUnzFile()));
  else if(openMode()&WriteOnly)
    if(isRaw()) setZipError(zipCloseFileInZipRaw(zip->getZipFile(), uncompressedSize, crc));
    else setZipError(zipCloseFileInZip(zip->getZipFile()));
  else {
    qWarning("Wrong open mode: %d", (int)openMode());
    return;
  }
  if(zipError==UNZ_OK) setOpenMode(QIODevice::NotOpen);
  else return;
  if(internal) {
    zip->close();
    setZipError(zip->getZipError());
  }
}

qint64 QuaZipFile::readData(char *data, qint64 maxSize)
{
  setZipError(UNZ_OK);
  qint64 bytesRead=unzReadCurrentFile(zip->getUnzFile(), data, (unsigned)maxSize);
  if(bytesRead<0) setZipError((int)bytesRead);
  return bytesRead;
}

qint64 QuaZipFile::writeData(const char* data, qint64 maxSize)
{
  setZipError(ZIP_OK);
  setZipError(zipWriteInFileInZip(zip->getZipFile(), data, (uint)maxSize));
  if(zipError!=ZIP_OK) return -1;
  else {
    writePos+=maxSize;
    return maxSize;
  }
}
