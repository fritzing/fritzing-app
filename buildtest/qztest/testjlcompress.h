#ifndef QUAZIP_TEST_JLCOMPRESS_H
#define QUAZIP_TEST_JLCOMPRESS_H

/*
Copyright (C) 2005-2014 Sergey A. Tachenov

This file is part of QuaZIP test suite.

QuaZIP is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

QuaZIP is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with QuaZIP.  If not, see <http://www.gnu.org/licenses/>.

See COPYING file for the full LGPL text.

Original ZIP package is copyrighted by Gilles Vollant and contributors,
see quazip/(un)zip.h files for details. Basically it's the zlib license.
*/

#include <QObject>

class TestJlCompress: public QObject {
    Q_OBJECT
private slots:
    void compressFile_data();
    void compressFile();
    void compressFiles_data();
    void compressFiles();
    void compressDir_data();
    void compressDir();
    void extractFile_data();
    void extractFile();
    void extractFiles_data();
    void extractFiles();
    void extractDir_data();
    void extractDir();
    void zeroPermissions();
};

#endif // QUAZIP_TEST_JLCOMPRESS_H
