/*
 * tile.c --
 *
 * Basic tile manipulation
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
#include <stdlib.h>
#include <limits>
#include "tile.h"

void dupTileBody(Tile * oldtp, Tile * newtp);

/*
 * Debugging version of TiSetBody() macro in tile.h
 * Includes sanity check that a tile at "infinity"
 * is not being set to a type other than space.
 */
/*
void
TiSetBody(tp, b)
   Tile *tp;
   BodyPointer b;
{
   if (b != (BodyPointer)0 && b != (BodyPointer)(-1))
	if (RIGHT(tp) == INFINITY || YMAX(tp) == INFINITY ||
		LEFT(tp) == MINFINITY || YMIN(tp) == MINFINITY)
	    TxError("Error:  Tile at infinity set to non-space value %d\n", (int)b);
   tp->ti_body = b;
}
*/


/*
 * Rectangle that defines the maximum extent of any plane.
 * No tile created by the user should ever extend outside of
 * this area.
 */

int TINFINITY = (std::numeric_limits<int>::max() >> 2) - 4;
int MINFINITY	= -TINFINITY;


/*
 * --------------------------------------------------------------------
 *
 * TiNewPlane --
 *
 * Allocate and initialize a new tile plane.
 *
 * Results:
 *	A newly allocated Plane with all corner stitches set
 *	appropriately.
 *
 * Side effects:
 *	Adjusts the corner stitches of the Tile supplied to
 *	point to the appropriate bounding tile in the newly
 *	created Plane.
 *
 * --------------------------------------------------------------------
 */

Plane *
TiNewPlane(Tile *tile, int minx, int miny, int maxx, int maxy)
    /* Tile to become initial tile of plane.
			 * May be NULL.
			 */
{
    Plane *newplane;
    static Tile *infinityTile = (Tile *) NULL;

    newplane = new Plane;
    newplane->pl_top = TiAlloc();
    newplane->pl_right = TiAlloc();
    newplane->pl_bottom = TiAlloc();
    newplane->pl_left = TiAlloc();

    newplane->maxRect.xmini = minx;
    newplane->maxRect.ymini = miny;
    newplane->maxRect.xmaxi = maxx;
    newplane->maxRect.ymaxi = maxy;

    /*
     * Since the lower left coordinates of the TR and RT
     * stitches of a tile are used to determine its upper right,
     * we must give the boundary tiles a meaningful TR and RT.
     * To make certain that these tiles don't have zero width
     * or height, we use a dummy tile at (TINFINITY+1,TINFINITY+1).
     */

    if (infinityTile == (Tile *) NULL)
    {
	infinityTile = TiAlloc();
        SETLEFT(infinityTile, TINFINITY+1);
        SETYMIN(infinityTile, TINFINITY+1);
    }

    if (tile)
    {
	SETRT(tile, newplane->pl_top);
	SETTR(tile, newplane->pl_right);
	SETLB(tile, newplane->pl_bottom);
	SETBL(tile, newplane->pl_left);
    }

    SETLEFT(newplane->pl_bottom, MINFINITY);
    SETYMIN(newplane->pl_bottom, MINFINITY);
    SETRT(newplane->pl_bottom, tile);
    SETTR(newplane->pl_bottom, newplane->pl_right);
    SETLB(newplane->pl_bottom, BADTILE);
    SETBL(newplane->pl_bottom, newplane->pl_left);
    TiSetBody(newplane->pl_bottom, 0);
	TiSetType(newplane->pl_bottom, Tile::DUMMYBOTTOM);

    SETLEFT(newplane->pl_top, MINFINITY);
    SETYMIN(newplane->pl_top, TINFINITY);
    SETRT(newplane->pl_top, infinityTile);
    SETTR(newplane->pl_top, newplane->pl_right);
    SETLB(newplane->pl_top, tile);
    SETBL(newplane->pl_top, newplane->pl_left);
    TiSetBody(newplane->pl_top, 0);
    TiSetType(newplane->pl_bottom, Tile::DUMMYTOP);

    SETLEFT(newplane->pl_left, MINFINITY);
    SETYMIN(newplane->pl_left, MINFINITY);
    SETRT(newplane->pl_left, newplane->pl_top);
    SETTR(newplane->pl_left, tile);
    SETLB(newplane->pl_left, newplane->pl_bottom);
    SETBL(newplane->pl_left, BADTILE);
    TiSetBody(newplane->pl_left, 0);
    TiSetType(newplane->pl_bottom, Tile::DUMMYLEFT);

    SETLEFT(newplane->pl_right, TINFINITY);
    SETYMIN(newplane->pl_right, MINFINITY);
    SETRT(newplane->pl_right, newplane->pl_top);
    SETTR(newplane->pl_right, infinityTile);
    SETLB(newplane->pl_right, newplane->pl_bottom);
    SETBL(newplane->pl_right, tile);
    TiSetBody(newplane->pl_right, 0);
    TiSetType(newplane->pl_bottom, Tile::DUMMYRIGHT);

    newplane->pl_hint = tile;
    return (newplane);
}

/*
 * --------------------------------------------------------------------
 *
 * TiFreePlane --
 *
 * Free the storage associated with a tile plane.
 * Only the plane itself and its four border tiles are deallocated.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees memory.
 *
 * --------------------------------------------------------------------
 */

void
TiFreePlane(Plane *plane)
    /* Plane to be freed */
{
    TiFree(plane->pl_left);
    TiFree(plane->pl_right);
    TiFree(plane->pl_top);
    TiFree(plane->pl_bottom);
    delete plane;
}

/*
 * --------------------------------------------------------------------
 *
 * TiToRect --
 *
 * Convert a tile to a rectangle.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets *rect to the bounding box for the supplied tile.
 *
 * --------------------------------------------------------------------
 */

void
TiToRect(Tile *tile, TileRect *rect)
    /* Tile whose bounding box is to be stored in *rect */
    /* Pointer to rect to be set to bounding box */
{
    rect->xmini = LEFT(tile);
    rect->xmaxi = RIGHT(tile);
    rect->ymini = YMIN(tile);
    rect->ymaxi = YMAX(tile);
}

/*
 * --------------------------------------------------------------------
 *
 * TiSplitX --
 *
 * Given a tile and an X coordinate, split the tile into two
 * along a line running vertically through the given coordinate.
 *
 * Results:
 *	Returns the new tile resulting from the splitting, which
 *	is the tile occupying the right-hand half of the original
 *	tile.
 *
 * Side effects:
 *	Modifies the corner stitches in the database to reflect
 *	the presence of two tiles in place of the original one.
 *
 * --------------------------------------------------------------------
 */

Tile *
TiSplitX(Tile *tile, int x)
    /* Tile to be split */
    /* X coordinate of split */
{
    Tile *newtile;
    Tile *tp;

    //ASSERT(x > LEFT(tile) && x < RIGHT(tile), "TiSplitX");

    newtile = TiAlloc();
	dupTileBody(tile, newtile);

    SETLEFT(newtile, x);
    SETYMIN(newtile, YMIN(tile));
    SETBL(newtile, tile);
    SETTR(newtile, TR(tile));
    SETRT(newtile, RT(tile));

    /*
     * Adjust corner stitches along the right edge
     */

    for (tp = TR(tile); BL(tp) == tile; tp = LB(tp))
	SETBL(tp, newtile);
    SETTR(tile, newtile);

    /*
     * Adjust corner stitches along the top edge
     */

    for (tp = RT(tile); LEFT(tp) >= x; tp = BL(tp))
	SETLB(tp, newtile);
    SETRT(tile, tp);

    /*
     * Adjust corner stitches along the bottom edge
     */

    for (tp = LB(tile); RIGHT(tp) <= x; tp = TR(tp))
	/* nothing */;
    SETLB(newtile, tp);
    while (RT(tp) == tile)
    {
	SETRT(tp, newtile);
	tp = TR(tp);
    }

    return (newtile);
}

/*
 * --------------------------------------------------------------------
 *
 * TiSplitY --
 *
 * Given a tile and a Y coordinate, split the tile into two
 * along a horizontal line running through the given coordinate.
 *
 * Results:
 *	Returns the new tile resulting from the splitting, which
 *	is the tile occupying the top half of the original
 *	tile.
 *
 * Side effects:
 *	Modifies the corner stitches in the database to reflect
 *	the presence of two tiles in place of the original one.
 *
 * --------------------------------------------------------------------
 */

Tile *
TiSplitY(Tile *tile, int y)
    /* Tile to be split */
    /* Y coordinate of split */
{
    Tile *newtile;
    Tile *tp;

    //ASSERT(y > YMIN(tile) && y < YMAX(tile), "TiSplitY");

    newtile = TiAlloc();
	dupTileBody(tile, newtile);

    SETLEFT(newtile, LEFT(tile));
    SETYMIN(newtile, y);
    SETLB(newtile, tile);
    SETRT(newtile, RT(tile));
    SETTR(newtile, TR(tile));

    /*
     * Adjust corner stitches along top edge
     */

    for (tp = RT(tile); LB(tp) == tile; tp = BL(tp))
	SETLB(tp, newtile);
    SETRT(tile, newtile);

    /*
     * Adjust corner stitches along right edge
     */

    for (tp = TR(tile); YMIN(tp) >= y; tp = LB(tp))
	SETBL(tp, newtile);
    SETTR(tile, tp);

    /*
     * Adjust corner stitches along left edge
     */

    for (tp = BL(tile); YMAX(tp) <= y; tp = RT(tp))
	/* nothing */;
    SETBL(newtile, tp);
    while (TR(tp) == tile)
    {
	SETTR(tp, newtile);
	tp = RT(tp);
    }

    return (newtile);
}

/*
 * --------------------------------------------------------------------
 *
 * TiSplitX_Left --
 *
 * Given a tile and an X coordinate, split the tile into two
 * along a line running vertically through the given coordinate.
 * Intended for use when plowing to the left.
 *
 * Results:
 *	Returns the new tile resulting from the splitting, which
 *	is the tile occupying the left-hand half of the original
 *	tile.
 *
 * Side effects:
 *	Modifies the corner stitches in the database to reflect
 *	the presence of two tiles in place of the original one.
 *
 * --------------------------------------------------------------------
 */

Tile *
TiSplitX_Left(Tile *tile, int x)
    /* Tile to be split */
    /* X coordinate of split */
{
    Tile *newtile;
    Tile *tp;

    //ASSERT(x > LEFT(tile) && x < RIGHT(tile), "TiSplitX");

    newtile = TiAlloc();
	dupTileBody(tile, newtile);

    SETLEFT(newtile, LEFT(tile));
    SETLEFT(tile, x);
    SETYMIN(newtile, YMIN(tile));

    SETBL(newtile, BL(tile));
    SETLB(newtile, LB(tile));
    SETTR(newtile, tile);
    SETBL(tile, newtile);

    /* Adjust corner stitches along the left edge */
    for (tp = BL(newtile); TR(tp) == tile; tp = RT(tp))
	SETTR(tp, newtile);

    /* Adjust corner stitches along the top edge */
    for (tp = RT(tile); LEFT(tp) >= x; tp = BL(tp))
	/* nothing */;
    SETRT(newtile, tp);
    for ( ; LB(tp) == tile; tp = BL(tp))
	SETLB(tp, newtile);

    /* Adjust corner stitches along the bottom edge */
    for (tp = LB(tile); RIGHT(tp) <= x; tp = TR(tp))
	SETRT(tp, newtile);
    SETLB(tile, tp);

    return (newtile);
}

/*
 * --------------------------------------------------------------------
 *
 * TiSplitY_Bottom --
 *
 * Given a tile and a Y coordinate, split the tile into two
 * along a horizontal line running through the given coordinate.
 * Used when plowing down.
 *
 * Results:
 *	Returns the new tile resulting from the splitting, which
 *	is the tile occupying the bottom half of the original
 *	tile.
 *
 * Side effects:
 *	Modifies the corner stitches in the database to reflect
 *	the presence of two tiles in place of the original one.
 *
 * --------------------------------------------------------------------
 */

Tile *
TiSplitY_Bottom(Tile *tile, int y)
    /* Tile to be split */
    /* Y coordinate of split */
{
    Tile *newtile;
    Tile *tp;

    //ASSERT(y > YMIN(tile) && y < YMAX(tile), "TiSplitY");

    newtile = TiAlloc();
	dupTileBody(tile, newtile);

    SETLEFT(newtile, LEFT(tile));
    SETYMIN(newtile, YMIN(tile));
    SETYMIN(tile, y);

    SETRT(newtile, tile);
    SETLB(newtile, LB(tile));
    SETBL(newtile, BL(tile));
    SETLB(tile, newtile);

    /* Adjust corner stitches along bottom edge */
    for (tp = LB(newtile); RT(tp) == tile; tp = TR(tp))
	SETRT(tp, newtile);

    /* Adjust corner stitches along right edge */
    for (tp = TR(tile); YMIN(tp) >= y; tp = LB(tp))
	/* nothing */;
    SETTR(newtile, tp);
    for ( ; BL(tp) == tile; tp = LB(tp))
	SETBL(tp, newtile);

    /* Adjust corner stitches along left edge */
    for (tp = BL(tile); YMAX(tp) <= y; tp = RT(tp))
	SETTR(tp, newtile);
    SETBL(tile, tp);

    return (newtile);
}

/*
 * --------------------------------------------------------------------
 *
 * TiJoinX --
 *
 * Given two tiles sharing an entire common vertical edge, replace
 * them with a single tile occupying the union of their areas.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The first tile is simply relinked to reflect its new size.
 *	The second tile is deallocated.  Corner stitches in the
 *	neighboring tiles are updated to reflect the new structure.
 *	If the hint tile pointer in the supplied plane pointed to
 *	the second tile, it is adjusted to point instead to the
 *	first.
 *
 * --------------------------------------------------------------------
 */

void
TiJoinX(Tile *tile1, Tile *tile2, Plane *plane)
    /* First tile, remains allocated after call */
    /* Second tile, deallocated by call */
    /* Plane in which hint tile is updated */
{
	Tile *tp;

    /*
     * Basic algorithm:
     *
     *	Update all the corner stitches in the neighbors of tile2
     *	to point to tile1.
     *	Update the corner stitches of tile1 along the shared edge
     *	to be those of tile2.
     *	Change the bottom or left coordinate of tile1 if appropriate.
     *	Deallocate tile2.
     */

    //ASSERT(YMIN(tile1)==YMIN(tile2) && YMAX(tile1)==YMAX(tile2), "TiJoinX");
    //ASSERT(LEFT(tile1)==RIGHT(tile2) || RIGHT(tile1)==LEFT(tile2), "TiJoinX");

    /*
     * Update stitches along top of tile
     */

    for (tp = RT(tile2); LB(tp) == tile2; tp = BL(tp))
	SETLB(tp, tile1);

    /*
     * Update stitches along bottom of tile
     */

    for (tp = LB(tile2); RT(tp) == tile2; tp = TR(tp))
	SETRT(tp, tile1);

    /*
     * Update stitches along either left or right, depending
     * on relative position of the two tiles.
     */

    //ASSERT(LEFT(tile1) != LEFT(tile2), "TiJoinX");
    if (LEFT(tile1) < LEFT(tile2))
    {
	for (tp = TR(tile2); BL(tp) == tile2; tp = LB(tp))
	    SETBL(tp, tile1);
	SETTR(tile1, TR(tile2));
	SETRT(tile1, RT(tile2));
    }
    else
    {
	for (tp = BL(tile2); TR(tp) == tile2; tp = RT(tp))
	    SETTR(tp, tile1);
	SETBL(tile1, BL(tile2));
	SETLB(tile1, LB(tile2));
	SETLEFT(tile1, LEFT(tile2));
    }

    if (plane->pl_hint == tile2)
	plane->pl_hint = tile1;
    TiFree(tile2);
}

/*
 * --------------------------------------------------------------------
 *
 * TiJoinY --
 *
 * Given two tiles sharing an entire common horizontal edge, replace
 * them with a single tile occupying the union of their areas.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The first tile is simply relinked to reflect its new size.
 *	The second tile is deallocated.  Corner stitches in the
 *	neighboring tiles are updated to reflect the new structure.
 *	If the hint tile pointer in the supplied plane pointed to
 *	the second tile, it is adjusted to point instead to the
 *	first.
 *
 * --------------------------------------------------------------------
 */

void
TiJoinY(Tile *tile1, Tile *tile2, Plane *plane)
    /* First tile, remains allocated after call */
    /* Second tile, deallocated by call */
    /* Plane in which hint tile is updated */
{
	if (tile1 == tile2) {
		return;
	}

    Tile *tp;

    /*
     * Basic algorithm:
     *
     *	Update all the corner stitches in the neighbors of tile2
     *	to point to tile1.
     *	Update the corner stitches of tile1 along the shared edge
     *	to be those of tile2.
     *	Change the bottom or left coordinate of tile1 if appropriate.
     *	Deallocate tile2.
     */

    //ASSERT(LEFT(tile1)==LEFT(tile2) && RIGHT(tile1)==RIGHT(tile2), "TiJoinY");
    //ASSERT(YMAX(tile1)==YMIN(tile2) || YMIN(tile1)==YMAX(tile2), "TiJoinY");

    /*
     * Update stitches along right of tile.
     */

    for (tp = TR(tile2); BL(tp) == tile2; tp = LB(tp))
	SETBL(tp, tile1);

    /*
     * Update stitches along left of tile.
     */

    for (tp = BL(tile2); TR(tp) == tile2; tp = RT(tp))
	SETTR(tp, tile1);

    /*
     * Update stitches along either top or bottom, depending
     * on relative position of the two tiles.
     */

    //ASSERT(YMIN(tile1) != YMIN(tile2), "TiJoinY");
    if (YMIN(tile1) < YMIN(tile2))
    {
		for (tp = RT(tile2); LB(tp) == tile2; tp = BL(tp)) 
			SETLB(tp, tile1);

	SETRT(tile1, RT(tile2));
	SETTR(tile1, TR(tile2));
    }
    else
    {
		for (tp = LB(tile2); RT(tp) == tile2; tp = TR(tp)) 
			SETRT(tp, tile1);
	SETLB(tile1, LB(tile2));
	SETBL(tile1, BL(tile2));
	SETYMIN(tile1, YMIN(tile2));
    }

    if (plane->pl_hint == tile2)
	plane->pl_hint = tile1;
    TiFree(tile2);
}


/*
 * --------------------------------------------------------------------
 *
 * TiAlloc ---
 *
 *	Memory allocation for tiles
 *
 * Results:
 *	Pointer to an initialized memory location for a tile.
 *
 * --------------------------------------------------------------------
 */

Tile *
TiAlloc()
{
    Tile *newtile;

    newtile = new Tile;
    TiSetClient(newtile, 0);
    TiSetBody(newtile, 0);
	TiSetType(newtile, Tile::NOTYPE);

	//qDebug() << "alloc" << (long) newtile;

	SETLB(newtile, 0); 
	SETBL(newtile, 0);
	SETRT(newtile, 0); 
	SETTR(newtile, 0);

    return (newtile);
}

/*
 * --------------------------------------------------------------------
 *
 * TiFree ---
 *
 *	Release memory allocation for tiles
 *
 * Results:
 *	None.
 *
 * --------------------------------------------------------------------
 */

void
TiFree(Tile *tp)
{
    delete tp;
}

Tile* gotoPoint(Tile * tp, TilePoint p) 
{ 
	if (p.yi < YMIN(tp)) 
	    do tp = LB(tp); while (p.yi < YMIN(tp)); 
	else 
	    while (p.yi >= YMAX(tp)) tp = RT(tp); 
	if (p.xi < LEFT(tp)) 
	    do  
	    { 
		do tp = BL(tp); while (p.xi < LEFT(tp)); 
		if (p.yi < YMAX(tp)) break; 
		do tp = RT(tp); while (p.yi >= YMAX(tp)); 
	    } 
	    while (p.xi < LEFT(tp)); 
	else 
	    while (p.xi >= RIGHT(tp)) 
	    { 
		do tp = TR(tp); while (p.xi >= RIGHT(tp)); 
		if (p.yi >= YMIN(tp)) break; 
		do tp = LB(tp); while (p.yi < YMIN(tp)); 
	    } 

	return tp;
}

/*
 * ----------------------------------------------------------------------------
 * dupTileBody -- 
 *
 * Duplicate the body of an old tile as the body for a new tile.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Allocates new CellTileBodies unless the old tile was a space tile.
 * ----------------------------------------------------------------------------
 */

void
dupTileBody (Tile * oldtp, Tile * newtp)
{
	TiSetBody(newtp, TiGetBody(oldtp));
	TiSetType(newtp, TiGetType(oldtp));
}
