// Compile with: sdl2 + optionally SDL_image
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "src/inkpaper.h"
#include "src/grid.h"

int main(int argc, char** argv)
{

    if(argc<2){ fprintf(stderr,"usage: m1 <image>\n"); return 1; }

    inkpaper(argv[1]);

    GridLines gl; 
    CellMap cm;
    detect_grid_lines("out.bmp", &gl, 0.5f, 0.5f);
    build_cells(&gl, &cm, 1);
    export_cells("out.bmp", &cm, "cells");
    free_cell_map(&cm);
    free_grid_lines(&gl);


    return 0;
}

