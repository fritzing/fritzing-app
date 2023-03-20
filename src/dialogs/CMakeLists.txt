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
  ${CMAKE_CURRENT_SOURCE_DIR}/prefsdialog.h
  ${CMAKE_CURRENT_SOURCE_DIR}/pinlabeldialog.h
  ${CMAKE_CURRENT_SOURCE_DIR}/groundfillseeddialog.h
  ${CMAKE_CURRENT_SOURCE_DIR}/quotedialog.h
  ${CMAKE_CURRENT_SOURCE_DIR}/recoverydialog.h
  ${CMAKE_CURRENT_SOURCE_DIR}/setcolordialog.h
  ${CMAKE_CURRENT_SOURCE_DIR}/translatorlistmodel.h
  ${CMAKE_CURRENT_SOURCE_DIR}/fabuploaddialog.h
  ${CMAKE_CURRENT_SOURCE_DIR}/fabuploadprogress.h
  ${CMAKE_CURRENT_SOURCE_DIR}/networkhelper.h
  PARENT_SCOPE)

set(FRITZING_SRCS ${FRITZING_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/prefsdialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pinlabeldialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/groundfillseeddialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/quotedialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/recoverydialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/setcolordialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/translatorlistmodel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/fabuploaddialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/fabuploadprogress.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/networkhelper.cpp
  PARENT_SCOPE)

set(FRITZING_FORMS ${FRITZING_FORMS}
  ${CMAKE_CURRENT_SOURCE_DIR}/fabuploaddialog.ui
  PARENT_SCOPE)

set(FRITZING_INC_DIRS ${FRITZING_INC_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
  PARENT_SCOPE)
