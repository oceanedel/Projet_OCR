#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

uint8_t gray_from_rgb(uint32_t px, const SDL_PixelFormat* f);
int isodata_threshold(const uint8_t *Y, int n);
void inkpaper(char* path);
