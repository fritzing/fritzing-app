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
  ${CMAKE_CURRENT_SOURCE_DIR}/breadboard.h
  ${CMAKE_CURRENT_SOURCE_DIR}/capacitor.h
  ${CMAKE_CURRENT_SOURCE_DIR}/clipablewire.h
  ${CMAKE_CURRENT_SOURCE_DIR}/dip.h
  ${CMAKE_CURRENT_SOURCE_DIR}/groundplane.h
  ${CMAKE_CURRENT_SOURCE_DIR}/hole.h
  ${CMAKE_CURRENT_SOURCE_DIR}/itembase.h
  ${CMAKE_CURRENT_SOURCE_DIR}/jumperitem.h
  ${CMAKE_CURRENT_SOURCE_DIR}/layerkinpaletteitem.h
  ${CMAKE_CURRENT_SOURCE_DIR}/led.h
  ${CMAKE_CURRENT_SOURCE_DIR}/logoitem.h
  ${CMAKE_CURRENT_SOURCE_DIR}/moduleidnames.h
  ${CMAKE_CURRENT_SOURCE_DIR}/mysterypart.h
  ${CMAKE_CURRENT_SOURCE_DIR}/note.h
  ${CMAKE_CURRENT_SOURCE_DIR}/pad.h
  ${CMAKE_CURRENT_SOURCE_DIR}/paletteitem.h
  ${CMAKE_CURRENT_SOURCE_DIR}/paletteitembase.h
  ${CMAKE_CURRENT_SOURCE_DIR}/partfactory.h
  ${CMAKE_CURRENT_SOURCE_DIR}/partlabel.h
  ${CMAKE_CURRENT_SOURCE_DIR}/perfboard.h
  ${CMAKE_CURRENT_SOURCE_DIR}/pinheader.h
  ${CMAKE_CURRENT_SOURCE_DIR}/propertydef.h
  ${CMAKE_CURRENT_SOURCE_DIR}/resistor.h
  ${CMAKE_CURRENT_SOURCE_DIR}/resizableboard.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ruler.h
  ${CMAKE_CURRENT_SOURCE_DIR}/schematicframe.h
  ${CMAKE_CURRENT_SOURCE_DIR}/schematicsubpart.h
  ${CMAKE_CURRENT_SOURCE_DIR}/screwterminal.h
  ${CMAKE_CURRENT_SOURCE_DIR}/stripboard.h
  ${CMAKE_CURRENT_SOURCE_DIR}/symbolpaletteitem.h
  ${CMAKE_CURRENT_SOURCE_DIR}/tracewire.h
  ${CMAKE_CURRENT_SOURCE_DIR}/via.h
  ${CMAKE_CURRENT_SOURCE_DIR}/virtualwire.h
  ${CMAKE_CURRENT_SOURCE_DIR}/wire.h
  PARENT_SCOPE)

set(FRITZING_SRCS ${FRITZING_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/breadboard.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/capacitor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/clipablewire.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/dip.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/groundplane.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hole.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/itembase.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/jumperitem.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/layerkinpaletteitem.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/led.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/logoitem.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/moduleidnames.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mysterypart.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/note.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pad.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/paletteitem.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/paletteitembase.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/partfactory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/partlabel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/perfboard.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pinheader.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/propertydef.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/resistor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/resizableboard.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ruler.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/schematicframe.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/schematicsubpart.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/screwterminal.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/stripboard.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/symbolpaletteitem.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tracewire.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/via.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/virtualwire.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wire.cpp
  PARENT_SCOPE)
