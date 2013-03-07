#include "sprig.h"

int main(int argc, char* argv[])
{
	SDL_Surface* screen = SPG_InitSDL(800, 600, 0, 0, 0);
	
	SDL_Surface* img = SDL_LoadBMP("data/face.bmp");
	SDL_Surface* scaled = SPG_Scale(img, 1.0f, 1.0f);
	
	float xScale = 1.0f;
	float yScale = 1.0f;
	
	Uint8* keystates = SDL_GetKeyState(NULL);
	
	Uint8 done = 0;
	SDL_Event event;
	while(!done)
	{
		while(SDL_PollEvent(&event))
		{
			if(event.type == SDL_QUIT)
				done = 1;
			else if(event.type == SDL_KEYDOWN)
			{
				if(event.key.keysym.sym == SDLK_ESCAPE)
					done = 1;
			}
		}
		
		Uint8 pressed = 0;
		if(keystates[SDLK_UP])
		{
			yScale -= 0.05f;
			if(yScale < 0.05f)
				yScale = 0.05f;
			pressed = 1;
		}
		else if(keystates[SDLK_DOWN])
		{
			yScale += 0.05f;
			pressed = 1;
		}
		if(keystates[SDLK_RIGHT])
		{
			xScale += 0.05f;
			pressed = 1;
		}
		else if(keystates[SDLK_LEFT])
		{
			xScale -= 0.05f;
			if(xScale < 0.05f)
				xScale = 0.05f;
			pressed = 1;
		}
		
		if(pressed)
		{
			SDL_FreeSurface(scaled);
			scaled = SPG_Scale(img, xScale, yScale);
			printf("Scale: %.2f, %.2f\n", xScale, yScale);
		}
			
		
		SDL_FillRect(screen, NULL, 0x000000);
		
		SDL_BlitSurface(scaled, NULL, screen, NULL);
		
		SDL_Flip(screen);
		SDL_Delay(10);
	}
	
	SDL_FreeSurface(img);
	SDL_FreeSurface(scaled);
	
	SDL_Quit();
	return 0;
}

