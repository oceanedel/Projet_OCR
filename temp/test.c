#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

Uint8 median(Uint8 a[], int size) {
    // simple bubble sort
    for (int i = 0; i < size-1; i++) {
        for (int j = 0; j < size-i-1; j++) {
            if (a[j] > a[j+1]) {
                Uint8 tmp = a[j];
                a[j] = a[j+1];
                a[j+1] = tmp;
            }
        }
    }
    return a[size/2];
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s input.bmp\n", argv[0]);
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Surface* input = SDL_LoadBMP(argv[1]);
    if (!input) {
        printf("Could not load image: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Surface* input_32 = SDL_ConvertSurfaceFormat(input, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(input);
    input = input_32;

    if (!input) {
        printf("Failed to convert surface: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }


    SDL_Surface* output = SDL_CreateRGBSurface(
        0,
        input->w, input->h,
        32,
        input->format->Rmask,
        input->format->Gmask,
        input->format->Bmask,
        input->format->Amask
    );

    if (!output) {
        printf("Surface creation error: %s\n", SDL_GetError());
        SDL_FreeSurface(input);
        SDL_Quit();
        return 1;
    }

    SDL_LockSurface(input);
    SDL_LockSurface(output);

    int w = input->w;
    int h = input->h;

    Uint32* in  = (Uint32*)input->pixels;
    Uint32* out = (Uint32*)output->pixels;

    for (int y = 1; y < h - 1; y++) {
    for (int x = 1; x < w - 1; x++) {

        Uint8 r[9], g[9], b[9];
        int idx = 0;

        for (int dy = -1; dy <= 1; dy++) {
            Uint32* row = (Uint32*)((Uint8*)input->pixels + (y + dy) * input->pitch);

            for (int dx = -1; dx <= 1; dx++) {
                Uint32 pixel = row[x + dx];

                Uint8 R, G, B;
                SDL_GetRGB(pixel, input->format, &R, &G, &B);

                r[idx] = R;
                g[idx] = G;
                b[idx] = B;
                idx++;
            }
        }

        Uint32* out_row = (Uint32*)((Uint8*)output->pixels + y * output->pitch);
        out_row[x] = SDL_MapRGB(output->format,
                                median(r, 9),
                                median(g, 9),
                                median(b, 9));
    }
}


    SDL_UnlockSurface(input);
    SDL_UnlockSurface(output);

    SDL_SaveBMP(output, "output.bmp");

    SDL_FreeSurface(input);
    SDL_FreeSurface(output);
    SDL_Quit();

    printf("Denoised image saved to output.bmp\n");
    return 0;
}
