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
  ${CMAKE_CURRENT_SOURCE_DIR}/bus.h
  ${CMAKE_CURRENT_SOURCE_DIR}/busshared.h
  ${CMAKE_CURRENT_SOURCE_DIR}/connector.h
  ${CMAKE_CURRENT_SOURCE_DIR}/connectoritem.h
  ${CMAKE_CURRENT_SOURCE_DIR}/nonconnectoritem.h
  ${CMAKE_CURRENT_SOURCE_DIR}/connectorshared.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ercdata.h
  ${CMAKE_CURRENT_SOURCE_DIR}/svgidlayer.h
  PARENT_SCOPE)

set(FRITZING_SRCS ${FRITZING_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/bus.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/busshared.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/connector.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/connectoritem.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/nonconnectoritem.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/connectorshared.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ercdata.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/svgidlayer.cpp
  PARENT_SCOPE)
