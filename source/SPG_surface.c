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


/*
*  Some of this code is taken from the "Introduction to SDL" and
*  John Garrison's PowerPak	
*/

#include "sprig.h"
#include "sprig_common.h"

#include <math.h>
#include <string.h>
#include <stdarg.h>


/* Global used for SPG_EnableAutolock */
SPG_bool spg_autolock = 1;
void (*spg_blitfunc)(SDL_Surface*, SDL_Rect*, SDL_Surface*, SDL_Rect*) = NULL;


struct spg_uint8_node
{
    Uint8 datum;
    struct spg_uint8_node* next;
};

struct spg_uint16_node
{
    Uint16 datum;
    struct spg_uint16_node* next;
};

struct spg_bool_node
{
    SPG_bool datum;
    struct spg_bool_node* next;
};

struct spg_string_node
{
    char* datum;
    struct spg_string_node* next;
};


struct spg_uint16_node* spg_thickness_state = NULL;
extern Uint16 spg_thickness;
struct spg_uint8_node* spg_blend_state = NULL;
struct spg_bool_node* spg_aa_state = NULL;
struct spg_bool_node* spg_blit_surface_alpha_state = NULL;

struct spg_string_node* _spg_errors = NULL;
struct spg_string_node* _spg_errors_tail = NULL;
Uint16 _spg_numerrors = 0;
SPG_bool spg_useerrors = 0;
extern SPG_bool spg_makedirtyrects;
extern SPG_DirtyTable* spg_dirtytable_front;


void spg_pixel(SDL_Surface *surface, Sint16 x, Sint16 y, Uint32 color);
void spg_pixelblend(SDL_Surface *surface, Sint16 x, Sint16 y, Uint32 color, Uint8 alpha);

void spg_pixel8(SDL_Surface *surface, Sint16 x, Sint16 y, Uint32 color);
void spg_pixel16(SDL_Surface *surface, Sint16 x, Sint16 y, Uint32 color);
void spg_pixel24(SDL_Surface *surface, Sint16 x, Sint16 y, Uint32 color);
void spg_pixel32(SDL_Surface *surface, Sint16 x, Sint16 y, Uint32 color);
void spg_pixelX(SDL_Surface *dest,Sint16 x,Sint16 y,Uint32 color);


/**********************************************************************************/
/**                            Misc. functions                                   **/
/**********************************************************************************/



void SPG_EnableAutolock(SPG_bool enable)
{ spg_autolock = enable;}

SPG_bool SPG_GetAutolock(void)
{ return spg_autolock;}


void SPG_PushThickness(Uint16 state)
{
    struct spg_uint16_node* node = (struct spg_uint16_node*)malloc(sizeof(struct spg_uint16_node));
    node->datum = state;
    node->next = spg_thickness_state;
    spg_thickness_state = node;
    spg_thickness = state;
}
Uint16 SPG_PopThickness(void)
{
    if(spg_thickness_state == NULL)
    {
        if(spg_useerrors)
            SPG_Error("SPG_PopThickness popped an empty stack!");
        spg_thickness = 1;
        return 1;
    }
    struct spg_uint16_node* temp = spg_thickness_state;
    spg_thickness_state = spg_thickness_state->next;
	if(spg_thickness_state == NULL)
		spg_thickness = 1;
	else
		spg_thickness = spg_thickness_state->datum;
    free(temp);
    return spg_thickness;
}
Uint16 SPG_GetThickness(void)
{
    return spg_thickness;
}

void SPG_PushBlend(Uint8 state)
{
    struct spg_uint8_node* node = (struct spg_uint8_node*)malloc(sizeof(struct spg_uint8_node));
    node->datum = state;
    node->next = spg_blend_state;
    spg_blend_state = node;
}
Uint8 SPG_PopBlend(void)
{
    if(spg_blend_state == NULL)
    {
        if(spg_useerrors)
            SPG_Error("SPG_PopBlend popped an empty stack!");
        return 0;
    }
    struct spg_uint8_node* temp = spg_blend_state;
    Uint8 result = spg_blend_state->datum;
    spg_blend_state = spg_blend_state->next;
    free(temp);
    return result;
}
Uint8 SPG_GetBlend(void)
{
    if(spg_blend_state == NULL)
    {
        // Without initialization, this overfills the stack.
        //if(spg_useerrors)
        //SPG_Error("SPG_GetBlend checked an empty stack!");
        return 0;
    }
    return spg_blend_state->datum;
}

void SPG_PushAA(SPG_bool state)
{
    struct spg_bool_node* node = (struct spg_bool_node*)malloc(sizeof(struct spg_bool_node));
    node->datum = state;
    node->next = spg_aa_state;
    spg_aa_state = node;
}
SPG_bool SPG_PopAA(void)
{
    if(spg_aa_state == NULL)
    {
        if(spg_useerrors)
            SPG_Error("SPG_PopAA popped an empty stack!");
        return 0;
    }
    struct spg_bool_node* temp = spg_aa_state;
    SPG_bool result = spg_aa_state->datum;
    spg_aa_state = spg_aa_state->next;
    free(temp);
    return result;
}
SPG_bool SPG_GetAA(void)
{
    if(spg_aa_state == NULL)
    {
        // Without initialization, this overfills the stack.
        //if(spg_useerrors)
        //SPG_Error("SPG_GetAA checked an empty stack!");
        return 0;
    }
    return spg_aa_state->datum;
}

void SPG_PushSurfaceAlpha(SPG_bool state)
{
    struct spg_bool_node* node = (struct spg_bool_node*)malloc(sizeof(struct spg_bool_node));
    node->datum = state;
    node->next = spg_blit_surface_alpha_state;
    spg_blit_surface_alpha_state = node;
}
SPG_bool SPG_PopSurfaceAlpha(void)
{
    if(spg_blit_surface_alpha_state == NULL)
    {
        if(spg_useerrors)
            SPG_Error("SPG_PopSurfaceAlpha popped an empty stack!");
        return 0;
    }
    struct spg_bool_node* temp = spg_blit_surface_alpha_state;
    SPG_bool result = spg_blit_surface_alpha_state->datum;
    spg_blit_surface_alpha_state = spg_blit_surface_alpha_state->next;
    free(temp);
    return result;
}
SPG_bool SPG_GetSurfaceAlpha(void)
{
    if(spg_blit_surface_alpha_state == NULL)
    {
        // Without initialization, this overfills the stack.
        //if(spg_useerrors)
        //SPG_Error("SPG_GetSurfaceAlpha checked an empty stack!");
        return 0;
    }
    return spg_blit_surface_alpha_state->datum;
}


void SPG_Error(const char* err)
{
    if(err == NULL || _spg_numerrors >= SPG_MAX_ERRORS)
        return;
    struct spg_string_node* node = (struct spg_string_node*)malloc(sizeof(struct spg_string_node));
    node->datum = (char*)malloc(strlen(err)+20);
    sprintf(node->datum, "%s at time %ums", err, SDL_GetTicks());
    // push to front
    //node->next = _spg_errors;
    //_spg_errors = node;
    
    // push to back
    node->next = NULL;
    if(_spg_errors == NULL || _spg_errors_tail == NULL)
        _spg_errors = node;
    else
        _spg_errors_tail->next = node;
    _spg_errors_tail = node;
    
    _spg_numerrors++;
}

char* SPG_GetError(void)
{
    if(_spg_numerrors == 0 || _spg_errors == NULL)
        return NULL;
    
    struct spg_string_node* temp = _spg_errors;
    char* result = _spg_errors->datum;
    _spg_errors = _spg_errors->next;
    _spg_numerrors--;
    free(temp);
    return result;
}

Uint16 SPG_NumErrors(void)
{
    return _spg_numerrors;
}

void SPG_EnableErrors(SPG_bool enable)
{
    spg_useerrors = enable;
}



/*
*  Get the smallest bounding box from two (SDL_Rect) rectangles
*/
void SPG_RectOR(const SDL_Rect rect1, const SDL_Rect rect2, SDL_Rect* dst_rect)
{
    if(dst_rect == NULL)
        return;
    
	dst_rect->x = (rect1.x < rect2.x)? rect1.x : rect2.x;
	dst_rect->y = (rect1.y < rect2.y)? rect1.y : rect2.y;
	dst_rect->w = (rect1.x + rect1.w > rect2.x + rect2.w)? rect1.x + rect1.w - dst_rect->x : rect2.x + rect2.w - dst_rect->x;
	dst_rect->h = (rect1.y + rect1.h > rect2.y + rect2.h)? rect1.y + rect1.h - dst_rect->y : rect2.y + rect2.h - dst_rect->y;
}

// Adapted from SDL_IntersectRect
SPG_bool SPG_RectAND(const SDL_Rect A, const SDL_Rect B, SDL_Rect* intersection)
{
    int resX, resY, resW, resH;
	int Amin, Amax, Bmin, Bmax;

	// Horizontal intersection
	Amin = A.x;
	Amax = Amin + A.w;
	Bmin = B.x;
	Bmax = Bmin + B.w;
	if(Bmin > Amin)
	        Amin = Bmin;
	resX = Amin;
	if(Bmax < Amax)
	        Amax = Bmax;
	resW = Amax - Amin > 0 ? Amax - Amin : 0;

	// Vertical intersection
	Amin = A.y;
	Amax = Amin + A.h;
	Bmin = B.y;
	Bmax = Bmin + B.h;
	if(Bmin > Amin)
	        Amin = Bmin;
	resY = Amin;
	if(Bmax < Amax)
	        Amax = Bmax;
	resH = Amax - Amin > 0 ? Amax - Amin : 0;
    
    if(intersection != NULL)
    {
        intersection->x = resX;
        intersection->y = resY;
        intersection->w = resW;
        intersection->h = resH;
    }
    
	return (resW && resH);
}


// Adapted from SDL_SetClipRect
void SPG_SetClip(SDL_Surface *surface, const SDL_Rect rect)
{
	//SDL_Rect full_rect;
	int Amin, Amax, Bmax;
	
	if (!surface)
		return;
	
	// Set the clipping rectangle with the intersection of 
	//   the given rect with the full surface.
	// Horizontal intersection
	Amin = rect.x;
	Amax = Amin + rect.w;
	Bmax = surface->w;
	if(Amin < 0)
	        Amin = 0;
	surface->clip_rect.x = Amin;
	if(Bmax < Amax)
	        Amax = Bmax;
	surface->clip_rect.w = Amax - Amin > 0 ? Amax - Amin : 0;

	// Vertical intersection
	Amin = rect.y;
	Amax = Amin + rect.h;
	Bmax = surface->h;
	if(Amin < 0)
	        Amin = 0;
	surface->clip_rect.y = Amin;
	if(Bmax < Amax)
	        Amax = Bmax;
	surface->clip_rect.h = Amax - Amin > 0 ? Amax - Amin : 0;
}













//==================================================================================
// Calculate y pitch offset
// (the y pitch offset is constant for the same y coord and surface)
//==================================================================================
Sint32 SPG_CalcYPitch(SDL_Surface *dest,Sint16 y)
{
	if(y>=SPG_CLIP_YMIN(dest) && y<=SPG_CLIP_YMAX(dest)){
		switch ( dest->format->BytesPerPixel ) {
		case 1:
			return y*dest->pitch;
			break;
		case 2:
			return y*dest->pitch/2;
			break;
		case 3:
			return y*dest->pitch;
			break;
		case 4:
			return y*dest->pitch/4;
			break;
		}
	}
	
	return -1;
}


//==================================================================================
// Set pixel with precalculated y pitch offset
//==================================================================================
void SPG_pSetPixel(SDL_Surface *surface, Sint16 x, Sint32 ypitch, Uint32 color)
{
	if(x>=SPG_CLIP_XMIN(surface) && x<=SPG_CLIP_XMAX(surface) && ypitch>=0){
		switch (surface->format->BytesPerPixel) {
			case 1: { /* Assuming 8-bpp */
				*((Uint8 *)surface->pixels + ypitch + x) = color;
			}
			break;

			case 2: { /* Probably 15-bpp or 16-bpp */
				*((Uint16 *)surface->pixels + ypitch + x) = color;
			}
			break;

			case 3: { /* Slow 24-bpp mode, usually not used */
  				/* Gack - slow, but endian correct */
  				Uint8 *pix = (Uint8 *)surface->pixels + ypitch + x*3;

				*(pix+surface->format->Rshift/8) = color>>surface->format->Rshift;
  				*(pix+surface->format->Gshift/8) = color>>surface->format->Gshift;
  				*(pix+surface->format->Bshift/8) = color>>surface->format->Bshift;
				*(pix+surface->format->Ashift/8) = color>>surface->format->Ashift;
			}
			break;

			case 4: { /* Probably 32-bpp */
				*((Uint32 *)surface->pixels + ypitch + x) = color;
			}
			break;
		}
	}
}


//==================================================================================
// Get pixel
//==================================================================================
Uint32 SPG_GetPixel(SDL_Surface *surface, Sint16 x, Sint16 y)
{
	if(x<0 || x>=surface->w || y<0 || y>=surface->h)
	{
	    if(spg_useerrors)
            SPG_Error("SPG_GetPixel was used out of the surface bounds");
		return 0;
	}

	switch (surface->format->BytesPerPixel) {
		case 1: { /* Assuming 8-bpp */
			return *((Uint8 *)surface->pixels + y*surface->pitch + x);
		}
		break;

		case 2: { /* Probably 15-bpp or 16-bpp */
			return *((Uint16 *)surface->pixels + y*surface->pitch/2 + x);
		}
		break;

		case 3: { /* Slow 24-bpp mode, usually not used */
			Uint8 *pix;
			int shift;
			Uint32 color=0;

			pix = (Uint8 *)surface->pixels + y * surface->pitch + x*3;
			shift = surface->format->Rshift;
			color = *(pix+shift/8)<<shift;
			shift = surface->format->Gshift;
			color|= *(pix+shift/8)<<shift;
			shift = surface->format->Bshift;
			color|= *(pix+shift/8)<<shift;
			shift = surface->format->Ashift;
			color|= *(pix+shift/8)<<shift;
			return color;
		}
		break;

		case 4: { /* Probably 32-bpp */
			return *((Uint32 *)surface->pixels + y*surface->pitch/4 + x);
		}
		break;
	}
	return 0;
}








/**********************************************************************************/
/**                            Block functions                                   **/
/**********************************************************************************/

//==================================================================================
// The SPG_write_block* functions copies the given block (a surface line) directly
// to the surface. This is *much* faster then using the put pixel functions to
// update a line. The block consist of Surface->w (the width of the surface) numbers
// of color values. Note the difference in byte size for the block elements for
// different color depths. 24 bpp is slow and not included!
//==================================================================================
// See sprig_inline.h






// SDL's clipping
SDL_Rect* SPG_BlitClip(SDL_Surface* source, SDL_Rect* srect, SDL_Surface* dest, SDL_Rect* drect)
{
    // Clip rects
    SDL_Rect fulldst;
	int srcx, srcy, w, h;


	/* If the destination rectangle is NULL, use the entire dest surface */
	if ( drect == NULL ) {
	        fulldst.x = fulldst.y = 0;
	        fulldst.w = dest->w;
	        fulldst.h = dest->h;
		drect = &fulldst;
	}

	/* clip the source rectangle to the source surface */
	if(srect) {
	        int maxw, maxh;
	
		srcx = srect->x;
		w = srect->w;
		if(srcx < 0) {
		        w += srcx;
			drect->x -= srcx;
			srcx = 0;
		}
		maxw = source->w - srcx;
		if(maxw < w)
			w = maxw;

		srcy = srect->y;
		h = srect->h;
		if(srcy < 0) {
		        h += srcy;
			drect->y -= srcy;
			srcy = 0;
		}
		maxh = source->h - srcy;
		if(maxh < h)
			h = maxh;
	    
	} else {
	        srcx = srcy = 0;
		w = source->w;
		h = source->h;
	}

	/* clip the destination rectangle against the clip rectangle */
	{
	        SDL_Rect *clip = &dest->clip_rect;
		int dx, dy;

		dx = clip->x - drect->x;
		if(dx > 0) {
			w -= dx;
			drect->x += dx;
			srcx += dx;
		}
		dx = drect->x + w - clip->x - clip->w;
		if(dx > 0)
			w -= dx;

		dy = clip->y - drect->y;
		if(dy > 0) {
			h -= dy;
			drect->y += dy;
			srcy += dy;
		}
		dy = drect->y + h - clip->y - clip->h;
		if(dy > 0)
			h -= dy;
	}

	if(w <= 0 || h <= 0)
		return NULL;
    
    
    SDL_Rect* result = (SDL_Rect*)malloc(sizeof(SDL_Rect));
    
    
    result->x = srcx;
    result->y = srcy;
    result->w = drect->w = w;
    result->h = drect->h = h;
    
    return result;
}

void SPG_BlendBlit(SDL_Surface* source, SDL_Rect* srect, SDL_Surface* dest, SDL_Rect* drect)
{
    int lowSX, highSX, lowSY, highSY;
    
    if(srect)
    {
        lowSX = srect->x;
        highSX = srect->x + srect->w;
        lowSY = srect->y;
        highSY = srect->y + srect->h;
    }
    else
    {
        lowSX = 0;
        highSX = dest->w;
        lowSY = 0;
        highSY = dest->h;
    }
    
    int lowDX, highDX, lowDY, highDY;
    
    if(drect)
    {
        lowDX = drect->x;
        highDX = drect->x + drect->w;
        lowDY = drect->y;
        highDY = drect->y + drect->h;
    }
    else
    {
        lowDX = 0;
        highDX = dest->w;
        lowDY = 0;
        highDY = dest->h;
    }
    
    

    // Get the per-surface alpha
    Uint8 perSAlpha = source->format->alpha;

    // Ready the recycling loop variables
    int sx = 0, sy = 0, dx = 0, dy = 0;
    Uint32 color;
    Uint8 r;
    Uint8 g;
    Uint8 b;
    Uint8 a;
    
    
    /*if ( spg_lock(surface) < 0 )
            return;
    if ( spg_lock(dest) < 0 )
            return;*/
            
    SPG_bool ncolorkeyed = !(source->flags & SDL_SRCCOLORKEY);
    Uint32 colorkey = source->format->colorkey;
    
    // Go through the rect we made
    for (sx = lowSX, sy = lowSY, dx = lowDX, dy = lowDY; sy < highSY;)
    {
        // Get the source color
        color = SPG_GetPixel(source, sx, sy);
        SDL_GetRGBA(color, source->format, &r, &g, &b, &a);
        
        // Combine per-surface and per-pixel alpha
        if(SPG_GetSurfaceAlpha())
            a = (Uint8)(a*(perSAlpha)/255.0);
        
        // Convert color to dest color
        color = SDL_MapRGB(dest->format, r, g, b);
        // If not a colorkeyed color, then draw the pixel (blending done in put pixel function)
        if(ncolorkeyed || color != colorkey)
            spg_pixelblend(dest, dx, dy, color, a);
        
        // Increment here so we can use the auto test on dy
        sx++;
        dx++;

        // Check sx bound to move on to the next horizontal line
        if (sx >= highSX)
        {
            sx = lowSX;
            sy++;
            dx = lowDX;
            dy++;
        }
    }
    
    /*spg_unlock(surface);
    spg_unlock(dest);*/
    if(spg_makedirtyrects)
    {
        SDL_Rect rect;
        rect.x = lowDX;
        rect.y = lowDY;
        rect.w = highDX - lowDX;
        rect.h = highDY - lowDY;
        // Clip it to the screen
        SPG_DirtyClip(dest, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
    }
}

// SDL's clipping
int SPG_Blit(SDL_Surface* source, SDL_Rect* srect, SDL_Surface* dest, SDL_Rect* drect)
{
	/* Make sure the surfaces aren't locked */
	if ( ! source || ! dest ) {
		SDL_SetError("SPG_Blit was passed a NULL surface");
		return -1;
	}
	if ( source->locked || dest->locked ) {
		SDL_SetError("SPG_Blit was passed a locked surface");
		return -1;
	}
    
    
    srect = SPG_BlitClip(source, srect, dest, drect);
    if(spg_blitfunc == NULL)
        spg_blitfunc = SPG_BlendBlit;
    spg_blitfunc(source, srect, dest, drect);
    
    
    free(srect);
    
    return 0;
}

void SPG_SetBlit(void (*blitfn)(SDL_Surface*, SDL_Rect*, SDL_Surface*, SDL_Rect*))
{
    spg_blitfunc = blitfn;
}

void (*SPG_GetBlit(void))(SDL_Surface*, SDL_Rect*, SDL_Surface*, SDL_Rect*)
{
    return spg_blitfunc;
}


// Returns a new surface that is a copy of the dest surface but with 
// the color value replaced by the values on the src surface.
SDL_Surface* SPG_ReplaceColor(SDL_Surface* src, SDL_Rect* srcrect, SDL_Surface* dest, SDL_Rect* destrect, Uint32 color)
{
    if(src == NULL || dest == NULL)
        return NULL;
    
    // Save per-surface alpha
    Uint32 srcAlpha = src->flags & SDL_SRCALPHA;
    Uint32 destAlpha = dest->flags & SDL_SRCALPHA;
    // ... and disable blending.
    SDL_SetAlpha(src, 0, src->format->alpha);
    SDL_SetAlpha(dest, 0, dest->format->alpha);
    
    // Save colorkey info
    Uint32 destCK = dest->format->colorkey;
    Uint32 destCKflag = dest->flags & SDL_SRCCOLORKEY;
    // ... and set the key to the desired color.
    SPG_SetColorkey(dest, color);
    
    // srcrect != NULL: Take piece of src and use it (looks like placing piece on top of dest)
    // destrect != NULL: Move src somewhere before blitting
    SDL_Surface* temp = SDL_CreateRGBSurface(SDL_SWSURFACE, dest->w, dest->h, dest->format->BitsPerPixel, 
            dest->format->Rmask, dest->format->Gmask, dest->format->Bmask, dest->format->Amask);
    
    SDL_BlitSurface(src, srcrect, temp, destrect);  // BG color is always BLACK and TRANSPARENT...
    SDL_BlitSurface(dest, NULL, temp, NULL);
    
    // Restore stuff
    SDL_SetColorKey(dest, destCKflag, destCK);
    SDL_SetAlpha(src, srcAlpha, src->format->alpha);
    SDL_SetAlpha(dest, destAlpha, dest->format->alpha);
    return temp;
}












/**********************************************************************************/
/**                            Palette functions                                 **/
/**********************************************************************************/

// 8-bit palettes

// Returns an 8-bit rainbow palette with 256 colors.
// Code adapted from SDL_DitherColors in SDL_pixels.c
SDL_Color* SPG_ColorPalette(void)
{
    SDL_Color* colors = (SDL_Color*)malloc(sizeof(SDL_Color)*256);
    int i;
    
    for(i = 0; i < 256; i++) {
		int r, g, b;
		// map each bit field to the full [0, 255] interval,
		   //so 0 is mapped to (0, 0, 0) and 255 to (255, 255, 255) 
		r = i & 0xe0;
		r |= r >> 3 | r >> 6;
		colors[i].r = r;
		g = (i << 3) & 0xe0;
		g |= g >> 3 | g >> 6;
		colors[i].g = g;
		b = i & 0x3;
		b |= b << 2;
		b |= b << 4;
		colors[i].b = b;
	}
	return colors;
}

// Returns an 8-bit black and white (grayscale) palette with 256 colors.
SDL_Color* SPG_GrayPalette(void)
{
    SDL_Color* colors = (SDL_Color*)malloc(sizeof(SDL_Color)*256);
    int i;
    
    for(i = 0; i < 256; i++) {
		colors[i].r = i;
		colors[i].g = i;
		colors[i].b = i;
	}
	return colors;
}

SDL_Surface* SPG_CreateSurface8(Uint32 flags, Uint16 width, Uint16 height)
{
    SDL_Surface* result = SDL_CreateRGBSurface(flags, width, height, 8, 0, 0, 0, 0);
	
	// Try to use existing palette...
    SDL_Surface* videoSurf = SDL_GetVideoSurface();
    if(videoSurf != NULL && videoSurf->format->BitsPerPixel == 8)
    {
        SDL_SetColors(result, videoSurf->format->palette->colors, 0, videoSurf->format->palette->ncolors);
    }
    else  // else create one
    {
        SDL_Color* p = SPG_ColorPalette();
        SDL_SetColors(result, p, 0, 256);
        free(p);
    }
    return result;
}

//returns the closest index into the palette for a given r,g,b color
// Adapted from Meetul Kinarivala, SDL mailing list, 2001
Uint32 SPG_FindPaletteColor(SDL_Palette* palette, Uint8 r, Uint8 g, Uint8 b)
{
    SDL_Color* colors = palette->colors;
    int Distance, MinDistance = 0xFFFFFF; //init to large value
    Uint32 index = 0;
    int i;
    
    //find index with minimum spatial distance to given r,g,b
    for (i = 0; i < palette->ncolors; ++i)
    {
        // d = r^2 + g^2 + b^2
        Distance = ((int)(colors[i].r)-(int)(r))*((int)(colors[i].r)-(int)(r))
            +((int)(colors[i].g)-(int)(g))*((int)(colors[i].g)-(int)(g))
            +((int)(colors[i].b)-(int)(b))*((int)(colors[i].b)-(int)(b));

        if (MinDistance > Distance)
        {
            MinDistance = Distance;
            index = i;
            if(MinDistance == 0)
                break; //color match !!
        }
    }

    return index;
}

//converts any surface -> 8 bit indexed, using shared palette
//quality optimized but slow
//returns NULL on error
// Adapted from Meetul Kinarivala, SDL mailing list, 2001
SDL_Surface* SPG_PalettizeSurface(SDL_Surface* surface, SDL_Palette* palette)
{
    if(surface == NULL || palette == NULL)
        return NULL;
        
    SDL_Surface* result = SDL_CreateRGBSurface(surface->flags, surface->w, surface->h, 8, 0, 0, 0, 0);

    if(result == NULL)
        return NULL;
    
    int x, y;
    Uint8 r, g, b;
    
    for (y=0; y < surface->h; ++y)
    {
        for (x=0; x < surface->w; ++x)
        {
            SDL_GetRGB(SPG_GetPixel(surface, x, y), surface->format, &r, &g, &b);
            SPG_Pixel(result, x, y, SPG_FindPaletteColor(palette, r, g, b));
        }
    }

    return result;
}


// 32-bit palettes

//==================================================================================
// Fades from (sR,sG,sB) to (dR,dG,dB), puts result in ctab[start] to ctab[stop]
//==================================================================================
void SPG_FadedPalette32(SDL_PixelFormat* format, Uint32 color1, Uint32 color2, Uint32* colorArray, Uint16 startIndex, Uint16 stopIndex)
{
    Uint8 sR, sG, sB, dR, dG, dB;
    SDL_GetRGB(color1, format, &sR, &sG, &sB);
    SDL_GetRGB(color2, format, &dR, &dG, &dB);
	// (sR,sG,sB) and (dR,dG,dB) are two points in space (the RGB cube). 	

	/* The vector for the straight line */
	int v[3];
	v[0]=(int)dR-(int)sR; v[1]=(int)dG-(int)sG; v[2]=(int)dB-(int)sB;

	/* Ref. point */
	int x0=sR, y0=sG, z0=sB;

	// The line's equation is:
	// x= x0 + v[0] * t
	// y= y0 + v[1] * t
	// z= z0 + v[2] * t
	//
	// (x,y,z) will travel between the two points when t goes from 0 to 1.

	int i = startIndex;
 	float step = 1.0f/((stopIndex+1)-startIndex);
    float t;
	for(t=0.0f; t<=1.0f && i<=stopIndex ; t+=step){
		colorArray[i++]=SDL_MapRGB(format, (Uint8)(x0+v[0]*t), (Uint8)(y0+v[1]*t), (Uint8)(z0+v[2]*t) );
	}			
}


//==================================================================================
// Fades from (sR,sG,sB,sA) to (dR,dG,dB,dA), puts result in ctab[start] to ctab[stop]
//==================================================================================
void SPG_FadedPalette32Alpha(SDL_PixelFormat* format, Uint32 color1, Uint8 alpha1, Uint32 color2, Uint8 alpha2, Uint32* colorArray, Uint16 startIndex, Uint16 stopIndex)
{
    Uint8 sR, sG, sB, dR, dG, dB;
    SDL_GetRGB(color1, format, &sR, &sG, &sB);
    SDL_GetRGB(color2, format, &dR, &dG, &dB);
	// (sR,sG,sB,sA) and (dR,dG,dB,dA) are two points in hyperspace (the RGBA hypercube). 	

	/* The vector for the straight line */
	int v[4];
	v[0]=(int)dR-(int)sR; v[1]=(int)dG-(int)sG; v[2]=(int)dB-(int)sB; v[3]=(int)alpha2-(int)alpha1;

	/* Ref. point */
	int x0=sR, y0=sG, z0=sB, w0=alpha1;

	// The line's equation is:
	// x= x0 + v[0] * t
	// y= y0 + v[1] * t
	// z= z0 + v[2] * t
	// w= w0 + v[3] * t
	//
	// (x,y,z,w) will travel between the two points when t goes from 0 to 1.

	int i = startIndex;
 	float step = 1.0f/((stopIndex+1)-startIndex);
    float t;
	for(t=0.0f; t<=1.0f && i<=stopIndex ; t+=step)
		colorArray[i++]=SDL_MapRGBA(format, (Uint8)(x0+v[0]*t), (Uint8)(y0+v[1]*t), (Uint8)(z0+v[2]*t), (Uint8)(w0+v[3]*t));
    
}


//==================================================================================
// Copies a nice rainbow palette to the color table (ctab[start] to ctab[stop]).
// You must also set the intensity of the palette (0-dark 255-bright)
//==================================================================================
void SPG_RainbowPalette32(SDL_PixelFormat* format, Uint32* colorArray, Uint8 intensity, Uint16 startIndex, Uint16 stopIndex)
{
    intensity = 255 - intensity;
	int slice=(int)((stopIndex-startIndex)/6.0f);
    Uint32 red = SDL_MapRGB(format, 255, intensity, intensity),
        yellow = SDL_MapRGB(format, 255, 255, intensity),
        green = SDL_MapRGB(format, intensity, 255, intensity),
        teal = SDL_MapRGB(format, intensity, 255, 255),
        blue = SDL_MapRGB(format, intensity, intensity, 255),
        purple = SDL_MapRGB(format, 255, intensity, 255);
    
	/* R-Y */
	SPG_FadedPalette32(format, red, yellow, colorArray, startIndex, startIndex+slice);
 	/* Y-G */
	SPG_FadedPalette32(format, yellow, green, colorArray, startIndex+slice+1, startIndex+2*slice);
 	/* G-T */
	SPG_FadedPalette32(format, green, teal, colorArray, startIndex+2*slice+1, startIndex+3*slice);
 	/* T-B */
	SPG_FadedPalette32(format, teal, blue, colorArray, startIndex+3*slice+1, startIndex+4*slice);
 	/* B-P */
	SPG_FadedPalette32(format, blue, purple, colorArray, startIndex+4*slice+1, startIndex+5*slice);	
 	/* P-R */
	SPG_FadedPalette32(format, purple, red, colorArray, startIndex+5*slice+1, stopIndex);
}


//==================================================================================
// Copies a B&W palette to the color table (ctab[start] to ctab[stop]).
//==================================================================================
void SPG_GrayPalette32(SDL_PixelFormat* format, Uint32* colorArray, Uint16 startIndex, Uint16 stopIndex)
{
	SPG_FadedPalette32(format, SDL_MapRGB(format, 0,0,0), SDL_MapRGB(format, 255,255,255), colorArray, startIndex, stopIndex);
}



/**********************************************************************************/
/**                          Color filling functions                             **/
/**********************************************************************************/

//==================================================================================
// SPG_FloodFill: Fast non-recursive flood fill
//
// Algorithm originally written by
// Paul Heckbert, 13 Sept 1982, 28 Jan 1987
//==================================================================================
/* horizontal segment of scan line y */
typedef struct seg{ 
	Sint16 y, xl, xr, dy;
}seg;

#define MAXSTACK 1000		/* max depth of stack */

#define PUSH(Y, XL, XR, DY){\
	if (sp<stack+MAXSTACK && Y+(DY)>=SPG_CLIP_YMIN(dst) && Y+(DY)<=SPG_CLIP_YMAX(dst)){\
		sp->y = Y;\
		sp->xl = XL;\
		sp->xr = XR;\
		sp->dy = DY;\
		sp++;\
	}\
}

#define POP(Y, XL, XR, DY){\
	sp--;\
	DY = sp->dy;\
	Y = sp->y + sp->dy;\
	XL = sp->xl;\
	XR = sp->xr;\
}
	

/*
 * set the pixel at (x,y) and all of its 4-connected neighbors
 * with the same pixel value to the new pixel color.
 * A 4-connected neighbor is a pixel above, below, left, or right of a pixel.
 */
// First a generic (slow) version and then 8/16/32 bpp versions
void _FloodFillX(SDL_Surface *dst, Sint16 x, Sint16 y, Uint32 color)
{
	Sint16 l, x1, x2, dy;
	Uint32 oc;						/* old pixel color */
	seg stack[MAXSTACK], *sp = stack;	/* stack of filled segments */

	if (x<SPG_CLIP_XMIN(dst) || x>SPG_CLIP_XMAX(dst) || y<SPG_CLIP_YMIN(dst) || y>SPG_CLIP_YMAX(dst))
		return;
	
	oc = SPG_GetPixel(dst, x,y);	/* read color at seed point */
	
	if (oc == color) 
		return;
	
	PUSH(y, x, x, 1);			/* needed in some cases */
	PUSH(y+1, x, x, -1);		/* seed segment (popped 1st) */

	while (sp>stack) {
		/* pop segment off stack and fill a neighboring scan line */
		POP(y, x1, x2, dy);

		/*
		* segment of scan line y-dy for x1<=x<=x2 was previously filled,
		* now explore adjacent pixels in scan line y
		*/
		for (x=x1; x>=SPG_CLIP_XMIN(dst); x--){
			if( SPG_GetPixel(dst, x,y) != oc )
				break;
			
			spg_pixel(dst, x, y, color);
		}
        
		if (x>=x1)
			goto skip;
		
		l = x+1;
		if (l<x1) 
			PUSH(y, l, x1-1, -dy);		/* leak on left? */
	
		x = x1+1;
		
		do {
			for (; x<=SPG_CLIP_XMAX(dst); x++){
				if( SPG_GetPixel(dst, x,y) != oc )
					break;
					
				spg_pixel(dst, x, y, color);
			}
	    
			PUSH(y, l, x-1, dy);
			
			if (x>x2+1) 
				PUSH(y, x2+1, x-1, -dy);	/* leak on right? */
skip:		
			for (x++; x<=x2; x++)
				if( SPG_GetPixel(dst, x,y) == oc )
					break;
			
			l = x;
		} while (x<=x2);
	}
}

/* Macro for 8/16/32 bpp */
#define DO_FILL(UintXX, label)\
{\
	Sint16 l, x1, x2, dy;\
	Uint32 oc;						/* old pixel color */\
	seg stack[MAXSTACK], *sp = stack;	/* stack of filled segments */\
	Uint16 pitch = dst->pitch/dst->format->BytesPerPixel;\
	UintXX *row = (UintXX*)dst->pixels + y*pitch;\
	UintXX *pixel = row + x;\
\
	if (x<SPG_CLIP_XMIN(dst) || x>SPG_CLIP_XMAX(dst) || y<SPG_CLIP_YMIN(dst) || y>SPG_CLIP_YMAX(dst))\
		return;\
\
	oc = *pixel;	/* read color at seed point */\
\
	if (oc == color)\
		return;\
\
	PUSH(y, x, x, 1);			/* needed in some cases */\
	PUSH(y+1, x, x, -1);		/* seed segment (popped 1st) */\
\
	while (sp>stack) {\
		/* pop segment off stack and fill a neighboring scan line */\
		POP(y, x1, x2, dy);\
		row = (UintXX*)dst->pixels + y*pitch;\
		pixel = row + x1;\
\
		/*\
		* segment of scan line y-dy for x1<=x<=x2 was previously filled,
		* now explore adjacent pixels in scan line y
		*/\
		for (x=x1; x>=SPG_CLIP_XMIN(dst) && *pixel == oc; x--, pixel--)\
			*pixel = color;\
\
		if (x>=x1)\
			goto label;\
\
		l = x+1;\
		if (l<x1)\
			PUSH(y, l, x1-1, -dy);		/* leak on left? */\
\
		x = x1+1;\
		pixel = row + x;\
\
		do {\
			for (; x<=SPG_CLIP_XMAX(dst) && *pixel == oc; x++, pixel++)\
				*pixel = color;\
\
			PUSH(y, l, x-1, dy);\
\
			if (x>x2+1)\
				PUSH(y, x2+1, x-1, -dy);	/* leak on right? */\
label:\
			pixel++;\
\
			for (x++; x<=x2 && *pixel != oc; x++, pixel++);\
\
			l = x;\
		} while (x<=x2);\
	}\
}

// Wrapper function
void SPG_FloodFill(SDL_Surface *dst, Sint16 x, Sint16 y, Uint32 color)
{
    if ( spg_lock(dst) < 0 )
    {
        if(spg_useerrors)
            SPG_Error("SPG_FloodFill could not lock surface");
        return;
    }

	switch (dst->format->BytesPerPixel) {
		case 1: /* Assuming 8-bpp */
			DO_FILL(Uint8, skip8)
		break;

		case 2: /* Probably 15-bpp or 16-bpp */
			DO_FILL(Uint16, skip16)
		break;

		case 3: /* Slow 24-bpp mode, usually not used */
			_FloodFillX(dst, x,y, color);
		break;

		case 4: /* Probably 32-bpp */
			DO_FILL(Uint32, skip32)
		break;
	}

	spg_unlock(dst);

}



