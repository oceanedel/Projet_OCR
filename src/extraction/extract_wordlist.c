#include "extract_wordlist.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static inline bool is_black(uint32_t px) {
    return ((px & 0x00FFFFFF) == 0);
}

static int count_black_in_region(SDL_Surface* s, int x0, int y0, int w, int h) {
    if (x0 < 0 || y0 < 0 || w <= 0 || h <= 0) return 0;
    if (x0 + w > s->w || y0 + h > s->h) return 0;
    
    uint8_t* base = (uint8_t*)s->pixels;
    int pitch = s->pitch;
    int cnt = 0;
    
    for (int y = y0; y < y0 + h; y++) {
        uint32_t* row = (uint32_t*)(base + y * pitch);
        for (int x = x0; x < x0 + w; x++) {
            if (is_black(row[x])) cnt++;
        }
    }
    return cnt;
}

SDL_Surface* extract_wordlist(SDL_Surface* bin, int grid_x, int grid_y, int grid_w, int grid_h,
                               int* out_x, int* out_y, int* out_w, int* out_h) {
    if (!bin) return NULL;

    if (SDL_MUSTLOCK(bin)) SDL_LockSurface(bin);

    const int padding = 10; // Safety margin around grid

    // LEFT MARGIN
    int left_w = (grid_x > padding) ? grid_x - padding : 0;
    int left_h = bin->h;
    int left_cnt = 0;
    if (left_w > 0) {
        left_cnt = count_black_in_region(bin, 0, 0, left_w, left_h);
    }

    // RIGHT MARGIN
    int right_x = grid_x + grid_w + padding;
    int right_w = (right_x < bin->w) ? bin->w - right_x : 0;
    int right_h = bin->h;
    int right_cnt = 0;
    if (right_w > 0) {
        right_cnt = count_black_in_region(bin, right_x, 0, right_w, right_h);
    }

    // TOP MARGIN
    int top_w = bin->w;
    int top_h = (grid_y > padding) ? grid_y - padding : 0;
    int top_cnt = 0;
    if (top_h > 0) {
        top_cnt = count_black_in_region(bin, 0, 0, top_w, top_h);
    }

    // BOTTOM MARGIN
    int bottom_y = grid_y + grid_h + padding;
    int bottom_w = bin->w;
    int bottom_h = (bottom_y < bin->h) ? bin->h - bottom_y : 0;
    int bottom_cnt = 0;
    if (bottom_h > 0) {
        bottom_cnt = count_black_in_region(bin, 0, bottom_y, bottom_w, bottom_h);
    }

    if (SDL_MUSTLOCK(bin)) SDL_UnlockSurface(bin);

    // ============================
    // Choose margin with most ink
    // ============================

    printf("[extract_wordlist] Ink density analysis:\n");
    printf("  Left:   %d pixels (w=%d)\n", left_cnt, left_w);
    printf("  Right:  %d pixels (w=%d)\n", right_cnt, right_w);
    printf("  Top:    %d pixels (h=%d)\n", top_cnt, top_h);
    printf("  Bottom: %d pixels (h=%d)\n", bottom_cnt, bottom_h);

    int max_cnt = left_cnt;
    int wl_x = 0, wl_y = 0, wl_w = left_w, wl_h = left_h;
    const char* position = "left";

    if (right_cnt > max_cnt && right_w > 0) {
        max_cnt = right_cnt;
        wl_x = right_x; wl_y = 0; wl_w = right_w; wl_h = right_h;
        position = "right";
    }
    
    if (top_cnt > max_cnt && top_h > 0) {
        max_cnt = top_cnt;
        wl_x = 0; wl_y = 0; wl_w = top_w; wl_h = top_h;
        position = "top";
    }
    
    if (bottom_cnt > max_cnt && bottom_h > 0) {
        max_cnt = bottom_cnt;
        wl_x = 0; wl_y = bottom_y; wl_w = bottom_w; wl_h = bottom_h;
        position = "bottom";
    }

    if (max_cnt == 0 || wl_w <= 0 || wl_h <= 0) {
        fprintf(stderr, "[extract_wordlist] ERROR: no suitable word list margin found\n");
        return NULL;
    }

    printf("  → Selected: %s margin (%d pixels of ink)\n", position, max_cnt);

    // ============================
    // Crop the selected region
    // ============================

    SDL_Rect src_rect = { wl_x, wl_y, wl_w, wl_h };
    SDL_Surface* wl = SDL_CreateRGBSurfaceWithFormat(0, wl_w, wl_h, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!wl) {
        fprintf(stderr, "[extract_wordlist] ERROR: surface creation failed: %s\n", SDL_GetError());
        return NULL;
    }
    
    SDL_Rect dst_rect = { 0, 0, 0, 0 };
    if (SDL_BlitSurface(bin, &src_rect, wl, &dst_rect) != 0) {
        fprintf(stderr, "[extract_wordlist] ERROR: blit failed: %s\n", SDL_GetError());
        SDL_FreeSurface(wl);
        return NULL;
    }

    if (out_x) *out_x = wl_x;
    if (out_y) *out_y = wl_y;
    if (out_w) *out_w = wl_w;
    if (out_h) *out_h = wl_h;

    return wl;
}
