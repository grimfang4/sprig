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
#include "sprig_common.h"

#include "math.h"

/* Globals */
extern Uint8 spg_alphahack;
extern SPG_bool spg_useerrors;
extern SPG_bool spg_makedirtyrects;
extern SPG_DirtyTable* spg_dirtytable_front;
extern Uint16 spg_thickness;
extern SPG_bool spg_usedegrees;

/* We need some internal functions */
void spg_pixel(SDL_Surface *surface, Sint16 x, Sint16 y, Uint32 color);
void spg_pixelcallbackalpha(SDL_Surface *surf, Sint16 x, Sint16 y, Uint32 color);
void spg_line(SDL_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color);
void spg_lineblend(SDL_Surface *Surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 Color, Uint8 alpha);
void spg_lineh(SDL_Surface *Surface, Sint16 x1, Sint16 y, Sint16 x2, Uint32 Color);
void spg_linehblend(SDL_Surface *Surface, Sint16 x1, Sint16 y, Sint16 x2, Uint32 Color, Uint8 alpha);
void spg_lineblendaa(SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color, Uint8 alpha);
void spg_linefadeblendaa(SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color1, Uint8 alpha1, Uint32 color2, Uint8 alpha2);

void spg_thicknesscallback(SDL_Surface *Surf, Sint16 X, Sint16 Y, Uint32 Color);
void spg_thicknesscallbackalpha(SDL_Surface *Surf, Sint16 X, Sint16 Y, Uint32 Color);


/* Macro to inline RGB mapping */
#define MapRGB(format, r, g, b)\
	(r >> format->Rloss) << format->Rshift\
		| (g >> format->Gloss) << format->Gshift\
		| (b >> format->Bloss) << format->Bshift

// Arbitrary sorting start position for drawing polygons.
// This enables negative coords for polys.  Thanks, Huge!
#define NULL_POSITION    (-0x7ffe)


//==================================================================================
// Draws a horizontal line, fading the colors
//==================================================================================
void spg_linehfade(SDL_Surface *dest,Sint16 x1,Sint16 y,Sint16 x2,Uint8 r1,Uint8 g1,Uint8 b1,Uint8 r2,Uint8 g2,Uint8 b2)
{
	Sint16 x;
	Uint8 t;

	/* Fix coords */
	if ( x1 > x2 ) {
		SWAP(x1,x2,x);
		SWAP(r1,r2,t);
		SWAP(g1,g2,t);
		SWAP(b1,b2,t);
	}

	/* We use fixedpoint math */
	Sint32 R = r1<<16;
	Sint32 G = g1<<16;
	Sint32 B = b1<<16;

	/* Color step value */
	Sint32 rstep = (Sint32)((r2-r1)<<16) / (Sint32)(x2-x1+1);
	Sint32 gstep = (Sint32)((g2-g1)<<16) / (Sint32)(x2-x1+1);
	Sint32 bstep = (Sint32)((b2-b1)<<16) / (Sint32)(x2-x1+1);


	/* Clipping */
	if(x2<SPG_CLIP_XMIN(dest) || x1>SPG_CLIP_XMAX(dest) || y<SPG_CLIP_YMIN(dest) || y>SPG_CLIP_YMAX(dest))
		return;
	if (x1 < SPG_CLIP_XMIN(dest)){
		/* Update start colors */
		R += (SPG_CLIP_XMIN(dest)-x1)*rstep;
		G += (SPG_CLIP_XMIN(dest)-x1)*gstep;
		B += (SPG_CLIP_XMIN(dest)-x1)*bstep;
  		x1 = SPG_CLIP_XMIN(dest);
	}
	if (x2 > SPG_CLIP_XMAX(dest))
  		x2 = SPG_CLIP_XMAX(dest);


	switch (dest->format->BytesPerPixel) {
		case 1: { /* Assuming 8-bpp */
			Uint8 *pixel;
			Uint8 *row = (Uint8 *)dest->pixels + y*dest->pitch;

			for (x = x1; x <= x2; x++){
				pixel = row + x;

				*pixel = SDL_MapRGB( dest->format, R>>16, G>>16, B>>16 );

				R += rstep;
				G += gstep;
				B += bstep;
			}
		}
		break;

		case 2: { /* Probably 15-bpp or 16-bpp */
			Uint16 *pixel;
			Uint16 *row = (Uint16 *)dest->pixels + y*dest->pitch/2;

			for (x = x1; x <= x2; x++){
				pixel = row + x;

				*pixel = MapRGB( dest->format, R>>16, G>>16, B>>16 );

				R += rstep;
				G += gstep;
				B += bstep;
			}
		}
		break;

		case 3: { /* Slow 24-bpp mode, usually not used */
			Uint8 *pixel;
			Uint8 *row = (Uint8 *)dest->pixels + y*dest->pitch;

			Uint8 rshift8=dest->format->Rshift/8;
			Uint8 gshift8=dest->format->Gshift/8;
			Uint8 bshift8=dest->format->Bshift/8;

			for (x = x1; x <= x2; x++){
				pixel = row + x*3;

				*(pixel+rshift8) = R>>16;
  				*(pixel+gshift8) = G>>16;
  				*(pixel+bshift8) = B>>16;

				R += rstep;
				G += gstep;
				B += bstep;
			}
		}
		break;

		case 4: { /* Probably 32-bpp */
			Uint32 *pixel;
			Uint32 *row = (Uint32 *)dest->pixels + y*dest->pitch/4;

			for (x = x1; x <= x2; x++){
				pixel = row + x;

				*pixel = MapRGB( dest->format, R>>16, G>>16, B>>16 );

				R += rstep;
				G += gstep;
				B += bstep;
			}
		}
		break;
	}
}


void SPG_LineHFade(SDL_Surface *dest,Sint16 x1,Sint16 y,Sint16 x2,Uint32 color1, Uint32 color2)
{
    if ( spg_lock(dest) < 0 )
    {
        if(spg_useerrors)
            SPG_Error("SPG_LineHFade could not lock surface");
        return;
    }

    if(spg_thickness == 1) 
    {
        Uint8 r1, g1, b1, r2, g2, b2;
        SDL_GetRGB(color1, dest->format, &r1, &g1, &b1);
        SDL_GetRGB(color2, dest->format, &r2, &g2, &b2);
        spg_linehfade(dest,x1,y,x2,r1,g1,b1,r2,g2,b2);

        if(spg_makedirtyrects)
        {
            SDL_Rect rect;
            rect.x = MIN(x1, x2);
            rect.y = y;
            rect.w = MAX(x1, x2) - rect.x + 1;
            rect.h = 1;
            // Clip it to the screen
            SPG_DirtyClip(dest, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
    }
    else if(spg_thickness > 0)
        SPG_LineFade(dest, x1, y, x2, y, color1, color2);
    
    spg_unlock(dest);
    
}


//==================================================================================
// Draws a horizontal, textured line
//==================================================================================
void spg_linehtex(SDL_Surface *dest,Sint16 x1,Sint16 y,Sint16 x2,SDL_Surface *source,Sint16 sx1,Sint16 sy1,Sint16 sx2,Sint16 sy2)
{
	Sint16 x;

	/* Fix coords */
	if ( x1 > x2 ) {
		SWAP(x1,x2,x);
		SWAP(sx1,sx2,x);
		SWAP(sy1,sy2,x);
	}

	/* Fixed point texture starting coords */
	Sint32 srcx = sx1<<16;
	Sint32 srcy = sy1<<16;

	/* Texture coords stepping value */
	Sint32 xstep = (Sint32)((sx2-sx1)<<16) / (Sint32)(x2-x1+1);
	Sint32 ystep = (Sint32)((sy2-sy1)<<16) / (Sint32)(x2-x1+1);


	/* Clipping */
	if(x2<SPG_CLIP_XMIN(dest) || x1>SPG_CLIP_XMAX(dest) || y<SPG_CLIP_YMIN(dest) || y>SPG_CLIP_YMAX(dest))
		return;
	if (x1 < SPG_CLIP_XMIN(dest)){
		/* Fix texture starting coord */
		srcx += (SPG_CLIP_XMIN(dest)-x1)*xstep;
		srcy += (SPG_CLIP_XMIN(dest)-x1)*ystep;
  		x1 = SPG_CLIP_XMIN(dest);
	}
	if (x2 > SPG_CLIP_XMAX(dest))
  		x2 = SPG_CLIP_XMAX(dest);


	if(dest->format->BytesPerPixel == source->format->BytesPerPixel){
		/* Fast mode. Just copy the pixel */

		switch (dest->format->BytesPerPixel) {
			case 1: { /* Assuming 8-bpp */
				Uint8 *pixel;
				Uint8 *row = (Uint8 *)dest->pixels + y*dest->pitch;

				for (x = x1; x <= x2; x++){
					pixel = row + x;

					*pixel = *((Uint8 *)source->pixels + (srcy>>16)*source->pitch + (srcx>>16));

					srcx += xstep;
					srcy += ystep;
				}
			}
			break;

			case 2: { /* Probably 15-bpp or 16-bpp */
				Uint16 *pixel;
				Uint16 *row = (Uint16 *)dest->pixels + y*dest->pitch/2;

				Uint16 pitch = source->pitch/2;

				for (x = x1; x <= x2; x++){
					pixel = row + x;

					*pixel = *((Uint16 *)source->pixels + (srcy>>16)*pitch + (srcx>>16));

					srcx += xstep;
					srcy += ystep;
				}
			}
			break;

			case 3: { /* Slow 24-bpp mode, usually not used */
				Uint8 *pixel, *srcpixel;
				Uint8 *row = (Uint8 *)dest->pixels + y*dest->pitch;

				Uint8 rshift8=dest->format->Rshift/8;
				Uint8 gshift8=dest->format->Gshift/8;
				Uint8 bshift8=dest->format->Bshift/8;

				for (x = x1; x <= x2; x++){
					pixel = row + x*3;
					srcpixel = (Uint8 *)source->pixels + (srcy>>16)*source->pitch + (srcx>>16)*3;

					*(pixel+rshift8) = *(srcpixel+rshift8);
  					*(pixel+gshift8) = *(srcpixel+gshift8);
  					*(pixel+bshift8) = *(srcpixel+bshift8);

					srcx += xstep;
					srcy += ystep;
				}
			}
			break;

			case 4: { /* Probably 32-bpp */
				Uint32 *pixel;
				Uint32 *row = (Uint32 *)dest->pixels + y*dest->pitch/4;

				Uint16 pitch = source->pitch/4;

				for (x = x1; x <= x2; x++){
					pixel = row + x;

					*pixel = *((Uint32 *)source->pixels + (srcy>>16)*pitch + (srcx>>16));

					srcx += xstep;
					srcy += ystep;
				}
			}
			break;
		}
	}else{
		/* Slow mode. We must translate every pixel color! */

		Uint8 r=0,g=0,b=0;

		switch (dest->format->BytesPerPixel) {
			case 1: { /* Assuming 8-bpp */
				Uint8 *pixel;
				Uint8 *row = (Uint8 *)dest->pixels + y*dest->pitch;

				for (x = x1; x <= x2; x++){
					pixel = row + x;

					SDL_GetRGB(SPG_GetPixel(source, srcx>>16, srcy>>16), source->format, &r, &g, &b);
					*pixel = SDL_MapRGB( dest->format, r, g, b );

					srcx += xstep;
					srcy += ystep;
				}
			}
			break;

			case 2: { /* Probably 15-bpp or 16-bpp */
				Uint16 *pixel;
				Uint16 *row = (Uint16 *)dest->pixels + y*dest->pitch/2;

				for (x = x1; x <= x2; x++){
					pixel = row + x;

					SDL_GetRGB(SPG_GetPixel(source, srcx>>16, srcy>>16), source->format, &r, &g, &b);
					*pixel = MapRGB( dest->format, r, g, b );

					srcx += xstep;
					srcy += ystep;
				}
			}
			break;

			case 3: { /* Slow 24-bpp mode, usually not used */
				Uint8 *pixel, *srcpixel;
				Uint8 *row = (Uint8 *)dest->pixels + y*dest->pitch;

				Uint8 rshift8=dest->format->Rshift/8;
				Uint8 gshift8=dest->format->Gshift/8;
				Uint8 bshift8=dest->format->Bshift/8;

				for (x = x1; x <= x2; x++){
					pixel = row + x*3;
					srcpixel = (Uint8 *)source->pixels + (srcy>>16)*source->pitch + (srcx>>16)*3;

					SDL_GetRGB(SPG_GetPixel(source, srcx>>16, srcy>>16), source->format, &r, &g, &b);

					*(pixel+rshift8) = r;
  					*(pixel+gshift8) = g;
  					*(pixel+bshift8) = b;

					srcx += xstep;
					srcy += ystep;
				}
			}
			break;

			case 4: { /* Probably 32-bpp */
				Uint32 *pixel;
				Uint32 *row = (Uint32 *)dest->pixels + y*dest->pitch/4;

				for (x = x1; x <= x2; x++){
					pixel = row + x;

					SDL_GetRGB(SPG_GetPixel(source, srcx>>16, srcy>>16), source->format, &r, &g, &b);
					*pixel = MapRGB( dest->format, r, g, b );

					srcx += xstep;
					srcy += ystep;
				}
			}
			break;
		}
	}
}

void SPG_LineHTex(SDL_Surface *dest,Sint16 x1,Sint16 y,Sint16 x2,SDL_Surface *source,Sint16 sx1,Sint16 sy1,Sint16 sx2,Sint16 sy2)
{
    if ( spg_lock(dest) < 0 )
    {
        if(spg_useerrors)
            SPG_Error("SPG_LineHTex could not lock dest surface");
        return;
    }
    if ( spg_lock(source) < 0 )
    {
        if(spg_useerrors)
            SPG_Error("SPG_LineHTex could not lock source surface");
        return;
    }

	spg_linehtex(dest,x1,y,x2,source,sx1,sy1,sx2,sy2);

	spg_unlock(dest);
	spg_unlock(source);
    if(spg_makedirtyrects)
    {
        SDL_Rect rect;
        rect.x = MIN(x1, x2);
        rect.y = y;
        rect.w = MAX(x1, x2) - rect.x + 1;
        rect.h = 1;
        // Clip it to the screen
        SPG_DirtyClip(dest, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
    }

}

//==================================================================================
// Draws a trigon
//==================================================================================
void SPG_Trigon(SDL_Surface *dest,Sint16 x1,Sint16 y1,Sint16 x2,Sint16 y2,Sint16 x3,Sint16 y3,Uint32 color)
{
    if ( spg_lock(dest) < 0 )
    {
        if(spg_useerrors)
            SPG_Error("SPG_Trigon could not lock surface");
        return;
    }

    if(spg_thickness == 1)
    {
        if(SPG_GetAA())
        {
            spg_lineblendaa(dest,x1,y1,x2,y2,color, SDL_ALPHA_OPAQUE);
            spg_lineblendaa(dest,x1,y1,x3,y3,color, SDL_ALPHA_OPAQUE);
            spg_lineblendaa(dest,x3,y3,x2,y2,color, SDL_ALPHA_OPAQUE);
        }
        else
        {
            spg_line(dest,x1,y1,x2,y2,color);
            spg_line(dest,x1,y1,x3,y3,color);
            spg_line(dest,x3,y3,x2,y2,color);
        }

        if(spg_makedirtyrects)
        {
            Sint16 xmax=x1, ymax=y1, xmin=x1, ymin=y1;
            xmax= (xmax>x2)? xmax : x2;  ymax= (ymax>y2)? ymax : y2;
            xmin= (xmin<x2)? xmin : x2;  ymin= (ymin<y2)? ymin : y2;
            xmax= (xmax>x3)? xmax : x3;  ymax= (ymax>y3)? ymax : y3;
            xmin= (xmin<x3)? xmin : x3;  ymin= (ymin<y3)? ymin : y3;
        
            SDL_Rect rect;
            rect.x = xmin;
            rect.y = ymin;
            rect.w = xmax-xmin + 1;
            rect.h = ymax-ymin + 1;
            // Clip it to the screen
            SPG_DirtyClip(dest, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
    }
    else if(spg_thickness > 0)
    {
        SPG_LineFn(dest,x1,y1,x2,y2,color, spg_thicknesscallback);
        SPG_LineFn(dest,x1,y1,x3,y3,color, spg_thicknesscallback);
        SPG_LineFn(dest,x3,y3,x2,y2,color, spg_thicknesscallback);

        if(spg_makedirtyrects)
        {
            Sint16 xmax=x1, ymax=y1, xmin=x1, ymin=y1;
            xmax= (xmax>x2)? xmax : x2;  ymax= (ymax>y2)? ymax : y2;
            xmin= (xmin<x2)? xmin : x2;  ymin= (ymin<y2)? ymin : y2;
            xmax= (xmax>x3)? xmax : x3;  ymax= (ymax>y3)? ymax : y3;
            xmin= (xmin<x3)? xmin : x3;  ymin= (ymin<y3)? ymin : y3;
        
            SDL_Rect rect;
            rect.x = xmin - spg_thickness/2;
            rect.y = ymin - spg_thickness/2;
            rect.w = xmax-xmin + 1 + spg_thickness;
            rect.h = ymax-ymin + 1 + spg_thickness;
            // Clip it to the screen
            SPG_DirtyClip(dest, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
    }

    spg_unlock(dest);

}



//==================================================================================
// Draws a trigon (alpha)
//==================================================================================
void SPG_TrigonBlend(SDL_Surface *dest,Sint16 x1,Sint16 y1,Sint16 x2,Sint16 y2,Sint16 x3,Sint16 y3,Uint32 color, Uint8 alpha)
{
    if ( spg_lock(dest) < 0 )
    {
        if(spg_useerrors)
            SPG_Error("SPG_TrigonBlend could not lock surface");
        return;
    }

    if(spg_thickness == 1)
    {
        if(SPG_GetAA())
        {
            spg_lineblendaa(dest,x1,y1,x2,y2,color, alpha);
            spg_lineblendaa(dest,x1,y1,x3,y3,color, alpha);
            spg_lineblendaa(dest,x3,y3,x2,y2,color, alpha);
        }
        else
        {
            spg_lineblend(dest,x1,y1,x2,y2,color, alpha);
            spg_lineblend(dest,x1,y1,x3,y3,color, alpha);
            spg_lineblend(dest,x3,y3,x2,y2,color, alpha);
        }

        if(spg_makedirtyrects)
        {
            Sint16 xmax=x1, ymax=y1, xmin=x1, ymin=y1;
            xmax= (xmax>x2)? xmax : x2;  ymax= (ymax>y2)? ymax : y2;
            xmin= (xmin<x2)? xmin : x2;  ymin= (ymin<y2)? ymin : y2;
            xmax= (xmax>x3)? xmax : x3;  ymax= (ymax>y3)? ymax : y3;
            xmin= (xmin<x3)? xmin : x3;  ymin= (ymin<y3)? ymin : y3;
        
            SDL_Rect rect;
            rect.x = xmin;
            rect.y = ymin;
            rect.w = xmax-xmin + 1;
            rect.h = ymax-ymin + 1;
            // Clip it to the screen
            SPG_DirtyClip(dest, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
    }
    else if(spg_thickness > 0)
    {
        spg_alphahack = alpha;
        SPG_LineFn(dest,x1,y1,x2,y2,color, spg_thicknesscallbackalpha);
        SPG_LineFn(dest,x1,y1,x3,y3,color, spg_thicknesscallbackalpha);
        SPG_LineFn(dest,x3,y3,x2,y2,color, spg_thicknesscallbackalpha);

        if(spg_makedirtyrects)
        {
            Sint16 xmax=x1, ymax=y1, xmin=x1, ymin=y1;
            xmax= (xmax>x2)? xmax : x2;  ymax= (ymax>y2)? ymax : y2;
            xmin= (xmin<x2)? xmin : x2;  ymin= (ymin<y2)? ymin : y2;
            xmax= (xmax>x3)? xmax : x3;  ymax= (ymax>y3)? ymax : y3;
            xmin= (xmin<x3)? xmin : x3;  ymin= (ymin<y3)? ymin : y3;
        
            SDL_Rect rect;
            rect.x = xmin - spg_thickness/2;
            rect.y = ymin - spg_thickness/2;
            rect.w = xmax-xmin + 1 + spg_thickness;
            rect.h = ymax-ymin + 1 + spg_thickness;
            // Clip it to the screen
            SPG_DirtyClip(dest, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
    }

    spg_unlock(dest);

}



//==================================================================================
// Draws a filled trigon
//==================================================================================
void SPG_TrigonFilled(SDL_Surface *dest,Sint16 x1,Sint16 y1,Sint16 x2,Sint16 y2,Sint16 x3,Sint16 y3,Uint32 color)
{
	Sint16 y;


    // AA hack
	if(SPG_GetAA())
	{
	    // Rough guess at center
	    float cx = (x1 + x2 + x3)/3.0f,
	          cy = (y1 + y2 + y3)/3.0f;

        // Draw AA lines
	    spg_lineblendaa(dest,x1,y1,x2,y2,color, SDL_ALPHA_OPAQUE);
	    spg_lineblendaa(dest,x2,y2,x3,y3,color, SDL_ALPHA_OPAQUE);
	    spg_lineblendaa(dest,x3,y3,x1,y1,color, SDL_ALPHA_OPAQUE);

        // Push in all points by one
        cx > x1? x1++ : x1--;
        cx > x2? x2++ : x2--;
        cx > x3? x3++ : x3--;
        cy > y1? y1++ : y1--;
        cy > y2? y2++ : y2--;
        cy > y3? y3++ : y3--;
	}

	//if( y1==y3 )
	//	return;

	/* Sort coords */
	if ( y1 > y2 ) {
		SWAP(y1,y2,y);
		SWAP(x1,x2,y);
	}
	if ( y2 > y3 ) {
		SWAP(y2,y3,y);
		SWAP(x2,x3,y);
	}
	if ( y1 > y2 ) {
		SWAP(y1,y2,y);
		SWAP(x1,x2,y);
	}

	/*
	 * How do we calculate the starting and ending x coordinate of the horizontal line
	 * on each y coordinate?  We can do this by using a standard line algorithm but
	 * instead of plotting pixels, use the x coordinates as start and stop
	 * coordinates for the horizontal line.
	 * So we will simply trace the outlining of the triangle; this will require 3 lines.
	 * Line 1 is the line between (x1,y1) and (x2,y2)
	 * Line 2 is the line between (x1,y1) and (x3,y3)
	 * Line 3 is the line between (x2,y2) and (x3,y3)
	 *
	 * We can divide the triangle into 2 halfs. The upper half will be outlined by line
	 * 1 and 2. The lower half will be outlined by line line 2 and 3.
	*/


	/* Starting coords for the three lines */
	Sint32 xa = (Sint32)(x1<<16);
	Sint32 xb = xa;
	Sint32 xc = (Sint32)(x2<<16);

	/* Lines step values */
	Sint32 m1 = 0;
	Sint32 m2 = (y3 != y1)? (Sint32)((x3 - x1)<<16)/(Sint32)(y3 - y1) : 400000;  // That oughta be enough?
	Sint32 m3 = 0;


    /* Upper half of the triangle */
    if( y1==y2 )
        spg_lineh(dest, x1, y1, x2, color);
    else{
        m1 = (Sint32)((x2 - x1)<<16)/(Sint32)(y2 - y1);

        for ( y = y1; y <= y2; y++) {
            spg_lineh(dest, xa>>16, y, xb>>16, color);

            xa += m1;
            xb += m2;
        }
    }

    /* Lower half of the triangle */
    if( y2==y3 )
        spg_lineh(dest, x2, y2, x3, color);
    else{
        m3 = (Sint32)((x3 - x2)<<16)/(Sint32)(y3 - y2);

        for ( y = y2+1; y <= y3; y++) {
            spg_lineh(dest, xb>>16, y, xc>>16, color);

            xb += m2;
            xc += m3;
        }
    }
    if(spg_makedirtyrects)
    {
        Sint16 xmax=x1, ymax=y1, xmin=x1, ymin=y1;
        xmax= (xmax>x2)? xmax : x2;  ymax= (ymax>y2)? ymax : y2;
        xmin= (xmin<x2)? xmin : x2;  ymin= (ymin<y2)? ymin : y2;
        xmax= (xmax>x3)? xmax : x3;  ymax= (ymax>y3)? ymax : y3;
        xmin= (xmin<x3)? xmin : x3;  ymin= (ymin<y3)? ymin : y3;
	
        SDL_Rect rect;
        rect.x = xmin;
        rect.y = ymin;
        rect.w = xmax-xmin + 1;
        rect.h = ymax-ymin + 1;
        // Clip it to the screen
        SPG_DirtyClip(dest, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
    }


}



//==================================================================================
// Draws a filled trigon (alpha)
//==================================================================================
void SPG_TrigonFilledBlend(SDL_Surface *dest,Sint16 x1,Sint16 y1,Sint16 x2,Sint16 y2,Sint16 x3,Sint16 y3,Uint32 color, Uint8 alpha)
{
	Sint16 y;


    // AA hack
	if(SPG_GetAA())
	{
	    // Rough guess at center
	    float cx = (x1 + x2 + x3)/3.0f,
	          cy = (y1 + y2 + y3)/3.0f;

        // Draw AA lines
	    spg_lineblendaa(dest,x1,y1,x2,y2,color, alpha);
	    spg_lineblendaa(dest,x2,y2,x3,y3,color, alpha);
	    spg_lineblendaa(dest,x3,y3,x1,y1,color, alpha);

        // Push in all points by one
        cx > x1? x1++ : x1--;
        cx > x2? x2++ : x2--;
        cx > x3? x3++ : x3--;
        cy > y1? y1++ : y1--;
        cy > y2? y2++ : y2--;
        cy > y3? y3++ : y3--;
	}

	//if( y1==y3 )
	//	return;

	/* Sort coords */
	if ( y1 > y2 ) {
		SWAP(y1,y2,y);
		SWAP(x1,x2,y);
	}
	if ( y2 > y3 ) {
		SWAP(y2,y3,y);
		SWAP(x2,x3,y);
	}
	if ( y1 > y2 ) {
		SWAP(y1,y2,y);
		SWAP(x1,x2,y);
	}

	Sint32 xa = (Sint32)(x1<<16);
	Sint32 xb = xa;
	Sint32 xc = (Sint32)(x2<<16);

	Sint32 m1 = 0;
	Sint32 m2 = (y3 != y1)? (Sint32)((x3 - x1)<<16)/(Sint32)(y3 - y1) : 400000;  // That oughta be enough?
	Sint32 m3 = 0;

    if ( spg_lock(dest) < 0 )
    {
        if(spg_useerrors)
            SPG_Error("SPG_TrigonFilledBlend could not lock surface");
        return;
    }

	/* Upper half of the triangle */
	if( y1==y2 )
		spg_linehblend(dest, x1, y1, x2, color, alpha);
	else{
		m1 = (Sint32)((x2 - x1)<<16)/(Sint32)(y2 - y1);

		for ( y = y1; y <= y2; y++) {
			spg_linehblend(dest, xa>>16, y, xb>>16, color, alpha);

			xa += m1;
			xb += m2;
		}
	}

	/* Lower half of the triangle */
	if( y2==y3 )
		spg_linehblend(dest, x2, y2, x3, color, alpha);
	else{
		m3 = (Sint32)((x3 - x2)<<16)/(Sint32)(y3 - y2);

		for ( y = y2+1; y <= y3; y++) {
			spg_linehblend(dest, xb>>16, y, xc>>16, color, alpha);

			xb += m2;
			xc += m3;
		}
	}

	spg_unlock(dest);
    if(spg_makedirtyrects)
    {
        Sint16 xmax=x1, ymax=y1, xmin=x1, ymin=y1;
        xmax= (xmax>x2)? xmax : x2;  ymax= (ymax>y2)? ymax : y2;
        xmin= (xmin<x2)? xmin : x2;  ymin= (ymin<y2)? ymin : y2;
        xmax= (xmax>x3)? xmax : x3;  ymax= (ymax>y3)? ymax : y3;
        xmin= (xmin<x3)? xmin : x3;  ymin= (ymin<y3)? ymin : y3;
	
        SDL_Rect rect;
        rect.x = xmin;
        rect.y = ymin;
        rect.w = xmax-xmin + 1;
        rect.h = ymax-ymin + 1;
        // Clip it to the screen
        SPG_DirtyClip(dest, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
    }


}


//==================================================================================
// Draws a gourand shaded trigon
//==================================================================================
void SPG_TrigonFade(SDL_Surface *dest,Sint16 x1,Sint16 y1,Sint16 x2,Sint16 y2,Sint16 x3,Sint16 y3,Uint32 c1,Uint32 c2,Uint32 c3)
{
	Sint16 y;

	//if( y1==y3 )
	//	return;

	Uint8 c=0;
	SDL_Color col1;
	SDL_Color col2;
	SDL_Color col3;

	col1 = SPG_GetColor(dest,c1);
	col2 = SPG_GetColor(dest,c2);
	col3 = SPG_GetColor(dest,c3);

	/* Sort coords */
	if ( y1 > y2 ) {
		SWAP(y1,y2,y);
		SWAP(x1,x2,y);
		SWAP(col1.r,col2.r,c);
		SWAP(col1.g,col2.g,c);
		SWAP(col1.b,col2.b,c);
	}
	if ( y2 > y3 ) {
		SWAP(y2,y3,y);
		SWAP(x2,x3,y);
		SWAP(col2.r,col3.r,c);
		SWAP(col2.g,col3.g,c);
		SWAP(col2.b,col3.b,c);
	}
	if ( y1 > y2 ) {
		SWAP(y1,y2,y);
		SWAP(x1,x2,y);
		SWAP(col1.r,col2.r,c);
		SWAP(col1.g,col2.g,c);
		SWAP(col1.b,col2.b,c);
	}

	/*
	 * We trace three lines exactly like in SPG_FilledTrigon(), but here we
	 * must also keep track of the colors. We simply calculate how the color
	 * will change along the three lines.
	*/

	/* Starting coords for the three lines */
	Sint32 xa = (Sint32)(x1<<16);
	Sint32 xb = xa;
	Sint32 xc = (Sint32)(x2<<16);

	/* Starting colors (rgb) for the three lines */
	Sint32 r1 = (Sint32)(col1.r<<16);
	Sint32 r2 = r1;
	Sint32 r3 = (Sint32)(col2.r<<16);

	Sint32 g1 = (Sint32)(col1.g<<16);
	Sint32 g2 = g1;
	Sint32 g3 = (Sint32)(col2.g<<16);

	Sint32 b1 = (Sint32)(col1.b<<16);
	Sint32 b2 = b1;
	Sint32 b3 = (Sint32)(col2.b<<16);

	/* Lines step values */
	Sint32 m1 = 0;
	Sint32 m2 = (y3 != y1)? (Sint32)((x3 - x1)<<16)/(Sint32)(y3 - y1) : 400000;  // That oughta be enough?
	Sint32 m3 = 0;

	/* Colors step values */
	Sint32 rstep1 = 0;
	Sint32 rstep2 = (y3 != y1)? (Sint32)((col3.r - col1.r) << 16) / (Sint32)(y3 - y1) : 400000;
	Sint32 rstep3 = 0;

	Sint32 gstep1 = 0;
	Sint32 gstep2 = (y3 != y1)? (Sint32)((col3.g - col1.g) << 16) / (Sint32)(y3 - y1) : 400000;
	Sint32 gstep3 = 0;

	Sint32 bstep1 = 0;
	Sint32 bstep2 = (y3 != y1)? (Sint32)((col3.b - col1.b) << 16) / (Sint32)(y3 - y1) : 400000;
	Sint32 bstep3 = 0;

    if ( spg_lock(dest) < 0 )
    {
        if(spg_useerrors)
            SPG_Error("SPG_TrigonFade could not lock surface");
        return;
    }

	/* Upper half of the triangle */
	if( y1==y2 )
		spg_linehfade(dest, x1, y1, x2, col1.r, col1.g, col1.b, col2.r, col2.g, col2.b);
	else{
		m1 = (Sint32)((x2 - x1)<<16)/(Sint32)(y2 - y1);

		rstep1 = (Sint32)((col2.r - col1.r) << 16) / (Sint32)(y2 - y1);
		gstep1 = (Sint32)((col2.g - col1.g) << 16) / (Sint32)(y2 - y1);
		bstep1 = (Sint32)((col2.b - col1.b) << 16) / (Sint32)(y2 - y1);

		for ( y = y1; y <= y2; y++) {
			spg_linehfade(dest, xa>>16, y, xb>>16, r1>>16, g1>>16, b1>>16, r2>>16, g2>>16, b2>>16);

			xa += m1;
			xb += m2;

			r1 += rstep1;
			g1 += gstep1;
			b1 += bstep1;

			r2 += rstep2;
			g2 += gstep2;
			b2 += bstep2;
		}
	}

	/* Lower half of the triangle */
	if( y2==y3 )
		spg_linehfade(dest, x2, y2, x3, col2.r, col2.g, col2.b, col3.r, col3.g, col3.b);
	else{
		m3 = (Sint32)((x3 - x2)<<16)/(Sint32)(y3 - y2);

		rstep3 = (Sint32)((col3.r - col2.r) << 16) / (Sint32)(y3 - y2);
		gstep3 = (Sint32)((col3.g - col2.g) << 16) / (Sint32)(y3 - y2);
		bstep3 = (Sint32)((col3.b - col2.b) << 16) / (Sint32)(y3 - y2);

		for ( y = y2+1; y <= y3; y++) {
			spg_linehfade(dest, xb>>16, y, xc>>16, r2>>16, g2>>16, b2>>16, r3>>16, g3>>16, b3>>16);

			xb += m2;
			xc += m3;

			r2 += rstep2;
			g2 += gstep2;
			b2 += bstep2;

			r3 += rstep3;
			g3 += gstep3;
			b3 += bstep3;
		}
	}

	spg_unlock(dest);
    if(spg_makedirtyrects)
    {
        Sint16 xmax=x1, ymax=y1, xmin=x1, ymin=y1;
        xmax= (xmax>x2)? xmax : x2;  ymax= (ymax>y2)? ymax : y2;
        xmin= (xmin<x2)? xmin : x2;  ymin= (ymin<y2)? ymin : y2;
        xmax= (xmax>x3)? xmax : x3;  ymax= (ymax>y3)? ymax : y3;
        xmin= (xmin<x3)? xmin : x3;  ymin= (ymin<y3)? ymin : y3;
	
        SDL_Rect rect;
        rect.x = xmin;
        rect.y = ymin;
        rect.w = xmax-xmin + 1;
        rect.h = ymax-ymin + 1;
        // Clip it to the screen
        SPG_DirtyClip(dest, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
    }

}


//==================================================================================
// Draws a texured trigon (fast)
//==================================================================================
void SPG_TrigonTex(SDL_Surface *dest,Sint16 x1,Sint16 y1,Sint16 x2,Sint16 y2,Sint16 x3,Sint16 y3,SDL_Surface *source,Sint16 sx1,Sint16 sy1,Sint16 sx2,Sint16 sy2,Sint16 sx3,Sint16 sy3)
{
	Sint16 y;

	//if( y1==y3 )
	//	return;

	/* Sort coords */
	if ( y1 > y2 ) {
		SWAP(y1,y2,y);
		SWAP(x1,x2,y);
		SWAP(sx1,sx2,y);
		SWAP(sy1,sy2,y);
	}
	if ( y2 > y3 ) {
		SWAP(y2,y3,y);
		SWAP(x2,x3,y);
		SWAP(sx2,sx3,y);
		SWAP(sy2,sy3,y);
	}
	if ( y1 > y2 ) {
		SWAP(y1,y2,y);
		SWAP(x1,x2,y);
		SWAP(sx1,sx2,y);
		SWAP(sy1,sy2,y);
	}

	/*
	 * Again we do the same thing as in SPG_FilledTrigon(). But here we must keep track of how the
	 * texture coords change along the lines.
	*/

	/* Starting coords for the three lines */
	Sint32 xa = (Sint32)(x1<<16);
	Sint32 xb = xa;
	Sint32 xc = (Sint32)(x2<<16);

	/* Lines step values */
	Sint32 m1 = 0;
	Sint32 m2 = (y3 != y1)? (Sint32)((x3 - x1)<<16)/(Sint32)(y3 - y1) : 400000;  // That oughta be enough?
	Sint32 m3 = 0;

	/* Starting texture coords for the three lines */
	Sint32 srcx1 = (Sint32)(sx1<<16);
	Sint32 srcx2 = srcx1;
	Sint32 srcx3 = (Sint32)(sx2<<16);

	Sint32 srcy1 = (Sint32)(sy1<<16);
	Sint32 srcy2 = srcy1;
	Sint32 srcy3 = (Sint32)(sy2<<16);

	/* Texture coords stepping value */
	Sint32 xstep1 = 0;
	Sint32 xstep2 = (y3 != y1)? (Sint32)((sx3 - sx1) << 16) / (Sint32)(y3 - y1) : 400000;
	Sint32 xstep3 = 0;

	Sint32 ystep1 = 0;
	Sint32 ystep2 = (y3 != y1)? (Sint32)((sy3 - sy1) << 16) / (Sint32)(y3 - y1) : 400000;
	Sint32 ystep3 = 0;

    if ( spg_lock(dest) < 0 )
    {
        if(spg_useerrors)
            SPG_Error("SPG_TrigonTex could not lock dest surface");
        return;
    }

    if ( spg_lock(source) < 0 )
    {
        if(spg_useerrors)
            SPG_Error("SPG_TrigonTex could not lock source surface");
        return;
    }

	/* Upper half of the triangle */
	if( y1==y2 )
		spg_linehtex(dest,x1,y1,x2,source,sx1,sy1,sx2,sy2);
	else{
		m1 = (Sint32)((x2 - x1)<<16)/(Sint32)(y2 - y1);

		xstep1 = (Sint32)((sx2 - sx1) << 16) / (Sint32)(y2 - y1);
		ystep1 = (Sint32)((sy2 - sy1) << 16) / (Sint32)(y2 - y1);

		for ( y = y1; y <= y2; y++) {
			spg_linehtex(dest, xa>>16, y, xb>>16, source, srcx1>>16, srcy1>>16, srcx2>>16, srcy2>>16);

			xa += m1;
			xb += m2;

			srcx1 += xstep1;
			srcx2 += xstep2;
			srcy1 += ystep1;
			srcy2 += ystep2;
		}
	}

	/* Lower half of the triangle */
	if( y2==y3 )
		spg_linehtex(dest,x2,y2,x3,source,sx2,sy2,sx3,sy3);
	else{
		m3 = (Sint32)((x3 - x2)<<16)/(Sint32)(y3 - y2);

		xstep3 = (Sint32)((sx3 - sx2) << 16) / (Sint32)(y3 - y2);
		ystep3 = (Sint32)((sy3 - sy2) << 16) / (Sint32)(y3 - y2);

		for ( y = y2+1; y <= y3; y++) {
			spg_linehtex(dest, xb>>16, y, xc>>16, source, srcx2>>16, srcy2>>16, srcx3>>16, srcy3>>16);

			xb += m2;
			xc += m3;

			srcx2 += xstep2;
			srcx3 += xstep3;
			srcy2 += ystep2;
			srcy3 += ystep3;
		}
	}

	spg_unlock(dest);
	spg_unlock(source);

    if(spg_makedirtyrects)
    {
        Sint16 xmax=x1, ymax=y1, xmin=x1, ymin=y1;
        xmax= (xmax>x2)? xmax : x2;  ymax= (ymax>y2)? ymax : y2;
        xmin= (xmin<x2)? xmin : x2;  ymin= (ymin<y2)? ymin : y2;
        xmax= (xmax>x3)? xmax : x3;  ymax= (ymax>y3)? ymax : y3;
        xmin= (xmin<x3)? xmin : x3;  ymin= (ymin<y3)? ymin : y3;
	
        SDL_Rect rect;
        rect.x = xmin;
        rect.y = ymin;
        rect.w = xmax-xmin + 1;
        rect.h = ymax-ymin + 1;
        // Clip it to the screen
        SPG_DirtyClip(dest, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
    }

}


//==================================================================================
// Draws a textured quadrilateral
//==================================================================================
void SPG_QuadTex(SDL_Surface *dest, Sint16 x1,Sint16 y1,Sint16 x2,Sint16 y2,Sint16 x3,Sint16 y3,Sint16 x4,Sint16 y4,
                 SDL_Surface *source, Sint16 sx1,Sint16 sy1,Sint16 sx2,Sint16 sy2,Sint16 sx3,Sint16 sy3,Sint16 sx4,Sint16 sy4)
{
	Sint16 y;

	//if( y1==y3 || y1 == y4 || y4 == y2 )
	//	return;

	/* Sort the coords */
	if ( y1 > y2 ) {
		SWAP(x1,x2,y);
		SWAP(y1,y2,y);
		SWAP(sx1,sx2,y);
		SWAP(sy1,sy2,y);
	}
	if ( y2 > y3 ) {
		SWAP(x3,x2,y);
		SWAP(y3,y2,y);
		SWAP(sx3,sx2,y);
		SWAP(sy3,sy2,y);
	}
	if ( y1 > y2 ) {
		SWAP(x1,x2,y);
		SWAP(y1,y2,y);
		SWAP(sx1,sx2,y);
		SWAP(sy1,sy2,y);
	}
	if ( y3 > y4 ) {
		SWAP(x3,x4,y);
		SWAP(y3,y4,y);
		SWAP(sx3,sx4,y);
		SWAP(sy3,sy4,y);
	}
	if ( y2 > y3 ) {
		SWAP(x3,x2,y);
		SWAP(y3,y2,y);
		SWAP(sx3,sx2,y);
		SWAP(sy3,sy2,y);
	}
	if ( y1 > y2 ) {
		SWAP(x1,x2,y);
		SWAP(y1,y2,y);
		SWAP(sx1,sx2,y);
		SWAP(sy1,sy2,y);
	}

	/*
	 * We do this exactly like SPG_TexturedTrigon(), but here we must trace four lines.
	*/

	Sint32 xa = (Sint32)(x1<<16);
	Sint32 xb = xa;
	Sint32 xc = (Sint32)(x2<<16);
	Sint32 xd = (Sint32)(x3<<16);

	Sint32 m1 = 0;
	Sint32 m2 = (y3 != y1)? (Sint32)((x3 - x1)<<16)/(Sint32)(y3 - y1) : 400000;
	Sint32 m3 = (y4 != y2)? (Sint32)((x4 - x2)<<16)/(Sint32)(y4 - y2) : 400000;
	Sint32 m4 = 0;

	Sint32 srcx1 = (Sint32)(sx1<<16);
	Sint32 srcx2 = srcx1;
	Sint32 srcx3 = (Sint32)(sx2<<16);
	Sint32 srcx4 = (Sint32)(sx3<<16);

	Sint32 srcy1 = (Sint32)(sy1<<16);
	Sint32 srcy2 = srcy1;
	Sint32 srcy3 = (Sint32)(sy2<<16);
	Sint32 srcy4 = (Sint32)(sy3<<16);

	Sint32 xstep1 = 0;
	Sint32 xstep2 = (y3 != y1)? (Sint32)((sx3 - sx1) << 16) / (Sint32)(y3 - y1) : 400000;
	Sint32 xstep3 = (y4 != y2)? (Sint32)((sx4 - sx2) << 16) / (Sint32)(y4 - y2) : 400000;
	Sint32 xstep4 = 0;

	Sint32 ystep1 = 0;
	Sint32 ystep2 = (y3 != y1)? (Sint32)((sy3 - sy1) << 16) / (Sint32)(y3 - y1) : 400000;
	Sint32 ystep3 = (y4 != y2)? (Sint32)((sy4 - sy2) << 16) / (Sint32)(y4 - y2) : 400000;
	Sint32 ystep4 = 0;

    if ( spg_lock(dest) < 0 )
    {
        if(spg_useerrors)
            SPG_Error("SPG_QuadTex could not lock surface");
        return;
    }

	/* Upper bit of the rectangle */
	if( y1==y2 )
		spg_linehtex(dest,x1,y1,x2,source,sx1,sy1,sx2,sy2);
	else{
		m1 = (Sint32)((x2 - x1)<<16)/(Sint32)(y2 - y1);

		xstep1 = (Sint32)((sx2 - sx1) << 16) / (Sint32)(y2 - y1);
		ystep1 = (Sint32)((sy2 - sy1) << 16) / (Sint32)(y2 - y1);

		for ( y = y1; y <= y2; y++) {
			spg_linehtex(dest, xa>>16, y, xb>>16, source, srcx1>>16, srcy1>>16, srcx2>>16, srcy2>>16);

			xa += m1;
			xb += m2;

			srcx1 += xstep1;
			srcx2 += xstep2;
			srcy1 += ystep1;
			srcy2 += ystep2;
		}
	}

	/* Middle bit of the rectangle */
	for ( y = y2+1; y <= y3; y++) {
		spg_linehtex(dest, xb>>16, y, xc>>16, source, srcx2>>16, srcy2>>16, srcx3>>16, srcy3>>16);

		xb += m2;
		xc += m3;

		srcx2 += xstep2;
		srcx3 += xstep3;
		srcy2 += ystep2;
		srcy3 += ystep3;
	}

	/* Lower bit of the rectangle */
	if( y3==y4 )
		spg_linehtex(dest,x3,y3,x4,source,sx3,sy3,sx4,sy4);
	else{
		m4 = (Sint32)((x4 - x3)<<16)/(Sint32)(y4 - y3);

		xstep4 = (Sint32)((sx4 - sx3) << 16) / (Sint32)(y4 - y3);
		ystep4 = (Sint32)((sy4 - sy3) << 16) / (Sint32)(y4 - y3);

		for ( y = y3+1; y <= y4; y++) {
			spg_linehtex(dest, xc>>16, y, xd>>16, source, srcx3>>16, srcy3>>16, srcx4>>16, srcy4>>16);

			xc += m3;
			xd += m4;

			srcx3 += xstep3;
			srcx4 += xstep4;
			srcy3 += ystep3;
			srcy4 += ystep4;
		}

	}

	spg_unlock(dest);

    if(spg_makedirtyrects)
    {
        Sint16 xmax=x1, ymax=y1, xmin=x1, ymin=y1;
        xmax= (xmax>x2)? xmax : x2;  ymax= (ymax>y2)? ymax : y2;
        xmin= (xmin<x2)? xmin : x2;  ymin= (ymin<y2)? ymin : y2;
        xmax= (xmax>x3)? xmax : x3;  ymax= (ymax>y3)? ymax : y3;
        xmin= (xmin<x3)? xmin : x3;  ymin= (ymin<y3)? ymin : y3;
        xmax= (xmax>x4)? xmax : x4;  ymax= (ymax>y4)? ymax : y4;
        xmin= (xmin<x4)? xmin : x4;  ymin= (ymin<y4)? ymin : y4;
        
        SDL_Rect rect;
        rect.x = xmin;
        rect.y = ymin;
        rect.w = xmax - xmin + 1;
        rect.h = ymax - ymin + 1;
        // Clip it to the screen
        SPG_DirtyClip(dest, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
    }

}


void SPG_CopyPoints(Uint16 n, SPG_Point* points, SPG_Point* buffer)
{
    if(points == NULL || buffer == NULL)
        return;
    
    int i = 0;
    for(; i < n; ++i)
    {
        buffer[i].x = points[i].x;
        buffer[i].y = points[i].y;
    }
}

void SPG_RotatePointsXY(Uint16 n, SPG_Point* points, float cx, float cy, float angle)
{
    if(points == NULL)
        return;
    int i = 0;
    float dx, dy;
    if(spg_usedegrees)
        angle = angle*RADPERDEG;
    float ct = cos(angle), st = sin(angle);
    for(; i < n; ++i)
    {
        // Get distances from center
        dx = points[i].x - cx;
        dy = points[i].y - cy;
        // Simple 2d rotation
        points[i].x = dx*ct - dy*st + cx;
        points[i].y = dx*st + dy*ct + cy;
    }
    
}

void SPG_ScalePointsXY(Uint16 n, SPG_Point* points, float cx, float cy, float xscale, float yscale)
{
    if(points == NULL)
        return;
    int i = 0;
    float dx, dy;
    for(; i < n; ++i)
    {
        // Get distances from center
        dx = points[i].x - cx;
        dy = points[i].y - cy;
        // Simple 2d scale
        points[i].x = cx + xscale*dx;
        points[i].y = cy + yscale*dy;
    }
}

void SPG_SkewPointsXY(Uint16 n, SPG_Point* points, float cx, float cy, float xskew, float yskew)
{
    if(points == NULL)
        return;
    int i = 0;
    float dx, dy;
    for(; i < n; ++i)
    {
        // Get distances from center
        dx = points[i].x - cx;
        dy = points[i].y - cy;
        // Simple 2d skew
        points[i].x += xskew*dy;
        points[i].y += yskew*dx;
    }
}


void SPG_TranslatePoints(Uint16 n, SPG_Point* points, float x1, float y1)
{
    if(points == NULL)
        return;
    int i = 0;
    for(; i < n; ++i)
    {
        points[i].x += x1;
        points[i].y += y1;
    }
}







//==================================================================================
// And now to something completly different: Polygons!
//==================================================================================

void SPG_Polygon(SDL_Surface *dest, Uint16 n, SPG_Point* points, Uint32 color)
{
    if(points == NULL)
        return;
    if(n < 3)
    {
        if(spg_useerrors)
            SPG_Error("SPG_Polygon given n < 3");
        return;
    }

    if ( spg_lock(dest) < 0 )
    {
        if(spg_useerrors)
                SPG_Error("SPG_Polygon could not lock surface");
        return;
    }

    Sint16 ox = points[n-1].x;
	Sint16 oy = points[n-1].y;
	int i;
	if(spg_thickness == 1)
	{
        if(SPG_GetAA())
        {
            for(i = 0; i < n; i++)
            {
                spg_lineblendaa(dest,ox,oy,points[i].x,points[i].y,color, SDL_ALPHA_OPAQUE);
                ox = points[i].x;
                oy = points[i].y;
            }
        }
        else
        {
            for(i = 0; i < n; i++)
            {
                spg_line(dest,ox,oy,points[i].x,points[i].y,color);
                ox = points[i].x;
                oy = points[i].y;
            }
        }


        if(spg_makedirtyrects)
        {
            Sint16 xmax=points[0].x, ymax=points[0].y, xmin=points[0].x, ymin=points[0].y;
            int i = 0;
            for(; i < n; ++i)
            {
                xmax= (xmax>points[i].x)? xmax : points[i].x;  ymax= (ymax>points[i].y)? ymax : points[i].y;
                xmin= (xmin<points[i].x)? xmin : points[i].x;  ymin= (ymin<points[i].y)? ymin : points[i].y;
            }
            
            SDL_Rect rect;
            rect.x = xmin;
            rect.y = ymin;
            rect.w = xmax - xmin + 1;
            rect.h = ymax - ymin + 1;
            // Clip it to the screen
            SPG_DirtyClip(dest, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
	}
	else if(spg_thickness > 0)
	{        
        for(i = 0; i < n; i++)
        {
            SPG_LineFn(dest,ox,oy,points[i].x,points[i].y,color, spg_thicknesscallback);
            ox = points[i].x;
            oy = points[i].y;
        }


        if(spg_makedirtyrects)
        {
            Sint16 xmax=points[0].x, ymax=points[0].y, xmin=points[0].x, ymin=points[0].y;
            int i = 0;
            for(; i < n; ++i)
            {
                xmax= (xmax>points[i].x)? xmax : points[i].x;  ymax= (ymax>points[i].y)? ymax : points[i].y;
                xmin= (xmin<points[i].x)? xmin : points[i].x;  ymin= (ymin<points[i].y)? ymin : points[i].y;
            }
            
            SDL_Rect rect;
            rect.x = xmin - spg_thickness/2;
            rect.y = ymin - spg_thickness/2;
            rect.w = xmax - xmin + 1 + spg_thickness;
            rect.h = ymax - ymin + 1 + spg_thickness;
            // Clip it to the screen
            SPG_DirtyClip(dest, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
	}

    spg_unlock(dest);

}

void SPG_PolygonBlend(SDL_Surface *dest, Uint16 n, SPG_Point* points, Uint32 color, Uint8 alpha)
{
    if(points == NULL)
        return;
    if(n < 3)
    {
        if(spg_useerrors)
            SPG_Error("SPG_PolygonBlend given n < 3");
        return;
    }

    if ( spg_lock(dest) < 0 )
    {
        if(spg_useerrors)
            SPG_Error("SPG_PolygonBlend could not lock surface");
        return;
    }

    Sint16 ox = points[n-1].x;
	Sint16 oy = points[n-1].y;
	int i;
	if(spg_thickness == 1)
	{
        if(SPG_GetAA())
        {
            for(i = 0; i < n; i++)
            {
                spg_lineblendaa(dest,ox,oy,points[i].x,points[i].y,color, alpha);
                ox = points[i].x;
                oy = points[i].y;
            }
        }
        else
        {
            spg_alphahack = alpha;
            for(i = 0; i < n; i++)
            {
                SPG_LineFn(dest, ox, oy, points[i].x,points[i].y,color, spg_pixelcallbackalpha);
                ox = points[i].x;
                oy = points[i].y;
            }
        }

        if(spg_makedirtyrects)
        {
            Sint16 xmax=points[0].x, ymax=points[0].y, xmin=points[0].x, ymin=points[0].y;
            int i = 0;
            for(; i < n; ++i)
            {
                xmax= (xmax>points[i].x)? xmax : points[i].x;  ymax= (ymax>points[i].y)? ymax : points[i].y;
                xmin= (xmin<points[i].x)? xmin : points[i].x;  ymin= (ymin<points[i].y)? ymin : points[i].y;
            }
            
            SDL_Rect rect;
            rect.x = xmin;
            rect.y = ymin;
            rect.w = xmax - xmin + 1;
            rect.h = ymax - ymin + 1;
            // Clip it to the screen
            SPG_DirtyClip(dest, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
	}
	if(spg_thickness > 0)
	{
        
        spg_alphahack = alpha;
        for(i = 0; i < n; i++)
        {
            SPG_LineFn(dest, ox, oy, points[i].x,points[i].y,color, spg_thicknesscallbackalpha);
            ox = points[i].x;
            oy = points[i].y;
        }

        if(spg_makedirtyrects)
        {
            Sint16 xmax=points[0].x, ymax=points[0].y, xmin=points[0].x, ymin=points[0].y;
            int i = 0;
            for(; i < n; ++i)
            {
                xmax= (xmax>points[i].x)? xmax : points[i].x;  ymax= (ymax>points[i].y)? ymax : points[i].y;
                xmin= (xmin<points[i].x)? xmin : points[i].x;  ymin= (ymin<points[i].y)? ymin : points[i].y;
            }
            
            SDL_Rect rect;
            rect.x = xmin - spg_thickness/2;
            rect.y = ymin - spg_thickness/2;
            rect.w = xmax - xmin + 1 + spg_thickness;
            rect.h = ymax - ymin + 1 + spg_thickness;
            // Clip it to the screen
            SPG_DirtyClip(dest, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
	}

    spg_unlock(dest);

}

/* Base polygon structure */
typedef struct pline{
	Sint16 x1,x2, y1,y2;

	Sint32 fx, fm;

	Sint16 x;

	struct pline *next;

}pline;

void _pline_update(struct pline* pl)
{
    pl->x = (Sint16)(pl->fx>>16);
    pl->fx += pl->fm;
}

/* Pointer storage (to preserve polymorphism) */
typedef struct pline_p{
	pline *p;
}pline_p;

/* Radix sort */
pline* rsort(pline *inlist)
{
	if(!inlist)
		return NULL;

	// 16 radix-buckets
	pline* bucket[16] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
	pline* bi[16];     // bucket itterator (points to last element in bucket)

	pline *plist = inlist;

	int i,k;
	pline *j;
	Uint8 nr;

	// Radix sort in 4 steps (16-bit numbers)
	for( i = 0; i < 4; i++ ){
		for( j = plist; j; j = j->next ){
			nr = (Uint8)( ( (Uint16)(j->x+0x7fff) >> (4*i) ) & 0x000F);  // Get bucket number

			if( !bucket[nr] )
				bucket[nr] = j;   // First in bucket
			else
				bi[nr]->next = j; // Put last in bucket

			bi[nr] = j;           // Update bucket itterator
		}

		// Empty buckets (recombine list)
		j = NULL;
		for( k = 0; k < 16; k++ ){
			if( bucket[k] ){
				if( j )
					j->next = bucket[k]; // Connect elements in buckets
				else
					plist = bucket[k];   // First element

				j = bi[k];
			}
			bucket[k] = NULL;            // Empty
		}
		j->next = NULL;                  // Terminate list
	}

	return plist;
}

/* Calculate the scanline for y */
struct pline* get_scanline(pline_p *plist, Uint16 n, Sint32 y)
{
	struct pline* p = NULL;
	struct pline* list = NULL;
	struct pline* li = NULL;
    int i;
	for(i = 0; i < n; i++ ){
		// Is polyline on this scanline?
		p = plist[i].p;
		if( p->y1 <= y  &&  p->y2 >= y  &&  (p->y1 != p->y2) ){
			if( list )
				li->next = p; // Add last in list
			else
				list = p;     // Add first in list

			li = p;           // Update itterator

			// Calculate x
			_pline_update(p);
		}
	}

	if( li )
		li->next = NULL;  // terminate

	// Sort list
	return rsort(list);
}

/* Removes duplicates if needed */
static inline void remove_dup(pline *li, Sint16 y)
{
	if( li->next )
		if( (y==li->y1 || y==li->y2) && (y==li->next->y1 || y==li->next->y2) )
			if( ((y == li->y1)? -1:1) != ((y == li->next->y1)? -1:1) )
				li->next = li->next->next;
}


//==================================================================================
// Draws a n-points filled polygon
//==================================================================================

void SPG_PolygonFilledBlend(SDL_Surface *dest, Uint16 n, SPG_Point* points, Uint32 color, Uint8 alpha)
{
    if(points == NULL)
        return;
	if(n<3)
	{
	    if(spg_useerrors)
            SPG_Error("SPG_PolygonFilledBlend given n < 3");
		return;
	}

    if (spg_lock(dest) < 0)
    {
        if(spg_useerrors)
            SPG_Error("SPG_PolygonFilledBlend could not lock surface");
        return;
    }

	struct pline *line = (pline*)malloc(sizeof(struct pline)*n);//pline[n];
	struct pline_p *plist = (pline_p*)malloc(sizeof(struct pline_p)*n);

	Sint16 y1,y2, x1, x2, tmp, sy;
	Sint16 ymin = points[1].y, ymax=points[1].y;
	Sint16 xmin = points[1].x, xmax=points[1].x;
	Uint16 i;

	/* Decompose polygon into straight lines */
	for( i = 0; i < n; i++ ){
		x1 = points[i].x;
		y1 = points[i].y;

		if( i == n-1 ){
			// Last point == First point
			x2 = points[0].x;
			y2 = points[0].y;
		}else{
			x2 = points[i+1].x;
		 	y2 = points[i+1].y;
		}

		// Make sure y1 <= y2
		if( y1 > y2 ) {
			SWAP(y1,y2,tmp);
			SWAP(x1,x2,tmp);
		}

		// No longer Reject polygons with negative coords
                /*
		if( y1 < 0  ||  x1 < 0  ||  x2 < 0 ){
			spg_unlock(dest);

			delete[] line;
			delete[] plist;
			return;
		}
                */

		if( y1 < ymin )
			ymin = y1;
		if( y2 > ymax )
			ymax = y2;
		if( x1 < xmin )
			xmin = x1;
		else if( x1 > xmax )
			xmax = x1;
		if( x2 < xmin )
			xmin = x2;
		else if( x2 > xmax )
			xmax = x2;

		//Fill structure
		line[i].y1 = y1;
		line[i].y2 = y2;
		line[i].x1 = x1;
		line[i].x2 = x2;

		// Start x-value (fixed point)
		line[i].fx = (Sint32)(x1<<16);

		// Lines step value (fixed point)
		if( y1 != y2)
			line[i].fm = (Sint32)((x2 - x1)<<16)/(Sint32)(y2 - y1);
		else
			line[i].fm = 0;

		line[i].next = NULL;

		// Add to list
		plist[i].p = &line[i];

		// Draw the polygon outline (looks nicer)
		if( alpha == SDL_ALPHA_OPAQUE )
			spg_line(dest,x1,y1,x2,y2,color); // Can't do this with alpha, might overlap with the filling
	}

	/* Remove surface lock if spg_lineh() is to be used */
	if (alpha == SDL_ALPHA_OPAQUE)
		spg_unlock(dest);

	pline* list = NULL;
	pline* li = NULL;   // list itterator

	// Scan y-lines
	for( sy = ymin; sy <= ymax; sy++){
		list = get_scanline(plist, n, sy);

		if( !list )
			continue;     // nothing in list... hmmmm

		x1 = x2 = NULL_POSITION;

		// Draw horizontal lines between pairs
		for( li = list; li; li = li->next ){
			remove_dup(li, sy);

			if( x1 == NULL_POSITION )
				x1 = li->x+1;
			else if( x2 == NULL_POSITION )
				x2 = li->x;

			if( x1 != NULL_POSITION  &&  x2 !=NULL_POSITION ){
				if( x2-x1 < 0  && alpha == SDL_ALPHA_OPAQUE ){
					// Already drawn by the outline
					x1 = x2 = NULL_POSITION;
					continue;
				}

				if( alpha == SDL_ALPHA_OPAQUE )
					spg_lineh(dest, x1, sy, x2, color);
				else
					spg_linehblend(dest, x1-1, sy, x2, color, alpha);

				x1 = x2 = NULL_POSITION;
			}
		}
	}

	if (alpha != SDL_ALPHA_OPAQUE)
		spg_unlock(dest);

	free(line);
	free(plist);


    if(spg_makedirtyrects)
    {
        SDL_Rect rect;
        rect.x = xmin;
        rect.y = ymin;
        rect.w = xmax - xmin + 1;
        rect.h = ymax - ymin + 1;
        // Clip it to the screen
        SPG_DirtyClip(dest, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
    }
	return;
}


//==================================================================================
// Draws a n-points (AA) filled polygon
//==================================================================================

void spg_polygonfilledaa(SDL_Surface *dest, Uint16 n, SPG_Point* points, Uint32 color)
{
    if(points == NULL)
        return;
	if(n<3)
	{
	    if(spg_useerrors)
            SPG_Error("spg_polygonfilledaa given n < 3");
		return;
	}


    if (spg_lock(dest) < 0)
    {
        if(spg_useerrors)
            SPG_Error("spg_polygonfilledaa could not lock surface");
        return;
    }

	struct pline *line = (pline*)malloc(sizeof(struct pline)*n);
	struct pline_p *plist = (pline_p*)malloc(sizeof(struct pline_p)*n);

	Sint16 y1,y2, x1, x2, tmp, sy;
	Sint16 xmin = points[1].x, xmax=points[1].x;
	Sint16 ymin = points[1].y, ymax=points[1].y;
	Uint16 i;

	/* Decompose polygon into straight lines */
	for( i = 0; i < n; i++ ){
		y1 = points[i].y;
		x1 = points[i].x;

		if( i == n-1 ){
			// Last point == First point
			y2 = points[0].y;
			x2 = points[0].x;
		}else{
		 	y2 = points[i+1].y;
			x2 = points[i+1].x;
		}

		// Make sure y1 <= y2
		if( y1 > y2 ) {
			SWAP(y1,y2,tmp);
			SWAP(x1,x2,tmp);
		}

		// No longer Reject polygons with negative coords
                /*
		if( y1 < 0  ||  x1 < 0  ||  x2 < 0 ){
			spg_unlock(dest);

			delete[] line;
			delete[] plist;
			return;
		}
                */

		if( y1 < ymin )
			ymin = y1;
		if( y2 > ymax )
			ymax = y2;
		if( x1 < xmin )
			xmin = x1;
		else if( x1 > xmax )
			xmax = x1;
		if( x2 < xmin )
			xmin = x2;
		else if( x2 > xmax )
			xmax = x2;

		//Fill structure
		line[i].y1 = y1;
		line[i].y2 = y2;
		line[i].x1 = x1;
		line[i].x2 = x2;

		// Start x-value (fixed point)
		line[i].fx = (Sint32)(x1<<16);

		// Lines step value (fixed point)
		if( y1 != y2)
			line[i].fm = (Sint32)((x2 - x1)<<16)/(Sint32)(y2 - y1);
		else
			line[i].fm = 0;

		line[i].next = NULL;

		// Add to list
		plist[i].p = &line[i];

		// Draw AA Line
		spg_lineblendaa(dest,x1,y1,x2,y2,color, SDL_ALPHA_OPAQUE);
	}

    spg_unlock(dest);


	pline* list = NULL;
	pline* li = NULL;   // list itterator

	// Scan y-lines
	for( sy = ymin; sy <= ymax; sy++){
		list = get_scanline(plist, n, sy);

		if( !list )
			continue;     // nothing in list... hmmmm

		x1 = x2 = NULL_POSITION;

		// Draw horizontal lines between pairs
		for( li = list; li; li = li->next ){
			remove_dup(li, sy);

			if( x1 ==NULL_POSITION )
				x1 = li->x+1;
			else if( x2 ==NULL_POSITION )
				x2 = li->x;

			if( x1 != NULL_POSITION  &&  x2 !=NULL_POSITION ){
				if( x2-x1 < 0 ){
					x1 = x2 = NULL_POSITION;
					continue;
				}

				spg_lineh(dest, x1, sy, x2, color);

				x1 = x2 = NULL_POSITION;
			}
		}
	}

	free(line);
	free(plist);
    
    if(spg_makedirtyrects)
    {
        SDL_Rect rect;
        rect.x = xmin;
        rect.y = ymin;
        rect.w = xmax - xmin + 1;
        rect.h = ymax - ymin + 1;
        // Clip it to the screen
        SPG_DirtyClip(dest, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
    }

	return;
}


void SPG_PolygonFilled(SDL_Surface *dest, Uint16 n, SPG_Point* points, Uint32 color)
{
    if(SPG_GetAA())
        spg_polygonfilledaa(dest, n, points, color);
    else
        SPG_PolygonFilledBlend(dest, n, points, color, SDL_ALPHA_OPAQUE);
}





//==================================================================================
// Draws a n-points Gouraud shaded polygon
//==================================================================================

/* faded polygon structure */
typedef struct fpline{
    Sint16 x1,x2, y1,y2;

	Sint32 fx, fm;

	Sint16 x;

	struct fpline *next;
	
	Uint8 r1, r2;
	Uint8 g1, g2;
	Uint8 b1, b2;

	Uint32 fr, fg, fb;
	Sint32 fmr, fmg, fmb;

	Uint8 r,g,b;

}fpline;

void _fpline_update(struct fpline* fpl)
{
    fpl->x = (Sint16)(fpl->fx>>16);
    fpl->fx += fpl->fm;

    fpl->r = (Uint8)(fpl->fr>>16);
    fpl->g = (Uint8)(fpl->fg>>16);
    fpl->b = (Uint8)(fpl->fb>>16);

    fpl->fr += fpl->fmr;
    fpl->fg += fpl->fmg;
    fpl->fb += fpl->fmb;
}

typedef struct fpline_p{
	fpline *p;
}fpline_p;
static inline void remove_dupf(fpline *li, Sint16 y)
{
	if( li->next )
		if( (y==li->y1 || y==li->y2) && (y==li->next->y1 || y==li->next->y2) )
			if( ((y == li->y1)? -1:1) != ((y == li->next->y1)? -1:1) )
				li->next = li->next->next;
}/* Radix sort */
fpline* rsortf(fpline *inlist)
{
	if(!inlist)
		return NULL;

	// 16 radix-buckets
	fpline* bucket[16] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
	fpline* bi[16];     // bucket itterator (points to last element in bucket)

	fpline *plist = inlist;

	int i,k;
	fpline *j;
	Uint8 nr;

	// Radix sort in 4 steps (16-bit numbers)
	for( i = 0; i < 4; i++ ){
		for( j = plist; j; j = j->next ){
			nr = (Uint8)( ( (Uint16)(j->x+0x7fff) >> (4*i) ) & 0x000F);  // Get bucket number

			if( !bucket[nr] )
				bucket[nr] = j;   // First in bucket
			else
				bi[nr]->next = j; // Put last in bucket

			bi[nr] = j;           // Update bucket itterator
		}

		// Empty buckets (recombine list)
		j = NULL;
		for( k = 0; k < 16; k++ ){
			if( bucket[k] ){
				if( j )
					j->next = bucket[k]; // Connect elements in buckets
				else
					plist = bucket[k];   // First element

				j = bi[k];
			}
			bucket[k] = NULL;            // Empty
		}
		j->next = NULL;                  // Terminate list
	}

	return plist;
}

/* Calculate the scanline for y */
struct fpline* get_scanlinef(fpline_p *plist, Uint16 n, Sint32 y)
{
	struct fpline* p = NULL;
	struct fpline* list = NULL;
	struct fpline* li = NULL;
    int i;
	for(i = 0; i < n; i++ ){
		// Is polyline on this scanline?
		p = plist[i].p;
		if( p->y1 <= y  &&  p->y2 >= y  &&  (p->y1 != p->y2) ){
			if( list )
				li->next = p; // Add last in list
			else
				list = p;     // Add first in list

			li = p;           // Update iterator

			// Calculate x
			_fpline_update(p);
		}
	}

	if( li )
		li->next = NULL;  // terminate

	// Sort list
	return rsortf(list);
}

void SPG_PolygonFadeBlend(SDL_Surface *dest, Uint16 n, SPG_Point* points, Uint32* colors, Uint8 alpha)
{
    if(points == NULL)
        return;
	if(n<3)
	{
	    if(spg_useerrors)
            SPG_Error("SPG_PolygonFadeBlend given n < 3");
		return;
	}

    if (spg_lock(dest) < 0)
    {
        if(spg_useerrors)
            SPG_Error("SPG_PolygonFadeBlend could not lock surface");
        return;
    }

	struct fpline *line = (fpline*)malloc(sizeof(struct fpline)*n);
	struct fpline_p *plist = (fpline_p*)malloc(sizeof(struct fpline_p)*n);

	Sint16 y1,y2, x1, x2, tmp, sy;
	Sint16 ymin = points[1].y, ymax=points[1].y;
	Sint16 xmin = points[1].x, xmax=points[1].x;
	Uint16 i;
	Uint8 r1=0, g1=0, b1=0, r2=0, g2=0, b2=0, t;

	// Decompose polygon into straight lines
	for( i = 0; i < n; i++ )
	{
		y1 = points[i].y;
		x1 = points[i].x;
		if(dest->format->BitsPerPixel == 8)
		{
		    SDL_GetRGB(colors[i], dest->format, &r1, &g1, &b1);
		}
		else
		{
            r1 = (colors[i] & dest->format->Rmask) >> dest->format->Rshift;
            g1 = (colors[i] & dest->format->Gmask) >> dest->format->Gshift;
            b1 = (colors[i] & dest->format->Bmask) >> dest->format->Bshift;
		}

		if( i == n-1 )
		{
			// Last point == First point
			y2 = points[0].y;
			x2 = points[0].x;
            if(dest->format->BitsPerPixel == 8)
            {
                SDL_GetRGB(colors[0], dest->format, &r2, &g2, &b2);
            }
            else
            {
                r2 = (colors[0] & dest->format->Rmask) >> dest->format->Rshift;
                g2 = (colors[0] & dest->format->Gmask) >> dest->format->Gshift;
                b2 = (colors[0] & dest->format->Bmask) >> dest->format->Bshift;
            }
		}
		else
		{
		 	y2 = points[i+1].y;
			x2 = points[i+1].x;
			
            if(dest->format->BitsPerPixel == 8)
            {
                SDL_GetRGB(colors[i+1], dest->format, &r2, &g2, &b2);
            }
            else
            {
                r2 = (colors[i+1] & dest->format->Rmask) >> dest->format->Rshift;
                g2 = (colors[i+1] & dest->format->Gmask) >> dest->format->Gshift;
                b2 = (colors[i+1] & dest->format->Bmask) >> dest->format->Bshift;
            }
		}

		// Make sure y1 <= y2
		if( y1 > y2 ) {
			SWAP(y1,y2,tmp);
			SWAP(x1,x2,tmp);
			SWAP(r1,r2,t);
			SWAP(g1,g2,t);
			SWAP(b1,b2,t);
		}

		// No longer Reject polygons with negative coords
                /*
		if( y1 < 0  ||  x1 < 0  ||  x2 < 0 ){
			spg_unlock(dest);

			delete[] line;
			delete[] plist;
			return;
		}
                */

		if( y1 < ymin )
			ymin = y1;
		if( y2 > ymax )
			ymax = y2;
		if( x1 < xmin )
			xmin = x1;
		else if( x1 > xmax )
			xmax = x1;
		if( x2 < xmin )
			xmin = x2;
		else if( x2 > xmax )
			xmax = x2;

		//Fill structure
		line[i].y1 = y1;
		line[i].y2 = y2;
		line[i].x1 = x1;
		line[i].x2 = x2;
		line[i].r1 = r1;
		line[i].g1 = g1;
		line[i].b1 = b1;
		line[i].r2 = r2;
		line[i].g2 = g2;
		line[i].b2 = b2;

		// Start x-value (fixed point)
		line[i].fx = (Sint32)(x1<<16);

		line[i].fr = (Uint32)(r1<<16);
		line[i].fg = (Uint32)(g1<<16);
		line[i].fb = (Uint32)(b1<<16);

		// Lines step value (fixed point)
		if( y1 != y2){
			line[i].fm  = (Sint32)((x2 - x1)<<16)/(Sint32)(y2 - y1);

			line[i].fmr = (Sint32)((r2 - r1)<<16)/(Sint32)(y2 - y1);
			line[i].fmg = (Sint32)((g2 - g1)<<16)/(Sint32)(y2 - y1);
			line[i].fmb = (Sint32)((b2 - b1)<<16)/(Sint32)(y2 - y1);
		}
		else
		{
			line[i].fm  = 0;
			line[i].fmr = 0;
			line[i].fmg = 0;
			line[i].fmb = 0;
		}

		line[i].next = NULL;

		// Add to list
		plist[i].p = &line[i];

		// Draw the polygon outline (looks nicer)
		if( alpha == SDL_ALPHA_OPAQUE )
			SPG_LineFadeFn(dest,x1,y1,x2,y2,SDL_MapRGB(dest->format, r1,g1,b1),SDL_MapRGB(dest->format, r2,g2,b2), spg_pixel); // Can't do this with alpha, might overlap with the filling
	}

	fpline* list = NULL;
	fpline* li = NULL;   // list iterator

	// Scan y-lines
	for( sy = ymin; sy <= ymax; sy++){
		list = (fpline *)get_scanlinef(plist, n, sy);

		if( !list )
			continue;     // nothing in list... hmmmm

		x1 = x2 = NULL_POSITION;

		// Draw horizontal lines between pairs
		for( li = list; li; li = (fpline *)li->next ){
			remove_dupf(li, sy);

			if( x1 == NULL_POSITION ){
				x1 = li->x+1;
				r1 = li->r;
				g1 = li->g;
				b1 = li->b;
			}else if( x2 == NULL_POSITION ){
				x2 = li->x;
				r2 = li->r;
				g2 = li->g;
				b2 = li->b;
			}

			if( x1 !=NULL_POSITION  &&  x2 !=NULL_POSITION ){
				if( x2-x1 < 0 && alpha == SDL_ALPHA_OPAQUE){
					x1 = x2 = NULL_POSITION;
					continue;
				}

				if( alpha == SDL_ALPHA_OPAQUE )
					spg_linehfade(dest, x1, sy, x2, r1, g1, b1, r2, g2, b2);
				else{
					spg_alphahack = alpha;
					SPG_LineFadeFn(dest, x1-1, sy, x2, sy, SDL_MapRGB(dest->format, r1, g1, b1), SDL_MapRGB(dest->format, r2, g2, b2), spg_pixelcallbackalpha);
				}

				x1 = x2 = NULL_POSITION;
			}
		}
	}

    spg_unlock(dest);

	free(line);
	free(plist);

    if(spg_makedirtyrects)
    {
        SDL_Rect rect;
        rect.x = xmin;
        rect.y = ymin;
        rect.w = xmax - xmin + 1;
        rect.h = ymax - ymin + 1;
        // Clip it to the screen
        SPG_DirtyClip(dest, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
    }
	return;
}

//==================================================================================
// Draws a n-points (AA) gourand shaded polygon
//==================================================================================
void spg_polygonfadeaa(SDL_Surface *dest, Uint16 n, SPG_Point* points, Uint32* colors)
{
    if(points == NULL)
        return;
	if(n<3)
	{
	    if(spg_useerrors)
            SPG_Error("spg_polygonfadeaa given n < 3");
		return;
	}

    if (spg_lock(dest) < 0)
    {
        if(spg_useerrors)
            SPG_Error("spg_polygonfadeaa could not lock surface");
        return;
    }

	struct fpline *line = (fpline*)malloc(sizeof(struct fpline)*n);
	struct fpline_p *plist = (fpline_p*)malloc(sizeof(struct fpline_p)*n);

	Sint16 y1,y2, x1, x2, tmp, sy;
	Sint16 ymin = points[1].y, ymax=points[1].y;
	Sint16 xmin = points[1].x, xmax=points[1].x;
	Uint16 i;
	Uint8 r1=0, g1=0, b1=0, r2=0, g2=0, b2=0, t;

	// Decompose polygon into straight lines
	for( i = 0; i < n; i++ ){
		y1 = points[i].y;
		x1 = points[i].x;
		if(dest->format->BitsPerPixel == 8)
		{
		    SDL_GetRGB(colors[i], dest->format, &r1, &g1, &b1);
		}
		else
		{
            r1 = (colors[i] & dest->format->Rmask) >> dest->format->Rshift;
            g1 = (colors[i] & dest->format->Gmask) >> dest->format->Gshift;
            b1 = (colors[i] & dest->format->Bmask) >> dest->format->Bshift;
		}

		if( i == n-1 ){
			// Last point == First point
			y2 = points[0].y;
			x2 = points[0].x;
            if(dest->format->BitsPerPixel == 8)
            {
                SDL_GetRGB(colors[0], dest->format, &r2, &g2, &b2);
            }
            else
            {
                r2 = (colors[0] & dest->format->Rmask) >> dest->format->Rshift;
                g2 = (colors[0] & dest->format->Gmask) >> dest->format->Gshift;
                b2 = (colors[0] & dest->format->Bmask) >> dest->format->Bshift;
            }
		}else{
		 	y2 = points[i+1].y;
			x2 = points[i+1].x;
            if(dest->format->BitsPerPixel == 8)
            {
                SDL_GetRGB(colors[i+1], dest->format, &r2, &g2, &b2);
            }
            else
            {
                r2 = (colors[i+1] & dest->format->Rmask) >> dest->format->Rshift;
                g2 = (colors[i+1] & dest->format->Gmask) >> dest->format->Gshift;
                b2 = (colors[i+1] & dest->format->Bmask) >> dest->format->Bshift;
            }
		}

		// Make sure y1 <= y2
		if( y1 > y2 ) {
			SWAP(y1,y2,tmp);
			SWAP(x1,x2,tmp);
			SWAP(r1,r2,t);
			SWAP(g1,g2,t);
			SWAP(b1,b2,t);
		}

		// No longer Reject polygons with negative coords
                /*
		if( y1 < 0  ||  x1 < 0  ||  x2 < 0 ){
			
            spg_unlock(dest);

			delete[] line;
			delete[] plist;
			return;
		}
                */

		if( y1 < ymin )
			ymin = y1;
		if( y2 > ymax )
			ymax = y2;
		if( x1 < xmin )
			xmin = x1;
		else if( x1 > xmax )
			xmax = x1;
		if( x2 < xmin )
			xmin = x2;
		else if( x2 > xmax )
			xmax = x2;

		//Fill structure
		line[i].y1 = y1;
		line[i].y2 = y2;
		line[i].x1 = x1;
		line[i].x2 = x2;
		line[i].r1 = r1;
		line[i].g1 = g1;
		line[i].b1 = b1;
		line[i].r2 = r2;
		line[i].g2 = g2;
		line[i].b2 = b2;

		// Start x-value (fixed point)
		line[i].fx = (Sint32)(x1<<16);

		line[i].fr = (Uint32)(r1<<16);
		line[i].fg = (Uint32)(g1<<16);
		line[i].fb = (Uint32)(b1<<16);

		// Lines step value (fixed point)
		if( y1 != y2){
			line[i].fm  = (Sint32)((x2 - x1)<<16)/(Sint32)(y2 - y1);

			line[i].fmr = (Sint32)((r2 - r1)<<16)/(Sint32)(y2 - y1);
			line[i].fmg = (Sint32)((g2 - g1)<<16)/(Sint32)(y2 - y1);
			line[i].fmb = (Sint32)((b2 - b1)<<16)/(Sint32)(y2 - y1);
		}else{
		    // Is this really right?  I personally would set them very high...
			line[i].fm  = 0;
			line[i].fmr = 0;
			line[i].fmg = 0;
			line[i].fmb = 0;
		}

		line[i].next = NULL;

		// Add to list
		plist[i].p = &line[i];

		// Draw the polygon outline (AA)
		spg_linefadeblendaa(dest,x1,y1,x2,y2,SDL_MapRGB(dest->format, r1,g1,b1), SDL_ALPHA_OPAQUE,SDL_MapRGB(dest->format, r2,g2,b2), SDL_ALPHA_OPAQUE);
	}

	fpline* list = NULL;
	fpline* li = NULL;   // list itterator

	// Scan y-lines
	for( sy = ymin; sy <= ymax; sy++){
		list = (fpline *)get_scanlinef(plist, n, sy);

		if( !list )
			continue;     // nothing in list... hmmmm

		x1 = x2 = NULL_POSITION;

		// Draw horizontal lines between pairs
		for( li = list; li; li = (fpline *)li->next ){
			remove_dupf(li, sy);

			if( x1 < 0 ){
				x1 = li->x+1;
				r1 = li->r;
				g1 = li->g;
				b1 = li->b;
			}else if( x2 < 0 ){
				x2 = li->x;
				r2 = li->r;
				g2 = li->g;
				b2 = li->b;
			}

			if( x1 != NULL_POSITION &&  x2 !=NULL_POSITION ){
				if( x2-x1 < 0 ){
					x1 = x2 = NULL_POSITION;
					continue;
				}

				spg_linehfade(dest, x1, sy, x2, r1, g1, b1, r2, g2, b2);

				x1 = x2 = NULL_POSITION;
			}
		}
	}

    spg_unlock(dest);

	free(line);
	free(plist);
    
    if(spg_makedirtyrects)
    {
        SDL_Rect rect;
        rect.x = xmin;
        rect.y = ymin;
        rect.w = xmax - xmin + 1;
        rect.h = ymax - ymin + 1;
        // Clip it to the screen
        SPG_DirtyClip(dest, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
    }


	return;
}

void SPG_PolygonFade(SDL_Surface *dest, Uint16 n, SPG_Point* points, Uint32* colors)
{
    if(SPG_GetAA())
        spg_polygonfadeaa(dest, n, points, colors);
    else
        SPG_PolygonFadeBlend(dest, n, points, colors, SDL_ALPHA_OPAQUE);
}

