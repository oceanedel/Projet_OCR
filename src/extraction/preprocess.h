#pragma once
#include <SDL2/SDL.h>


SDL_Surface* binarize_image(const char* input_path);

int isodata_threshold(const uint8_t* gray, int n);
