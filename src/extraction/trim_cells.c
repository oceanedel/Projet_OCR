#include "trim_cells.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

static inline bool is_black(uint32_t px) {
    return ((px & 0x00FFFFFF) == 0);
}


static int find_left_bound(SDL_Surface* img) {
    uint8_t* base = (uint8_t*)img->pixels;
    int pitch = img->pitch;
    
    for (int x = 0; x < img->w; x++) {
        for (int y = 0; y < img->h; y++) {
            uint32_t* row = (uint32_t*)(base + y * pitch);
            if (is_black(row[x])) {
                return x;
            }
        }
    }
    return 0;
}


static int find_right_bound(SDL_Surface* img) {
    uint8_t* base = (uint8_t*)img->pixels;
    int pitch = img->pitch;
    
    for (int x = img->w - 1; x >= 0; x--) {
        for (int y = 0; y < img->h; y++) {
            uint32_t* row = (uint32_t*)(base + y * pitch);
            if (is_black(row[x])) {
                return x + 1; // Exclusive bound
            }
        }
    }
    return img->w;
}

static int find_top_bound(SDL_Surface* img) {
    uint8_t* base = (uint8_t*)img->pixels;
    int pitch = img->pitch;
    
    for (int y = 0; y < img->h; y++) {
        uint32_t* row = (uint32_t*)(base + y * pitch);
        for (int x = 0; x < img->w; x++) {
            if (is_black(row[x])) {
                return y;
            }
        }
    }
    return 0;
}

static int find_bottom_bound(SDL_Surface* img) {
    uint8_t* base = (uint8_t*)img->pixels;
    int pitch = img->pitch;
    
    for (int y = img->h - 1; y >= 0; y--) {
        uint32_t* row = (uint32_t*)(base + y * pitch);
        for (int x = 0; x < img->w; x++) {
            if (is_black(row[x])) {
                return y + 1; // Exclusive bound
            }
        }
    }
    return img->h;
}

// Trim a single image and return trimmed version
static SDL_Surface* trim_image(SDL_Surface* img) {
    if (!img) return NULL;
    
    if (SDL_MUSTLOCK(img)) SDL_LockSurface(img);
    
    int left = find_left_bound(img);
    int right = find_right_bound(img);
    int top = find_top_bound(img);
    int bottom = find_bottom_bound(img);
    
    if (SDL_MUSTLOCK(img)) SDL_UnlockSurface(img);
    
    // Ensure valid bounds
    if (left >= right || top >= bottom) {
        // Empty or all-white image - return 1x1 white image
        SDL_Surface* empty = SDL_CreateRGBSurfaceWithFormat(
            0, 1, 1, 32, SDL_PIXELFORMAT_ARGB8888);
        uint32_t white = 0xFFFFFFFF;
        memcpy(empty->pixels, &white, sizeof(uint32_t));
        return empty;
    }
    
    int new_w = right - left;
    int new_h = bottom - top;
    
    // Create trimmed surface
    SDL_Rect src = {left, top, new_w, new_h};
    SDL_Surface* trimmed = SDL_CreateRGBSurfaceWithFormat(
        0, new_w, new_h, 32, SDL_PIXELFORMAT_ARGB8888);
    
    SDL_Rect dst = {0, 0, 0, 0};
    SDL_BlitSurface(img, &src, trimmed, &dst);
    
    return trimmed;
}

// Count cell files
static int count_cells(const char* cells_dir) {
    int count = 0;
    for (int r = 0; r < 50; r++) {
        for (int c = 0; c < 50; c++) {
            char path[512];
            snprintf(path, sizeof(path), "%s/c_%02d_%02d.bmp", cells_dir, r, c);
            struct stat st;
            if (stat(path, &st) == 0) {
                count++;
            }
        }
    }
    return count;
}


int trim_cells(const char* cells_dir) {
    printf("[TRIM_CELLS] Processing cells in: %s\n", cells_dir);
    
    int total_cells = count_cells(cells_dir);
    if (total_cells == 0) {
        fprintf(stderr, "[TRIM_CELLS] No cells found in %s\n", cells_dir);
        return -1;
    }
    
    printf("[TRIM_CELLS] Found %d cells to trim\n", total_cells);
    
    int trimmed_count = 0;
    
    for (int r = 0; r < 50; r++) {
        for (int c = 0; c < 50; c++) {
            char path[512];
            snprintf(path, sizeof(path), "%s/c_%02d_%02d.bmp", cells_dir, r, c);
            
            struct stat st;
            if (stat(path, &st) != 0) continue;
            
            SDL_Surface* cell = SDL_LoadBMP(path);
            if (!cell) {
                fprintf(stderr, "[TRIM_CELLS] Failed to load: %s\n", path);
                continue;
            }
            
            //int orig_w = cell->w;
            //int orig_h = cell->h;
            
            SDL_Surface* trimmed = trim_image(cell);
            SDL_FreeSurface(cell);
            
            if (!trimmed) {
                continue;
            }
            
            SDL_SaveBMP(trimmed, path);
            
            
            SDL_FreeSurface(trimmed);
            
            trimmed_count++;
        }
    }
    
    printf("[TRIM_CELLS] ✓ Trimmed %d cells\n", trimmed_count);
    
    return 0;
}
