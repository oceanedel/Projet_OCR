#ifndef SLICE_GRID_NO_LINES_H
#define SLICE_GRID_NO_LINES_H

#include <SDL2/SDL.h>

// Découpe une grille sans quadrillage en détectant les alignements de texte
int slice_grid_no_lines(SDL_Surface* grid, const char* output_dir);

#endif
