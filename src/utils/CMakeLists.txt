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
  ${CMAKE_CURRENT_SOURCE_DIR}/abstractstatesbutton.h
  ${CMAKE_CURRENT_SOURCE_DIR}/autoclosemessagebox.h
  ${CMAKE_CURRENT_SOURCE_DIR}/bendpointaction.h
  ${CMAKE_CURRENT_SOURCE_DIR}/bezier.h
  ${CMAKE_CURRENT_SOURCE_DIR}/bezierdisplay.h
  ${CMAKE_CURRENT_SOURCE_DIR}/boundedregexpvalidator.h
  ${CMAKE_CURRENT_SOURCE_DIR}/bundler.h
  ${CMAKE_CURRENT_SOURCE_DIR}/clickablelabel.h
  ${CMAKE_CURRENT_SOURCE_DIR}/cursormaster.h
  ${CMAKE_CURRENT_SOURCE_DIR}/expandinglabel.h
  ${CMAKE_CURRENT_SOURCE_DIR}/familypropertycombobox.h
  ${CMAKE_CURRENT_SOURCE_DIR}/fileprogressdialog.h
  ${CMAKE_CURRENT_SOURCE_DIR}/flineedit.h
  ${CMAKE_CURRENT_SOURCE_DIR}/fmessagebox.h
  ${CMAKE_CURRENT_SOURCE_DIR}/focusoutcombobox.h
  ${CMAKE_CURRENT_SOURCE_DIR}/fsizegrip.h
  ${CMAKE_CURRENT_SOURCE_DIR}/lockmanager.h
  ${CMAKE_CURRENT_SOURCE_DIR}/misc.h
  ${CMAKE_CURRENT_SOURCE_DIR}/resizehandle.h
  ${CMAKE_CURRENT_SOURCE_DIR}/folderutils.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphicsutils.h
  ${CMAKE_CURRENT_SOURCE_DIR}/graphutils.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ratsnestcolors.h
  ${CMAKE_CURRENT_SOURCE_DIR}/schematicrectconstants.h
  ${CMAKE_CURRENT_SOURCE_DIR}/s2s.h
  ${CMAKE_CURRENT_SOURCE_DIR}/textutils.h
  ${CMAKE_CURRENT_SOURCE_DIR}/zoomslider.h
  PARENT_SCOPE)

set(FRITZING_SRCS ${FRITZING_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/autoclosemessagebox.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bendpointaction.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bezier.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bezierdisplay.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/clickablelabel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/cursormaster.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/expandinglabel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/fileprogressdialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/flineedit.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/fmessagebox.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/focusoutcombobox.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/fsizegrip.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lockmanager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/misc.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/resizehandle.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/folderutils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphicsutils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphutils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ratsnestcolors.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/schematicrectconstants.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/s2s.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/textutils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/zoomslider.cpp
  PARENT_SCOPE)
