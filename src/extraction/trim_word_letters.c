#include "trim_word_letters.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>

#define MARGIN 1 

static inline bool is_black(uint32_t px) {
    return ((px & 0x00FFFFFF) == 0);
}

// Trim left
static SDL_Surface* trim_left(SDL_Surface* img) {
    if (!img) return NULL;
    
    if (SDL_MUSTLOCK(img)) SDL_LockSurface(img);
    
    uint8_t* base = (uint8_t*)img->pixels;
    int pitch = img->pitch;
    int left_cut = 0;
    
    for (int x = 0; x < img->w; x++) {
        bool has_black = false;
        for (int y = 0; y < img->h; y++) {
            uint32_t* row = (uint32_t*)(base + y * pitch);
            if (is_black(row[x])) {
                has_black = true;
                break;
            }
        }
        if (has_black) {
            left_cut = x;
            break;
        }
        left_cut = x + 1;
    }
    
    if (SDL_MUSTLOCK(img)) SDL_UnlockSurface(img);
    
    // Apply margin
    if (left_cut > MARGIN) left_cut -= MARGIN; else left_cut = 0;

    if (left_cut == 0) return img;
    
    if (left_cut >= img->w) {
        SDL_Surface* empty = SDL_CreateRGBSurfaceWithFormat(0, 1, 1, 32, SDL_PIXELFORMAT_ARGB8888);
        uint32_t white = 0xFFFFFFFF;
        memcpy(empty->pixels, &white, sizeof(uint32_t));
        SDL_FreeSurface(img);
        return empty;
    }
    
    int new_w = img->w - left_cut;
    SDL_Rect src = {left_cut, 0, new_w, img->h};
    SDL_Surface* trimmed = SDL_CreateRGBSurfaceWithFormat(0, new_w, img->h, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_Rect dst = {0, 0, 0, 0};
    SDL_BlitSurface(img, &src, trimmed, &dst);
    SDL_FreeSurface(img);
    
    return trimmed;
}

// Trim top
static SDL_Surface* trim_top(SDL_Surface* img) {
    if (!img) return NULL;
    
    if (SDL_MUSTLOCK(img)) SDL_LockSurface(img);
    
    uint8_t* base = (uint8_t*)img->pixels;
    int pitch = img->pitch;
    int top_cut = 0;
    
    for (int y = 0; y < img->h; y++) {
        uint32_t* row = (uint32_t*)(base + y * pitch);
        bool has_black = false;
        for (int x = 0; x < img->w; x++) {
            if (is_black(row[x])) {
                has_black = true;
                break;
            }
        }
        if (has_black) {
            top_cut = y;
            break;
        }
        top_cut = y + 1;
    }
    
    if (SDL_MUSTLOCK(img)) SDL_UnlockSurface(img);
    
    // Apply margin
    if (top_cut > MARGIN) top_cut -= MARGIN; else top_cut = 0;

    if (top_cut == 0) return img;
    
    if (top_cut >= img->h) {
        SDL_Surface* empty = SDL_CreateRGBSurfaceWithFormat(0, 1, 1, 32, SDL_PIXELFORMAT_ARGB8888);
        uint32_t white = 0xFFFFFFFF;
        memcpy(empty->pixels, &white, sizeof(uint32_t));
        SDL_FreeSurface(img);
        return empty;
    }
    
    int new_h = img->h - top_cut;
    SDL_Rect src = {0, top_cut, img->w, new_h};
    SDL_Surface* trimmed = SDL_CreateRGBSurfaceWithFormat(0, img->w, new_h, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_Rect dst = {0, 0, 0, 0};
    SDL_BlitSurface(img, &src, trimmed, &dst);
    SDL_FreeSurface(img);
    
    return trimmed;
}

// Trim right
static SDL_Surface* trim_right(SDL_Surface* img) {
    if (!img) return NULL;
    
    if (SDL_MUSTLOCK(img)) SDL_LockSurface(img);
    
    uint8_t* base = (uint8_t*)img->pixels;
    int pitch = img->pitch;
    int right_cut = img->w;
    
    for (int x = img->w - 1; x >= 0; x--) {
        bool has_black = false;
        for (int y = 0; y < img->h; y++) {
            uint32_t* row = (uint32_t*)(base + y * pitch);
            if (is_black(row[x])) {
                has_black = true;
                break;
            }
        }
        if (has_black) {
            right_cut = x + 1;
            break;
        }
    }
    
    if (SDL_MUSTLOCK(img)) SDL_UnlockSurface(img);
    
    // Apply margin (only if we are trimming)
    if (right_cut < img->w) {
        right_cut += MARGIN;
        if (right_cut > img->w) right_cut = img->w;
    }

    if (right_cut == img->w) return img;
    
    if (right_cut <= 0) {
        SDL_Surface* empty = SDL_CreateRGBSurfaceWithFormat(0, 1, 1, 32, SDL_PIXELFORMAT_ARGB8888);
        uint32_t white = 0xFFFFFFFF;
        memcpy(empty->pixels, &white, sizeof(uint32_t));
        SDL_FreeSurface(img);
        return empty;
    }
    
    SDL_Rect src = {0, 0, right_cut, img->h};
    SDL_Surface* trimmed = SDL_CreateRGBSurfaceWithFormat(0, right_cut, img->h, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_Rect dst = {0, 0, 0, 0};
    SDL_BlitSurface(img, &src, trimmed, &dst);
    SDL_FreeSurface(img);
    
    return trimmed;
}

// Trim bottom
static SDL_Surface* trim_bottom(SDL_Surface* img) {
    if (!img) return NULL;
    
    if (SDL_MUSTLOCK(img)) SDL_LockSurface(img);
    
    uint8_t* base = (uint8_t*)img->pixels;
    int pitch = img->pitch;
    int bottom_cut = img->h;
    
    for (int y = img->h - 1; y >= 0; y--) {
        uint32_t* row = (uint32_t*)(base + y * pitch);
        bool has_black = false;
        for (int x = 0; x < img->w; x++) {
            if (is_black(row[x])) {
                has_black = true;
                break;
            }
        }
        if (has_black) {
            bottom_cut = y + 1;
            break;
        }
    }
    
    if (SDL_MUSTLOCK(img)) SDL_UnlockSurface(img);
    
    // Apply margin
    if (bottom_cut < img->h) {
        bottom_cut += MARGIN;
        if (bottom_cut > img->h) bottom_cut = img->h;
    }

    if (bottom_cut == img->h) return img;
    
    if (bottom_cut <= 0) {
        SDL_Surface* empty = SDL_CreateRGBSurfaceWithFormat(0, 1, 1, 32, SDL_PIXELFORMAT_ARGB8888);
        uint32_t white = 0xFFFFFFFF;
        memcpy(empty->pixels, &white, sizeof(uint32_t));
        SDL_FreeSurface(img);
        return empty;
    }
    
    SDL_Rect src = {0, 0, img->w, bottom_cut};
    SDL_Surface* trimmed = SDL_CreateRGBSurfaceWithFormat(0, img->w, bottom_cut, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_Rect dst = {0, 0, 0, 0};
    SDL_BlitSurface(img, &src, trimmed, &dst);
    SDL_FreeSurface(img);
    
    return trimmed;
}

// Trim image (4 passes)
static SDL_Surface* trim_image(SDL_Surface* img) {
    if (!img) return NULL;
    
    SDL_Surface* working = SDL_ConvertSurfaceFormat(img, SDL_PIXELFORMAT_ARGB8888, 0);
    if (!working) return NULL;
    
    working = trim_left(working);
    if (!working) return NULL;
    
    working = trim_top(working);
    if (!working) return NULL;
    
    working = trim_right(working);
    if (!working) return NULL;
    
    working = trim_bottom(working);
    
    return working;
}

// Main function - trim all word letters
int trim_word_letters(const char* letters_dir) {
    printf("[TRIM_LETTERS] Processing: %s\n", letters_dir);
    
    int trimmed_count = 0;
    
    // Process word_XX_letter_YY.bmp files
    for (int w = 0; w < 50; w++) {
        for (int l = 0; l < 30; l++) {
            char path[512];
            snprintf(path, sizeof(path), "%s/word_%02d_letter_%02d.bmp", letters_dir, w, l);
            
            struct stat st;
            if (stat(path, &st) != 0) continue;
            
            SDL_Surface* original = SDL_LoadBMP(path);
            if (!original) {
                continue;
            }
            
            SDL_Surface* trimmed = trim_image(original);
            SDL_FreeSurface(original);
            
            if (!trimmed) {
                continue;
            }
            
            SDL_SaveBMP(trimmed, path);
            SDL_FreeSurface(trimmed);
            
            trimmed_count++;
        }
    }
    
    printf("[TRIM_LETTERS] ✓ Trimmed %d letters\n", trimmed_count);
    
    return 0;
}
