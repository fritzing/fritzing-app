# /*******************************************************************
#
# Part of the Fritzing project - http://fritzing.org
# Copyright (c) 2007-23 Fritzing
#
# Fritzing is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Fritzing is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.
#
#********************************************************************/

set(FRITZING_HDRS ${FRITZING_HDRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/pemainwindow.h
  ${CMAKE_CURRENT_SOURCE_DIR}/pemetadataview.h
  ${CMAKE_CURRENT_SOURCE_DIR}/pecommands.h
  ${CMAKE_CURRENT_SOURCE_DIR}/peconnectorsview.h
  ${CMAKE_CURRENT_SOURCE_DIR}/pesvgview.h
  ${CMAKE_CURRENT_SOURCE_DIR}/petoolview.h
  ${CMAKE_CURRENT_SOURCE_DIR}/peutils.h
  ${CMAKE_CURRENT_SOURCE_DIR}/pegraphicsitem.h
  ${CMAKE_CURRENT_SOURCE_DIR}/kicadmoduledialog.h
  ${CMAKE_CURRENT_SOURCE_DIR}/hashpopulatewidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/baseremovebutton.h
  PARENT_SCOPE)

set(FRITZING_SRCS ${FRITZING_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/pemainwindow.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pemetadataview.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pecommands.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/peconnectorsview.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pesvgview.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/petoolview.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/peutils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pegraphicsitem.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/kicadmoduledialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hashpopulatewidget.cpp
  PARENT_SCOPE)