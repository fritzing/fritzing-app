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
  ${CMAKE_CURRENT_SOURCE_DIR}/svgfilesplitter.h
  ${CMAKE_CURRENT_SOURCE_DIR}/svgpathparser.h
  ${CMAKE_CURRENT_SOURCE_DIR}/svgpathgrammar_p.h
  ${CMAKE_CURRENT_SOURCE_DIR}/svgpathlexer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/svgpathrunner.h
  ${CMAKE_CURRENT_SOURCE_DIR}/svg2gerber.h
  ${CMAKE_CURRENT_SOURCE_DIR}/svgflattener.h
  ${CMAKE_CURRENT_SOURCE_DIR}/gerbergenerator.h
  ${CMAKE_CURRENT_SOURCE_DIR}/groundplanegenerator.h
  ${CMAKE_CURRENT_SOURCE_DIR}/x2svg.h
  ${CMAKE_CURRENT_SOURCE_DIR}/kicad2svg.h
  ${CMAKE_CURRENT_SOURCE_DIR}/kicadmodule2svg.h
  ${CMAKE_CURRENT_SOURCE_DIR}/kicadschematic2svg.h
  ${CMAKE_CURRENT_SOURCE_DIR}/gedaelement2svg.h
  ${CMAKE_CURRENT_SOURCE_DIR}/gedaelementparser.h
  ${CMAKE_CURRENT_SOURCE_DIR}/gedaelementgrammar_p.h
  ${CMAKE_CURRENT_SOURCE_DIR}/gedaelementlexer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/svgtext.h
  PARENT_SCOPE)

set(FRITZING_SRCS ${FRITZING_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/svgfilesplitter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/svgpathparser.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/svgpathgrammar.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/svgpathlexer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/svgpathrunner.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/svg2gerber.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/svgflattener.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/gerbergenerator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/groundplanegenerator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/x2svg.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/kicad2svg.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/kicadmodule2svg.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/kicadschematic2svg.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/gedaelement2svg.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/gedaelementparser.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/gedaelementgrammar.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/gedaelementlexer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/svgtext.cpp
  PARENT_SCOPE)
