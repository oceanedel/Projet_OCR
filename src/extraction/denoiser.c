#include "denoiser.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

// Helper: Find median of array using bubble sort
static Uint8 median(Uint8 a[], int size) {
    // Simple bubble sort
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (a[j] > a[j + 1]) {
                Uint8 tmp = a[j];
                a[j] = a[j + 1];
                a[j + 1] = tmp;
            }
        }
    }
    return a[size / 2];
}

SDL_Surface* denoise_image(SDL_Surface* input) {
    if (!input) {
        fprintf(stderr, "[denoiser] Input surface is NULL\n");
        return NULL;
    }
    
    // Convert to 32-bit ARGB if needed
    SDL_Surface* input_32 = NULL;
    if (input->format->format != SDL_PIXELFORMAT_ARGB8888) {
        input_32 = SDL_ConvertSurfaceFormat(input, SDL_PIXELFORMAT_ARGB8888, 0);
        if (!input_32) {
            fprintf(stderr, "[denoiser] Format conversion failed: %s\n", SDL_GetError());
            return NULL;
        }
    } else {
        input_32 = input; // Already correct format
    }
    
    // Create output surface
    SDL_Surface* output = SDL_CreateRGBSurface(
        0,
        input_32->w, input_32->h,
        32,
        input_32->format->Rmask,
        input_32->format->Gmask,
        input_32->format->Bmask,
        input_32->format->Amask
    );
    
    if (!output) {
        fprintf(stderr, "[denoiser] Surface creation error: %s\n", SDL_GetError());
        if (input_32 != input) SDL_FreeSurface(input_32);
        return NULL;
    }
    
    SDL_LockSurface(input_32);
    SDL_LockSurface(output);
    
    int w = input_32->w;
    int h = input_32->h;
    
    // Copy border pixels as-is (no filtering)
    Uint32* in_pixels = (Uint32*)input_32->pixels;
    Uint32* out_pixels = (Uint32*)output->pixels;
    
    // Top and bottom rows
    for (int x = 0; x < w; x++) {
        out_pixels[x] = in_pixels[x]; // Top row
        out_pixels[(h - 1) * w + x] = in_pixels[(h - 1) * w + x]; // Bottom row
    }
    
    // Left and right columns
    for (int y = 0; y < h; y++) {
        Uint32* out_row = (Uint32*)((Uint8*)output->pixels + y * output->pitch);
        Uint32* in_row = (Uint32*)((Uint8*)input_32->pixels + y * input_32->pitch);
        out_row[0] = in_row[0]; // Left column
        out_row[w - 1] = in_row[w - 1]; // Right column
    }
    
    // Apply 3x3 median filter to interior pixels
    for (int y = 1; y < h - 1; y++) {
        for (int x = 1; x < w - 1; x++) {
            Uint8 r[9], g[9], b[9];
            int idx = 0;
            
            // Collect 3x3 neighborhood
            for (int dy = -1; dy <= 1; dy++) {
                Uint32* row = (Uint32*)((Uint8*)input_32->pixels + (y + dy) * input_32->pitch);
                
                for (int dx = -1; dx <= 1; dx++) {
                    Uint32 pixel = row[x + dx];
                    
                    Uint8 R, G, B;
                    SDL_GetRGB(pixel, input_32->format, &R, &G, &B);
                    
                    r[idx] = R;
                    g[idx] = G;
                    b[idx] = B;
                    idx++;
                }
            }
            
            // Apply median to each channel
            Uint32* out_row = (Uint32*)((Uint8*)output->pixels + y * output->pitch);
            out_row[x] = SDL_MapRGB(output->format,
                                    median(r, 9),
                                    median(g, 9),
                                    median(b, 9));
        }
    }
    
    SDL_UnlockSurface(input_32);
    SDL_UnlockSurface(output);
    
    // Free temporary surface if we created one
    if (input_32 != input) {
        SDL_FreeSurface(input_32);
    }
    
    printf("[denoiser] ✓ Median filter applied (%dx%d)\n", w, h);
    
    return output;
}
