/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2026 Fritzing

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

********************************************************************/

#ifndef GERBERPARSER_H
#define GERBERPARSER_H

#include "gerberdocument.h"

class QIODevice;

/**
 * @brief Clean-room RS-274X parser.
 *
 * Implements the subset of the Ucamco spec that Fritzing's
 * GerberGenerator can emit:
 *
 *   - Format spec        %FSLAX{n}{m}Y{n}{m}*%
 *   - Units              %MOMM*%, %MOIN*%
 *   - Aperture defs      %ADD{code}{C|R|O|P},...*%
 *   - Aperture select    D{code}*
 *   - Move               X..Y..D02*
 *   - Stroke             X..Y..D01*
 *   - Flash              X..Y..D03*
 *   - Region begin/end   G36*, G37*
 *   - Linear interp      G01*  (default; G02/G03 ignored with warn)
 *   - End of file        M02*
 *   - Modal X / Y        (omitted coordinate reuses last value)
 *
 * Unsupported (warned and skipped, not fatal):
 *   - Aperture macros    %AM ... *%
 *   - Step-and-repeat    %SR ... *%
 *   - Arcs               G02/G03 (rendered as straight stroke)
 *
 * The parser is stateless across calls and does not own any I/O —
 * pass it a raw string or a QIODevice.
 */
class GerberParser {
public:
	GerberParser() = default;

	/**
	 * @brief Parse the contents of @p text into a GerberDocument.
	 * @param text Full file contents (UTF-8 / ASCII; spec is ASCII).
	 * @return Populated document; warnings field lists anything we
	 *         could not handle. Never throws.
	 */
	GerberDocument parse(const QString &text) const;

	/// Convenience overload reading from a device (opens if needed).
	GerberDocument parse(QIODevice *device) const;
};

#endif // GERBERPARSER_H
