# /*******************************************************************
# Part of the Fritzing project - http://fritzing.org
# Copyright (c) 2007-08 Fritzing
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
# ********************************************************************
# $Revision: 5721 $:
# $Author: cohen@irascible.com $:
# $Date: 2012-01-03 16:53:58 +0100 (Di, 03. Jan 2012) $
# ********************************************************************/
HEADERS += src/svg/svgfilesplitter.h \
    src/svg/svgpathparser.h \
    src/svg/svgpathgrammar_p.h \
    src/svg/svgpathlexer.h \
    src/svg/svgpathrunner.h \
    src/svg/svg2gerber.h \
    src/svg/svgflattener.h \
    src/svg/gerbergenerator.h \
    src/svg/groundplanegenerator.h \
    src/svg/x2svg.h \
    src/svg/kicad2svg.h \
    src/svg/kicadmodule2svg.h \
    src/svg/kicadschematic2svg.h \
    src/svg/gedaelement2svg.h \
    src/svg/gedaelementparser.h \
    src/svg/gedaelementgrammar_p.h \
    src/svg/gedaelementlexer.h
    
SOURCES += src/svg/svgfilesplitter.cpp \
    src/svg/svgpathparser.cpp \
    src/svg/svgpathgrammar.cpp \
    src/svg/svgpathlexer.cpp \
    src/svg/svgpathrunner.cpp \
    src/svg/svg2gerber.cpp \
    src/svg/svgflattener.cpp \
    src/svg/gerbergenerator.cpp \
    src/svg/groundplanegenerator.cpp \
    src/svg/x2svg.cpp \
    src/svg/kicad2svg.cpp \
    src/svg/kicadmodule2svg.cpp \
    src/svg/kicadschematic2svg.cpp \
    src/svg/gedaelement2svg.cpp \
    src/svg/gedaelementparser.cpp \
    src/svg/gedaelementgrammar.cpp \
    src/svg/gedaelementlexer.cpp
