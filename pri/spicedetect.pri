# /*******************************************************************
# Part of the Fritzing project - http://fritzing.org
# Copyright (c) 2021 Fritzing GmbH
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


INCLUDEPATH += $$absolute_path($$_PRO_FILE_PWD_/../ngspice/src/include)
INCLUDEPATH += $$absolute_path($$_PRO_FILE_PWD_/../ngspice/include)

#
#LIBS += -L../ngspice/release/src/.libs -lngspice
# We use QLibrary to load ngspice
