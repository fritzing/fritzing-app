/*
 * search.c --
 *
 * Point searching.
 *
 *     ********************************************************************* 
 *     * Copyright (C) 1985, 1990 Regents of the University of California. * 
 *     * Permission to use, copy, modify, and distribute this              * 
 *     * software and its documentation for any purpose and without        * 
 *     * fee is hereby granted, provided that the above copyright          * 
 *     * notice appear in all copies.  The University of California        * 
 *     * makes no representations about the suitability of this            * 
 *     * software for any purpose.  It is provided "as is" without         * 
 *     * express or implied warranty.  Export of this software outside     * 
 *     * of the United States of America may require an export license.    * 
 *     *********************************************************************
 */

#include <stdio.h>

#include "tile.h"


/*
 * --------------------------------------------------------------------
 *
 * TiSrPoint --
 *
 * Search for a point.
 *
 * Results:
 *	A pointer to the tile containing the point.
 *	The bottom and left edge of a tile are considered part of
 *	the tile; the top and right edge are not.
 *
 * Side effects:
 *	Updates the hint tile in the supplied plane to point
 *	to the tile found.
 *
 * --------------------------------------------------------------------
 */

Tile *
TiSrPoint(Tile * hintTile, Plane * plane, int x, int y)
    /* Pointer to tile at which to begin search.
				 * If this is NULL, use the hint tile stored
				 * with the plane instead.
				 */
    /* Plane (containing hint tile pointer) */
    /* Point for which to search */
{
    Tile *tp = (hintTile) ? hintTile : plane->pl_hint;

	TilePoint point; 
	point.xi = x;
	point.yi = y;
    plane->pl_hint = tp = gotoPoint(tp, point);
    return(tp);
}
