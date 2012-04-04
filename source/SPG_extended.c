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
#ifdef SPG_USE_EXTENDED


//the stack - This is huge, but it seems that only the 
//actual used space is counted against my memory footprint.
// Probably compiler-dependent... (shows how much I know)
#define stackSize 16777216
Sint16 stack8x[stackSize];
Sint16 stack8y[stackSize];
Sint32 stackPointer = -1;

bool pop(SDL_Surface* dest, Sint16 &x, Sint16 &y)
{
    if(stackPointer >= 0)
    {
        x = stack8x[stackPointer];
        y = stack8y[stackPointer];
        stackPointer--;
        return 1;
    }
    else
    {
        return 0;
    }
}

bool push(SDL_Surface* dest, Sint16 x, Sint16 y)
{
    if(stackPointer < stackSize - 1)
    {
        stackPointer++;
        stack8x[stackPointer] = x;
        stack8y[stackPointer] = y;
        return 1;
    }
    else
    {
        return 0;
    }
}

void emptyStack(SDL_Surface* dest)
{
    Sint16 x, y;
    while(pop(dest, x, y));
}


// From http://student.kuleuven.be/~m0216922/CG/floodfill.html
//8-way floodfill using our own stack routines
void SPG_FloodFill8(SDL_Surface* dest, Sint16 x, Sint16 y, Uint32 newColor)
{
    Uint32 oldColor = SPG_GetPixel(dest, x, y);
    if(newColor == oldColor)
        return; //avoid infinite loop
    
    stackPointer = -1;
      
    if(!push(dest, x, y))
        return;
    
    while(pop(dest, x, y))
    {
        SPG_Pixel(dest, x, y, newColor);
        
        if(x + 1 < dest->w && SPG_GetPixel(dest, x+1, y) == oldColor)
        {          
            if(!push(dest, x + 1, y)) return;
        }    
        if(x - 1 >= 0 && SPG_GetPixel(dest, x-1, y) == oldColor)
        {
            if(!push(dest, x - 1, y)) return;
        }    
        if(y + 1 < dest->h && SPG_GetPixel(dest, x, y+1) == oldColor)
        {
            if(!push(dest, x, y + 1)) return;
        }    
        if(y - 1 >= 0 && SPG_GetPixel(dest, x, y-1) == oldColor)
        {
            if(!push(dest, x, y - 1)) return;
        }
        if(x + 1 < dest->w && y + 1 < dest->h && SPG_GetPixel(dest, x+1, y+1) == oldColor)
        {      
            if(!push(dest, x + 1, y + 1)) return;
        }        
        if(x + 1 < dest->w && y - 1 >= 0 && SPG_GetPixel(dest, x+1, y-1) == oldColor)
        {  
            if(!push(dest, x + 1, y - 1)) return;
        } 
        if(x - 1 >= 0 && y + 1 < dest->h && SPG_GetPixel(dest, x-1, y+1) == oldColor)
        {          
            if(!push(dest, x - 1, y + 1)) return;
        }
        if(x - 1 >= 0 && y - 1 >= 0 && SPG_GetPixel(dest, x-1, y-1) == oldColor)
        {
            if(!push(dest, x - 1, y - 1)) return;
        }
    }
}


#endif
