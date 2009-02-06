/*
    SPriG - SDL Primitive Generator
    by Jonathan Dearborn

    Based on SGE: SDL Graphics Extension r030809
    by Anders Lindström
*/


/*********************************************************************
 *  This library is free software; you can redistribute it and/or    *
 *  modify it under the terms of the GNU Library General Public      *
 *  License as published by the Free Software Foundation; either     *
 *  version 2 of the License, or (at your option) any later version. *
 *********************************************************************/


#include "sprig.h"

// Degrees here for nicer-looking tests
SPG_bool spg_usedegrees = 1;

void SPG_EnableRadians(SPG_bool enable)
{
    spg_usedegrees = !enable;
}

SPG_bool SPG_GetRadians()
{
    return !spg_usedegrees;
}


/*
Dirty rect handling
Adapted from Fixed Rate Pig by David Olofson
*/

SPG_bool spg_makedirtyrects = 0;

// These two variables are exposed so that changes in the implementation 
// don't officially break compatibility, but control is still there.

/* Approximate worth of one dirtyrect in pixels. */
int spg_dirty_worst_merge = 300;

/*
 * If the merged result gets at most this many percent
 * bigger than the larger of the two input rects,
 * accept it as Perfect.
 */
int spg_dirty_instant_merge = 10;

SPG_DirtyTable* spg_dirtytable_front = NULL;
SPG_DirtyTable* spg_dirtytable_back = NULL;

SDL_Surface* SPG_InitSDL(Uint16 w, Uint16 h, Uint8 bitsperpixel, Uint32 systemFlags, Uint32 screenFlags)
{

    if ( SDL_Init(systemFlags) < 0 )
    {
        printf("Couldn't initialize SDL: %s\n", SDL_GetError());
        return NULL;
    }

    SDL_Surface* screen = SDL_SetVideoMode(w, h, bitsperpixel, screenFlags);

    if ( screen == NULL )
    {
        printf("Couldn't set video mode %dx%d: %s\n", w, h, SDL_GetError());
        return NULL;
    }

    return screen;
}

// Reset the tables to the new size
void SPG_DirtyInit(Uint16 maxsize)
{
    if(spg_dirtytable_front != NULL)
        SPG_DirtyFree(spg_dirtytable_front);
    if(spg_dirtytable_back != NULL)
        SPG_DirtyFree(spg_dirtytable_back);
    if(maxsize > 0)
    {
        // This could be protected better from out-of-memory errors.
        spg_dirtytable_front = SPG_DirtyMake(maxsize);
        spg_dirtytable_back = SPG_DirtyMake(maxsize);
    }
    else
    {
        spg_dirtytable_front = NULL;
        spg_dirtytable_back = NULL;
    }
}

void SPG_EnableDirty(SPG_bool enable)
{
    spg_makedirtyrects = enable;
}

SPG_bool SPG_DirtyEnabled()
{
    return spg_makedirtyrects;
}

void SPG_DirtyClip(SDL_Surface* screen, SDL_Rect* rect)
{
    if(screen == NULL || rect == NULL)
        return;
        
    if(rect->x < 0)
    {
        if(rect->w + rect->x > 0)
            rect->w += rect->x;
        else
            rect->w = 0;
        rect->x = 0;
    }
    if(rect->x >= screen->w)
    {
        rect->x = 0;
        rect->w = 0;
    }
    else if(rect->x + rect->w >= screen->w)
        rect->w = screen->w - rect->x;
    
    if(rect->y < 0)
    {
        if(rect->h + rect->y > 0)
            rect->h += rect->y;
        else
            rect->h = 0;
        rect->y = 0;
    }
    else if(rect->y >= screen->h)
    {
        rect->y = 0;
        rect->h = 0;
    }
    else if(rect->y + rect->h >= screen->h)
        rect->h = screen->h - rect->y;
}

void SPG_DirtyLevel(Uint16 optimizationLevel)
{
    if(optimizationLevel > 0)
        spg_dirty_worst_merge = optimizationLevel;
}

// Unsafe for general use.  Use SPG_RectOR
void SPG_RectUnion(SDL_Rect *from, SDL_Rect *to)
{
	int x1 = from->x;
	int y1 = from->y;
	int x2 = from->x + from->w;
	int y2 = from->y + from->h;
	if(to->x < x1)
		x1 = to->x;
	if(to->y < y1)
		y1 = to->y;
	if(to->x + to->w > x2)
		x2 = to->x + to->w;
	if(to->y + to->h > y2)
		y2 = to->y + to->h;
	to->x = x1;
	to->y = y1;
	to->w = x2 - x1;
	to->h = y2 - y1;
}

// Unsafe for general use.  Use SPG_RectAND
void SPG_RectIntersect(SDL_Rect *from, SDL_Rect *to)
{
	int Amin, Amax, Bmin, Bmax;
	Amin = to->x;
	Amax = Amin + to->w;
	Bmin = from->x;
	Bmax = Bmin + from->w;
	if(Bmin > Amin)
		Amin = Bmin;
	to->x = Amin;
	if(Bmax < Amax)
		Amax = Bmax;
	to->w = Amax - Amin > 0 ? Amax - Amin : 0;

	Amin = to->y;
	Amax = Amin + to->h;
	Bmin = from->y;
	Bmax = Bmin + from->h;
	if(Bmin > Amin)
		Amin = Bmin;
	to->y = Amin;
	if(Bmax < Amax)
		Amax = Bmax;
	to->h = Amax - Amin > 0 ? Amax - Amin : 0;
}

void SPG_DirtyAdd(SDL_Rect* rect)
{
    SPG_DirtyAddTo(spg_dirtytable_front, rect);
}

void SPG_DirtyAddTo(SPG_DirtyTable* table, SDL_Rect* rect)
{
    int i, j, best_i, best_loss;
    
    if(table == NULL)
        return;
    if(rect->w == 0 || rect->h == 0)
        return;
	/*
	 * Look for merger candidates.
	 *
	 * We start right before the best match we
	 * had the last time around. This can give
	 * us large numbers of direct or quick hits
	 * when dealing with old/new rects for moving
	 * objects and the like.
	 */
	best_i = -1;
	best_loss = 100000000;
	if(table->count)
		i = (table->best + table->count - 1) % table->count;
	for(j = 0; j < table->count; ++j)
	{
		int a1, a2, am, ratio, loss;
		SDL_Rect testr;

		a1 = rect->w * rect->h;

		testr = table->rects[i];
		a2 = testr.w * testr.h;

		SPG_RectUnion(rect, &testr);
		am = testr.w * testr.h;

        //printf("a1,a2,am: %d, %d, %d", a1, a2, am);
        
		/* Perfect or Instant Pick? */
		ratio = 100 * am / (a1 > a2 ? a1 : a2);
		if(ratio < spg_dirty_instant_merge)
		{
			/* Ok, this is good enough! Stop searching. */
			SPG_RectUnion(rect, &table->rects[i]);
			table->best = i;
			return;
		}

		loss = am - a1 - a2;
		if(loss < best_loss)
		{
			best_i = i;
			best_loss = loss;
			table->best = i;
		}

		++i;
		i %= table->count;
	}
	/* ...and if the best result is good enough, merge! */
	if((best_i >= 0) && (best_loss < spg_dirty_worst_merge))
	{
		SPG_RectUnion(rect, &table->rects[best_i]);
		return;
	}

	/* Try to add to table... */
	if(table->count < table->size)
	{
		table->rects[table->count++] = *rect;
		return;
	}

	/* Emergency: Table full! Grab best candidate... */
	SPG_RectUnion(rect, &table->rects[best_i]);
}

SPG_DirtyTable* SPG_DirtyGet()
{
    return spg_dirtytable_front;
}

void SPG_DirtyClear(SPG_DirtyTable* table)
{
    if(table == NULL)
        return;
    table->count = 0;
    table->best = 0;
}

void SPG_DirtyFree(SPG_DirtyTable* table)
{
    if(table == NULL)
        return;
    free(table->rects);
    free(table);
}

SPG_DirtyTable* SPG_DirtyMake(Uint16 maxsize)
{
    SPG_DirtyTable* table = (SPG_DirtyTable *)malloc(sizeof(SPG_DirtyTable));
    if(table == NULL)
        return NULL;

    table->size = maxsize;
    table->rects = (SDL_Rect *)calloc(maxsize, sizeof(SDL_Rect));
    if(table->rects == NULL)
    {
        free(table);
        return NULL;
    }

    table->count = 0;
    table->best = 0;
    return table;
}

SPG_DirtyTable* SPG_DirtyUpdate(SDL_Surface* screen)
{
    if(spg_dirtytable_front != NULL && spg_dirtytable_back != NULL)
    {
        // Add all of the front table's rects to the back table.
        int i;
        for(i = 0; i < spg_dirtytable_front->count; ++i)
            SPG_DirtyAddTo(spg_dirtytable_back, spg_dirtytable_front->rects + i);  // pointer arithmetic
        
        // Use back table for updating the screen
        SDL_UpdateRects(screen, spg_dirtytable_back->count, spg_dirtytable_back->rects);
        // User should now use this table to replace backgrounds.
        // For that, we'll return the back table to the user.
        // He'd better not free this table!  I'll be mad!
        return spg_dirtytable_back;
    }
    return NULL;  // User can test and choose to update entire screen or whatever
}

void SPG_DirtySwap()
{
    if(spg_dirtytable_front != NULL && spg_dirtytable_back != NULL)
    {
        // Clear back table
        spg_dirtytable_back->count = 0;
        spg_dirtytable_back->best = 0;
        // Swap tables (clean one up front, old one in back; The old list 
        //              will be merged so we can update where stuff used to be)
        SPG_DirtyTable* temp = spg_dirtytable_front;
        spg_dirtytable_front = spg_dirtytable_back;
        spg_dirtytable_back = temp;
    }
}
