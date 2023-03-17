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
  ${CMAKE_CURRENT_SOURCE_DIR}/modelbase.h
  ${CMAKE_CURRENT_SOURCE_DIR}/modelpart.h
  ${CMAKE_CURRENT_SOURCE_DIR}/modelpartshared.h
  ${CMAKE_CURRENT_SOURCE_DIR}/palettemodel.h
  ${CMAKE_CURRENT_SOURCE_DIR}/sketchmodel.h
  PARENT_SCOPE)

set(FRITZING_SRCS ${FRITZING_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/modelbase.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/modelpart.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/modelpartshared.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/palettemodel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sketchmodel.cpp
  PARENT_SCOPE)
