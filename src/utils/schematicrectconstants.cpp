/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2012 Fachhochschule Potsdam - http://fh-potsdam.de

Fritzing is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Fritzing is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************

$Revision: 6904 $:
$Author: irascibl@gmail.com $:
$Date: 2013-02-26 16:26:03 +0100 (Di, 26. Feb 2013) $

********************************************************************/

#include "schematicrectconstants.h"

// all measurements in millimeters

const double SchematicRectConstants::PinWidth = 0.246944;  // millimeters
const double SchematicRectConstants::PinSmallTextHeight = 0.881944444;
const double SchematicRectConstants::PinBigTextHeight = 1.23472222;
const double SchematicRectConstants::PinTextIndent = PinWidth * 2;   // was PinWidth * 3;
const double SchematicRectConstants::PinTextVert = PinWidth * 1;
const double SchematicRectConstants::PinSmallTextVert = -PinWidth / 2;
const double SchematicRectConstants::LabelTextHeight = 1.49930556;
const double SchematicRectConstants::LabelTextSpace = 0.1;
const double SchematicRectConstants::RectStrokeWidth = 0.3175;
const double SchematicRectConstants::NewUnit = 0.1 * 25.4;      // .1in in mm

const QString SchematicRectConstants::PinColor("#787878");
const QString SchematicRectConstants::PinTextColor("#8c8c8c");
const QString SchematicRectConstants::TitleColor("#000000");
const QString SchematicRectConstants::RectStrokeColor("#000000");
const QString SchematicRectConstants::RectFillColor("#FFFFFF");

const QString SchematicRectConstants::FontFamily("'Droid Sans'");