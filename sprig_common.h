/*
This header file is used internally by Sprig when it is compiled.  You do not
need to copy this to your compiler's 'include' folder.
*/


#ifndef _SPRIG_INTERNAL_H__
#define _SPRIG_INTERNAL_H__

extern SPG_bool spg_autolock;


#define SWAP(x,y,temp) temp=x;x=y;y=temp
#define MIN(x,y) (x < y? x : y)
#define MAX(x,y) (x > y? x : y)

/* Lock the surface, returning negative on error */
static inline int spg_lock(SDL_Surface* surface)
{
	if(spg_autolock && SDL_MUSTLOCK(surface) && SDL_LockSurface(surface) < 0)
        return -1;
	return 0;
}

/* Unlock the surface */
static inline void spg_unlock(SDL_Surface* surface)
{
    if(spg_autolock && SDL_MUSTLOCK(surface))
        SDL_UnlockSurface(surface);
}


#endif
