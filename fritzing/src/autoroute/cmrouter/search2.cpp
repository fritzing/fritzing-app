/*
 * search2.c --
 *
 * Area searching.
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

int MINDIFF = 1;


/* -------------------- Local function headers ------------------------ */

int tiSrAreaEnum(Tile *enumRT, int enumBottom,  TileRect *rect, TileCallback, UserData arg);

/*
 * --------------------------------------------------------------------
 *
 * TiSrArea --
 *
 * Find all tiles contained in or incident upon a given area.
 * Applies the given procedure to all tiles found.  The procedure
 * should be of the following form:
 *
 *	int
 *	func(tile, cdata)
 *	    Tile *tile;
 *	    UserData cdata;
 *	{
 *	}
 *
 * Func normally should return 0.  If it returns 1 then the search
 * will be aborted.
 *
 * THIS PROCEDURE IS OBSOLETE EXCEPT FOR THE SUBCELL PLANE.  USE
 * DBSrPaintArea() IF YOU WANT TO SEARCH FOR PAINT TILES.
 *
 * Results:
 *	0 is returned if the search completed normally.  1 is returned
 *	if it aborted.
 *
 * Side effects:
 *	Whatever side effects result from application of the
 *	supplied procedure.
 *
 * NOTE:
 *	The procedure called is free to do anything it wishes to tiles
 *	which have already been visited in the area search, but it must
 *	not affect anything about tiles not visited other than possibly
 *	corner stitches to tiles already visited.
 *
 * *************************************************************************
 * *************************************************************************
 * ****									****
 * ****				  WARNING				****
 * ****									****
 * ****		This code is INCREDIBLY sensitive to modification!	****
 * ****		Change it only with the utmost caution, or you'll	****
 * ****		be verrry sorry!					****
 * ****									****
 * *************************************************************************
 * *************************************************************************
 *
 * --------------------------------------------------------------------
 */

int
TiSrArea(Tile *hintTile, Plane *plane, TileRect *rect, TileCallback tileCallback, UserData arg)
    /* Tile at which to begin search, if not NULL.
			 * If this is NULL, use the hint tile supplied
			 * with plane.
			 */
    /* Plane in which tiles lie.  This is used to
			 * provide a hint tile in case hintTile == NULL.
			 * The hint tile in the plane is updated to be
			 * the last tile visited in the area enumeration.
			 */
    /* Area to search */
    /* Function to apply at each tile */
    /* Additional argument to pass to (*func)() */
{
    TilePoint here;
    Tile *tp, *enumTR, *enumTile;
    int enumRight, enumBottom;

    /*
     * We will scan from top to bottom along the left hand edge
     * of the search area, searching for tiles.  Each tile we
     * find in this search will be enumerated.
     */

    here.xi = rect->xmini;
    here.yi = rect->ymaxi - MINDIFF;
    enumTile = hintTile ? hintTile : plane->pl_hint;
    plane->pl_hint = enumTile = gotoPoint(enumTile, here);

    while (here.yi >= rect->ymini)
    {
	/*
	 * Find the tile (tp) immediately below the one to be
	 * enumerated (enumTile).  This must be done before we enumerate
	 * the tile, as the filter function applied to enumerate
	 * it can result in its deallocation or modification in
	 * some other way.
	 */
	here.yi = YMIN(enumTile) - MINDIFF;
	tp = enumTile;
	plane->pl_hint = tp = gotoPoint(tp, here);

	enumRight = RIGHT(enumTile);
	enumBottom = YMIN(enumTile);
	enumTR = TR(enumTile);
	if ((*tileCallback)(enumTile, arg)) return 1;

	/*
	 * If the right boundary of the tile being enumerated is
	 * inside of the search area, recursively enumerate
	 * tiles to its right.
	 */

	if (enumRight < rect->xmaxi)
	    if (tiSrAreaEnum(enumTR, enumBottom, rect, tileCallback, arg))
		return 1;
	enumTile = tp;
    }
    return 0;
}

/*
 * --------------------------------------------------------------------
 *
 * tiSrAreaEnum --
 *
 * Perform the recursive edge search of the tile which has just been
 * enumerated in an area search.  The arguments passed are the RT
 * corner stitch and bottom coordinate of the tile just enumerated.
 *
 * Results:
 *	0 is returned if the search completed normally, 1 if
 *	it was aborted.
 *
 * Side effects:
 *	Attempts to enumerate recursively each tile found in walking
 *	along the right edge of the tile just enumerated.  Whatever
 *	side effects occur result from the application of the client's
 *	filter function.
 *
 * --------------------------------------------------------------------
 */

int
tiSrAreaEnum( Tile *enumRT, int enumBottom, TileRect *rect, TileCallback tileCallback, UserData arg)
   /* TR corner stitch of tile just enumerated */
   /* Bottom coordinate of tile just enumerated */
   /* Area to search */
   /* Function to apply at each tile */
   /* Additional argument to pass to (*func)() */
{
    Tile *tp, *tpLB, *tpTR;
    int tpRight, tpNextTop, tpBottom, srchBottom;
    int atBottom = (enumBottom <= rect->ymini);


    /*
     * Begin examination of tiles along right edge.
     * A tile to the right of the one being enumerated is enumerable if:
     *	- its bottom lies at or above that of the tile being enumerated, or,
     *	- the bottom of the tile being enumerated lies at or below the
     *	  bottom of the search rectangle.
     */

    if ((srchBottom = enumBottom) < rect->ymini)
	srchBottom = rect->ymini;

    for (tp = enumRT, tpNextTop = YMAX(tp); tpNextTop > srchBottom; tp = tpLB)
    {

	/*
	 * Since the client's filter function may result in this tile
	 * being deallocated or otherwise modified, we must extract
	 * all the information we will need from the tile before we
	 * apply the filter function.
	 */

	tpLB = LB(tp);
	tpNextTop = YMAX(tpLB);	/* Since YMAX(tpLB) comes from tp */

	if (YMIN(tp) < rect->ymaxi && (atBottom || YMIN(tp) >= enumBottom))
	{
	    /*
	     * We extract more information from the tile, which we will use
	     * after applying the filter function.
	     */

	    tpRight = RIGHT(tp);
	    tpBottom = YMIN(tp);
	    tpTR = TR(tp);
	    if ((*tileCallback)(tp, arg)) return 1;

	    /*
	     * If the right boundary of the tile being enumerated is
	     * inside of the search area, recursively enumerate
	     * tiles to its right.
	     */

	    if (tpRight < rect->xmaxi)
		if (tiSrAreaEnum(tpTR, tpBottom, rect, tileCallback, arg))
		    return 1;
	}
    }
    return 0;
}
