# /*******************************************************************
# Part of the Fritzing project - http://fritzing.org
# Copyright (c) 2007-23 Fritzing GmbH
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

HEADERS += \
	$$PWD/../src/connectors/debugconnectorsprobe.h \
src/connectors/bus.h \
src/connectors/busshared.h \
src/connectors/connector.h \
src/connectors/connectoritem.h \
src/connectors/nonconnectoritem.h \
src/connectors/connectorshared.h \
src/connectors/ercdata.h \
src/connectors/svgidlayer.h \
src/connectors/debugconnectors.h

SOURCES += \
	$$PWD/../src/connectors/debugconnectorsprobe.cpp \
src/connectors/bus.cpp \
src/connectors/busshared.cpp \
src/connectors/connector.cpp \
src/connectors/connectoritem.cpp \
src/connectors/nonconnectoritem.cpp \
src/connectors/connectorshared.cpp \
src/connectors/ercdata.cpp \
src/connectors/svgidlayer.cpp \
src/connectors/debugconnectors.cpp
