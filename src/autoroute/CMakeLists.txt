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
  ${CMAKE_CURRENT_SOURCE_DIR}/autorouter.h
  ${CMAKE_CURRENT_SOURCE_DIR}/cmrouter/cmrouter.h
  ${CMAKE_CURRENT_SOURCE_DIR}/autorouteprogressdialog.h
  ${CMAKE_CURRENT_SOURCE_DIR}/autoroutersettingsdialog.h
  ${CMAKE_CURRENT_SOURCE_DIR}/panelizer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/cmrouter/tile.h
  ${CMAKE_CURRENT_SOURCE_DIR}/cmrouter/tileutils.h 
  ${CMAKE_CURRENT_SOURCE_DIR}/mazerouter/mazerouter.h
  ${CMAKE_CURRENT_SOURCE_DIR}/zoomcontrols.h
  ${CMAKE_CURRENT_SOURCE_DIR}/drc.h
  PARENT_SCOPE)

set(FRITZING_SRCS ${FRITZING_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/autorouter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cmrouter/cmrouter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/autorouteprogressdialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/autoroutersettingsdialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/panelizer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cmrouter/tile.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cmrouter/DBcell.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cmrouter/search.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cmrouter/search2.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mazerouter/mazerouter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/zoomcontrols.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/drc.cpp
  PARENT_SCOPE)
