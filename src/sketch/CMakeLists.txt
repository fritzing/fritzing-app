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
  ${CMAKE_CURRENT_SOURCE_DIR}/renderthing.h
  ${CMAKE_CURRENT_SOURCE_DIR}/fgraphicsscene.h
  ${CMAKE_CURRENT_SOURCE_DIR}/breadboardsketchwidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/infographicsview.h
  ${CMAKE_CURRENT_SOURCE_DIR}/pcbsketchwidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/schematicsketchwidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/sketchwidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/welcomeview.h
  ${CMAKE_CURRENT_SOURCE_DIR}/zoomablegraphicsview.h
  PARENT_SCOPE)

set(FRITZING_SRCS ${FRITZING_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/renderthing.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/fgraphicsscene.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/breadboardsketchwidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/infographicsview.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pcbsketchwidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/schematicsketchwidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sketchwidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/welcomeview.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/zoomablegraphicsview.cpp
  PARENT_SCOPE)