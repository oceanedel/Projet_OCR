#pragma once
#include <SDL2/SDL.h>


SDL_Surface* extract_wordlist(SDL_Surface* bin, int grid_x, int grid_y, int grid_w, int grid_h, int* out_x, int* out_y, int* out_w, int* out_h);