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
#include <stdlib.h>


void spg_pixel(SDL_Surface *surface, Sint16 x, Sint16 y, Uint32 color);
void spg_pixelblend(SDL_Surface *surface, Sint16 x, Sint16 y, Uint32 color, Uint8 alpha);

/* Globals */
extern SPG_bool spg_useerrors;
extern SPG_bool spg_usedegrees;
extern SPG_bool spg_makedirtyrects;
extern SPG_DirtyTable* spg_dirtytable_front;

Uint16 spg_thickness = 1;
Uint8 spg_alphahack = 0;





void spg_thicknesscallback(SDL_Surface *Surf, Sint16 X, Sint16 Y, Uint32 Color)
{
    if(spg_thickness > 0)
    {
        // Disable autolock and dirty rects
        SPG_bool lock = spg_autolock;
        spg_autolock = 0;
        SPG_bool dirty = spg_makedirtyrects;
        spg_makedirtyrects = 0;
        SPG_CircleFilled(Surf, X, Y, (spg_thickness-1)/2.0f, Color);
        spg_autolock = lock;
        spg_makedirtyrects = dirty;
    }
}

void spg_thicknesscallbackalpha(SDL_Surface *Surf, Sint16 X, Sint16 Y, Uint32 Color)
{
    if(spg_thickness > 0)
    {
        // Disable autolock and dirty rects
        SPG_bool lock = spg_autolock;
        spg_autolock = 0;
        SPG_bool dirty = spg_makedirtyrects;
        spg_makedirtyrects = 0;
        SPG_CircleFilledBlend(Surf, X, Y, (spg_thickness-1)/2.0f, Color, spg_alphahack);
        spg_autolock = lock;
        spg_makedirtyrects = dirty;
    }
}



/**********************************************************************************/
/**                            Pixel functions                                   **/
/**********************************************************************************/

//==================================================================================
// Fast set pixel
//==================================================================================
void spg_pixel(SDL_Surface *surface, Sint16 x, Sint16 y, Uint32 color)
{
	if(x>=SPG_CLIP_XMIN(surface) && x<=SPG_CLIP_XMAX(surface) && y>=SPG_CLIP_YMIN(surface) && y<=SPG_CLIP_YMAX(surface)){
		switch (surface->format->BytesPerPixel) {
			case 1: { /* Assuming 8-bpp */
				*((Uint8 *)surface->pixels + y*surface->pitch + x) = color;
			}
			break;

			case 2: { /* Probably 15-bpp or 16-bpp */
				*((Uint16 *)surface->pixels + y*surface->pitch/2 + x) = color;
			}
			break;

			case 3: { /* Slow 24-bpp mode, usually not used */
				Uint8 *pix = (Uint8 *)surface->pixels + y * surface->pitch + x*3;

  				/* Gack - slow, but endian correct */
				*(pix+surface->format->Rshift/8) = color>>surface->format->Rshift;
  				*(pix+surface->format->Gshift/8) = color>>surface->format->Gshift;
  				*(pix+surface->format->Bshift/8) = color>>surface->format->Bshift;
				*(pix+surface->format->Ashift/8) = color>>surface->format->Ashift;
			}
			break;

			case 4: { /* Probably 32-bpp */
				*((Uint32 *)surface->pixels + y*surface->pitch/4 + x) = color;
			}
			break;
		}
	}

}


//==================================================================================
// Fast set pixel (RGB)
//==================================================================================
void spg_pixelRGB(SDL_Surface *surface, Sint16 x, Sint16 y, Uint8 R, Uint8 G, Uint8 B)
{
	spg_pixel(surface,x,y, SDL_MapRGB(surface->format, R, G, B));
}


//==================================================================================
// Fastest set pixel functions (don't mess up indata, thank you)
//==================================================================================
void spg_pixel8(SDL_Surface *surface, Sint16 x, Sint16 y, Uint32 color)
{
	*((Uint8 *)surface->pixels + y*surface->pitch + x) = color;
}
void spg_pixel16(SDL_Surface *surface, Sint16 x, Sint16 y, Uint32 color)
{
	*((Uint16 *)surface->pixels + y*surface->pitch/2 + x) = color;
}
void spg_pixel24(SDL_Surface *surface, Sint16 x, Sint16 y, Uint32 color)
{
	Uint8 *pix = (Uint8 *)surface->pixels + y * surface->pitch + x*3;

  	/* Gack - slow, but endian correct */
	*(pix+surface->format->Rshift/8) = color>>surface->format->Rshift;
  	*(pix+surface->format->Gshift/8) = color>>surface->format->Gshift;
  	*(pix+surface->format->Bshift/8) = color>>surface->format->Bshift;
	*(pix+surface->format->Ashift/8) = color>>surface->format->Ashift;
}
void spg_pixel32(SDL_Surface *surface, Sint16 x, Sint16 y, Uint32 color)
{
	*((Uint32 *)surface->pixels + y*surface->pitch/4 + x) = color;
}
void spg_pixelX(SDL_Surface *dest,Sint16 x,Sint16 y,Uint32 color)
{
	switch ( dest->format->BytesPerPixel ) {
	case 1:
		*((Uint8 *)dest->pixels + y*dest->pitch + x) = color;
		break;
	case 2:
		*((Uint16 *)dest->pixels + y*dest->pitch/2 + x) = color;
		break;
	case 3:
		spg_pixel24(dest,x,y,color);
		break;
	case 4:
		*((Uint32 *)dest->pixels + y*dest->pitch/4 + x) = color;
		break;
	}
}



//==================================================================================
// Safe set pixel
//==================================================================================
void SPG_Pixel(SDL_Surface *surface, Sint16 x, Sint16 y, Uint32 color)
{
    if(spg_lock(surface) < 0)
    {
        if(spg_useerrors)
            SPG_Error("SPG_Pixel could not lock surface");
        return;
    }

    if(spg_thickness == 1)
    {
        spg_pixel(surface, x, y, color);
        
        if(spg_makedirtyrects)
        {
            SDL_Rect rect;
            rect.x = x;
            rect.y = y;
            rect.w = 1;
            rect.h = 1;
            SPG_DirtyClip(surface, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
    }
    else
    {
        if(spg_thickness == 0) return;
        spg_thicknesscallback(surface, x, y, color);
        if(spg_makedirtyrects)
        {
            SDL_Rect rect;
            rect.w = spg_thickness;
            rect.h = spg_thickness;
            rect.x = x - rect.w/2;
            rect.y = y - rect.h/2;
            SPG_DirtyClip(surface, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
    }
    
    spg_unlock(surface);

}

//==================================================================================
// Put pixel with alpha blending
//==================================================================================
void spg_pixelblend(SDL_Surface *surface, Sint16 x, Sint16 y, Uint32 color, Uint8 alpha)
{
	if(x>=SPG_CLIP_XMIN(surface) && x<=SPG_CLIP_XMAX(surface) && y>=SPG_CLIP_YMIN(surface) && y<=SPG_CLIP_YMAX(surface)){
		Uint32 Rmask = surface->format->Rmask, Gmask = surface->format->Gmask, Bmask = surface->format->Bmask, Amask = surface->format->Amask;
		Uint32 R,G,B,A=0;//SDL_ALPHA_OPAQUE;
	Uint32* pixel;
		switch (surface->format->BytesPerPixel) {
			case 1: { /* Assuming 8-bpp */
			    
					Uint8 *pixel = (Uint8 *)surface->pixels + y*surface->pitch + x;
					
					Uint8 dR = surface->format->palette->colors[*pixel].r;
					Uint8 dG = surface->format->palette->colors[*pixel].g;
					Uint8 dB = surface->format->palette->colors[*pixel].b;
					Uint8 sR = surface->format->palette->colors[color].r;
					Uint8 sG = surface->format->palette->colors[color].g;
					Uint8 sB = surface->format->palette->colors[color].b;
					
                    dR = dR + ((sR-dR)*alpha >> 8);
                    dG = dG + ((sG-dG)*alpha >> 8);
                    dB = dB + ((sB-dB)*alpha >> 8);
				
					*pixel = SDL_MapRGB(surface->format, dR, dG, dB);
					
			}
			break;

			case 2: { /* Probably 15-bpp or 16-bpp */		
			    
					Uint16 *pixel = (Uint16 *)surface->pixels + y*surface->pitch/2 + x;
					Uint32 dc = *pixel;
				
					R = ((dc & Rmask) + (( (color & Rmask) - (dc & Rmask) ) * alpha >> 8)) & Rmask;
					G = ((dc & Gmask) + (( (color & Gmask) - (dc & Gmask) ) * alpha >> 8)) & Gmask;
					B = ((dc & Bmask) + (( (color & Bmask) - (dc & Bmask) ) * alpha >> 8)) & Bmask;
					if( Amask )
						A = ((dc & Amask) + (( (color & Amask) - (dc & Amask) ) * alpha >> 8)) & Amask;

					*pixel= R | G | B | A;
					
			}
			break;

			case 3: { /* Slow 24-bpp mode, usually not used */
				Uint8 *pix = (Uint8 *)surface->pixels + y * surface->pitch + x*3;
				Uint8 rshift8=surface->format->Rshift/8;
				Uint8 gshift8=surface->format->Gshift/8;
				Uint8 bshift8=surface->format->Bshift/8;
				Uint8 ashift8=surface->format->Ashift/8;
				
				
				
					Uint8 dR, dG, dB, dA=0;
					Uint8 sR, sG, sB, sA=0;
					
					pix = (Uint8 *)surface->pixels + y * surface->pitch + x*3;
					
					dR = *((pix)+rshift8); 
            		dG = *((pix)+gshift8);
            		dB = *((pix)+bshift8);
					dA = *((pix)+ashift8);
					
					sR = (color>>surface->format->Rshift)&0xff;
					sG = (color>>surface->format->Gshift)&0xff;
					sB = (color>>surface->format->Bshift)&0xff;
					sA = (color>>surface->format->Ashift)&0xff;
					
					dR = dR + ((sR-dR)*alpha >> 8);
					dG = dG + ((sG-dG)*alpha >> 8);
					dB = dB + ((sB-dB)*alpha >> 8);
					dA = dA + ((sA-dA)*alpha >> 8);

					*((pix)+rshift8) = dR; 
            		*((pix)+gshift8) = dG;
            		*((pix)+bshift8) = dB;
					*((pix)+ashift8) = dA;
					
			}
			break;

			case 4: /* Probably 32-bpp */
                pixel = (Uint32*)surface->pixels + y*surface->pitch/4 + x;
                Uint32 dc = *pixel;
                R = color & Rmask;
                G = color & Gmask;
                B = color & Bmask;
                A = 0;  // keep this as 0 to avoid corruption of non-alpha surfaces
                
                
                switch(SPG_GetBlend())
                {
                    case SPG_COMBINE_ALPHA:  // Blend and combine src and dest alpha
                        if( alpha != SDL_ALPHA_OPAQUE ){
                            R = ((dc & Rmask) + (( R - (dc & Rmask) ) * alpha >> 8)) & Rmask;
                            G = ((dc & Gmask) + (( G - (dc & Gmask) ) * alpha >> 8)) & Gmask;
                            B = ((dc & Bmask) + (( B - (dc & Bmask) ) * alpha >> 8)) & Bmask;
                        }
                        if(Amask)
                            A = ((((dc & Amask) >> surface->format->Ashift) + alpha) >> 1) << surface->format->Ashift;
                        break;
                    case SPG_DEST_ALPHA:  // Blend and keep dest alpha
                        if( alpha != SDL_ALPHA_OPAQUE ){
                            R = ((dc & Rmask) + (( R - (dc & Rmask) ) * alpha >> 8)) & Rmask;
                            G = ((dc & Gmask) + (( G - (dc & Gmask) ) * alpha >> 8)) & Gmask;
                            B = ((dc & Bmask) + (( B - (dc & Bmask) ) * alpha >> 8)) & Bmask;
                        }
                        if(Amask)
                            A = (dc & Amask);
                        break;
                    case SPG_SRC_ALPHA:  // Blend and keep src alpha
                        if( alpha != SDL_ALPHA_OPAQUE ){
                            R = ((dc & Rmask) + (( R - (dc & Rmask) ) * alpha >> 8)) & Rmask;
                            G = ((dc & Gmask) + (( G - (dc & Gmask) ) * alpha >> 8)) & Gmask;
                            B = ((dc & Bmask) + (( B - (dc & Bmask) ) * alpha >> 8)) & Bmask;
                        }
                        if(Amask)
                            A = (alpha << surface->format->Ashift);
                        break;
                    case SPG_COPY_SRC_ALPHA: // Direct copy with src alpha
                        if(Amask)
                            A = (alpha << surface->format->Ashift);
                        break;
                    case SPG_COPY_DEST_ALPHA: // Direct copy with dest alpha
                        if(Amask)
                            A = (dc & Amask);
                        break;
                    case SPG_COPY_COMBINE_ALPHA: // Direct copy with combined alpha
                        if(Amask)
                            A = ((((dc & Amask) >> surface->format->Ashift) + alpha) >> 1) << surface->format->Ashift;
                        break;
                    case SPG_COPY_NO_ALPHA:  // Direct copy, alpha opaque
                        if(Amask)
                            A = (SDL_ALPHA_OPAQUE << surface->format->Ashift);
                        break;
                    case SPG_COPY_ALPHA_ONLY:  // Direct copy of just the alpha
                        R = dc & Rmask;
                        G = dc & Gmask;
                        B = dc & Bmask;
                        if(Amask)
                            A = (alpha << surface->format->Ashift);
                        break;
                    case SPG_COMBINE_ALPHA_ONLY:  // Blend of just the alpha
                        R = dc & Rmask;
                        G = dc & Gmask;
                        B = dc & Bmask;
                        if(Amask)
                            A = ((((dc & Amask) >> surface->format->Ashift) + alpha) >> 1) << surface->format->Ashift;
                        break;
                    case SPG_REPLACE_COLORKEY:  // Replace the colorkeyed color
                        if(!(surface->flags & SDL_SRCCOLORKEY) || dc != surface->format->colorkey)
                            return;
                        if(Amask)
                            A = (alpha << surface->format->Ashift);
                        break;
                }
                
                *pixel = R | G | B | A;
			break;
		}
	}
}

void SPG_PixelBlend(SDL_Surface *surface, Sint16 x, Sint16 y, Uint32 color, Uint8 alpha)
{
    if(spg_lock(surface) < 0)
    {
        if(spg_useerrors)
            SPG_Error("SPG_PixelBlend could not lock surface");
        return;
    }

    if(spg_thickness == 1)
    {
        spg_pixelblend(surface,x,y,color, alpha);
        /* unlock the display */
        
        if(spg_makedirtyrects)
        {
            SDL_Rect rect;
            rect.x = x;
            rect.y = y;
            rect.w = 1;
            rect.h = 1;
            // Clip it to the screen
            SPG_DirtyClip(surface, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
    }
    else
    {
        if(spg_thickness == 0) return;
        spg_alphahack = alpha;
        spg_thicknesscallbackalpha(surface, x, y, color);
        
        if(spg_makedirtyrects)
        {
            SDL_Rect rect;
            rect.w = spg_thickness;
            rect.h = spg_thickness;
            rect.x = x - rect.w/2;
            rect.y = y - rect.h/2;
            SPG_DirtyClip(surface, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
    }
    
    spg_unlock(surface);
	
}

void SPG_PixelPattern(SDL_Surface *surface, SDL_Rect target, SPG_bool* pattern, Uint32* colors)
{
    if(spg_lock(surface) < 0)
    {
        if(spg_useerrors)
            SPG_Error("SPG_PixelPattern could not lock surface");
        return;
    }

    int x = target.x, y = target.y;
    int ox = x;//, oy = y;
    int w = target.w, h = target.h, wh = w*h;
    int xw = x+w, yh = y+h;
    Uint32 color;
    int i;


            switch (surface->format->BytesPerPixel)
            {
            case 1:   /* Assuming 8-bpp */
            for (i = 0; i < wh; i++)
            {
                color = colors[i];
                if (pattern[i] && x>=SPG_CLIP_XMIN(surface) && x<=SPG_CLIP_XMAX(surface) && y>=SPG_CLIP_YMIN(surface) && y<=SPG_CLIP_YMAX(surface))
                {
                    *((Uint8 *)surface->pixels + y*surface->pitch + x) = color;
                }

                x++;
                if (x >= xw)
                {
                    x = ox;
                    y++;
                    if (y >= yh)
                        break;
                }
            }
            break;

            case 2:   /* Probably 15-bpp or 16-bpp */
            for (i = 0; i < wh; i++)
            {
                color = colors[i];
                if (pattern[i] && x>=SPG_CLIP_XMIN(surface) && x<=SPG_CLIP_XMAX(surface) && y>=SPG_CLIP_YMIN(surface) && y<=SPG_CLIP_YMAX(surface))
                {
                    *((Uint16 *)surface->pixels + y*surface->pitch/2 + x) = color;
                }

                x++;
                if (x >= xw)
                {
                    x = ox;
                    y++;
                    if (y >= yh)
                        break;
                }
            }
            break;

            case 3:   /* Slow 24-bpp mode, usually not used */
            for (i = 0; i < wh; i++)
            {
                color = colors[i];
                if (pattern[i] && x>=SPG_CLIP_XMIN(surface) && x<=SPG_CLIP_XMAX(surface) && y>=SPG_CLIP_YMIN(surface) && y<=SPG_CLIP_YMAX(surface))
                {
                    Uint8 *pix = (Uint8 *)surface->pixels + y * surface->pitch + x*3;

                    /* Gack - slow, but endian correct */
                    *(pix+surface->format->Rshift/8) = color>>surface->format->Rshift;
                    *(pix+surface->format->Gshift/8) = color>>surface->format->Gshift;
                    *(pix+surface->format->Bshift/8) = color>>surface->format->Bshift;
                    *(pix+surface->format->Ashift/8) = color>>surface->format->Ashift;
                }

                x++;
                if (x >= xw)
                {
                    x = ox;
                    y++;
                    if (y >= yh)
                        break;
                }
            }
            break;

            case 4:   /* Probably 32-bpp */
            for (i = 0; i < wh; i++)
            {
                color = colors[i];
                if (pattern[i] && x>=SPG_CLIP_XMIN(surface) && x<=SPG_CLIP_XMAX(surface) && y>=SPG_CLIP_YMIN(surface) && y<=SPG_CLIP_YMAX(surface))
                {
                    *((Uint32 *)surface->pixels + y*surface->pitch/4 + x) = color;
                }

                x++;
                if (x >= xw)
                {
                    x = ox;
                    y++;
                    if (y >= yh)
                        break;
                }
            }
            break;
            }




    spg_unlock(surface);
    
    if(spg_makedirtyrects)
    {
        SPG_DirtyClip(surface, &target);
        SPG_DirtyAddTo(spg_dirtytable_front, &target);
    }
}



void SPG_PixelPatternBlend(SDL_Surface *surface, SDL_Rect target, SPG_bool* pattern, Uint32* colors, Uint8* pixelAlpha)
{
    if(spg_lock(surface) < 0)
    {
        if(spg_useerrors)
            SPG_Error("SPG_PixelPatternBlend could not lock surface");
        return;
    }

    int x = target.x, y = target.y;
    int ox = x;//, oy = y;
    int w = target.w, h = target.h, wh = w*h;
    int xw = x+w, yh = y+h;
    Uint32 color;
    Uint8 alpha;
    Uint32 Rmask = surface->format->Rmask, Gmask = surface->format->Gmask, Bmask = surface->format->Bmask, Amask = surface->format->Amask;
    Uint32 R,G,B,A=SDL_ALPHA_OPAQUE;
    int i;
    
    for (i = 0; i < wh; i++)
    {
        if (pattern[i] && x>=SPG_CLIP_XMIN(surface) && x<=SPG_CLIP_XMAX(surface) && y>=SPG_CLIP_YMIN(surface) && y<=SPG_CLIP_YMAX(surface))
        {
            color = colors[i];
            alpha = pixelAlpha[i];

            switch (surface->format->BytesPerPixel)
            {
            case 1:   /* Assuming 8-bpp */
            {
                if ( alpha == SDL_ALPHA_OPAQUE )
                {
                    *((Uint8 *)surface->pixels + y*surface->pitch + x) = color;
                }
                else
                {
                    Uint8 *pixel = (Uint8 *)surface->pixels + y*surface->pitch + x;

                    Uint8 dR = surface->format->palette->colors[*pixel].r;
                    Uint8 dG = surface->format->palette->colors[*pixel].g;
                    Uint8 dB = surface->format->palette->colors[*pixel].b;
                    Uint8 sR = surface->format->palette->colors[color].r;
                    Uint8 sG = surface->format->palette->colors[color].g;
                    Uint8 sB = surface->format->palette->colors[color].b;

                    dR = dR + ((sR-dR)*alpha >> 8);
                    dG = dG + ((sG-dG)*alpha >> 8);
                    dB = dB + ((sB-dB)*alpha >> 8);

                    *pixel = SDL_MapRGB(surface->format, dR, dG, dB);
                }
            }
            break;

            case 2:   /* Probably 15-bpp or 16-bpp */
            {
                if ( alpha == SDL_ALPHA_OPAQUE )
                {
                    *((Uint16 *)surface->pixels + y*surface->pitch/2 + x) = color;
                }
                else
                {
                    Uint16 *pixel = (Uint16 *)surface->pixels + y*surface->pitch/2 + x;
                    Uint32 dc = *pixel;

                    R = ((dc & Rmask) + (( (color & Rmask) - (dc & Rmask) ) * alpha >> 8)) & Rmask;
                    G = ((dc & Gmask) + (( (color & Gmask) - (dc & Gmask) ) * alpha >> 8)) & Gmask;
                    B = ((dc & Bmask) + (( (color & Bmask) - (dc & Bmask) ) * alpha >> 8)) & Bmask;
                    if ( Amask )
                        A = ((dc & Amask) + (( (color & Amask) - (dc & Amask) ) * alpha >> 8)) & Amask;

                    *pixel= R | G | B | A;
                }
            }
            break;

            case 3:   /* Slow 24-bpp mode, usually not used */
            {
                Uint8 *pix = (Uint8 *)surface->pixels + y * surface->pitch + x*3;
                Uint8 rshift8=surface->format->Rshift/8;
                Uint8 gshift8=surface->format->Gshift/8;
                Uint8 bshift8=surface->format->Bshift/8;
                Uint8 ashift8=surface->format->Ashift/8;


                if ( alpha == SDL_ALPHA_OPAQUE )
                {
                    *(pix+rshift8) = color>>surface->format->Rshift;
                    *(pix+gshift8) = color>>surface->format->Gshift;
                    *(pix+bshift8) = color>>surface->format->Bshift;
                    *(pix+ashift8) = color>>surface->format->Ashift;
                }
                else
                {
                    Uint8 dR, dG, dB, dA=0;
                    Uint8 sR, sG, sB, sA=0;

                    pix = (Uint8 *)surface->pixels + y * surface->pitch + x*3;

                    dR = *((pix)+rshift8);
                    dG = *((pix)+gshift8);
                    dB = *((pix)+bshift8);
                    dA = *((pix)+ashift8);

                    sR = (color>>surface->format->Rshift)&0xff;
                    sG = (color>>surface->format->Gshift)&0xff;
                    sB = (color>>surface->format->Bshift)&0xff;
                    sA = (color>>surface->format->Ashift)&0xff;

                    dR = dR + ((sR-dR)*alpha >> 8);
                    dG = dG + ((sG-dG)*alpha >> 8);
                    dB = dB + ((sB-dB)*alpha >> 8);
                    dA = dA + ((sA-dA)*alpha >> 8);

                    *((pix)+rshift8) = dR;
                    *((pix)+gshift8) = dG;
                    *((pix)+bshift8) = dB;
                    *((pix)+ashift8) = dA;
                }
            }
            break;

            case 4:   /* Probably 32-bpp */
            {
                if ( alpha == SDL_ALPHA_OPAQUE )
                {
                    *((Uint32 *)surface->pixels + y*surface->pitch/4 + x) = color;
                }
                else
                {
                    Uint32 *pixel = (Uint32 *)surface->pixels + y*surface->pitch/4 + x;
                    Uint32 dc = *pixel;
                    R = color & Rmask;
                    G = color & Gmask;
                    B = color & Bmask;
                    A = 0;


                    switch (SPG_GetBlend())
                    {
                    case SPG_COMBINE_ALPHA:  // Blend and combine src and dest alpha, SLOW IMPLEMENTATION
                        R = ((dc & Rmask) + (( R - (dc & Rmask) ) * alpha >> 8)) & Rmask;
                        G = ((dc & Gmask) + (( G - (dc & Gmask) ) * alpha >> 8)) & Gmask;
                        B = ((dc & Bmask) + (( B - (dc & Bmask) ) * alpha >> 8)) & Bmask;
                        if (Amask)
                            A = ((((dc & Amask) >> surface->format->Ashift) + alpha) >> 1) << surface->format->Ashift;
                        break;
                    case SPG_DEST_ALPHA:  // Blend and keep dest alpha
                        R = ((dc & Rmask) + (( R - (dc & Rmask) ) * alpha >> 8)) & Rmask;
                        G = ((dc & Gmask) + (( G - (dc & Gmask) ) * alpha >> 8)) & Gmask;
                        B = ((dc & Bmask) + (( B - (dc & Bmask) ) * alpha >> 8)) & Bmask;
                        if (Amask)
                            A = (dc & Amask);
                        break;
                    case SPG_SRC_ALPHA:  // Blend and keep src alpha
                        R = ((dc & Rmask) + (( R - (dc & Rmask) ) * alpha >> 8)) & Rmask;
                        G = ((dc & Gmask) + (( G - (dc & Gmask) ) * alpha >> 8)) & Gmask;
                        B = ((dc & Bmask) + (( B - (dc & Bmask) ) * alpha >> 8)) & Bmask;
                        if (Amask)
                            A = (alpha << surface->format->Ashift);
                        break;
                    case SPG_COPY_SRC_ALPHA: // Direct copy with src alpha
                        if (Amask)
                            A = (alpha << surface->format->Ashift);
                        break;
                    case SPG_COPY_DEST_ALPHA: // Direct copy with dest alpha
                        if (Amask)
                            A = (dc & Amask);
                        break;
                    case SPG_COPY_COMBINE_ALPHA: // Direct copy with combined alpha, SLOW IMPLEMENTATION
                        if (Amask)
                            A = ((((dc & Amask) >> surface->format->Ashift) + alpha) >> 1) << surface->format->Ashift;
                        break;
                    case SPG_COPY_NO_ALPHA:  // Direct copy, alpha opaque
                        break;
                    case SPG_COPY_ALPHA_ONLY:  // Direct copy of just the alpha
                        R = dc & Rmask;
                        G = dc & Gmask;
                        B = dc & Bmask;
                        if(Amask)
                            A = (alpha << surface->format->Ashift);
                        break;
                    case SPG_COMBINE_ALPHA_ONLY:  // Blend of just the alpha
                        R = dc & Rmask;
                        G = dc & Gmask;
                        B = dc & Bmask;
                        if(Amask)
                            A = ((((dc & Amask) >> surface->format->Ashift) + alpha) >> 1) << surface->format->Ashift;
                        break;
                    case SPG_REPLACE_COLORKEY:  // Replace the colorkeyed color
                        if(!(surface->flags & SDL_SRCCOLORKEY) || dc != surface->format->colorkey)
                            return;
                        if(Amask)
                            A = (alpha << surface->format->Ashift);
                        break;
                    }

                    *pixel = R | G | B | A;
                }
            }
            break;
            }
        }


        x++;
        if (x >= xw)
        {
            x = ox;
            y++;
            if (y >= yh)
                break;
        }

    }

    spg_unlock(surface);
    
    if(spg_makedirtyrects)
    {
        SPG_DirtyClip(surface, &target);
        SPG_DirtyAddTo(spg_dirtytable_front, &target);
    }
}







/**********************************************************************************/
/**                             Line functions                                   **/
/**********************************************************************************/

//==================================================================================
// Internal draw horizontal line
//==================================================================================
void spg_lineh(SDL_Surface *Surface, Sint16 x1, Sint16 y, Sint16 x2, Uint32 Color)
{
    if (x1>x2)
    {
        Sint16 tmp=x1;
        x1=x2;
        x2=tmp;
    }

    //Do the clipping
#if SDL_VERSIONNUM(SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL) < \
    SDL_VERSIONNUM(1, 1, 5)
    if (y<Surface->clip_miny || y>Surface->clip_maxy || x1>Surface->clip_maxx || x2<Surface->clip_minx)
        return;
    if (x1<Surface->clip_minx)
        x1=Surface->clip_minx;
    if (x2>Surface->clip_maxx)
        x2=Surface->clip_maxx;
#endif

    SDL_Rect l;
    l.x=x1;
    l.y=y;
    l.w=x2-x1+1;
    l.h=1;

    SDL_FillRect(Surface, &l, Color);
}

//==================================================================================
// Draw horizontal line
//==================================================================================
void SPG_LineH(SDL_Surface *Surface, Sint16 x1, Sint16 y, Sint16 x2, Uint32 Color)
{
    if (x1>x2)
    {
        Sint16 tmp=x1;
        x1=x2;
        x2=tmp;
    }

    //Do the clipping
#if SDL_VERSIONNUM(SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL) < \
    SDL_VERSIONNUM(1, 1, 5)
    if (y<Surface->clip_miny || y>Surface->clip_maxy || x1>Surface->clip_maxx || x2<Surface->clip_minx)
        return;
    if (x1<Surface->clip_minx)
        x1=Surface->clip_minx;
    if (x2>Surface->clip_maxx)
        x2=Surface->clip_maxx;
#endif

    SDL_Rect l;
    l.x=x1;
    l.y=y;
    l.w=x2-x1+1;
    l.h=1;
    
    if(spg_thickness == 1)
        SDL_FillRect(Surface, &l, Color);
    else
    {
        if(spg_thickness == 0) return;
        l.h = spg_thickness;
        l.y -= (l.h - 1)/2;
        SDL_FillRect(Surface, &l, Color);
        //SPG_DirtyClip(Surface, &l);
    }
    
    if(spg_makedirtyrects)
    {
        SPG_DirtyAddTo(spg_dirtytable_front, &l);
    }
}


//==================================================================================
// Internal draw horizontal line (alpha)
//==================================================================================
void spg_linehblend(SDL_Surface *Surface, Sint16 x1, Sint16 y, Sint16 x2, Uint32 Color, Uint8 alpha)
{
    // Disable autolock so that other functions can use this one a lot.
    Uint8 lock = spg_autolock;
    spg_autolock = 0;

    SPG_RectFilledBlend(Surface, x1,y,x2,y, Color, alpha);

    spg_autolock = lock;
}

//==================================================================================
// Draw horizontal line (alpha)
//==================================================================================
void SPG_LineHBlend(SDL_Surface *Surface, Sint16 x1, Sint16 y, Sint16 x2, Uint32 Color, Uint8 alpha)
{
    if(spg_thickness == 1)
        SPG_RectFilledBlend(Surface, x1,y,x2,y, Color, alpha);
    else
    {
        if(spg_thickness == 0) return;
        Sint16 h = spg_thickness;
        y -= (h - 1)/2;
        SPG_RectFilledBlend(Surface, x1,y,x2,y+h-1, Color, alpha);
    }
}



//==================================================================================
// Internal draw vertical line
//==================================================================================
void spg_linev(SDL_Surface *Surface, Sint16 x, Sint16 y1, Sint16 y2, Uint32 Color)
{
    if (y1>y2)
    {
        Sint16 tmp=y1;
        y1=y2;
        y2=tmp;
    }

    //Do the clipping
#if SDL_VERSIONNUM(SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL) < \
    SDL_VERSIONNUM(1, 1, 5)
    if (x<Surface->clip_minx || x>Surface->clip_maxx || y1>Surface->clip_maxy || y2<Surface->clip_miny)
        return;
    if (y1<Surface->clip_miny)
        y1=Surface->clip_miny;
    if (y2>Surface->clip_maxy)
        y2=Surface->clip_maxy;
#endif

    SDL_Rect l;
    l.x=x;
    l.y=y1;
    l.w=1;
    l.h=y2-y1+1;

    SDL_FillRect(Surface, &l, Color);
}

//==================================================================================
// Draw vertical line
//==================================================================================
void SPG_LineV(SDL_Surface *Surface, Sint16 x, Sint16 y1, Sint16 y2, Uint32 Color)
{
    if (y1>y2)
    {
        Sint16 tmp=y1;
        y1=y2;
        y2=tmp;
    }

    //Do the clipping
#if SDL_VERSIONNUM(SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL) < \
    SDL_VERSIONNUM(1, 1, 5)
    if (x<Surface->clip_minx || x>Surface->clip_maxx || y1>Surface->clip_maxy || y2<Surface->clip_miny)
        return;
    if (y1<Surface->clip_miny)
        y1=Surface->clip_miny;
    if (y2>Surface->clip_maxy)
        y2=Surface->clip_maxy;
#endif

    SDL_Rect l;
    l.x=x;
    l.y=y1;
    l.w=1;
    l.h=y2-y1+1;
    
    if(spg_thickness == 1)
        SDL_FillRect(Surface, &l, Color);
    else
    {
        if(spg_thickness == 0) return;
        l.w = spg_thickness;
        l.x -= (l.w - 1)/2;
        SDL_FillRect(Surface, &l, Color);
        //SPG_DirtyClip(Surface, &l);
    }
    
    if(spg_makedirtyrects)
    {
        SPG_DirtyAddTo(spg_dirtytable_front, &l);
    }
}




//==================================================================================
// Internal draw vertical line (alpha - no update)
//==================================================================================
void spg_linevblend(SDL_Surface *Surface, Sint16 x, Sint16 y1, Sint16 y2, Uint32 Color, Uint8 alpha)
{
    // Disable autolock so other functions can use this one a lot.
    Uint8 lock = spg_autolock;
    spg_autolock = 0;
    SPG_RectFilledBlend(Surface, x,y1,x,y2, Color, alpha);
    spg_autolock = lock;
}

//==================================================================================
// Draw vertical line (alpha)
//==================================================================================
void SPG_LineVBlend(SDL_Surface *Surface, Sint16 x, Sint16 y1, Sint16 y2, Uint32 Color, Uint8 alpha)
{
    if(spg_thickness == 1)
        SPG_RectFilledBlend(Surface, x,y1,x,y2, Color, alpha);
    else
    {
        if(spg_thickness == 0) return;
        Sint16 w = spg_thickness;
        x -= (w - 1)/2;
        SPG_RectFilledBlend(Surface, x,y1,x+w-1,y2, Color, alpha);
    }
}




//==================================================================================
// Performs Callback at each line point. (From PowerPak)
//==================================================================================
void SPG_LineFn(SDL_Surface *Surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 Color, void Callback(SDL_Surface *Surf, Sint16 X, Sint16 Y, Uint32 Color))
{
    Sint16 dx, dy, sdx, sdy, x, y, px, py;

    dx = x2 - x1;
    dy = y2 - y1;

    sdx = (dx < 0) ? -1 : 1;
    sdy = (dy < 0) ? -1 : 1;

    dx = sdx * dx + 1;
    dy = sdy * dy + 1;

    x = y = 0;

    px = x1;
    py = y1;

    if (dx >= dy)
    {
        for (x = 0; x < dx; x++)
        {
            Callback(Surface, px, py, Color);

            y += dy;
            if (y >= dx)
            {
                y -= dx;
                py += sdy;
            }
            px += sdx;
        }
    }
    else
    {
        for (y = 0; y < dy; y++)
        {
            Callback(Surface, px, py, Color);

            x += dx;
            if (x >= dy)
            {
                x -= dy;
                px += sdx;
            }
            py += sdy;
        }
    }
}




//==================================================================================
// Line clipping
// Standard Cohen-Sutherland algorithm (from gfxPrimitives)
//==================================================================================
#define CLIP_LEFT_EDGE   0x1
#define CLIP_RIGHT_EDGE  0x2
#define CLIP_BOTTOM_EDGE 0x4
#define CLIP_TOP_EDGE    0x8
#define CLIP_INSIDE(a)   (!a)
#define CLIP_REJECT(a,b) (a&b)
#define CLIP_ACCEPT(a,b) (!(a|b))

int spg_clipencode(Sint16 x, Sint16 y, Sint16 left, Sint16 top, Sint16 right, Sint16 bottom)
{
    int code = 0;

    if (x < left)
        code |= CLIP_LEFT_EDGE;
    else if (x > right)
        code |= CLIP_RIGHT_EDGE;

    if (y < top)
        code |= CLIP_TOP_EDGE;
    else if (y > bottom)
        code |= CLIP_BOTTOM_EDGE;

    return code;
}

int spg_clipline(SDL_Surface *dst, Sint16 *x1, Sint16 *y1, Sint16 *x2, Sint16 *y2)
{
    int code1, code2;
    SPG_bool draw = 0;

    Sint16 tmp;
    float m;

    /* Get clipping boundary */
    Sint16 left, right, top, bottom;
    left = SPG_CLIP_XMIN(dst);
    right = SPG_CLIP_XMAX(dst);
    top = SPG_CLIP_YMIN(dst);
    bottom = SPG_CLIP_YMAX(dst);

    while (1)
    {
        code1 = spg_clipencode(*x1, *y1, left, top, right, bottom);
        code2 = spg_clipencode(*x2, *y2, left, top, right, bottom);

        if (CLIP_ACCEPT(code1, code2))
        {
            draw = 1;
            break;
        }
        else if (CLIP_REJECT(code1, code2))
            break;
        else
        {
            if (CLIP_INSIDE(code1))
            {
                tmp = *x2;
                *x2 = *x1;
                *x1 = tmp;
                tmp = *y2;
                *y2 = *y1;
                *y1 = tmp;
                tmp = code2;
                code2 = code1;
                code1 = tmp;
            }
            if (*x2 != *x1)
                m = (*y2 - *y1) / (float)(*x2 - *x1);
            else
                m = 1.0;


            if (code1 & CLIP_LEFT_EDGE)
            {
                *y1 += (Sint16)( (left - *x1) * m );
                *x1 = left;
            }
            else if (code1 & CLIP_RIGHT_EDGE)
            {
                *y1 += (Sint16)( (right - *x1) * m );
                *x1 = right;
            }
            else if (code1 & CLIP_BOTTOM_EDGE)
            {
                if (*x2 != *x1)
                {
                    *x1 += (Sint16)( (bottom - *y1) / m );
                }
                *y1 = bottom;
            }
            else if (code1 & CLIP_TOP_EDGE)
            {
                if (*x2 != *x1)
                {
                    *x1 += (Sint16)( (top - *y1) / m );
                }
                *y1 = top;
            }
        }
    }

    return draw;
}


//==================================================================================
// Draws a line
//==================================================================================
void spg_line(SDL_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color)
{
    if ( !spg_clipline(surface, &x1, &y1, &x2, &y2) )
        return;

    Sint16 dx, dy, sdx, sdy, x, y;

    dx = x2 - x1;
    dy = y2 - y1;

    sdx = (dx < 0) ? -1 : 1;
    sdy = (dy < 0) ? -1 : 1;

    dx = sdx * dx + 1;
    dy = sdy * dy + 1;

    x = y = 0;

    Sint16 pixx = surface->format->BytesPerPixel;
    Sint16 pixy = surface->pitch;
    Uint8 *pixel = (Uint8*)surface->pixels + y1*pixy + x1*pixx;

    pixx *= sdx;
    pixy *= sdy;

    if (dx < dy)
    {
        Sint32 tmp = dx;
        dx = dy;
        dy = (Sint16)(tmp);
        tmp = pixx;
        pixx = pixy;
        pixy = tmp;
    }

    switch (surface->format->BytesPerPixel)
    {
    case 1:
    {
        for (x=0; x < dx; x++)
        {
            *pixel = color;

            y += dy;
            if (y >= dx)
            {
                y -= dx;
                pixel += pixy;
            }
            pixel += pixx;
        }
    }
    break;

    case 2:
    {
        for (x=0; x < dx; x++)
        {
            *(Uint16*)pixel = color;

            y += dy;
            if (y >= dx)
            {
                y -= dx;
                pixel += pixy;
            }
            pixel += pixx;
        }
    }
    break;

    case 3:
    {
        Uint8 rshift8 = surface->format->Rshift/8;
        Uint8 gshift8 = surface->format->Gshift/8;
        Uint8 bshift8 = surface->format->Bshift/8;
        Uint8 ashift8 = surface->format->Ashift/8;

        Uint8 R = (color>>surface->format->Rshift)&0xff;
        Uint8 G = (color>>surface->format->Gshift)&0xff;
        Uint8 B = (color>>surface->format->Bshift)&0xff;
        Uint8 A = (color>>surface->format->Ashift)&0xff;

        for (x=0; x < dx; x++)
        {
            *(pixel+rshift8) = R;
            *(pixel+gshift8) = G;
            *(pixel+bshift8) = B;
            *(pixel+ashift8) = A;

            y += dy;
            if (y >= dx)
            {
                y -= dx;
                pixel += pixy;
            }
            pixel += pixx;
        }
    }
    break;

    case 4:
    {
        for (x=0; x < dx; x++)
        {
            *(Uint32*)pixel = color;

            y += dy;
            if (y >= dx)
            {
                y -= dx;
                pixel += pixy;
            }
            pixel += pixx;
        }
    }
    break;
    }
}

void spg_lineblendaa(SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color, Uint8 alpha);

void SPG_Line(SDL_Surface *Surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color)
{
    if (spg_lock(Surface) < 0)
        return;
    
    if(spg_thickness == 1)
    {
        if(SPG_GetAA())
            spg_lineblendaa(Surface,x1,y1,x2,y2,color, SDL_ALPHA_OPAQUE);
        else
            spg_line(Surface, x1,y1, x2,y2, color);

        if(spg_makedirtyrects)
        {
            SDL_Rect rect;
            rect.x = MIN(x1, x2);
            rect.y = MIN(y1, y2);
            rect.w = MAX(x1, x2) - rect.x + 1;
            rect.h = MAX(y1, y2) - rect.y + 1;
            // Clip it to the screen
            SPG_DirtyClip(Surface, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
    }
    else if(spg_thickness > 0)
    {
        SPG_LineFn(Surface, x1,y1, x2,y2, color, spg_thicknesscallback);

        if(spg_makedirtyrects)
        {
            SDL_Rect rect;
            rect.x = MIN(x1, x2) - spg_thickness/2;
            rect.y = MIN(y1, y2) - spg_thickness/2;
            rect.w = MAX(x1, x2) - rect.x + 1 + spg_thickness;
            rect.h = MAX(y1, y2) - rect.y + 1 + spg_thickness;
            // Clip it to the screen
            SPG_DirtyClip(Surface, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
    }

    spg_unlock(Surface);
    
}




//==================================================================================
// A quick hack to get alpha working with callbacks
//==================================================================================

void spg_pixelcallbackalpha(SDL_Surface *surf, Sint16 x, Sint16 y, Uint32 color)
{
    spg_pixelblend(surf,x,y,color,spg_alphahack);
}


//==================================================================================
// Draws a line (alpha)
//==================================================================================
void spg_lineblend(SDL_Surface *Surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 Color, Uint8 alpha)
{
    spg_alphahack = alpha;

    /* Draw the line */
    SPG_LineFn(Surface, x1, y1, x2, y2, Color, spg_pixelcallbackalpha);
}

//==================================================================================
// Anti-aliased line
// From SDL_gfxPrimitives written by A. Schiffler (aschiffler@home.com)
//==================================================================================
#define AAbits 8
#define AAlevels 256  /* 2^AAbits */
void spg_lineblendaa(SDL_Surface *dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color, Uint8 alpha)
{

    Uint32 erracc=0, erradj;
    Uint32 erracctmp, wgt;
    Sint16 tmp, y0p1, x0pxdir;
    Uint8 a;

    /* Keep on working with 32bit numbers */
    Sint32 xx0=x1;
    Sint32 yy0=y1;
    Sint32 xx1=x2;
    Sint32 yy1=y2;

    /* Reorder points if required */
    if (yy0 > yy1)
    {
        SWAP(yy0, yy1, tmp);
        SWAP(xx0, xx1, tmp);
    }

    /* Calculate distance */
    Sint16 dx = xx1 - xx0;
    Sint16 dy = yy1 - yy0;

    /* Adjust for negative dx and set xdir */
    Sint16 xdir = 1;
    if (dx < 0)
    {
        xdir=-1;
        dx=(-dx);
    }

    /* Check for special cases */
    if (dx==0 || dy==0 || dx==dy)
    {
        if (alpha==SDL_ALPHA_OPAQUE)
            spg_line(dst,x1,y1,x2,y2,color);
        else
            spg_lineblend(dst,x1,y1,x2,y2,color,alpha);
        return;
    }

    float alpha_pp = (float)(alpha)/255;  /* Used to calculate alpha level if alpha != 255 */

    Uint32 intshift    = 32 - AAbits;   /* # of bits by which to shift erracc to get intensity level */

    /* Draw the initial pixel in the foreground color */
    if (alpha==SDL_ALPHA_OPAQUE)
        spg_pixel(dst,x1,y1, color);
    else
        spg_pixelblend(dst,x1,y1, color, alpha);

    /* x-major or y-major? */
    if (dy > dx)
    {

        /* y-major.  Calculate 16-bit fixed point fractional part of a pixel that
        X advances every time Y advances 1 pixel, truncating the result so that
        we won't overrun the endpoint along the X axis */
        erradj = ((dx << 16) / dy)<<16;

        /* draw all pixels other than the first and last */
        x0pxdir=xx0+xdir;
        while (--dy)
        {
            erracctmp = erracc;
            erracc += erradj;
            if (erracc <= erracctmp)
            {
                /* rollover in error accumulator, x coord advances */
                xx0=x0pxdir;
                x0pxdir += xdir;
            }
            yy0++;			/* y-major so always advance Y */

            /* the AAbits most significant bits of erracc give us the intensity
            weighting for this pixel, and the complement of the weighting for
            the paired pixel. */
            wgt = (erracc >> intshift) & 255;

            a = (Uint8)(255-wgt);
            if (alpha != SDL_ALPHA_OPAQUE)
                a = (Uint8)(a*alpha_pp);

            spg_pixelblend(dst,xx0,yy0,color,a);

            a = (Uint8)(wgt);
            if (alpha != SDL_ALPHA_OPAQUE)
                a = (Uint8)(a*alpha_pp);

            spg_pixelblend(dst,x0pxdir,yy0,color,a);
        }
    }
    else
    {

        /* x-major line.  Calculate 16-bit fixed-point fractional part of a pixel
        that Y advances each time X advances 1 pixel, truncating the result so
        that we won't overrun the endpoint along the X axis. */
        erradj = ((dy << 16) / dx)<<16;

        /* draw all pixels other than the first and last */
        y0p1=yy0+1;
        while (--dx)
        {

            erracctmp = erracc;
            erracc += erradj;
            if (erracc <= erracctmp)
            {
                /* Accumulator turned over, advance y */
                yy0=y0p1;
                y0p1++;
            }
            xx0 += xdir;  /* x-major so always advance X */

            /* the AAbits most significant bits of erracc give us the intensity
            weighting for this pixel, and the complement of the weighting for
            the paired pixel. */
            wgt = (erracc >> intshift) & 255;

            a = (Uint8)(255-wgt);
            if (alpha != SDL_ALPHA_OPAQUE)
                a = (Uint8)(a*alpha_pp);

            spg_pixelblend(dst,xx0,yy0,color,a);

            a = (Uint8)(wgt);
            if (alpha != SDL_ALPHA_OPAQUE)
                a = (Uint8)(a*alpha_pp);

            spg_pixelblend(dst,xx0,y0p1,color,a);
        }
    }

    /* Draw final pixel, always exactly intersected by the line and doesn't
    need to be weighted. */
    if (alpha==SDL_ALPHA_OPAQUE)
        spg_pixel(dst,x2,y2, color);
    else
        spg_pixelblend(dst,x2,y2, color, alpha);

}

void SPG_LineBlend(SDL_Surface *Surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color, Uint8 alpha)
{
    if (spg_lock(Surface) < 0)
    {
        if(spg_useerrors)
            SPG_Error("SPG_LineBlend could not lock surface");
        return;
    }
    
    if(spg_thickness == 1)
    {
        if(SPG_GetAA())
            spg_lineblendaa(Surface,x1,y1,x2,y2,color, alpha);
        else
            spg_lineblend(Surface, x1,  y1, x2, y2, color, alpha);

        if(spg_makedirtyrects)
        {
            SDL_Rect rect;
            rect.x = MIN(x1, x2);
            rect.y = MIN(y1, y2);
            rect.w = MAX(x1, x2) - rect.x + 1;
            rect.h = MAX(y1, y2) - rect.y + 1;
            // Clip it to the screen
            SPG_DirtyClip(Surface, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
    }
    else if(spg_thickness > 0)
    {
        spg_alphahack = alpha;
        SPG_LineFn(Surface, x1, y1, x2, y2, color, spg_thicknesscallbackalpha);

        if(spg_makedirtyrects)
        {
            SDL_Rect rect;
            rect.x = MIN(x1, x2) - spg_thickness/2;
            rect.y = MIN(y1, y2) - spg_thickness/2;
            rect.w = MAX(x1, x2) - rect.x + 1 + spg_thickness;
            rect.h = MAX(y1, y2) - rect.y + 1 + spg_thickness;
            // Clip it to the screen
            SPG_DirtyClip(Surface, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
    }
    
    spg_unlock(Surface);
    
}






//==================================================================================
// Draws a multicolored line
//==================================================================================
void SPG_LineFadeFn(SDL_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color1, Uint32 color2, void Callback(SDL_Surface *Surf, Sint16 X, Sint16 Y, Uint32 Color))
{
    Sint16 dx, dy, sdx, sdy, x, y, px, py;

    dx = x2 - x1;
    dy = y2 - y1;

    sdx = (dx < 0) ? -1 : 1;
    sdy = (dy < 0) ? -1 : 1;

    dx = sdx * dx + 1;
    dy = sdy * dy + 1;

    x = y = 0;

    px = x1;
    py = y1;

    Uint8 r1, g1, b1, r2, g2, b2;
    
    if(surface->format->BitsPerPixel == 8)
    {
        SDL_GetRGB(color1, surface->format, &r1, &g1, &b1);
        SDL_GetRGB(color2, surface->format, &r2, &g2, &b2);
    }
    else
    {
        r1 = (color1 & surface->format->Rmask) >> surface->format->Rshift;
        g1 = (color1 & surface->format->Gmask) >> surface->format->Gshift;
        b1 = (color1 & surface->format->Bmask) >> surface->format->Bshift;
        r2 = (color2 & surface->format->Rmask) >> surface->format->Rshift;
        g2 = (color2 & surface->format->Gmask) >> surface->format->Gshift;
        b2 = (color2 & surface->format->Bmask) >> surface->format->Bshift;
    }
    

    /* We use fixedpoint math for the color fading */
    Sint32 R = r1<<16;
    Sint32 G = g1<<16;
    Sint32 B = b1<<16;
    Sint32 R2 = r2<<16;
    Sint32 G2 = g2<<16;
    Sint32 B2 = b2<<16;
    Sint32 rstep;
    Sint32 gstep;
    Sint32 bstep;

    if (dx >= dy)
    {
        rstep = (R2-R) / (Sint32)(dx);
        gstep = (G2-G) / (Sint32)(dx);
        bstep = (B2-B) / (Sint32)(dx);

        for (x = 0; x < dx; x++)
        {
            Callback(surface, px, py, SDL_MapRGB(surface->format, (Uint8)(R>>16), (Uint8)(G>>16), (Uint8)(B>>16)) );

            y += dy;
            if (y >= dx)
            {
                y -= dx;
                py += sdy;
            }
            px += sdx;

            R += rstep;
            G += gstep;
            B += bstep;
        }
    }
    else
    {
        // Why is this different than above?
        rstep = (Sint32)((r2-r1)<<16) / (Sint32)(dy);
        gstep = (Sint32)((g2-g1)<<16) / (Sint32)(dy);
        bstep = (Sint32)((b2-b1)<<16) / (Sint32)(dy);

        for (y = 0; y < dy; y++)
        {
            Callback(surface, px, py, SDL_MapRGB(surface->format, (Uint8)(R>>16), (Uint8)(G>>16), (Uint8)(B>>16)) );

            x += dx;
            if (x >= dy)
            {
                x -= dy;
                px += sdx;
            }
            py += sdy;

            R += rstep;
            G += gstep;
            B += bstep;
        }
    }
}
void spg_linefadeblendaa(SDL_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color1, Uint8 alpha1, Uint32 color2, Uint8 alpha2);

void SPG_LineFade(SDL_Surface *Surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color1, Uint32 color2)
{
    if (spg_lock(Surface) < 0)
    {
        if(spg_useerrors)
            SPG_Error("SPG_LineFade could not lock surface");
        return;
    }

    if(spg_thickness == 1)
    {
        if(SPG_GetAA())
            spg_linefadeblendaa(Surface, x1, y1, x2, y2, color1, 255, color2, 255);
        else
            SPG_LineFadeFn(Surface, x1,y1, x2,y2, color1, color2, spg_pixel);

        if(spg_makedirtyrects)
        {
            SDL_Rect rect;
            rect.x = MIN(x1, x2);
            rect.y = MIN(y1, y2);
            rect.w = MAX(x1, x2) - rect.x + 1;
            rect.h = MAX(y1, y2) - rect.y + 1;
            // Clip it to the screen
            SPG_DirtyClip(Surface, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
    }
    if(spg_thickness > 0)
    {
        SPG_LineFadeFn(Surface, x1,y1, x2,y2, color1, color2, spg_thicknesscallback);

        if(spg_makedirtyrects)
        {
            SDL_Rect rect;
            rect.x = MIN(x1, x2) - spg_thickness/2;
            rect.y = MIN(y1, y2) - spg_thickness/2;
            rect.w = MAX(x1, x2) - rect.x + 1 + spg_thickness;
            rect.h = MAX(y1, y2) - rect.y + 1 + spg_thickness;
            // Clip it to the screen
            SPG_DirtyClip(Surface, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
    }
    
    spg_unlock(Surface);

}

//==================================================================================
// Draws a anti-aliased multicolored line
//==================================================================================
void spg_linefadeblendaa(SDL_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color1, Uint8 alpha1, Uint32 color2, Uint8 alpha2)
{
    Uint8 r1, g1, b1, r2, g2, b2;
    
    if(surface->format->BitsPerPixel == 8)
    {
        SDL_GetRGB(color1, surface->format, &r1, &g1, &b1);
        SDL_GetRGB(color2, surface->format, &r2, &g2, &b2);
    }
    else
    {
        r1 = (color1 & surface->format->Rmask) >> surface->format->Rshift;
        g1 = (color1 & surface->format->Gmask) >> surface->format->Gshift;
        b1 = (color1 & surface->format->Bmask) >> surface->format->Bshift;

        r2 = (color2 & surface->format->Rmask) >> surface->format->Rshift;
        g2 = (color2 & surface->format->Gmask) >> surface->format->Gshift;
        b2 = (color2 & surface->format->Bmask) >> surface->format->Bshift;
    }
    
    Uint32 erracc=0, erradj;
    Uint32 erracctmp, wgt;
    Sint16 tmp, y0p1, x0pxdir;
    Uint8 a;

    /* Keep on working with 32bit numbers */
    Sint32 xx0=x1;
    Sint32 yy0=y1;
    Sint32 xx1=x2;
    Sint32 yy1=y2;

    /* Reorder points if required */
    if (yy0 > yy1)
    {
        SWAP(yy0, yy1, tmp);
        SWAP(xx0, xx1, tmp);

        SWAP(r1, r2, a);
        SWAP(g1, g2, a);
        SWAP(b1, b2, a);
    }

    /* Calculate distance */
    Sint16 dx = xx1 - xx0;
    Sint16 dy = yy1 - yy0;

    /* Adjust for negative dx and set xdir */
    Sint16 xdir=1;
    if (dx < 0)
    {
        xdir=-1;
        dx=(-dx);
    }

    /* Check for special cases */
    if (dx==0 || dy==0  || dx==dy)
    {
        //SPG_LineFadeBlend(surface, x1, y1, x2, y2, color1, alpha1, color2, alpha2);
        spg_alphahack = alpha1;
        SPG_LineFadeFn(surface, x1,y1, x2,y2, color1, color2, spg_pixelcallbackalpha);
        return;
    }

    /* We use fixedpoint math for the color fading */
    Sint32 R = r1<<16;
    Sint32 G = g1<<16;
    Sint32 B = b1<<16;
    Sint32 A = alpha1<<16;
    //Sint32 R2 = r1<<16;
    //Sint32 G2 = g1<<16;
    //Sint32 B2 = b1<<16;
    //Sint32 A2 = alpha2<<16;
    Sint32 rstep;
    Sint32 gstep;
    Sint32 bstep;
    Sint32 astep;


    int alpha = alpha1;
    //float alpha_pp = (float)(alpha)/255;  /* Used to calculate alpha level if alpha != 255 */
    Uint32 intshift    = 32 - AAbits;   /* # of bits by which to shift erracc to get intensity level */

    if (alpha1==SDL_ALPHA_OPAQUE)
        spg_pixel(surface,x1,y1, SDL_MapRGB(surface->format, r1, g1, b1) );  /* Draw the initial pixel in the foreground color */
    else
        spg_pixelblend(surface,x1,y1, SDL_MapRGB(surface->format, r1, g1, b1), alpha);

    /* x-major or y-major? */
    if (dy > dx)
    {

        /* y-major.  Calculate 16-bit fixed point fractional part of a pixel that
        X advances every time Y advances 1 pixel, truncating the result so that
        we won't overrun the endpoint along the X axis */
        erradj = ((dx << 16) / dy)<<16;

        rstep = (Sint32)((r2-r1)<<16) / (Sint32)(dy);
        gstep = (Sint32)((g2-g1)<<16) / (Sint32)(dy);
        bstep = (Sint32)((b2-b1)<<16) / (Sint32)(dy);
        astep = (Sint32)((alpha2-alpha1)<<16) / (Sint32)(dy);

        /* draw all pixels other than the first and last */
        x0pxdir=xx0+xdir;
        while (--dy)
        {
            R += rstep;
            G += gstep;
            B += bstep;
            A += astep;

            erracctmp = erracc;
            erracc += erradj;
            if (erracc <= erracctmp)
            {
                /* rollover in error accumulator, x coord advances */
                xx0=x0pxdir;
                x0pxdir += xdir;
            }
            yy0++;			/* y-major so always advance Y */

            /* the AAbits most significant bits of erracc give us the intensity
            weighting for this pixel, and the complement of the weighting for
            the paired pixel. */
            wgt = (erracc >> intshift) & 255;

            a = (Uint8)(255-wgt);
            if (alpha != 255)
                a = (Uint8)(a*(float)(A>>16)/255);

            spg_pixelblend(surface,xx0,yy0,SDL_MapRGB(surface->format, (Uint8)(R>>16), (Uint8)(G>>16), (Uint8)(B>>16)),a);

            a = (Uint8)(wgt);
            if (alpha != 255)
                a = (Uint8)(a*(float)(A>>16)/255);

            spg_pixelblend(surface,x0pxdir,yy0,SDL_MapRGB(surface->format, (Uint8)(R>>16), (Uint8)(G>>16), (Uint8)(B>>16)),a);
        }
    }
    else
    {

        /* x-major line.  Calculate 16-bit fixed-point fractional part of a pixel
        that Y advances each time X advances 1 pixel, truncating the result so
        that we won't overrun the endpoint along the X axis. */
        erradj = ((dy << 16) / dx)<<16;

        rstep = (Sint32)((r2-r1)<<16) / (Sint32)(dx);
        gstep = (Sint32)((g2-g1)<<16) / (Sint32)(dx);
        bstep = (Sint32)((b2-b1)<<16) / (Sint32)(dx);
        astep = (Sint32)((alpha2-alpha1)<<16) / (Sint32)(dx);

        /* draw all pixels other than the first and last */
        y0p1=yy0+1;
        while (--dx)
        {
            R += rstep;
            G += gstep;
            B += bstep;
            A += astep;

            erracctmp = erracc;
            erracc += erradj;
            if (erracc <= erracctmp)
            {
                /* Accumulator turned over, advance y */
                yy0=y0p1;
                y0p1++;
            }
            xx0 += xdir;  /* x-major so always advance X */

            /* the AAbits most significant bits of erracc give us the intensity
            weighting for this pixel, and the complement of the weighting for
            the paired pixel. */
            wgt = (erracc >> intshift) & 255;

            a = (Uint8)(255-wgt);
            if (alpha != 255)
                a = (Uint8)(a*(float)(A>>16)/255);

            spg_pixelblend(surface,xx0,yy0,SDL_MapRGB(surface->format, (Uint8)(R>>16), (Uint8)(G>>16), (Uint8)(B>>16)),a);

            a = (Uint8)(wgt);
            if (alpha != 255)
                a = (Uint8)(a*(float)(A>>16)/255);

            spg_pixelblend(surface,xx0,y0p1,SDL_MapRGB(surface->format, (Uint8)(R>>16), (Uint8)(G>>16), (Uint8)(B>>16)),a);
        }
    }

    /* Draw final pixel, always exactly intersected by the line and doesn't
    need to be weighted. */
    if (alpha2==SDL_ALPHA_OPAQUE)
        spg_pixel(surface,x2,y2, SDL_MapRGB(surface->format,r2, g2, b2));
    else
        spg_pixelblend(surface,x2,y2, SDL_MapRGB(surface->format,r2, g2, b2), alpha2);

}

void SPG_LineFadeBlend(SDL_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color1, Uint8 alpha1, Uint32 color2, Uint8 alpha2)
{
    if (spg_lock(surface) < 0)
    {
        if(spg_useerrors)
            SPG_Error("SPG_LineFadeBlend could not lock surface");
        return;
    }

    if(spg_thickness == 1)
    {
        if(SPG_GetAA())
            spg_linefadeblendaa(surface, x1, y1, x2, y2, color1, alpha1, color2, alpha2);
        else
        {
            Sint16 dx, dy, sdx, sdy, x, y, px, py;

            dx = x2 - x1;
            dy = y2 - y1;

            sdx = (dx < 0) ? -1 : 1;
            sdy = (dy < 0) ? -1 : 1;

            dx = sdx * dx + 1;
            dy = sdy * dy + 1;

            x = y = 0;

            px = x1;
            py = y1;
            
            Uint8 r1, g1, b1, r2, g2, b2;
            
            if(surface->format->BitsPerPixel == 8)
            {
                SDL_GetRGB(color1, surface->format, &r1, &g1, &b1);
                SDL_GetRGB(color2, surface->format, &r2, &g2, &b2);
            }
            else
            {
                r1 = (color1 & surface->format->Rmask) >> surface->format->Rshift;
                g1 = (color1 & surface->format->Gmask) >> surface->format->Gshift;
                b1 = (color1 & surface->format->Bmask) >> surface->format->Bshift;
                r2 = (color2 & surface->format->Rmask) >> surface->format->Rshift;
                g2 = (color2 & surface->format->Gmask) >> surface->format->Gshift;
                b2 = (color2 & surface->format->Bmask) >> surface->format->Bshift;
            }

            /* We use fixedpoint math for the color fading */
            Sint32 R = r1<<16;
            Sint32 G = g1<<16;
            Sint32 B = b1<<16;
            Sint32 A = alpha1<<16;
            Sint32 R2 = r2<<16;
            Sint32 G2 = g2<<16;
            Sint32 B2 = b2<<16;
            Sint32 A2 = alpha2<<16;
            Sint32 rstep;
            Sint32 gstep;
            Sint32 bstep;
            Sint32 astep;

            if (dx >= dy)
            {
                rstep = (R2-R) / (Sint32)(dx);
                gstep = (G2-G) / (Sint32)(dx);
                bstep = (B2-B) / (Sint32)(dx);
                astep = (A2-A) / (Sint32)(dx);

                for (x = 0; x < dx; x++)
                {
                    spg_pixelblend(surface, px, py, SDL_MapRGB(surface->format, (Uint8)(R>>16), (Uint8)(G>>16), (Uint8)(B>>16)), (Uint8)(A>>16));

                    y += dy;
                    if (y >= dx)
                    {
                        y -= dx;
                        py += sdy;
                    }
                    px += sdx;

                    R += rstep;
                    G += gstep;
                    B += bstep;
                    A += astep;
                }
            }
            else
            {
                rstep = (Sint32)(R2-R) / (Sint32)(dy);
                gstep = (Sint32)(G2-G) / (Sint32)(dy);
                bstep = (Sint32)(B2-B) / (Sint32)(dy);
                astep = (Sint32)(A2-A) / (Sint32)(dy);

                for (y = 0; y < dy; y++)
                {
                    spg_pixelblend(surface, px, py, SDL_MapRGB(surface->format, (Uint8)(R>>16), (Uint8)(G>>16), (Uint8)(B>>16)), (Uint8)(A>>16));

                    x += dx;
                    if (x >= dy)
                    {
                        x -= dy;
                        px += sdx;
                    }
                    py += sdy;

                    R += rstep;
                    G += gstep;
                    B += bstep;
                    A += astep;
                }
            }
        }


        
        if(spg_makedirtyrects)
        {
            SDL_Rect rect;
            rect.x = MIN(x1, x2);
            rect.y = MIN(y1, y2);
            rect.w = MAX(x1, x2) - rect.x + 1;
            rect.h = MAX(y1, y2) - rect.y + 1;
            // Clip it to the screen
            SPG_DirtyClip(surface, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
    }
    else if(spg_thickness > 0)
    {
        
        Sint16 dx, dy, sdx, sdy, x, y, px, py;

        dx = x2 - x1;
        dy = y2 - y1;

        sdx = (dx < 0) ? -1 : 1;
        sdy = (dy < 0) ? -1 : 1;

        dx = sdx * dx + 1;
        dy = sdy * dy + 1;

        x = y = 0;

        px = x1;
        py = y1;

        Uint8 r1, g1, b1, r2, g2, b2;
        
        if(surface->format->BitsPerPixel == 8)
        {
            SDL_GetRGB(color1, surface->format, &r1, &g1, &b1);
            SDL_GetRGB(color2, surface->format, &r2, &g2, &b2);
        }
        else
        {
            r1 = (color1 & surface->format->Rmask) >> surface->format->Rshift;
            g1 = (color1 & surface->format->Gmask) >> surface->format->Gshift;
            b1 = (color1 & surface->format->Bmask) >> surface->format->Bshift;
            r2 = (color2 & surface->format->Rmask) >> surface->format->Rshift;
            g2 = (color2 & surface->format->Gmask) >> surface->format->Gshift;
            b2 = (color2 & surface->format->Bmask) >> surface->format->Bshift;
        }

        /* We use fixedpoint math for the color fading */
        Sint32 R = r1<<16;
        Sint32 G = g1<<16;
        Sint32 B = b1<<16;
        Sint32 A = alpha1<<16;
        Sint32 R2 = r2<<16;
        Sint32 G2 = g2<<16;
        Sint32 B2 = b2<<16;
        Sint32 A2 = alpha2<<16;
        Sint32 rstep;
        Sint32 gstep;
        Sint32 bstep;
        Sint32 astep;

        if (dx >= dy)
        {
            rstep = (R2-R) / (Sint32)(dx);
            gstep = (G2-G) / (Sint32)(dx);
            bstep = (B2-B) / (Sint32)(dx);
            astep = (A2-A) / (Sint32)(dx);

            for (x = 0; x < dx; x++)
            {
                spg_alphahack = (Uint8)(A>>16);
                spg_thicknesscallbackalpha(surface, px, py, SDL_MapRGB(surface->format, (Uint8)(R>>16), (Uint8)(G>>16), (Uint8)(B>>16)));

                y += dy;
                if (y >= dx)
                {
                    y -= dx;
                    py += sdy;
                }
                px += sdx;

                R += rstep;
                G += gstep;
                B += bstep;
                A += astep;
            }
        }
        else
        {
            rstep = (Sint32)(R2-R) / (Sint32)(dy);
            gstep = (Sint32)(G2-G) / (Sint32)(dy);
            bstep = (Sint32)(B2-B) / (Sint32)(dy);
            astep = (Sint32)(A2-A) / (Sint32)(dy);

            for (y = 0; y < dy; y++)
            {
                spg_alphahack = (Uint8)(A>>16);
                spg_thicknesscallbackalpha(surface, px, py, SDL_MapRGB(surface->format, (Uint8)(R>>16), (Uint8)(G>>16), (Uint8)(B>>16)));

                x += dx;
                if (x >= dy)
                {
                    x -= dy;
                    px += sdx;
                }
                py += sdy;

                R += rstep;
                G += gstep;
                B += bstep;
                A += astep;
            }
        }


        
        if(spg_makedirtyrects)
        {
            SDL_Rect rect;
            rect.x = MIN(x1, x2) - spg_thickness/2;
            rect.y = MIN(y1, y2) - spg_thickness/2;
            rect.w = MAX(x1, x2) - rect.x + 1 + spg_thickness;
            rect.h = MAX(y1, y2) - rect.y + 1 + spg_thickness;
            // Clip it to the screen
            SPG_DirtyClip(surface, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
    }

    spg_unlock(surface);

}







/**********************************************************************************/
/**                           Figure functions                                   **/
/**********************************************************************************/

//==================================================================================
// Draws a rectangle
//==================================================================================
void SPG_Rect(SDL_Surface *Surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color)
{
    if(spg_thickness == 1)
    {
        SPG_LineH(Surface,x1,y1,x2,color);
        SPG_LineH(Surface,x1,y2,x2,color);
        SPG_LineV(Surface,x1,y1,y2,color);
        SPG_LineV(Surface,x2,y1,y2,color);
    }
    else if(spg_thickness > 0)
    {
        SPG_Line(Surface,x1,y1,x2,y1,color);
        SPG_Line(Surface,x1,y2,x2,y2,color);
        SPG_Line(Surface,x1,y1,x1,y2,color);
        SPG_Line(Surface,x2,y1,x2,y2,color);
    }
    /*if(spg_makedirtyrects)
    {
        SDL_Rect rect;
        rect.x = x1;
        rect.y = MIN(y1, y2);
        rect.w = 1;
        rect.h = MAX(y1, y2) - rect.y + 1;
        
        SPG_DirtyClip(Surface, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        rect.x = x2;
        rect.y = MIN(y1, y2);
        rect.w = 1;
        rect.h = MAX(y1, y2) - rect.y + 1;
        
        SPG_DirtyClip(Surface, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        rect.x = MIN(x1, x2);
        rect.y = y1;
        rect.w = MAX(x1, x2) - rect.x + 1;
        rect.h = 1;
        
        SPG_DirtyClip(Surface, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        rect.x = MIN(x1, x2);
        rect.y = y2;
        rect.w = MAX(x1, x2) - rect.x + 1;
        rect.h = 1;
        
        SPG_DirtyClip(Surface, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
    }*/
}



//==================================================================================
// Draws a rectangle (alpha)
//==================================================================================
void SPG_RectBlend(SDL_Surface *Surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color, Uint8 alpha)
{
    /*if (spg_lock(Surface) < 0)
    {
        if(spg_useerrors)
            SPG_Error("SPG_RectBlend could not lock surface");
        return;
    }*/
    if(spg_thickness == 1)
    {
        SPG_LineHBlend(Surface,x1,y1,x2,color,alpha);
        SPG_LineHBlend(Surface,x1,y2,x2,color,alpha);
        SPG_LineVBlend(Surface,x1,y1,y2,color,alpha);
        SPG_LineVBlend(Surface,x2,y1,y2,color,alpha);
    }
    else if(spg_thickness > 0)
    {
        SPG_LineBlend(Surface,x1,y1,x2, y1, color,alpha);
        SPG_LineBlend(Surface,x1,y2,x2, y2, color,alpha);
        SPG_LineBlend(Surface,x1,y1,x1,y2,color,alpha);
        SPG_LineBlend(Surface,x2,y1,x2,y2,color,alpha);
    }
    //spg_unlock(Surface);
/*
    if(spg_makedirtyrects)
    {
        SDL_Rect rect;
        rect.x = x1;
        rect.y = MIN(y1, y2);
        rect.w = 1;
        rect.h = MAX(y1, y2) - rect.y + 1;
        // Clip it to the screen
        SPG_DirtyClip(Surface, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        rect.x = x2;
        rect.y = MIN(y1, y2);
        rect.w = 1;
        rect.h = MAX(y1, y2) - rect.y + 1;
        // Clip it to the screen
        SPG_DirtyClip(Surface, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        rect.x = MIN(x1, x2);
        rect.y = y1;
        rect.w = MAX(x1, x2) - rect.x + 1;
        rect.h = 1;
        // Clip it to the screen
        SPG_DirtyClip(Surface, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        rect.x = MIN(x1, x2);
        rect.y = y2;
        rect.w = MAX(x1, x2) - rect.x + 1;
        rect.h = 1;
        // Clip it to the screen
        SPG_DirtyClip(Surface, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
    }*/
}



//==================================================================================
// Draws a filled rectangle
//==================================================================================
void SPG_RectFilled(SDL_Surface *Surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color)
{
    Sint16 tmp;
    if (x1>x2)
    {
        tmp=x1;
        x1=x2;
        x2=tmp;
    }
    if (y1>y2)
    {
        tmp=y1;
        y1=y2;
        y2=tmp;
    }

#if SDL_VERSIONNUM(SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL) < \
    SDL_VERSIONNUM(1, 1, 5)
    if (x2<Surface->clip_minx || x1>Surface->clip_maxx || y2<Surface->clip_miny || y1>Surface->clip_maxy)
        return;
    if (x1 < Surface->clip_minx)
        x1=Surface->clip_minx;
    if (x2 > Surface->clip_maxx)
        x2=Surface->clip_maxx;
    if (y1 < Surface->clip_miny)
        y1=Surface->clip_miny;
    if (y2 > Surface->clip_maxy)
        y2=Surface->clip_maxy;
#endif

    SDL_Rect area;
    area.x=x1;
    area.y=y1;
    area.w=x2-x1+1;
    area.h=y2-y1+1;

    SDL_FillRect(Surface,&area,color);
    
    if(spg_makedirtyrects)
    {
        //SPG_DirtyClip(Surface, &area);
        SPG_DirtyAddTo(spg_dirtytable_front, &area);
    }
}



//==================================================================================
// Draws a filled rectangle (alpha)
//==================================================================================
void SPG_RectFilledBlend(SDL_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color, Uint8 alpha)
{

    if ( alpha == SDL_ALPHA_OPAQUE )
    {
        SPG_RectFilled(surface,x1,y1,x2,y2,color);
        return;
    }

    /* Fix coords */
    Sint16 tmp;
    if (x1>x2)
    {
        tmp=x1;
        x1=x2;
        x2=tmp;
    }
    if (y1>y2)
    {
        tmp=y1;
        y1=y2;
        y2=tmp;
    }

    /* Clipping */
    if (x2<SPG_CLIP_XMIN(surface) || x1>SPG_CLIP_XMAX(surface) || y2<SPG_CLIP_YMIN(surface) || y1>SPG_CLIP_YMAX(surface))
        return;
    if (x1 < SPG_CLIP_XMIN(surface))
        x1 = SPG_CLIP_XMIN(surface);
    if (x2 > SPG_CLIP_XMAX(surface))
        x2 = SPG_CLIP_XMAX(surface);
    if (y1 < SPG_CLIP_YMIN(surface))
        y1 = SPG_CLIP_YMIN(surface);
    if (y2 > SPG_CLIP_YMAX(surface))
        y2 = SPG_CLIP_YMAX(surface);

    Uint32 Rmask = surface->format->Rmask, Gmask = surface->format->Gmask, Bmask = surface->format->Bmask, Amask = surface->format->Amask;
    Uint32 R,G,B,A=0;
    Sint16 x,y;




    if (spg_lock(surface) < 0)
    {
        if(spg_useerrors)
            SPG_Error("SPG_RectFilledBlend could not lock surface");
        return;
    }

    switch (surface->format->BytesPerPixel)
    {
    case 1:   /* Assuming 8-bpp */
    {
        Uint8 *row, *pixel;
        Uint8 dR, dG, dB;

        Uint8 sR = surface->format->palette->colors[color].r;
        Uint8 sG = surface->format->palette->colors[color].g;
        Uint8 sB = surface->format->palette->colors[color].b;

        for (y = y1; y<=y2; y++)
        {
            row = (Uint8 *)surface->pixels + y*surface->pitch;
            for (x = x1; x <= x2; x++)
            {
                pixel = row + x;

                dR = surface->format->palette->colors[*pixel].r;
                dG = surface->format->palette->colors[*pixel].g;
                dB = surface->format->palette->colors[*pixel].b;

                dR = dR + ((sR-dR)*alpha >> 8);
                dG = dG + ((sG-dG)*alpha >> 8);
                dB = dB + ((sB-dB)*alpha >> 8);

                *pixel = SDL_MapRGB(surface->format, dR, dG, dB);
            }
        }
    }
    break;

    case 2:   /* Probably 15-bpp or 16-bpp */
    {
        Uint16 *row, *pixel;
        Uint32 dR=(color & Rmask),dG=(color & Gmask),dB=(color & Bmask),dA=(color & Amask);

        for (y = y1; y<=y2; y++)
        {
            row = (Uint16 *)surface->pixels + y*surface->pitch/2;
            for (x = x1; x <= x2; x++)
            {
                pixel = row + x;

                R = ((*pixel & Rmask) + (( dR - (*pixel & Rmask) ) * alpha >> 8)) & Rmask;
                G = ((*pixel & Gmask) + (( dG - (*pixel & Gmask) ) * alpha >> 8)) & Gmask;
                B = ((*pixel & Bmask) + (( dB - (*pixel & Bmask) ) * alpha >> 8)) & Bmask;
                if ( Amask )
                    A = ((*pixel & Amask) + (( dA - (*pixel & Amask) ) * alpha >> 8)) & Amask;

                *pixel = R | G | B | A;
            }
        }
    }
    break;

    case 3:   /* Slow 24-bpp mode, usually not used */
    {
        Uint8 *row,*pix;
        Uint8 dR, dG, dB, dA;
        Uint8 rshift8=surface->format->Rshift/8;
        Uint8 gshift8=surface->format->Gshift/8;
        Uint8 bshift8=surface->format->Bshift/8;
        Uint8 ashift8=surface->format->Ashift/8;

        Uint8 sR = (color>>surface->format->Rshift)&0xff;
        Uint8 sG = (color>>surface->format->Gshift)&0xff;
        Uint8 sB = (color>>surface->format->Bshift)&0xff;
        Uint8 sA = (color>>surface->format->Ashift)&0xff;
        
        for (y = y1; y<=y2; y++)
        {
            row = (Uint8 *)surface->pixels + y * surface->pitch;
            for (x = x1; x <= x2; x++)
            {
                pix = row + x*3;

                dR = *((pix)+rshift8);
                dG = *((pix)+gshift8);
                dB = *((pix)+bshift8);
                dA = *((pix)+ashift8);

                dR = dR + ((sR-dR)*alpha >> 8);
                dG = dG + ((sG-dG)*alpha >> 8);
                dB = dB + ((sB-dB)*alpha >> 8);
                dA = dA + ((sA-dA)*alpha >> 8);

                *((pix)+rshift8) = dR;
                *((pix)+gshift8) = dG;
                *((pix)+bshift8) = dB;
                *((pix)+ashift8) = dA;
            }
        }
    }
    break;

    case 4:   /* Probably 32-bpp */
    {
        Uint32 *row, *pixel;
        Uint32 dR=(color & Rmask),dG=(color & Gmask),dB=(color & Bmask),dA=(color & Amask);

        switch (SPG_GetBlend())
        {
        case SPG_COMBINE_ALPHA:  // Blend and combine src and dest alpha
            dA=((alpha << surface->format->Ashift) & Amask);  // correct
            for (y = y1; y<=y2; y++)
            {
                row = (Uint32 *)surface->pixels + y*surface->pitch/4;
                for (x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    R = ((*pixel & Rmask) + (( dR - (*pixel & Rmask) ) * alpha >> 8)) & Rmask;
                    G = ((*pixel & Gmask) + (( dG - (*pixel & Gmask) ) * alpha >> 8)) & Gmask;
                    B = ((*pixel & Bmask) + (( dB - (*pixel & Bmask) ) * alpha >> 8)) & Bmask;
                    if ( Amask )
                        A = ((((*pixel & Amask) >> surface->format->Ashift) + alpha) >> 1) << surface->format->Ashift;

                    *pixel= R | G | B | A;
                }
            }
            break;
        case SPG_DEST_ALPHA:  // Blend and keep dest alpha
            for (y = y1; y<=y2; y++)
            {
                row = (Uint32 *)surface->pixels + y*surface->pitch/4;
                for (x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    R = ((*pixel & Rmask) + (( dR - (*pixel & Rmask) ) * alpha >> 8)) & Rmask;
                    G = ((*pixel & Gmask) + (( dG - (*pixel & Gmask) ) * alpha >> 8)) & Gmask;
                    B = ((*pixel & Bmask) + (( dB - (*pixel & Bmask) ) * alpha >> 8)) & Bmask;
                    if ( Amask )
                        A = (*pixel & Amask);


                    *pixel= R | G | B | A;
                }
            }

            break;
        case SPG_SRC_ALPHA:  // Blend and keep src alpha
            if (Amask)
                A = (alpha << surface->format->Ashift);
            else
                A = SDL_ALPHA_OPAQUE;
            for (y = y1; y<=y2; y++)
            {
                row = (Uint32 *)surface->pixels + y*surface->pitch/4;
                for (x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    R = ((*pixel & Rmask) + (( dR - (*pixel & Rmask) ) * alpha >> 8)) & Rmask;
                    G = ((*pixel & Gmask) + (( dG - (*pixel & Gmask) ) * alpha >> 8)) & Gmask;
                    B = ((*pixel & Bmask) + (( dB - (*pixel & Bmask) ) * alpha >> 8)) & Bmask;

                    *pixel= R | G | B | A;  // A is src alpha here
                }
            }

            break;
        case SPG_COPY_SRC_ALPHA: // Direct copy with src alpha
            if (Amask)
                A = (alpha << surface->format->Ashift);
            else
                A = SDL_ALPHA_OPAQUE;
            for (y = y1; y<=y2; y++)
            {
                row = (Uint32 *)surface->pixels + y*surface->pitch/4;
                for (x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    *pixel= dR | dG | dB | A;  // A is src alpha here
                }
            }
            break;
        case SPG_COPY_DEST_ALPHA: // Direct copy with dest alpha
            for (y = y1; y<=y2; y++)
            {
                row = (Uint32 *)surface->pixels + y*surface->pitch/4;
                for (x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    *pixel= dR | dG | dB | (*pixel & Amask);
                }
            }
            break;
        case SPG_COPY_COMBINE_ALPHA: // Direct copy with combined alpha
            for (y = y1; y<=y2; y++)
            {
                row = (Uint32 *)surface->pixels + y*surface->pitch/4;
                for (x = x1; x <= x2; x++)
                {
                    pixel = row + x;
                    if (Amask)
                        A = ((((*pixel & Amask) >> surface->format->Ashift) + alpha) >> 1) << surface->format->Ashift;

                    *pixel= dR | dG | dB | A;
                }
            }
            break;
        case SPG_COPY_NO_ALPHA:  // Direct copy, alpha opaque
            SPG_RectFilled(surface,x1,y1,x2,y2,color);
            break;
        case SPG_COPY_ALPHA_ONLY:  // Direct copy of just the alpha
            for (y = y1; y<=y2; y++)
            {
                row = (Uint32 *)surface->pixels + y*surface->pitch/4;
                for (x = x1; x <= x2; x++)
                {
                    pixel = row + x;
                    if(Amask)
                        A = (alpha << surface->format->Ashift);

                    *pixel= dR | dG | dB | A;
                }
            }
            break;
        case SPG_COMBINE_ALPHA_ONLY:  // Blend of just the alpha
            for (y = y1; y<=y2; y++)
            {
                row = (Uint32 *)surface->pixels + y*surface->pitch/4;
                for (x = x1; x <= x2; x++)
                {
                    pixel = row + x;
                    if(Amask)
                        A = ((((*pixel & Amask) >> surface->format->Ashift) + alpha) >> 1) << surface->format->Ashift;

                    *pixel= dR | dG | dB | A;
                }
            }
            break;
        case SPG_REPLACE_COLORKEY:  // Replace the colorkeyed color
            for (y = y1; y<=y2; y++)
            {
                row = (Uint32 *)surface->pixels + y*surface->pitch/4;
                for (x = x1; x <= x2; x++)
                {
                    pixel = row + x;
                    if(!(surface->flags & SDL_SRCCOLORKEY) || *pixel != surface->format->colorkey)
                        return;
                    if(Amask)
                        A = (alpha << surface->format->Ashift);
                    *pixel= dR | dG | dB | A;
                }
            }
            break;
        }


    }
    break;
    }


    spg_unlock(surface);

    if(spg_makedirtyrects)
    {
        SDL_Rect rect;
        rect.x = MIN(x1, x2);
        rect.y = MIN(y1, y2);
        rect.w = MAX(x1, x2) - rect.x + 1;
        rect.h = MAX(y1, y2) - rect.y + 1;
        // Clip it to the screen
        SPG_DirtyClip(surface, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
    }

}



//==================================================================================
// Draws a rounded rectangle
//==================================================================================
void SPG_RectRound(SDL_Surface *Surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, float r, Uint32 color)
{
    Sint16 minX = (x1 < x2? x1 : x2) + (Sint16)(r);
    Sint16 maxX = (x1 > x2? x1 : x2) - (Sint16)(r);
    Sint16 minY = (y1 < y2? y1 : y2) + (Sint16)(r);
    Sint16 maxY = (y1 > y2? y1 : y2) - (Sint16)(r);
    SPG_LineH(Surface,minX,y1,maxX,color);
    SPG_LineH(Surface,minX,y2,maxX,color);
    SPG_LineV(Surface,x1,minY,maxY,color);
    SPG_LineV(Surface,x2,minY,maxY,color);

    SPG_Arc(Surface, minX, minY, r, 180, 270, color);
    SPG_Arc(Surface, maxX, minY, r, 270, 360, color);
    SPG_Arc(Surface, maxX, maxY, r, 0, 90, color);
    SPG_Arc(Surface, minX, maxY, r, 90, 180, color);
    /*if(spg_makedirtyrects)
    {
        SDL_Rect rect;
        rect.x = MIN(x1, x2);
        rect.y = MIN(y1, y2);
        rect.w = MAX(x1, x2) - rect.x + 1;
        rect.h = MAX(y1, y2) - rect.y + 1;
        // Clip it to the screen
        SPG_DirtyClip(Surface, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
    }*/
}



//==================================================================================
// Draws a rounded rectangle (alpha)
//==================================================================================
void SPG_RectRoundBlend(SDL_Surface *Surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, float r, Uint32 color, Uint8 alpha)
{
    if (spg_lock(Surface) < 0)
    {
        if(spg_useerrors)
            SPG_Error("SPG_RectRoundBlend could not lock surface");
        return;
    }

    Sint16 minX = (x1 < x2? x1 : x2) + (Sint16)(r) + 1;
    Sint16 maxX = (x1 > x2? x1 : x2) - (Sint16)(r) - 1;
    Sint16 minY = (y1 < y2? y1 : y2) + (Sint16)(r) + 1;
    Sint16 maxY = (y1 > y2? y1 : y2) - (Sint16)(r) - 1;
    SPG_LineHBlend(Surface,minX,y1,maxX,color,alpha);
    SPG_LineHBlend(Surface,minX,y2,maxX,color,alpha);
    SPG_LineVBlend(Surface,x1,minY,maxY,color,alpha);
    SPG_LineVBlend(Surface,x2,minY,maxY,color,alpha);
    SPG_ArcBlend(Surface, minX, minY, r, 180, 270, color, alpha);
    SPG_ArcBlend(Surface, maxX, minY, r, 270, 360, color, alpha);
    SPG_ArcBlend(Surface, maxX, maxY, r, 0, 90, color, alpha);
    SPG_ArcBlend(Surface, minX, maxY, r, 90, 180, color, alpha);


    spg_unlock(Surface);
    /*
    if(spg_makedirtyrects)
    {
        SDL_Rect rect;
        rect.x = MIN(x1, x2);
        rect.y = MIN(y1, y2);
        rect.w = MAX(x1, x2) - rect.x + 1;
        rect.h = MAX(y1, y2) - rect.y + 1;
        // Clip it to the screen
        SPG_DirtyClip(Surface, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
    }*/
}



//==================================================================================
// Draws a filled rounded rectangle
//==================================================================================
void SPG_RectRoundFilled(SDL_Surface *Surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, float r, Uint32 color)
{
    Sint16 tmp;
    if (x1>x2)
    {
        tmp=x1;
        x1=x2;
        x2=tmp;
    }
    if (y1>y2)
    {
        tmp=y1;
        y1=y2;
        y2=tmp;
    }

#if SDL_VERSIONNUM(SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL) < \
    SDL_VERSIONNUM(1, 1, 5)
    if (x2<Surface->clip_minx || x1>Surface->clip_maxx || y2<Surface->clip_miny || y1>Surface->clip_maxy)
        return;
    if (x1 < Surface->clip_minx)
        x1=Surface->clip_minx;
    if (x2 > Surface->clip_maxx)
        x2=Surface->clip_maxx;
    if (y1 < Surface->clip_miny)
        y1=Surface->clip_miny;
    if (y2 > Surface->clip_maxy)
        y2=Surface->clip_maxy;
#endif

    Sint16 minX = x1 + (Sint16)(r) + 1;
    Sint16 maxX = x2 - (Sint16)(r);
    Sint16 minY = y1 + (Sint16)(r);
    Sint16 maxY = y2 - (Sint16)(r) - 1;

    SDL_Rect area;
    area.x=minX;
    area.y=minY;
    area.w=maxX-minX+1;
    area.h=maxY-minY+1;
    SDL_FillRect(Surface,&area,color);

    area.x= minX;
    area.y= y1;
    area.w= maxX-minX+1;
    area.h= (Sint16)(r)+1;
    SDL_FillRect(Surface,&area,color);

    area.y= y2-(Sint16)(r);
    SDL_FillRect(Surface,&area,color);

    area.x= x1;
    area.y= minY;
    area.w= (Sint16)(r)+1;
    area.h= maxY-minY;
    SDL_FillRect(Surface,&area,color);

    area.x= x2-(Sint16)(r);
    SDL_FillRect(Surface,&area,color);

    SPG_CircleFilled(Surface, minX, minY+1, r+1, color);
    SPG_CircleFilled(Surface, maxX-1, minY+1, r+1, color);
    SPG_CircleFilled(Surface, maxX-1, maxY, r+1, color);
    SPG_CircleFilled(Surface, minX, maxY, r+1, color);
    if(spg_makedirtyrects)
    {
        SDL_Rect rect;
        rect.x = MIN(x1, x2);
        rect.y = MIN(y1, y2);
        rect.w = MAX(x1, x2) - rect.x + 1;
        rect.h = MAX(y1, y2) - rect.y + 1;
        // Clip it to the screen
        SPG_DirtyClip(Surface, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
    }
}

void SPG_RectRoundFilledBlend(SDL_Surface *Surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, float r, Uint32 color, Uint8 alpha)
{
    Sint16 tmp;
    if (x1>x2)
    {
        tmp=x1;
        x1=x2;
        x2=tmp;
    }
    if (y1>y2)
    {
        tmp=y1;
        y1=y2;
        y2=tmp;
    }

#if SDL_VERSIONNUM(SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL) < \
    SDL_VERSIONNUM(1, 1, 5)
    if (x2<Surface->clip_minx || x1>Surface->clip_maxx || y2<Surface->clip_miny || y1>Surface->clip_maxy)
        return;
    if (x1 < Surface->clip_minx)
        x1=Surface->clip_minx;
    if (x2 > Surface->clip_maxx)
        x2=Surface->clip_maxx;
    if (y1 < Surface->clip_miny)
        y1=Surface->clip_miny;
    if (y2 > Surface->clip_maxy)
        y2=Surface->clip_maxy;
#endif

    Sint16 minX = x1 + (Sint16)(r) + 1;
    Sint16 maxX = x2 - (Sint16)(r) - 1;
    Sint16 minY = y1 + (Sint16)(r) + 1;
    Sint16 maxY = y2 - (Sint16)(r) - 1;
    // Center
    SDL_Rect area;
    area.x=minX;
    area.y=minY;
    area.w=maxX-minX;
    area.h=maxY-minY;
    SPG_RectFilledBlend(Surface,area.x, area.y, area.x+area.w, area.y+area.h, color,alpha);
    // Top
    area.x= minX;
    area.y= y1;
    area.w= maxX-minX;
    area.h= (Sint16)(r);
    SPG_RectFilledBlend(Surface,area.x, area.y, area.x+area.w, area.y+area.h, color,alpha);
    // Bottom
    area.y= y2-(Sint16)(r);
    SPG_RectFilledBlend(Surface,area.x, area.y, area.x+area.w, area.y+area.h, color,alpha);
    // Left
    area.x= x1;
    area.y= minY-1;
    area.w= (Sint16)(r);
    area.h= maxY-minY+1;
    SPG_RectFilledBlend(Surface,area.x, area.y, area.x+area.w, area.y+area.h, color,alpha);
    // Right
    area.x= x2-(Sint16)(r);
    SPG_RectFilledBlend(Surface,area.x, area.y, area.x+area.w, area.y+area.h, color,alpha);


    // UL
    SPG_ArcFilledBlend(Surface, minX-1, minY-1, r, 180, 270, color, alpha);
    // UR
    SPG_ArcFilledBlend(Surface, maxX+1, minY-1, r, 270, 360, color, alpha);
    // DR
    SPG_ArcFilledBlend(Surface, maxX+1, maxY+1, r, 0, 90, color, alpha);
    // DL
    SPG_ArcFilledBlend(Surface, minX-1, maxY+1, r, 90, 180, color, alpha);
    if(spg_makedirtyrects)
    {
        SDL_Rect rect;
        rect.x = MIN(x1, x2);
        rect.y = MIN(y1, y2);
        rect.w = MAX(x1, x2) - rect.x + 1;
        rect.h = MAX(y1, y2) - rect.y + 1;
        // Clip it to the screen
        SPG_DirtyClip(Surface, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
    }

}






//==================================================================================
// Performs Callback at each ellipse point.
// (from Allegro)
//==================================================================================
void SPG_EllipseFn(SDL_Surface *Surface, Sint16 x, Sint16 y, float rx, float ry, Uint32 color, void Callback(SDL_Surface *Surf, Sint16 X, Sint16 Y, Uint32 Color) )
{
    int ix, iy;
    int h, i, j, k;
    int oh, oi, oj, ok;

    if (rx < 1)
        rx = 1;

    if (ry < 1)
        ry = 1;

    h = i = j = k = 0xFFFF;

    if (rx > ry)
    {
        ix = 0;
        iy = (Sint16)(rx) * 64;

        do
        {
            oh = h;
            oi = i;
            oj = j;
            ok = k;

            h = (ix + 32) >> 6;
            i = (iy + 32) >> 6;
            j = (h * (Sint16)(ry)) / (Sint16)(rx);
            k = (i * (Sint16)(ry)) / (Sint16)(rx);

            if (((h != oh) || (k != ok)) && (h < oi))
            {
                Callback(Surface, x+h, y+k, color);
                if (h)
                    Callback(Surface, x-h, y+k, color);
                if (k)
                {
                    Callback(Surface, x+h, y-k, color);
                    if (h)
                        Callback(Surface, x-h, y-k, color);
                }
            }

            if (((i != oi) || (j != oj)) && (h < i))
            {
                Callback(Surface, x+i, y+j, color);
                if (i)
                    Callback(Surface, x-i, y+j, color);
                if (j)
                {
                    Callback(Surface, x+i, y-j, color);
                    if (i)
                        Callback(Surface, x-i, y-j, color);
                }
            }

            ix = ix + iy / (Sint16)(rx);
            iy = iy - ix / (Sint16)(rx);

        }
        while (i > h);
    }
    else
    {
        ix = 0;
        iy = (Sint16)(ry) * 64;

        do
        {
            oh = h;
            oi = i;
            oj = j;
            ok = k;

            h = (ix + 32) >> 6;
            i = (iy + 32) >> 6;
            j = (h * (Sint16)(rx)) / (Sint16)(ry);
            k = (i * (Sint16)(rx)) / (Sint16)(ry);

            if (((j != oj) || (i != oi)) && (h < i))
            {
                Callback(Surface, x+j, y+i, color);
                if (j)
                    Callback(Surface, x-j, y+i, color);
                if (i)
                {
                    Callback(Surface, x+j, y-i, color);
                    if (j)
                        Callback(Surface, x-j, y-i, color);
                }
            }

            if (((k != ok) || (h != oh)) && (h < oi))
            {
                Callback(Surface, x+k, y+h, color);
                if (k)
                    Callback(Surface, x-k, y+h, color);
                if (h)
                {
                    Callback(Surface, x+k, y-h, color);
                    if (k)
                        Callback(Surface, x-k, y-h, color);
                }
            }

            ix = ix + iy / (Sint16)(ry);
            iy = iy - ix / (Sint16)(ry);

        }
        while (i > h);
    }
}



//==================================================================================
// Draws an anti-aliased ellipse (alpha)
// Some of this code is taken from "TwinLib" (http://www.twinlib.org) written by
// Nicolas Roard (nicolas@roard.com)
//==================================================================================
void spg_ellipseblendaa(SDL_Surface *surface, Sint16 xc, Sint16 yc, float rx, float ry, Uint32 color, Uint8 alpha)
{
    /* Sanity check */
    if (rx < 1)
        rx = 1;
    if (ry < 1)
        ry = 1;

    int a2 = (Sint16)(rx) * (Sint16)(rx);
    int b2 = (Sint16)(ry) * (Sint16)(ry);

    int ds = 2 * a2;
    int dt = 2 * b2;

    int dxt = (int)(a2 / sqrt(a2 + b2));

    int t = 0;
    int s = -2 * a2 * (Sint16)(ry);
    int d = 0;

    Sint16 x = xc;
    Sint16 y = yc - (Sint16)(ry);

    Sint16 xs, ys, dyt;
    float cp, is, ip, imax = 1.0;

    Uint8 s_alpha, p_alpha;
    float alpha_pp = (float)(alpha)/255;


    if ( spg_lock(surface) < 0 )
    {
        if(spg_useerrors)
            SPG_Error("spg_ellipseblendaa could not lock surface");
        return;
    }

    /* "End points" */
    spg_pixelblend(surface, x, y, color, alpha);
    spg_pixelblend(surface, 2*xc-x, y, color, alpha);

    spg_pixelblend(surface, x, 2*yc-y, color, alpha);
    spg_pixelblend(surface, 2*xc-x, 2*yc-y, color, alpha);

    int i;

    for (i = 1; i <= dxt; i++)
    {
        x--;
        d += t - b2;

        if (d >= 0)
            ys = y - 1;
        else if ((d - s - a2) > 0)
        {
            if ((2 * d - s - a2) >= 0)
                ys = y + 1;
            else
            {
                ys = y;
                y++;
                d -= s + a2;
                s += ds;
            }
        }
        else
        {
            y++;
            ys = y + 1;
            d -= s + a2;
            s += ds;
        }

        t -= dt;

        /* Calculate alpha */
        cp = (float)(abs(d)) / abs(s);
        is = (float)( cp * imax + 0.1 );
        ip = (float)( imax - is + 0.2 );

        /* Overflow check */
        if ( is > 1.0 )
            is = 1.0;
        if ( ip > 1.0 )
            ip = 1.0;

        /* Calculate alpha level */
        s_alpha = (Uint8)(is*255);
        p_alpha = (Uint8)(ip*255);
        if ( alpha != 255 )
        {
            s_alpha = (Uint8)(s_alpha*alpha_pp);
            p_alpha = (Uint8)(p_alpha*alpha_pp);
        }


        /* Upper half */
        spg_pixelblend(surface, x, y, color, p_alpha);
        spg_pixelblend(surface, 2*xc-x, y, color, p_alpha);

        spg_pixelblend(surface, x, ys, color, s_alpha);
        spg_pixelblend(surface, 2*xc-x, ys, color, s_alpha);


        /* Lower half */
        spg_pixelblend(surface, x, 2*yc-y, color, p_alpha);
        spg_pixelblend(surface, 2*xc-x, 2*yc-y, color, p_alpha);

        spg_pixelblend(surface, x, 2*yc-ys, color, s_alpha);
        spg_pixelblend(surface, 2*xc-x, 2*yc-ys, color, s_alpha);
    }

    dyt = abs(y - yc);

    for (i = 1; i <= dyt; i++)
    {
        y++;
        d -= s + a2;

        if (d <= 0)
            xs = x + 1;
        else if ((d + t - b2) < 0)
        {
            if ((2 * d + t - b2) <= 0)
                xs = x - 1;
            else
            {
                xs = x;
                x--;
                d += t - b2;
                t -= dt;
            }
        }
        else
        {
            x--;
            xs = x - 1;
            d += t - b2;
            t -= dt;
        }

        s += ds;

        /* Calculate alpha */
        cp = (float)(abs(d)) / abs(t);
        is = (float)( cp * imax + 0.1 );
        ip = (float)( imax - is + 0.2 );

        /* Overflow check */
        if ( is > 1.0 )
            is = 1.0;
        if ( ip > 1.0 )
            ip = 1.0;

        /* Calculate alpha level */
        s_alpha = (Uint8)(is*255);
        p_alpha = (Uint8)(ip*255);
        if ( alpha != 255 )
        {
            s_alpha = (Uint8)(s_alpha*alpha_pp);
            p_alpha = (Uint8)(p_alpha*alpha_pp);
        }


        /* Upper half */
        spg_pixelblend(surface, x, y, color, p_alpha);
        spg_pixelblend(surface, 2*xc-x, y, color, p_alpha);

        spg_pixelblend(surface, xs, y, color, s_alpha);
        spg_pixelblend(surface, 2*xc-xs, y, color, s_alpha);


        /* Lower half*/
        spg_pixelblend(surface, x, 2*yc-y, color, p_alpha);
        spg_pixelblend(surface, 2*xc-x, 2*yc-y, color, p_alpha);

        spg_pixelblend(surface, xs, 2*yc-y, color, s_alpha);
        spg_pixelblend(surface, 2*xc-xs, 2*yc-y, color, s_alpha);
    }


    spg_unlock(surface);

}


//==================================================================================
// Draws an ellipse
//==================================================================================
void SPG_Ellipse(SDL_Surface *Surface, Sint16 x, Sint16 y, float rx, float ry, Uint32 color)
{    
    if (spg_lock(Surface) < 0)
    {
        if(spg_useerrors)
            SPG_Error("SPG_Ellipse could not lock surface");
        return;
    }
 
    if(spg_thickness == 1)
    {
        if(SPG_GetAA())
            spg_ellipseblendaa(Surface, x, y, rx, ry, color, SDL_ALPHA_OPAQUE);
        else
            SPG_EllipseFn(Surface, x, y, rx, ry, color, spg_pixel);
            
        if(spg_makedirtyrects)
        {
            SDL_Rect rect;
            rect.x = x-rx;
            rect.y = y-ry;
            rect.w = 2*(Sint16)(rx) + 1;  // +1 for even diameter?
            rect.h = 2*(Sint16)(ry) + 1;
            // Clip it to the screen
            SPG_DirtyClip(Surface, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
    }
    else if(spg_thickness > 0)
    {
        SPG_EllipseFn(Surface, x, y, rx, ry, color, spg_thicknesscallback);

        if(spg_makedirtyrects)
        {
            SDL_Rect rect;
            rect.x = x-rx-spg_thickness/2;
            rect.y = y-ry-spg_thickness/2;
            rect.w = 2*(Sint16)(rx) + 1 + spg_thickness;  // +1 for even diameter?
            rect.h = 2*(Sint16)(ry) + 1 + spg_thickness;
            // Clip it to the screen
            SPG_DirtyClip(Surface, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
    }

    spg_unlock(Surface);
}




//==================================================================================
// Draws an ellipse (alpha)
//==================================================================================
void SPG_EllipseBlend(SDL_Surface *Surface, Sint16 x, Sint16 y, float rx, float ry, Uint32 color, Uint8 alpha)
{
    if (spg_lock(Surface) < 0)
    {
        if(spg_useerrors)
            SPG_Error("SPG_EllipseBlend could not lock surface");
        return;
    }
    
    if(spg_thickness == 1)
    {
        if(SPG_GetAA())
            spg_ellipseblendaa(Surface, x, y, rx, ry, color, alpha);
        else
        {
            spg_alphahack = alpha;
            SPG_EllipseFn(Surface, x, y, rx, ry, color, spg_pixelcallbackalpha);

            if(spg_makedirtyrects)
            {
                SDL_Rect rect;
                rect.x = x-rx;
                rect.y = y-ry;
                rect.w = 2*(Sint16)(rx) + 1;  // +1 for even diameter?
                rect.h = 2*(Sint16)(ry) + 1;
                // Clip it to the screen
                SPG_DirtyClip(Surface, &rect);
                SPG_DirtyAddTo(spg_dirtytable_front, &rect);
            }
        }
    }                
    else if(spg_thickness > 0)
    {
        SPG_EllipseFn(Surface, x, y, rx, ry, color, spg_thicknesscallbackalpha);

        if(spg_makedirtyrects)
        {
            SDL_Rect rect;
            rect.x = x-rx-spg_thickness/2;
            rect.y = y-ry-spg_thickness/2;
            rect.w = 2*(Sint16)(rx) + 1 + spg_thickness;  // +1 for even diameter?
            rect.h = 2*(Sint16)(ry) + 1 + spg_thickness;
            // Clip it to the screen
            SPG_DirtyClip(Surface, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
    }

    spg_unlock(Surface);
}




//==================================================================================
// Draws a filled anti-aliased ellipse
// This is just a quick hack...
//==================================================================================
void spg_ellipsefilledaa(SDL_Surface *surface, Sint16 xc, Sint16 yc, float rx, float ry, Uint32 color)
{
    /* Sanity check */
    if (rx < 1)
        rx = 1;
    if (ry < 1)
        ry = 1;

    int a2 = (Sint16)(rx) * (Sint16)(rx);
    int b2 = (Sint16)(ry) * (Sint16)(ry);

    int ds = 2 * a2;
    int dt = 2 * b2;

    int dxt = (int)(a2 / sqrt(a2 + b2));

    int t = 0;
    int s = -2 * a2 * (Sint16)(ry);
    int d = 0;

    Sint16 x = xc;
    Sint16 y = yc - (Sint16)(ry);

    Sint16 xs, ys, dyt;
    float cp, is, ip, imax = 1.0;

    if ( spg_lock(surface) < 0 )
    {
        if(spg_useerrors)
            SPG_Error("spg_ellipsefilledaa could not lock surface");
        return;
    }

    /* "End points" */
    spg_pixel(surface, x, y, color);
    spg_pixel(surface, 2*xc-x, y, color);

    spg_pixel(surface, x, 2*yc-y, color);
    spg_pixel(surface, 2*xc-x, 2*yc-y, color);

    spg_unlock(surface);

    spg_linev(surface, x, y+1, 2*yc-y-1, color);

    int i;

    for (i = 1; i <= dxt; i++)
    {
        x--;
        d += t - b2;

        if (d >= 0)
            ys = y - 1;
        else if ((d - s - a2) > 0)
        {
            if ((2 * d - s - a2) >= 0)
                ys = y + 1;
            else
            {
                ys = y;
                y++;
                d -= s + a2;
                s += ds;
            }
        }
        else
        {
            y++;
            ys = y + 1;
            d -= s + a2;
            s += ds;
        }

        t -= dt;

        /* Calculate alpha */
        cp = (float) abs(d) / abs(s);
        is = cp * imax;
        ip = imax - is;

        if ( spg_lock(surface) < 0 )
            return;

        /* Upper half */
        spg_pixelblend(surface, x, y, color, (Uint8)(ip*255));
        spg_pixelblend(surface, 2*xc-x, y, color, (Uint8)(ip*255));

        spg_pixelblend(surface, x, ys, color, (Uint8)(is*255));
        spg_pixelblend(surface, 2*xc-x, ys, color, (Uint8)(is*255));


        /* Lower half */
        spg_pixelblend(surface, x, 2*yc-y, color, (Uint8)(ip*255));
        spg_pixelblend(surface, 2*xc-x, 2*yc-y, color, (Uint8)(ip*255));

        spg_pixelblend(surface, x, 2*yc-ys, color, (Uint8)(is*255));
        spg_pixelblend(surface, 2*xc-x, 2*yc-ys, color, (Uint8)(is*255));

        spg_unlock(surface);


        /* Fill */
        spg_linev(surface, x, y+1, 2*yc-y-1, color);
        spg_linev(surface, 2*xc-x, y+1, 2*yc-y-1, color);
        spg_linev(surface, x, ys+1, 2*yc-ys-1, color);
        spg_linev(surface, 2*xc-x, ys+1, 2*yc-ys-1, color);
    }

    dyt = abs(y - yc);

    for (i = 1; i <= dyt; i++)
    {
        y++;
        d -= s + a2;

        if (d <= 0)
            xs = x + 1;
        else if ((d + t - b2) < 0)
        {
            if ((2 * d + t - b2) <= 0)
                xs = x - 1;
            else
            {
                xs = x;
                x--;
                d += t - b2;
                t -= dt;
            }
        }
        else
        {
            x--;
            xs = x - 1;
            d += t - b2;
            t -= dt;
        }

        s += ds;

        /* Calculate alpha */
        cp = (float) abs(d) / abs(t);
        is = cp * imax;
        ip = imax - is;


        if ( spg_lock(surface) < 0 )
        {
            if(spg_useerrors)
                SPG_Error("spg_ellipsefilledaa could not lock surface");
            return;
        }

        /* Upper half */
        spg_pixelblend(surface, x, y, color, (Uint8)(ip*255));
        spg_pixelblend(surface, 2*xc-x, y, color, (Uint8)(ip*255));

        spg_pixelblend(surface, xs, y, color, (Uint8)(is*255));
        spg_pixelblend(surface, 2*xc-xs, y, color, (Uint8)(is*255));


        /* Lower half*/
        spg_pixelblend(surface, x, 2*yc-y, color, (Uint8)(ip*255));
        spg_pixelblend(surface, 2*xc-x, 2*yc-y, color, (Uint8)(ip*255));

        spg_pixelblend(surface, xs, 2*yc-y, color, (Uint8)(is*255));
        spg_pixelblend(surface, 2*xc-xs, 2*yc-y, color, (Uint8)(is*255));

        spg_unlock(surface);

        /* Fill */
        spg_lineh(surface, x+1, y, 2*xc-x-1, color);
        spg_lineh(surface, xs+1, y, 2*xc-xs-1, color);
        spg_lineh(surface, x+1, 2*yc-y, 2*xc-x-1, color);
        spg_lineh(surface, xs+1, 2*yc-y, 2*xc-xs-1, color);
    }

}


//==================================================================================
// Draws a filled ellipse
//==================================================================================
void SPG_EllipseFilled(SDL_Surface *Surface, Sint16 x, Sint16 y, float rx, float ry, Uint32 color)
{

    if(SPG_GetAA())
        spg_ellipsefilledaa(Surface, x, y, rx, ry, color);
    else
    {
        int ix, iy;
        int h, i, j, k;
        int oh, oi, oj, ok;

        if (rx < 1)
            rx = 1;

        if (ry < 1)
            ry = 1;

        oh = oi = oj = ok = 0xFFFF;

        if (rx > ry)
        {
            ix = 0;
            iy = (Sint16)(rx) * 64;

            do
            {
                h = (ix + 32) >> 6;
                i = (iy + 32) >> 6;
                j = (h * (Sint16)(ry)) / (Sint16)(rx);
                k = (i * (Sint16)(ry)) / (Sint16)(rx);

                if ((k!=ok) && (k!=oj))
                {
                    if (k)
                    {
                        spg_lineh(Surface,x-h,y-k,x+h,color);
                        spg_lineh(Surface,x-h,y+k,x+h,color);
                    }
                    else
                        spg_lineh(Surface,x-h,y,x+h,color);
                    ok=k;
                }

                if ((j!=oj) && (j!=ok) && (k!=j))
                {
                    if (j)
                    {
                        spg_lineh(Surface,x-i,y-j,x+i,color);
                        spg_lineh(Surface,x-i,y+j,x+i,color);
                    }
                    else
                        spg_lineh(Surface,x-i,y,x+i,color);
                    oj=j;
                }

                ix = ix + iy / (Sint16)(rx);
                iy = iy - ix / (Sint16)(rx);

            }
            while (i > h);
        }
        else
        {
            ix = 0;
            iy = (Sint16)(ry) * 64;

            do
            {
                h = (ix + 32) >> 6;
                i = (iy + 32) >> 6;
                j = (h * (Sint16)(rx)) / (Sint16)(ry);
                k = (i * (Sint16)(rx)) / (Sint16)(ry);

                if ((i!=oi) && (i!=oh))
                {
                    if (i)
                    {
                        spg_lineh(Surface,x-j,y-i,x+j,color);
                        spg_lineh(Surface,x-j,y+i,x+j,color);
                    }
                    else
                        spg_lineh(Surface,x-j,y,x+j,color);
                    oi=i;
                }

                if ((h!=oh) && (h!=oi) && (i!=h))
                {
                    if (h)
                    {
                        spg_lineh(Surface,x-k,y-h,x+k,color);
                        spg_lineh(Surface,x-k,y+h,x+k,color);
                    }
                    else
                        spg_lineh(Surface,x-k,y,x+k,color);
                    oh=h;
                }

                ix = ix + iy / (Sint16)(ry);
                iy = iy - ix / (Sint16)(ry);

            }
            while (i > h);
        }
    }
    if(spg_makedirtyrects)
    {
        SDL_Rect rect;
        rect.x = x-rx;
        rect.y = y-ry;
        rect.w = 2*(Sint16)(rx) + 1;  // +1 for even diameter?
        rect.h = 2*(Sint16)(ry) + 1;
        // Clip it to the screen
        SPG_DirtyClip(Surface, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
    }


}



void spg_ellipsefilledblendaa(SDL_Surface *Surface, Sint16 x, Sint16 y, float rx, float ry, Uint32 color, Uint8 alpha)
{
    int ix, iy;
    int h, i, j, k;
    int oh, oi, oj, ok;

    if (spg_lock(Surface) < 0)
    {
        if(spg_useerrors)
            SPG_Error("spg_ellipsefilledblendaa could not lock surface");
        return;
    }


    if (rx < 1)
        rx = 1;

    if (ry < 1)
        ry = 1;

    oh = oi = oj = ok = 0xFFFF;

    if (rx > ry)
    {
        ix = 0;
        iy = (Sint16)(rx) * 64;

        do
        {
            h = (ix + 32) >> 6;
            i = (iy + 32) >> 6;
            j = (h * (Sint16)(ry)) / (Sint16)(rx);
            k = (i * (Sint16)(ry)) / (Sint16)(rx);

            if ((k!=ok) && (k!=oj))
            {
                if (k)
                {
                    spg_linehblend(Surface,x-h,y-k,x+h,color, alpha);
                    spg_linehblend(Surface,x-h,y+k,x+h,color, alpha);
                }
                else
                    spg_linehblend(Surface,x-h,y,x+h,color, alpha);
                ok=k;
            }

            if ((j!=oj) && (j!=ok) && (k!=j))
            {
                if (j)
                {
                    spg_linehblend(Surface,x-i,y-j,x+i,color, alpha);
                    spg_linehblend(Surface,x-i,y+j,x+i,color, alpha);
                }
                else
                    spg_linehblend(Surface,x-i,y,x+i,color, alpha);
                oj=j;
            }

            ix = ix + iy / (Sint16)(rx);
            iy = iy - ix / (Sint16)(rx);

        }
        while (i > h);
    }
    else
    {
        ix = 0;
        iy = (Sint16)(ry) * 64;

        do
        {
            h = (ix + 32) >> 6;
            i = (iy + 32) >> 6;
            j = (h * (Sint16)(rx)) / (Sint16)(ry);
            k = (i * (Sint16)(rx)) / (Sint16)(ry);

            if ((i!=oi) && (i!=oh))
            {
                if (i)
                {
                    spg_linehblend(Surface,x-j,y-i,x+j,color, alpha);
                    spg_linehblend(Surface,x-j,y+i,x+j,color, alpha);
                }
                else
                    spg_linehblend(Surface,x-j,y,x+j,color, alpha);
                oi=i;
            }

            if ((h!=oh) && (h!=oi) && (i!=h))
            {
                if (h)
                {
                    spg_linehblend(Surface,x-k,y-h,x+k,color, alpha);
                    spg_linehblend(Surface,x-k,y+h,x+k,color, alpha);
                }
                else
                    spg_linehblend(Surface,x-k,y,x+k,color, alpha);
                oh=h;
            }

            ix = ix + iy / (Sint16)(ry);
            iy = iy - ix / (Sint16)(ry);

        }
        while (i > h);
    }

    spg_ellipseblendaa(Surface, x, y, rx, ry, color, (Uint8)(alpha/1.5));

    spg_unlock(Surface);

}


//==================================================================================
// Draws a filled ellipse (alpha)
//==================================================================================
void SPG_EllipseFilledBlend(SDL_Surface *Surface, Sint16 x, Sint16 y, float rx, float ry, Uint32 color, Uint8 alpha)
{

    if(SPG_GetAA())
        spg_ellipsefilledblendaa(Surface, x, y, rx, ry, color, alpha);
    else
    {
        int ix, iy;
        int h, i, j, k;
        int oh, oi, oj, ok;

        if (spg_lock(Surface) < 0)
        {
            if(spg_useerrors)
                SPG_Error("SPG_EllipseFilledBlend could not lock surface");
            return;
        }

        if (rx < 1)
            rx = 1;

        if (ry < 1)
            ry = 1;

        oh = oi = oj = ok = 0xFFFF;

        if (rx > ry)
        {
            ix = 0;
            iy = (Sint16)(rx) * 64;

            do
            {
                h = (ix + 32) >> 6;
                i = (iy + 32) >> 6;
                j = (h * (Sint16)(ry)) / (Sint16)(rx);
                k = (i * (Sint16)(ry)) / (Sint16)(rx);

                if ((k!=ok) && (k!=oj))
                {
                    if (k)
                    {
                        spg_linehblend(Surface,x-h,y-k,x+h,color, alpha);
                        spg_linehblend(Surface,x-h,y+k,x+h,color, alpha);
                    }
                    else
                        spg_linehblend(Surface,x-h,y,x+h,color, alpha);
                    ok=k;
                }

                if ((j!=oj) && (j!=ok) && (k!=j))
                {
                    if (j)
                    {
                        spg_linehblend(Surface,x-i,y-j,x+i,color, alpha);
                        spg_linehblend(Surface,x-i,y+j,x+i,color, alpha);
                    }
                    else
                        spg_linehblend(Surface,x-i,y,x+i,color, alpha);
                    oj=j;
                }

                ix = ix + iy / (Sint16)(rx);
                iy = iy - ix / (Sint16)(rx);

            }
            while (i > h);
        }
        else
        {
            ix = 0;
            iy = (Sint16)(ry) * 64;

            do
            {
                h = (ix + 32) >> 6;
                i = (iy + 32) >> 6;
                j = (h * (Sint16)(rx)) / (Sint16)(ry);
                k = (i * (Sint16)(rx)) / (Sint16)(ry);

                if ((i!=oi) && (i!=oh))
                {
                    if (i)
                    {
                        spg_linehblend(Surface,x-j,y-i,x+j,color, alpha);
                        spg_linehblend(Surface,x-j,y+i,x+j,color, alpha);
                    }
                    else
                        spg_linehblend(Surface,x-j,y,x+j,color, alpha);
                    oi=i;
                }

                if ((h!=oh) && (h!=oi) && (i!=h))
                {
                    if (h)
                    {
                        spg_linehblend(Surface,x-k,y-h,x+k,color, alpha);
                        spg_linehblend(Surface,x-k,y+h,x+k,color, alpha);
                    }
                    else
                        spg_linehblend(Surface,x-k,y,x+k,color, alpha);
                    oh=h;
                }

                ix = ix + iy / (Sint16)(ry);
                iy = iy - ix / (Sint16)(ry);

            }
            while (i > h);
        }

        spg_unlock(Surface);
    }
    if(spg_makedirtyrects)
    {
        SDL_Rect rect;
        rect.x = x-rx;
        rect.y = y-ry;
        rect.w = 2*(Sint16)(rx) + 1;  // +1 for even diameter?
        rect.h = 2*(Sint16)(ry) + 1;
        // Clip it to the screen
        SPG_DirtyClip(Surface, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
    }


}



void SPG_EllipseBlendArb(SDL_Surface *Surface, Sint16 x, Sint16 y, float rx, float ry, float angle, Uint32 color, Uint8 alpha)
{
    
}

void SPG_EllipseArb(SDL_Surface *Surface, Sint16 x, Sint16 y, float rx, float ry, float angle, Uint32 color)
{
    SPG_EllipseBlendArb(Surface, x, y, rx, ry, angle, color, SDL_ALPHA_OPAQUE);
}

void SPG_EllipseFilledBlendArb(SDL_Surface *Surface, Sint16 x, Sint16 y, float rx, float ry, float angle, Uint32 color, Uint8 alpha)
{
    
}

void SPG_EllipseFilledArb(SDL_Surface *Surface, Sint16 x, Sint16 y, float rx, float ry, float angle, Uint32 color)
{
    SPG_EllipseFilledBlendArb(Surface, x, y, rx, ry, angle, color, SDL_ALPHA_OPAQUE);
}












//==================================================================================
// Performs Callback at each circle point.
//==================================================================================
void SPG_CircleFn(SDL_Surface *Surface, Sint16 x, Sint16 y, float r, Uint32 color, void Callback(SDL_Surface *Surf, Sint16 X, Sint16 Y, Uint32 Color))
{
    if(r < 0)
        return;
    SPG_bool offset = 0; // even diameter fix
    Sint16 effr = (Sint16)(r);
    if(r == 0)
    {
        //Callback(Surface, x, y, color);
        return;
    }
    else if(r - effr > 0.33333f && r - effr < 0.66667f)
        offset = 1;
    else if(r - effr >= 0.66667f)
        effr++;

    Sint16 cx = 0;
    Sint16 cy = effr;
    Sint16 df = 1 - effr;
    Sint16 d_e = 3;
    Sint16 d_se = -2 * effr + 5;


    do
    {
        if(cx || offset)
        {
            Callback(Surface, x+cx+offset, y+cy+offset, color);
            Callback(Surface, x+cy+offset, y-cx, color);
            Callback(Surface, x-cx, y-cy, color);
            Callback(Surface, x-cy, y-cx, color);
        }
        
        Callback(Surface, x+cx+offset, y-cy, color);
        Callback(Surface, x-cx, y+cy+offset, color);
        Callback(Surface, x+cy+offset, y+cx+offset, color);
        Callback(Surface, x-cy, y+cx+offset, color);

        if (df < 0)
        {
            df += d_e;
            d_e += 2;
            d_se += 2;
        }
        else
        {
            df += d_se;
            d_e += 2;
            d_se += 4;
            cy--;
        }

        cx++;

    }
    while (cx <= cy);

}




//==================================================================================
// Draws a circle
//==================================================================================
void SPG_Circle(SDL_Surface *Surface, Sint16 x, Sint16 y, float r, Uint32 color)
{
    if(r < 0)
        return;
        
    if (spg_lock(Surface) < 0)
    {
        if(spg_useerrors)
            SPG_Error("SPG_Circle could not lock surface");
        return;
    }
    
    if(spg_thickness == 1)
    {
        if(SPG_GetAA())
            spg_ellipseblendaa(Surface, x, y, r, r, color, SDL_ALPHA_OPAQUE);
        else
            SPG_CircleFn(Surface, x, y, r, color, spg_pixel);

        if(spg_makedirtyrects)
        {
            Sint16 rr = (Sint16)(r);
            SDL_Rect rect = {x - rr, y - rr, 2*rr + 2, 2*rr + 2};  // +1 to get to the edge, +1 more for even-diameter
            // Clip it to the screen
            SPG_DirtyClip(Surface, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
            
    }
    else if(spg_thickness != 0)
    {
        SPG_CircleFn(Surface, x, y, r, color, spg_thicknesscallback);
        if(spg_makedirtyrects)
        {
            Sint16 rr = (Sint16)(r);
            SDL_Rect rect = {x - rr - spg_thickness/2, y - rr - spg_thickness/2, 2*rr + 2 + spg_thickness, 2*rr + 2 + spg_thickness};  // +1 to get to the edge, +1 more for even-diameter
            // Clip it to the screen
            SPG_DirtyClip(Surface, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
    }

    spg_unlock(Surface);

}



//==================================================================================
// Draws a circle (alpha)
//==================================================================================
void SPG_CircleBlend(SDL_Surface *Surface, Sint16 x, Sint16 y, float r, Uint32 color, Uint8 alpha)
{
    if(r < 0)
        return;
    
    if (spg_lock(Surface) < 0)
    {
        if(spg_useerrors)
            SPG_Error("SPG_CircleBlend could not lock surface");
        return;
    }
        
    if(spg_thickness == 1)
    {
        spg_alphahack = alpha;
        if(SPG_GetAA())
            spg_ellipseblendaa(Surface, x, y, r, r, color, alpha);
        else
            SPG_CircleFn(Surface, x, y, r, color, spg_pixelcallbackalpha);

        if(spg_makedirtyrects)
        {
            Sint16 rr = (Sint16)(r);
            SDL_Rect rect = {x - rr, y - rr, 2*rr + 2, 2*rr + 2};  // +1 to get to the edge, +1 more for even-diameter
            // Clip it to the screen
            SPG_DirtyClip(Surface, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
    }
    else if(spg_thickness != 0)
    {
        SPG_CircleFn(Surface, x, y, r, color, spg_thicknesscallbackalpha);
        if(spg_makedirtyrects)
        {
            Sint16 rr = (Sint16)(r);
            SDL_Rect rect = {x - rr - spg_thickness/2, y - rr - spg_thickness/2, 2*rr + 2 + spg_thickness, 2*rr + 2 + spg_thickness};  // +1 to get to the edge, +1 more for even-diameter
            // Clip it to the screen
            SPG_DirtyClip(Surface, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
    }
    
    spg_unlock(Surface);

}




//==================================================================================
// Draws a filled circle
//==================================================================================
void SPG_CircleFilled(SDL_Surface *Surface, Sint16 x, Sint16 y, float r, Uint32 color)
{
    if(r < 0)
        return;
    SPG_bool offset = 0; // even diameter fix
    Sint16 effr = (Sint16)(r);
    if(r == 0)
    {
        SPG_Pixel(Surface, x, y, color);  // this is necessary for thickness stuff
        return;
    }
    else if(r - effr > 0.33333f && r - effr < 0.66667f)
        offset = 1;
    else if(r - effr >= 0.66667f)
        effr++;
    
    
    if(SPG_GetAA())
        spg_ellipsefilledaa(Surface, x, y, r, r, color);
    else
    {
        Sint16 cx = 0;
        Sint16 cy = effr;
        Sint16 df = 1 - effr;
        Sint16 d_e = 3;
        Sint16 d_se = -2 * effr + 5;

        do
        {
            if(df >= 0)
            {
                spg_lineh(Surface,x-cx,y+cy+offset,x+cx+offset,color);
                spg_lineh(Surface,x-cx,y-cy,x+cx+offset,color);
            }
            
            if(cx != cy)
            {
                spg_lineh(Surface,x-cy,y+cx+offset,x+cy+offset,color);
                if(cx || offset)
                {
                    spg_lineh(Surface,x-cy,y-cx,x+cy+offset,color);
                }
            }

            if (df < 0)
            {
                df += d_e;
                d_e += 2;
                d_se += 2;
            }
            else
            {
                df += d_se;
                d_e += 2;
                d_se += 4;
                cy--;
            }
            cx++;
        }
        while (cx <= cy);
    }
    if(spg_makedirtyrects)
    {
        Sint16 rr = (Sint16)(r);
        SDL_Rect rect = {x - rr, y - rr, 2*rr + 2, 2*rr + 2};  // +1 to get to the edge, +1 more for even-diameter
        // Clip it to the screen
        SPG_DirtyClip(Surface, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
    }

}



//==================================================================================
// Draws a filled circle (alpha)
//==================================================================================
void SPG_CircleFilledBlend(SDL_Surface *Surface, Sint16 x, Sint16 y, float r, Uint32 color, Uint8 alpha)
{
    if(r < 0)
        return;
    if(SPG_GetAA())
        spg_ellipsefilledblendaa(Surface, x, y, r, r, color, alpha);
    else
    {
        SPG_bool offset = 0; // even diameter fix
        Sint16 effr = (Sint16)(r);
        if(r == 0)
        {
            SPG_PixelBlend(Surface, x, y, color, alpha);
            return;
        }
        else if(r - effr > 0.33333f && r - effr < 0.66667f)
            offset = 1;
        else if(r - effr >= 0.66667f)
            effr++;
        
        Sint16 cx = 0;
        Sint16 cy = effr;
        Sint16 df = 1 - effr;
        Sint16 d_e = 3;
        Sint16 d_se = -2 * effr + 5;

        if (spg_lock(Surface) < 0)
        {
            if(spg_useerrors)
                SPG_Error("SPG_CircleFilledBlend could not lock surface");
            return;
        }

        do
        {
            if(df >= 0)
            {
                spg_linehblend(Surface,x-cx,y+cy+offset,x+cx+offset,color, alpha);
                spg_linehblend(Surface,x-cx,y-cy,x+cx+offset,color, alpha);
            }
            
            if(cx != cy)
            {
                spg_linehblend(Surface,x-cy,y+cx+offset,x+cy+offset,color, alpha);
                if(cx || offset)
                {
                    spg_linehblend(Surface,x-cy,y-cx,x+cy+offset,color, alpha);
                }
            }

            if (df < 0)
            {
                df += d_e;
                d_e += 2;
                d_se += 2;
            }
            else
            {
                df += d_se;
                d_e += 2;
                d_se += 4;
                cy--;
            }
            cx++;
        }
        while (cx <= cy);

        spg_unlock(Surface);
    }
    if(spg_makedirtyrects)
    {
        Sint16 rr = (Sint16)(r);
        SDL_Rect rect = {x - rr, y - rr, 2*rr + 2, 2*rr + 2};  // +1 to get to the edge, +1 more for even-diameter
        // Clip it to the screen
        SPG_DirtyClip(Surface, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
    }

}




// based on Google answer by theta-ga
void SPG_ArcFn(SDL_Surface* surface, Sint16 cx, Sint16 cy, float radius, float startAngle, float endAngle, Uint32 color, void Callback(SDL_Surface *Surf, Sint16 X, Sint16 Y, Uint32 Color))
{
    float angle;  // use as swap then use later
    float originalSA = startAngle;

    if(startAngle > endAngle)
    {
        float swapa = endAngle;
        endAngle = startAngle;
        startAngle = swapa;
    }
    if(startAngle == endAngle)
        return;

    if(!spg_usedegrees)
    {
        startAngle *= DEGPERRAD;
        endAngle *= DEGPERRAD;
    }
    
    // Big angle
    if(endAngle - startAngle >= 360)
    {
        SPG_CircleFn(surface, cx, cy, radius, color, Callback);
        return;
    }
    
    // Shift together
    while(startAngle < 0 && endAngle < 0)
    {
        startAngle += 360;
        endAngle += 360;
    }
    while(startAngle > 360 && endAngle > 360)
    {
        startAngle -= 360;
        endAngle -= 360;
    }
    
    // Check if the angle to be drawn crosses 0
    SPG_bool crossesZero = (startAngle < 0 && endAngle > 0) || (startAngle < 360 && endAngle > 360);
    
    // Push all values to 0 <= angle < 360
    while(startAngle >= 360)
        startAngle -= 360;
    while(endAngle >= 360)
        endAngle -= 360;
    while(startAngle < 0)
        startAngle += 360;
    while(endAngle < 0)
        endAngle += 360;
    
    if(endAngle == 0)
        endAngle = 360;
    else if(crossesZero)
    {
        SPG_ArcFn(surface, cx, cy, radius, originalSA, (spg_usedegrees? 359 : PI2 - 0.02f) , color, Callback);
        startAngle = 0;
    }
    
    Sint16 x = 0;
    Sint16 y = (Sint16)(radius);
    Sint16 p = 1 - (Sint16)(radius);
    Sint16 d_e = 3;
    Sint16 d_se = -2 * (Sint16)(radius) + 5;
    
    do
    {
        // Calculate the angle the current point makes with the circle center
        angle = atan2((float)y, (float)x)*DEGPERRAD;
        
        // draw the circle points as long as they lie in the range specified
        
        if(x)
        {
            // draw point in range 45 to 90 degrees
            if (angle >= startAngle && angle <= endAngle) {
                Callback(surface, cx + x, cy + y, color);
            }
            // draw point in range 180 to 225 degrees
            if (270 - angle >= startAngle && 270 - angle <= endAngle) {
                Callback(surface, cx - y, cy - x, color);
            }
            // draw point in range 225 to 270 degrees
            if (angle + 180 >= startAngle && angle + 180 <= endAngle) {
                Callback(surface, cx - x, cy - y, color);
            }
            // draw point in range 315 to 360 degrees
            if (angle + 270 >= startAngle && angle + 270 <= endAngle) {
                Callback(surface, cx + y, cy - x, color);
            }
        }
        
        // draw point in range 0 to 45 degrees
        if (90 - angle >= startAngle && 90 - angle <= endAngle) {
            Callback(surface, cx + y, cy + x, color);
        }

        // draw point in range 90 to 135 degrees
        if (180 - angle >= startAngle && 180 - angle <= endAngle) {
            Callback(surface, cx - x, cy + y, color);
        }

        // draw point in range 135 to 180 degrees
        if (angle + 90 >= startAngle && angle + 90 <= endAngle) {
            Callback(surface, cx - y, cy + x, color);
        }

        // draw point in range 270 to 315 degrees
        if (360 - angle >= startAngle && 360 - angle <= endAngle) {
            Callback(surface, cx + x, cy - y, color);
        }


        x++;

        if (p < 0)
        {
            p += d_e;
            d_e += 2;
            d_se += 2;
        }
        else
        {
            p += d_se;
            d_e += 2;
            d_se += 4;
            y--;
        }
    }
    while (x <= y);
}

void SPG_Arc(SDL_Surface* surface, Sint16 x, Sint16 y, float radius, float angle1, float angle2, Uint32 color)
{
    if (spg_lock(surface) < 0)
    {
        if(spg_useerrors)
            SPG_Error("SPG_Arc could not lock surface");
        return;
    }
    
    if(spg_thickness == 1)
    {
        SPG_ArcFn(surface, x, y, radius, angle1, angle2, color, spg_pixel);
        if(spg_makedirtyrects)
        {
            Sint16 rr = (Sint16)(radius);
            SDL_Rect rect = {x - rr, y - rr, 2*rr + 2, 2*rr + 2};  // +1 to get to the edge, +1 more for even-diameter
            // Clip it to the screen
            SPG_DirtyClip(surface, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
    }
    else if(spg_thickness > 0)
    {
        SPG_ArcFn(surface, x, y, radius, angle1, angle2, color, spg_thicknesscallback);
        if(spg_makedirtyrects)
        {
            Sint16 rr = (Sint16)(radius);
            SDL_Rect rect = {x - rr - spg_thickness/2, y - rr - spg_thickness/2, 2*rr + 2 + spg_thickness, 2*rr + 2 + spg_thickness};  // +1 to get to the edge, +1 more for even-diameter
            // Clip it to the screen
            SPG_DirtyClip(surface, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
    }
    spg_unlock(surface);
    
    
}

void SPG_ArcBlend(SDL_Surface* surface, Sint16 x, Sint16 y, float radius, float angle1, float angle2, Uint32 color, Uint8 alpha)
{
    if (spg_lock(surface) < 0)
    {
        if(spg_useerrors)
            SPG_Error("SPG_ArcBlend could not lock surface");
        return;
    }

    spg_alphahack = alpha;
    
    if(spg_thickness == 1)
    {
        SPG_ArcFn(surface, x, y, radius, angle1, angle2, color, spg_pixelcallbackalpha);

        if(spg_makedirtyrects)
        {
            Sint16 rr = (Sint16)(radius);
            SDL_Rect rect = {x - rr, y - rr, 2*rr + 2, 2*rr + 2};  // +1 to get to the edge, +1 more for even-diameter
            // Clip it to the screen
            SPG_DirtyClip(surface, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
    }
    else if(spg_thickness > 0)
    {
        SPG_ArcFn(surface, x, y, radius, angle1, angle2, color, spg_thicknesscallbackalpha);

        if(spg_makedirtyrects)
        {
            Sint16 rr = (Sint16)(radius);
            SDL_Rect rect = {x - rr - spg_thickness/2, y - rr - spg_thickness/2, 2*rr + 2 + spg_thickness, 2*rr + 2 + spg_thickness};  // +1 to get to the edge, +1 more for even-diameter
            // Clip it to the screen
            SPG_DirtyClip(surface, &rect);
            SPG_DirtyAddTo(spg_dirtytable_front, &rect);
        }
    }

    spg_unlock(surface);

}

static inline float min(float a, float b)
{
    return a < b? a : b;
}

static inline float max(float a, float b)
{
    return a > b? a : b;
}


// Draws a pie shape from startAngle to endAngle going the quickest way, numerically
// e.g. 20 -> 210 goes positively, 180 -> 30 goes negatively, 30 -> 400 makes a full circle
void SPG_ArcFilledBlend(SDL_Surface* surf, Sint16 x, Sint16 y, float r, float startAngle, float endAngle, Uint32 color, Uint8 alpha)
{
    Sint16 cx = 0;
    Sint16 cy = (Sint16)r;
    Sint16 df = 1 - cy;
    Sint16 d_e = 3;
    Sint16 d_se = -2*cy + 5;
    Sint16 xl, xr;
    
    if(startAngle > endAngle)
    {
        float swapa = endAngle;
        endAngle = startAngle;
        startAngle = swapa;
    }
    if(startAngle == endAngle)
        return;

    if(!spg_usedegrees)
    {
        startAngle *= DEGPERRAD;
        endAngle *= DEGPERRAD;
    }
    
    SPG_bool reflex = endAngle - startAngle > 180;
    
    Sint16 sx = (Sint16)(r*cos(startAngle*RADPERDEG));
    Sint16 sy = (Sint16)(r*sin(startAngle*RADPERDEG));
    Sint16 ex = (Sint16)(r*cos(endAngle*RADPERDEG));
    Sint16 ey = (Sint16)(r*sin(endAngle*RADPERDEG));
    
    float sm = sy/(float)(sx);
    float em = ey/(float)(ex);
    
    // Big angle
    if(endAngle - startAngle >= 360)
    {
        SPG_CircleFilledBlend(surf, x, y, r, color, alpha);
        return;
    }
    // coordinates overlap
    if(sy == ey && sx >= ex - 1 && ex + 1 <= sx && endAngle - startAngle > 350)
    {
        SPG_CircleFilledBlend(surf, x, y, r, color, alpha);
        return;
    }
    if(sy == ey && sx == ex && endAngle - startAngle < 10)
        return;
    
    
    // Push all values to 0 <= angle < 360
    while(startAngle >= 360)
        startAngle -= 360;
    while(endAngle >= 360)
        endAngle -= 360;
    while(startAngle < 0)
        startAngle += 360;
    while(endAngle < 0)
        endAngle += 360;
    
    // I should change these to sy and ey in case the angle rounds to 180
    SPG_bool bothTop = (startAngle > 180 && endAngle > 180);
    SPG_bool bothBot = (startAngle < 180 && endAngle < 180);
    //bool bothTop = sy < 0 && ey < 0;
    //bool bothBot = sy >= 0 && ey >= 0;
    
    // Use thse to fix special cases
    SPG_bool s180 = sy == 0 && sx < 0;
    SPG_bool s0 = sy == 0 && sx >= 0;
    SPG_bool e180 = ey == 0 && ex < 0;
    SPG_bool e0 = ey == 0 && ex >= 0;
    
    
    do
    {
        if(df >= 0)
        {
            if(!bothTop)
            {
                // Draw bottom between lines
                // Intercept of lines vs edge of circle
                xl = ey > 0? (Sint16)max(cy/em + x, x - cx) : (Sint16)x - cx;
                xr = sy > 0? (Sint16)min(cy/sm + x, x + cx) : (Sint16)x + cx;
                if(xl <= xr && !(s180 && ey <= 0) && !(sy <= 0 && e0))
                    spg_linehblend(surf, xl, y + cy, xr, color, alpha);
            }
            else if(reflex)
            {
                // Draw entire bottom
                spg_linehblend(surf, x - cx, y + cy, x + cx, color, alpha);
                
                // Draw separated top 
                // sy <= 0, ey < 0, ex < sx
                xl = x - cx;  // Intercept of lines vs edge of circle
                xr = ey < 0? (Sint16)min(-cy/em + x, x + cx) : x + cx;
                if(xl <= xr)
                    spg_linehblend(surf, xl, y - cy, xr, color, alpha);
                
                xl = sy < 0? (Sint16)max(-cy/sm + x, x - cx) : x - cx;
                xr = x + cx;
                if(xl <= xr)
                    spg_linehblend(surf, xl, y - cy, xr, color, alpha);
                    
            }
            if(!bothBot)
            {
                // Draw top between lines
                xl = sy < 0? (Sint16)max(-cy/sm + x, x - cx) : (Sint16)x - cx;
                xr = ey < 0? (Sint16)min(-cy/em + x, x + cx) : (Sint16)x + cx;
                if(xl <= xr && !(e180 && sy >= 0))
                    spg_linehblend(surf, xl, y - cy, xr, color, alpha);
            }
            else if(reflex)
            {
                // Draw entire top
                spg_linehblend(surf, x - cx, y - cy, x + cx, color, alpha);
                
                // Draw separated bottom
                // sy >= 0, ey > 0, sx < ex
                xl = x - cx;  // Intercept of lines vs edge of circle
                xr = sy > 0? (Sint16)min(cy/sm + x, x + cx) : x + cx;
                if(xl <= xr && !s0 && !(sy > 0 && e0))
                    spg_linehblend(surf, xl, y + cy, xr, color, alpha);
                
                xl = ey > 0? (Sint16)max(cy/em + x, x - cx) : x - cx;
                xr = x + cx;
                if(xl <= xr && !(sy > 0 && e0))
                    spg_linehblend(surf, xl, y + cy, xr, color, alpha);
            }
        }
        if(cx != cy)
        {
            if(!bothTop)
            {
                // Draw bottom between lines
                xl = ey > 0? (Sint16)max(cx/em + x, x - cy) : (Sint16)x - cy;
                xr = sy > 0? (Sint16)min(cx/sm + x, x + cy) : (Sint16)x + cy;
                if(xl <= xr && !(s180 && ey <= 0) && !(sy <= 0 && e0))
                    spg_linehblend(surf, xl, y + cx, xr, color, alpha);
            }
            else if(reflex)
            {
                if(cx != 0)
                {
                    // Draw entire bottom
                    spg_linehblend(surf, x - cy, y + cx, x + cy, color, alpha);
                }
                
                // Draw separated top
                // sy <= 0, ey < 0, ex < sx
                xl = x - cy;  // Intercept of lines vs edge of circle
                xr = ey < 0? (Sint16)min(-cx/em + x, x + cy) : x + cy;
                if(xl <= xr)
                    spg_linehblend(surf, xl, y - cx, xr, color, alpha);
                
                xl = sy < 0? (Sint16)max(-cx/sm + x, x - cy) : x - cy;
                xr = x + cy;
                if(xl <= xr)
                    spg_linehblend(surf, xl, y - cx, xr, color, alpha);
                
            }
            if(cx && (!bothBot))  // Don't draw the center line twice
            {
                // Draw top between lines
                xl = sy < 0? (Sint16)max(-cx/sm + x, x - cy) : (Sint16)x - cy;
                xr = ey < 0? (Sint16)min(-cx/em + x, x + cy) : (Sint16)x + cy;
                if(xl <= xr && !(e180 && sy >= 0))
                    spg_linehblend(surf, xl, y - cx, xr, color, alpha);
            }
            else if(bothBot && reflex)
            {
                if(cx != 0)
                {
                    // Draw entire top
                    spg_linehblend(surf, x - cy, y - cx, x + cy, color, alpha);
                }
                
                // Draw separated bottom
                // sy >= 0, ey > 0, sx < ex
                xl = x - cy;  // Intercept of lines vs edge of circle
                xr = sy > 0? (Sint16)min(cx/sm + x, x + cy) : x + cy;
                if(xl <= xr && !s0 && !(sy > 0 && e0))
                    spg_linehblend(surf, xl, y + cx, xr, color, alpha);
                
                xl = ey > 0? (Sint16)max(cx/em + x, x - cy) : x - cy;
                xr = x + cy;
                if(xl <= xr && !(sy > 0 && e0))
                    spg_linehblend(surf, xl, y + cx, xr, color, alpha);
                
            }
                
        }
        if(df < 0)
        {
            df += d_e;
            d_e += 2;
            d_se += 2;
        }
        else
        {
            df += d_se;
            d_e += 2;
            d_se += 4;
            cy--;
        }
        cx++;
    }while(cx <= cy);
    if(spg_makedirtyrects)
    {
        Sint16 rr = (Sint16)(r);
        SDL_Rect rect = {x - rr, y - rr, 2*rr + 2, 2*rr + 2};  // +1 to get to the edge, +1 more for even-diameter
        // Clip it to the screen
        SPG_DirtyClip(surf, &rect);
        SPG_DirtyAddTo(spg_dirtytable_front, &rect);
    }
    
}

void SPG_ArcFilled(SDL_Surface* surface, Sint16 cx, Sint16 cy, float radius, float startAngle, float endAngle, Uint32 color)
{
    SPG_ArcFilledBlend(surface, cx, cy, radius, startAngle, endAngle, color, SDL_ALPHA_OPAQUE);
}





//==================================================================================
// Draws a bezier line
//==================================================================================
/* Macro to do the line... 'function' is the line drawing routine */
#define DO_BEZIER(function)\
	/*
*  Note:
I don't think there is any great performance win in translating this to fixed-point integer math,
*  most of the time is spent in the line drawing routine.
*/\
float x = (float)(startX), y = (float)(startY);\
float xp = x, yp = y;\
float delta;\
float dx, d2x, d3x;\
float dy, d2y, d3y;\
float a, b, c;\
int i;\
int n = 1;\
Sint16 xmax=startX, ymax=startY, xmin=startX, ymin=startY;\
\
/* compute number of iterations */\
if(quality < 1)\
quality=1;\
if(quality >= 15)\
quality=15; \
while (quality-- > 0)\
n*= 2;\
delta = 1.0f / n;\
\
/* compute finite differences */\
/* a, b, c are the coefficient of the polynom in t defining the parametric curve */\
/* The computation is done independently for x and y */\
a = (float)(-startX + 3*cx1 - 3*cx2 + endX);\
b = (float)(3*startX - 6*cx1 + 3*cx2);\
c = (float)(-3*startX + 3*cx1);\
\
d3x = 6 * a * delta*delta*delta;\
d2x = d3x + 2 * b * delta*delta;\
dx = a * delta*delta*delta + b * delta*delta + c * delta;\
\
a = (float)(-startY + 3*cy1 - 3*cy2 + endY);\
b = (float)(3*startY - 6*cy1 + 3*cy2);\
c = (float)(-3*startY + 3*cy1);\
\
d3y = 6 * a * delta*delta*delta;\
d2y = d3y + 2 * b * delta*delta;\
dy = a * delta*delta*delta + b * delta*delta + c * delta;\
\
if (spg_lock(surface) < 0) {\
if(spg_useerrors)\
SPG_Error("SPG_Bezier could not lock surface");\
return; }\
\
/* iterate */\
for (i = 0; i < n; i++) {\
x += dx; dx += d2x; d2x += d3x;\
y += dy; dy += d2y; d2y += d3y;\
if((Sint16)(xp) != (Sint16)(x) || (Sint16)(yp) != (Sint16)(y)){\
function;\
if(dirty){\
xmax= (xmax>(Sint16)(xp))? xmax : (Sint16)(xp);  ymax= (ymax>(Sint16)(yp))? ymax : (Sint16)(yp);\
xmin= (xmin<(Sint16)(xp))? xmin : (Sint16)(xp);  ymin= (ymin<(Sint16)(yp))? ymin : (Sint16)(yp);\
xmax= (xmax>(Sint16)(x))? xmax : (Sint16)(x);    ymax= (ymax>(Sint16)(y))? ymax : (Sint16)(y);\
xmin= (xmin<(Sint16)(x))? xmin : (Sint16)(x);    ymin= (ymin<(Sint16)(y))? ymin : (Sint16)(y);\
}\
}\
xp = x; yp = y;\
}\
\
spg_unlock(surface);\
if(dirty)\
{\
SDL_Rect rect = {xmin - spg_thickness/2, ymin - spg_thickness/2, xmax-xmin+1 + spg_thickness, ymax-ymin+1 + spg_thickness};\
/* Clip it to the screen */\
SPG_DirtyClip(surface, &rect);\
SPG_DirtyAddTo(spg_dirtytable_front, &rect);\
}


//==================================================================================
// Draws a bezier line
//==================================================================================
void SPG_Bezier(SDL_Surface *surface, Sint16 startX, Sint16 startY, Sint16 cx1, Sint16 cy1, Sint16 cx2, Sint16 cy2, Sint16 endX, Sint16 endY, Uint8 quality, Uint32 color)
{
    if (spg_lock(surface) < 0)
    {
        if(spg_useerrors)
            SPG_Error("SPG_Bezier could not lock surface");
        return;
    }
    
    // Disable autolock so we can continually use SPG_Line's AA
    Uint8 lock = spg_autolock;
    SPG_bool dirty = spg_makedirtyrects;
    spg_autolock = 0;
    spg_makedirtyrects = 0;


    DO_BEZIER(SPG_Line(surface, (Sint16)(xp),(Sint16)(yp), (Sint16)(x),(Sint16)(y), color));

    spg_unlock(surface);

    spg_autolock = lock;
    spg_makedirtyrects = dirty;
}



//==================================================================================
// Draws a bezier line (alpha)
//==================================================================================
void SPG_BezierBlend(SDL_Surface *surface, Sint16 startX, Sint16 startY, Sint16 cx1, Sint16 cy1, Sint16 cx2, Sint16 cy2, Sint16 endX, Sint16 endY, Uint8 quality, Uint32 color, Uint8 alpha)
{
    if (spg_lock(surface) < 0)
    {
        if(spg_useerrors)
            SPG_Error("SPG_BezierBlend could not lock surface");
        return;
    }
    
    // Disable autolock so we can continually use SPG_LineBlend's AA
    Uint8 lock = spg_autolock;
    SPG_bool dirty = spg_makedirtyrects;
    spg_autolock = 0;
    spg_makedirtyrects = 0;


    DO_BEZIER(SPG_LineBlend(surface, (Sint16)(xp),(Sint16)(yp), (Sint16)(x),(Sint16)(y), color, alpha));

    spg_unlock(surface);

    spg_autolock = lock;
    spg_makedirtyrects = dirty;
}






