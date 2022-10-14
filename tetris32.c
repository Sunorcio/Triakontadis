/* Triakontadis */
/* Triakontadomino */
#ifndef SDL2_DEF
#define SDL2_DEF
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_hints.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_log.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL.h>
#endif

#ifndef STD_DEF
#define STD_DEF
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#endif

uint8_t pixelsize = 11;
SDL_Window* window;
SDL_Renderer* renderer;
SDL_Rect rect = {0,0,20,20};
uint8_t width,height;

char catchSDLError(int functionreturn)
/* feed a negative literal to just print SDL error, else load with a function call to detect and if, print error */
{
	if(functionreturn<0){ printf("%s",SDL_GetError());}
	if(SDL_GetError() == 0){return -1;}
	return 0;
}
void initSDL()
{
	srand(time(0));
	SDL_Init(SDL_INIT_VIDEO);
	rect.h = rect.w = pixelsize;
	width=10,height=20;
	window= SDL_CreateWindow(0,SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,32*pixelsize,32*pixelsize,SDL_WINDOW_SHOWN);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
	if(catchSDLError(-1)){
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
	}
}

const uint8_t basespeed = 70;
uint8_t msize = 4;
uint8_t blocks = 0;
uint8_t buffersize = 0;
char l = 0;
char firstbuffer = 1;
char canbuffer = 1;
uint16_t speed;
uint8_t drycount = 0;
uint8_t drytotal = 0;
uint32_t matrix[32] = {0};
uint32_t buffer[32] = {0};
uint32_t swap[32] = {0};
int8_t offset[2];
uint32_t smatrix[65] = {0};

void rotateMatrix(char clockwise)
/* rotate nxn matrix of max n=64 90º anticlockwise (3 times if clockwise==0),
	where bit position is one dimendsion, so an array[n] of uintx>=n */
{
	drycount = 0;
	uint64_t buffer;
	for(int i = 0;i<1+2*!clockwise;i++)
	{
		for(int r = 0;r<msize/2;r++)
		{
			for(int c = r;c<msize-1-r;c++)
			{
				buffer = matrix[r]&0x1<<c;
				matrix[r] = matrix[r]&~(0x1<<c) | (matrix[msize-1-c]&0x1<<r) << (c-r);
				matrix[msize-1-c] = matrix[msize-1-c]&~(0x1<<r) | (matrix[msize-1-r]&(0x1<<(msize-1-c))) >>(msize-1-c-r);
				matrix[msize-1-r] = matrix[msize-1-r]&~(0x1<<(msize-1-c)) | (matrix[c]&(0x1<<(msize-1-r))) >>(c-r);
				matrix[c] = matrix[c]&~(0x1<<(msize-1-r)) | buffer << (msize-1-c-r);
			}
		}
	}
/*collision offset correction after rotation
 *
 *	0x01000000 check most/least significant bit for left/right offset limit, then correct,
 *	check collision adjusting by msize/2 times in order down,up,furthest side,the other side*/
	uint8_t beware = offset[1];
	int8_t offmin;
	int8_t offmax;
	for(int i=0;i<msize/2+1;i++)
	{
		for(int j = 0;j<msize;j++)
		{
			if(matrix[j]&0x1<<i)
			{
				offmin=-i;
				goto mindone;
			}
		}
	}
	mindone:
	for(int i=msize;i>msize/2-1;i--)
	{
		for(int j = 0;j<msize;j++)
		{
			if(matrix[j]&0x1<<(i-1))
			{
				offmax=width-i;
				goto maxdone;
			}
		}
	}
	maxdone:
	if(offset[1]<offmin)
	{
		offset[1]=offmin;
	}else if(offset[1]>offmax)
	{
		offset[1]=offmax;
	}
	
	uint8_t cani = 0;
	for(int k = 0;k<msize;k++)
	{
		if(offset[1]>=0)
			{if((matrix[k]<<offset[1])&smatrix[offset[0]-msize+k]){break;}else{cani++;}
		}else{if(matrix[k]>>-offset[1]&smatrix[offset[0]-msize+k]){break;}else{cani++;}}
	}
	if(cani>=msize){goto foundspot;}else{cani=0;}

	offset[1]=beware;
	rotateMatrix(!clockwise);
	foundspot:
	return;
}

void drawMatrix()
{
	if(l&&(blocks==msize)){
		SDL_SetRenderDrawColor(renderer,0x00,0xff,0x00,0xff);
	}else{
		SDL_SetRenderDrawColor(renderer,0xff,0xff,0xff,0xff);
	}

	for(int r = 0;r<msize; r++)
	{
		for(int c = 0;c<msize;c++)
		{
			if(matrix[r]&(0x1<<c))
			{
				rect.y = pixelsize * (r + offset[0] - msize);
				rect.x = pixelsize * (c + offset[1]);
				SDL_RenderFillRect(renderer,&rect);
			}
		}
	}
}

void drawSMatrix()
{
	SDL_SetRenderDrawColor(renderer,0x00,0x00,0x00,0x00);
	SDL_RenderClear(renderer);
	SDL_SetRenderDrawColor(renderer,0xc0,0xc0,0xc0,0xff);

	for(int r =0;r<height; r++)
	{
		for(int c = 0;c<width;c++)
		{
			if(smatrix[r]&(0x1<<c))
			{
				rect.y = pixelsize * r;
				rect.x = pixelsize * c;
				SDL_RenderFillRect(renderer,&rect);
			}
		}
	}
}

void resizeScreen()
{
	smatrix[height]=0x00;
	width =10+(msize-4)*22/28;
	height = width*2;
	if(width>(10+(msize-1-4)*22/28))
	{	for(int j = 0;j<(width-(10+(msize-1-4)*22/28))*2;j++){
			for(int i=height-1;i;i--)
			{
				smatrix[i]=smatrix[i-1];
			}
			smatrix[0] = 0;
		}
	}
	smatrix[height]=0xffffffff;
	SDL_SetWindowSize(window, pixelsize*width , pixelsize*height);
}

void generatePiece()
{
	canbuffer = 1;
	drytotal = 0;
	l=0;
	for(int i = 0;i<msize;i++)
		{matrix[i] = 0;}
	int w = msize/2,h = w;
	matrix[h] |= 0x1 << w;
	
	blocks = 4+(rand()%(msize-3));
	offset[0] = (msize-blocks)/2+(msize-blocks)%2;
	offset[1] = (width-msize)/2;
	if(rand()%(10-blocks*9/32))
	{
		for(int i = 0;i<blocks-1;i++)
		{
			if(rand()%32-msize)
			{
				w = h = msize/2;
			}
			
			while(matrix[h] & (0x1<<w))
			{
				switch(rand()%4)
				{
					case 0:
						if(h>msize/2-blocks/2){h--;break;}
					case 1:
						if(w>msize/2-blocks/2){w--;break;}
					case 2:
						if(h<msize/2+blocks/2-1+blocks%2){h++;break;}
					case 3:
						if(w<msize/2+blocks/2-1+blocks%2){w++;break;}
				}
			}
			matrix[h] |= (0x1<<w);
		}
	}else{
		offset[0]=msize/2-2;
		if(blocks>4){l = 1;}
		for(int i=msize/2-blocks/2;i<msize/2+blocks/2+blocks%2;i++)
		{
			matrix[h] |= 0x1 << i;
		}
	}
}

void bufferPiece()
{
	l = 0;
	canbuffer = 0;
	for(int i = 0;i<32;i++)
	{
		swap[i]=matrix[i];
		matrix[i] = 0;
	}
	for(int i = 0;i<buffersize;i++)
	{
		matrix[i+(msize-buffersize)/2] = buffer[i]<<(msize-buffersize)/2; 
	}
	for(int i = 0;i<32;i++)
	{
		buffer[i] = swap[i];
	}
	buffersize = msize;
	offset[0] = (msize-blocks)/2+(msize-blocks)%2;
	offset[1] = (width-msize)/2;
	if(firstbuffer){firstbuffer=0;generatePiece();}
	canbuffer=0;
}

void youwinjpg()
{
	SDL_Surface* s_won = SDL_LoadBMP("tertis.bmp");
	SDL_Texture* t_won = SDL_CreateTextureFromSurface(renderer,s_won);
	SDL_FreeSurface(s_won);
	SDL_SetWindowSize(window,64*pixelsize,64*pixelsize);
	SDL_RenderCopy(renderer,t_won,0,0);
	SDL_RenderPresent(renderer);
	SDL_Delay(1000);
	char endpls = 1;
	SDL_Event ev = {0};
	while(endpls)
	{
		SDL_Delay(10);
		while(SDL_PollEvent(&ev))
		{
			if(ev.type == SDL_QUIT)
			{
				endpls=0;
			}else if(ev.type  == SDL_KEYDOWN)
			{
				endpls=0;
			}
		}
	}
	SDL_FreeSurface(s_won);
	SDL_DestroyTexture(t_won);
}

char medusa()
{
	for(int i=0;i<msize;i++)
	{
		if((offset[0]+i-msize+1)<=0)
		{if(matrix[i]){return 0;}else{continue;}}
		if(offset[1]>0)
		{smatrix[offset[0]+i-msize] |= matrix[i]<<offset[1];
		}else
		{smatrix[offset[0]+i-msize] |= matrix[i]>>-offset[1];}
	}

	uint32_t scanline;
	uint8_t clearcount = 0;
	uint8_t gaps = width;
	for(int i = 0;i<height;i++)
	{
		scanline=1;
		for(int j = 0;j<width;j++)
		{
			if(scanline&smatrix[i])
			{gaps--;}
			scanline<<=1;
		}
		if(gaps<=msize/6)
		{
			clearcount++;
			for(int j = i;j;j--)
			{smatrix[j]=smatrix[j-1];}
			smatrix[0]=0;
		}
		gaps=width;
	}

	if(clearcount>=msize)
	{
		msize++;
		if(msize>32)
		{
			youwinjpg();
			return 0; 
		}else{resizeScreen();}
	}
	return 1;
}

void logicTick()
{
	for(int i=0;i<msize;i++)
	{
		if((offset[0]+i-msize+1)<0)
		{continue;}
		if(offset[1]>0)
		{if((matrix[i]<<offset[1])&smatrix[offset[0]+i-msize+1])
			{goto collidedg;}
		}else if((matrix[i]>>-offset[1])&smatrix[offset[0]+i-msize+1])
			{goto collidedg;}
	}
	offset[0]++;
	drycount = 0;
	return;
	collidedg:;
	drycount++;
	drytotal++;
}


int main(int argc, char* argv[])
{
	initSDL();

	SDL_Surface* s_splash = SDL_LoadBMP("splash.bmp");
	SDL_Texture* t_splash = SDL_CreateTextureFromSurface(renderer,s_splash);
	SDL_FreeSurface(s_splash);
	SDL_RenderCopy(renderer,t_splash,0,0);
	SDL_RenderPresent(renderer);

	char drop = 1;
	char move = 1;
	uint64_t timer = SDL_GetTicks64();
	uint64_t timer1 = SDL_GetTicks64();
	const uint8_t* state = SDL_GetKeyboardState(0);
	char running = 1;
	SDL_Event ev = {0};
	while(running)
	{
		SDL_Delay(10);
		while(SDL_PollEvent(&ev))
		{
			if(ev.type == SDL_QUIT)
			{
				goto quit;
			}else if(ev.type  == SDL_KEYDOWN)
			{
				running = 0;
			}
		}
	}
	SDL_DestroyTexture(t_splash);

	resizeScreen();
	SDL_SetWindowPosition(window,SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED);

	speed = basespeed;
	smatrix[20] = 0xffffffff;
	generatePiece();

	running =1;
	while(running)
	{
		while(SDL_PollEvent(&ev))
		{
			if(ev.type == SDL_QUIT)
			{
				running = 0;
			}else if(ev.type == SDL_KEYUP) 
			{
				if(ev.key.keysym.sym == SDLK_s)
				{
					speed = basespeed;
				}else if(ev.key.keysym.sym == SDLK_a)
				{
					move = 1;
				}else if(ev.key.keysym.sym == SDLK_d)
				{
					move = 1;
				}else if(ev.key.keysym.sym == SDLK_SPACE)
				{
					drop = 1;
				}
			}else if(ev.type == SDL_KEYDOWN) 
			{
				if(ev.key.keysym.sym == SDLK_s)
				{
					speed = basespeed*height/20*3;
				}else if(ev.key.keysym.sym == SDLK_k)
				{
					rotateMatrix(1);
				}else if(ev.key.keysym.sym == SDLK_j)
				{
					rotateMatrix(0);
				}else if(ev.key.keysym.sym == SDLK_q)
				{
					if(msize>4)
					{
						msize--;
						resizeScreen();
						generatePiece();
						SDL_Log("%d",msize);
					}
				}else if(ev.key.keysym.sym == SDLK_e)
				{
					if(msize<32)
					{
						msize++;
						resizeScreen();
						generatePiece();
						SDL_Log("%d",msize);
					}
				}else if(ev.key.keysym.sym == SDLK_p)
				{
					char paused = 1;
					while(paused)
					{
						SDL_Delay(10);
						while(SDL_PollEvent(&ev))
						{
							if(ev.type==SDL_QUIT)
							{
								running=0;
								paused=0;
							}if(ev.type==SDL_KEYDOWN)
							{
								paused=0;
							}
						}
					}
				}else if(ev.key.keysym.sym == SDLK_SPACE && drop)
				{
					canbuffer = 0;
					drop = 0;
					drytotal=4;
					while(drytotal<5)
					{logicTick();}
				}else if(ev.key.keysym.sym == SDLK_l && canbuffer)
				{
					bufferPiece();
				}
			}
		}
		if(SDL_GetTicks64()-timer1>115-width*3/2 || move)
		{
			move = 0;
			timer1 = SDL_GetTicks64();
			if(state[SDL_SCANCODE_A] && !state[SDL_SCANCODE_D])
			{
				drycount=0;
				for(int i = (msize-blocks)/2;i<-offset[1]+1;i++)
				{
					for(int j = (msize-blocks)/2;j<msize-(msize-blocks)/2-(msize-blocks)%2;j++)
					{
						if(matrix[j]&0x1<<i)
						{goto collideda;}
					}
				}
				for(int i = 0;i<msize;i++)
				{
					if(offset[1]>0)
					{if((matrix[i]<<(offset[1]-1))&smatrix[offset[0]+i-msize])
						{goto collideda;}
					}else if((matrix[i]>>(1-offset[1]))&smatrix[offset[0]+i-msize])
						{goto collideda;}
				}
				offset[1]--;
				collideda:;
			}else if(state[SDL_SCANCODE_D] && !state[SDL_SCANCODE_A])
			{
				move = 0;
				drycount = 0;
				for(int i = width-msize;i<offset[1]+1;i++)
				{
					for(int j = (msize-blocks)/2;j<msize-(msize-blocks)/2-(msize-blocks)%2;j++)
					{
						if(matrix[j]&0x1<<(msize-1-(offset[1]-i)))
						{goto collidedd;}
					}
				}
				for(int i = 0;i<msize;i++)
				{
					if(offset[1]>=0)
					{if((matrix[i]<<(offset[1]+1))&smatrix[offset[0]+i-msize])
						{goto collidedd;}
					}else if((matrix[i]>>(-offset[1]-1))&smatrix[offset[0]+i-msize])
						{goto collidedd;}
				}
				offset[1]++;
				collidedd:;
			}else{move = 1;}
		}
		if(SDL_GetTicks64()-timer>20000/(speed-blocks+4))
		{
			if(state[SDL_SCANCODE_S])
			{drycount = speed+1;}
			logicTick();
			timer = SDL_GetTicks64();
			if(drycount > 1 || drytotal > 4)
			{
				if(medusa())
				{generatePiece();}
				else{

					for(int i = 0;i<height;i++)
					{smatrix[i]=0;}
					for(int i = 0;i<32;i++)
					{matrix[i]=0;}
					msize=10;
					offset[1]=(width-10)/2;
					offset[0]=0;
					matrix[1]=0x38;
					matrix[2]=0x44;
					matrix[3]=0x40;
					matrix[4]=0x20;
					matrix[5]=0x10;
					matrix[6]=0x10;
					matrix[8]=0x10;


					uint64_t timer2 = SDL_GetTicks64();
					SDL_Delay(8000/height+1);
					char restart = 1;
					while(restart)
					{
						SDL_Delay(10);
						while(SDL_PollEvent(&ev))
						{
							if(ev.type == SDL_QUIT)
							{
								running = 0;
								restart = 0;
							}
							if(ev.type == SDL_KEYDOWN)
							{
								restart=0;
								if(ev.key.keysym.sym != SDLK_SPACE)
								{
									firstbuffer=1;
									canbuffer=1;
									msize=4;
									speed = basespeed;
									for(int i = 0;i<height;i++)
									{smatrix[i]=0;}
									resizeScreen();
									generatePiece();
								}
								else{offset[0]=height;timer2+=400;running=0;}
							}
						}
						if(SDL_GetTicks64()-timer2>8000/height)
						{
							timer2=SDL_GetTicks64();
							logicTick();
							if(offset[0]>height)
							{running=0;restart=0;
							}else{
							drawSMatrix();
							drawMatrix();
							SDL_RenderPresent(renderer);}
						}
					}
				}
			}
		}
		drawSMatrix();
		drawMatrix();
		SDL_RenderPresent(renderer);
	}

quit:
	SDL_Delay(200);
	renderer = NULL;
	window = NULL;
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
