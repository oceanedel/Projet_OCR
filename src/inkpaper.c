#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

uint8_t gray_from_rgb(uint32_t px, const SDL_PixelFormat* f)
{
    uint8_t r = (uint8_t)((px & f->Rmask) >> f->Rshift);
    uint8_t g = (uint8_t)((px & f->Gmask) >> f->Gshift);
    uint8_t b = (uint8_t)((px & f->Bmask) >> f->Bshift);
    // integer luma ~0.299R+0.587G+0.114B
    // most accurate and faster approximation founded online :
    return (uint8_t)((77*r + 150*g + 29*b) >> 8);
}

int isodata_threshold(const uint8_t *Y, int n)
{
    //I chose the Riddler-Calvard method instead of the Otsu s one bc it is simple to code and implement (cons : less accurate)
    //Implemented it with pseudo code found online
    // initial guess: mean gray
    long sum=0;
    for(int i=0;i<n;i++) sum += Y[i];

    int t = (int)(sum / n);
    int prev = -1;

    while(t != prev)
    {
        prev = t;
        long s0=0, s1=0;
	int n0=0,n1=0;

        for(int i=0;i<n;i++)
	{
            if(Y[i] <= t)
	    { 
		s0 += Y[i];
		n0++; 
	    }
	    else 
	    { 
		s1 += Y[i];
		n1++;
	    }
        }
        int m0 = n0? (int)(s0/n0) : t;
        int m1 = n1? (int)(s1/n1) : t;
        t = (m0 + m1) / 2;
    }
    return t;
}

void inkpaper(char* path)
{
    /* retrieve file name
    char *filename = malloc(4 + strlen(path) + 1);
    
    int temp = 4;
    strcpy(filename, "out_");
    while(path[temp - 4] != '\0')
    {
	filename[temp] = path[temp - 4];
	temp++;
    }
    filename[temp] = '\0';
    */

    //init 
    SDL_Init(SDL_INIT_VIDEO);
    //load bmp image
    SDL_Surface* src = SDL_LoadBMP(path);

    if(!src){ fprintf(stderr,"load fail: %s\n", SDL_GetError()); return; }

    //convert it
    SDL_Surface* img = SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(src);

    if(!img){ fprintf(stderr,"convert fail: %s\n", SDL_GetError()); return; }

    //optional but seems to help the code working with all GPU
    if(SDL_MUSTLOCK(img)) SDL_LockSurface(img);

    int W=img->w, H=img->h, N=W*H;

    uint32_t* p = (uint32_t*)img->pixels;
    const SDL_PixelFormat* f = img->format;

    uint8_t* Y = (uint8_t*)malloc(N);
    if(!Y){ printf("Y allocation failed:\n"); return; }
    //graying all pixels
    for(int i=0;i<N;i++) Y[i] = gray_from_rgb(p[i], f);

    //thresholding the image with the Riddler-Calvard thresholding algo
    int t = isodata_threshold(Y, N);
    printf("Threshold (ISODATA) = %d\n", t);
    //creating the output image
    SDL_Surface* bw = SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_ARGB8888);
    if(SDL_MUSTLOCK(bw)) SDL_LockSurface(bw);

    //writing manualy all pixels
    uint32_t* q = (uint32_t*)bw->pixels;
    for(int i=0;i<N;i++)
    { 
	//cheking if pixel is ink or paper
	uint8_t v = (Y[i] <= t)? 0:255;
	//         AA         RR        GG     BB    it write the pixel 2 bytes by 2 bytes
	q[i] = (0xFF<<24) | (v<<16) | (v<<8) | v;
    }
    if(SDL_MUSTLOCK(bw)) SDL_UnlockSurface(bw);
    if(SDL_MUSTLOCK(img)) SDL_UnlockSurface(img);

    SDL_SaveBMP(bw, "out.bmp");

    free(Y);
    SDL_FreeSurface(bw);
    SDL_FreeSurface(img);
    SDL_Quit();
    
}

