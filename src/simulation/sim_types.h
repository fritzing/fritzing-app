/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SIM_TYPES_H
#define SIM_TYPES_H

///> Possible simulation types
enum SIM_TYPE
{
    ST_UNKNOWN,
    ST_AC,
    ST_DC,
    ST_DISTORTION,
    ST_NOISE,
    ST_OP,
    ST_POLE_ZERO,
    ST_SENSITIVITY,
    ST_TRANS_FUNC,
    ST_TRANSIENT
};

///> Possible plot types
enum SIM_PLOT_TYPE
{
    // Y axis
    SPT_VOLTAGE  = 0x01,
    SPT_CURRENT  = 0x02,
    SPT_AC_PHASE = 0x04,
    SPT_AC_MAG   = 0x08,

    // X axis
    SPT_TIME          = 0x10,
    SPT_LIN_FREQUENCY = 0x20,
    SPT_LOG_FREQUENCY = 0x20,
    SPT_SWEEP         = 0x40,

    SPT_UNKNOWN = 0x00
};

#endif /* SIM_TYPES_H */
