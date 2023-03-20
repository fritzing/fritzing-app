# /*******************************************************************
# Part of the Fritzing project - http://fritzing.org
# Copyright (c) 2007-23 Fritzing
# Fritzing is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# Fritzing is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTIC,asMainWindowULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.
# ********************************************************************/

set(FRITZING_HDRS ${FRITZING_HDRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/partsbinpalettewidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/partsbinview.h
  ${CMAKE_CURRENT_SOURCE_DIR}/partsbinlistview.h
  ${CMAKE_CURRENT_SOURCE_DIR}/partsbiniconview.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphicsflowlayout.h
  ${CMAKE_CURRENT_SOURCE_DIR}/svgiconwidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/partsbincommands.h
  ${CMAKE_CURRENT_SOURCE_DIR}/searchlineedit.h
  ${CMAKE_CURRENT_SOURCE_DIR}/binmanager/binmanager.h
  ${CMAKE_CURRENT_SOURCE_DIR}/binmanager/stacktabbar.h
  ${CMAKE_CURRENT_SOURCE_DIR}/binmanager/stacktabwidget.h
  PARENT_SCOPE)

set(FRITZING_SRCS ${FRITZING_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/partsbinpalettewidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/partsbinview.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/partsbinlistview.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/partsbiniconview.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphicsflowlayout.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/svgiconwidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/partsbincommands.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/searchlineedit.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/binmanager/binmanager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/binmanager/stacktabbar.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/binmanager/stacktabwidget.cpp
  PARENT_SCOPE)
