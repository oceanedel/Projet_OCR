#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>


static inline uint8_t gray_from_rgb(uint32_t px) {
    uint8_t r = (px >> 16) & 0xFF;
    uint8_t g = (px >> 8) & 0xFF;
    uint8_t b = px & 0xFF;

    // integer luma ~0.299R+0.587G+0.114B
    // most accurate and faster approximation founded online :
    return (uint8_t)((77 * r + 150 * g + 29 * b) >> 8);
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


SDL_Surface* binarize_image(const char* input_path) 
{
    //import image
    SDL_Surface* src = SDL_LoadBMP(input_path);
    //test
    if (!src) {
        fprintf(stderr, "[preprocess] Failed to load BMP: %s\n", SDL_GetError());
        return NULL;
    }
    
    //convert it
    SDL_Surface* img = SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(src);
    //test
    if (!img) {
        fprintf(stderr, "[preprocess] Format conversion failed: %s\n", SDL_GetError());
        return NULL;
    }

    //Locking and unlocking help better compability between GPUs
    if (SDL_MUSTLOCK(img)) SDL_LockSurface(img);

    int W = img->w, H = img->h, N = W * H;
    uint32_t* p = (uint32_t*)img->pixels;
    
    uint8_t* Y = (uint8_t*)calloc(1,N);
    //graying all pixels
    for (int i = 0; i < N; i++) Y[i] = gray_from_rgb(p[i]);
    //then threshold the entire picture
    int t = isodata_threshold(Y, N);
    printf("[preprocess] Threshold (ISODATA) = %d\n", t);
    
    //convert the threshold process into surface
    SDL_Surface* bw = SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_ARGB8888);
    if (SDL_MUSTLOCK(bw)) SDL_LockSurface(bw);
    uint32_t* q = (uint32_t*)bw->pixels;
    for (int i = 0; i < N; i++) {
        uint8_t v = (Y[i] <= t) ? 0 : 255;
        q[i] = (0xFF << 24) | (v << 16) | (v << 8) | v;
    }

    //kind of "GPU helper"
    if (SDL_MUSTLOCK(bw)) SDL_UnlockSurface(bw);
    if (SDL_MUSTLOCK(img)) SDL_UnlockSurface(img);
    
    free(Y);
    SDL_FreeSurface(img);
    return bw;
}

