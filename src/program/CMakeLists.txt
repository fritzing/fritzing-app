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
  ${CMAKE_CURRENT_SOURCE_DIR}/highlighter.h
  ${CMAKE_CURRENT_SOURCE_DIR}/programtab.h
  ${CMAKE_CURRENT_SOURCE_DIR}/programwindow.h
  ${CMAKE_CURRENT_SOURCE_DIR}/syntaxer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/trienode.h
  ${CMAKE_CURRENT_SOURCE_DIR}/console.h
  ${CMAKE_CURRENT_SOURCE_DIR}/consolewindow.h
  ${CMAKE_CURRENT_SOURCE_DIR}/consolesettings.h
  ${CMAKE_CURRENT_SOURCE_DIR}/platform.h
  ${CMAKE_CURRENT_SOURCE_DIR}/platformarduino.h
  ${CMAKE_CURRENT_SOURCE_DIR}/platformpicaxe.h
  ${CMAKE_CURRENT_SOURCE_DIR}/platformlaunchpad.h
  PARENT_SCOPE)

set(FRITZING_SRCS ${FRITZING_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/highlighter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/programtab.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/programwindow.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/syntaxer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/trienode.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/console.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/consolewindow.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/consolesettings.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/platform.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/platformarduino.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/platformpicaxe.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/platformlaunchpad.cpp
  PARENT_SCOPE)

set(FRITZING_FORMS ${FRITZING_FORMS}
  ${CMAKE_CURRENT_SOURCE_DIR}/consolewindow.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/consolesettings.ui
  PARENT_SCOPE)
