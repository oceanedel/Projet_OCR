// Compile with: sdl2 + optionally SDL_image
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "src/inkpaper.h"

int main(int argc, char** argv)
{

    if(argc<2){ fprintf(stderr,"usage: m1 <image>\n"); return 1; }

    inkpaper(argv[1]);

    return 0;
}

