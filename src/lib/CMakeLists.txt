# /*******************************************************************
# Part of the Fritzing project - http://fritzing.org
# Copyright (c) 2007-23 Fritzing
# Fritzing is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# Fritzing is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with Fritzing. If not, see <http://www.gnu.org/licenses/>.
# ********************************************************************/

set(FRITZING_HDRS ${FRITZING_HDRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/qtsysteminfo/QtSystemInfo.h
  ${CMAKE_CURRENT_SOURCE_DIR}/quazip/crypt.h
  ${CMAKE_CURRENT_SOURCE_DIR}/quazip/ioapi.h
  ${CMAKE_CURRENT_SOURCE_DIR}/quazip/quazip.h
  ${CMAKE_CURRENT_SOURCE_DIR}/quazip/quazipfile.h
  ${CMAKE_CURRENT_SOURCE_DIR}/quazip/quazipfileinfo.h
  ${CMAKE_CURRENT_SOURCE_DIR}/quazip/quazipnewinfo.h
  ${CMAKE_CURRENT_SOURCE_DIR}/quazip/unzip.h
  ${CMAKE_CURRENT_SOURCE_DIR}/quazip/zip.h
  PARENT_SCOPE)

set(FRITZING_SRCS ${FRITZING_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/qtsysteminfo/QtSystemInfo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/quazip/ioapi.c
  ${CMAKE_CURRENT_SOURCE_DIR}/quazip/quazip.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/quazip/quazipfile.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/quazip/quazipnewinfo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/quazip/unzip.c
  ${CMAKE_CURRENT_SOURCE_DIR}/quazip/zip.c
  PARENT_SCOPE)
