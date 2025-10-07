#include "extract_wordlist.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static inline bool is_black(uint32_t px) {
    return ((px & 0x00FFFFFF) == 0);
}

static int count_black_in_region(SDL_Surface* s, int x0, int y0, int w, int h) {
    if (x0 < 0 || y0 < 0 || x0 + w > s->w || y0 + h > s->h) return 0;
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

    // Check left margin
    int left_w = (grid_x > 20) ? grid_x - 10 : 0;
    int left_cnt = count_black_in_region(bin, 0, 0, left_w, bin->h);

    // Check right margin
    int right_x = grid_x + grid_w + 10;
    int right_w = (right_x < bin->w) ? bin->w - right_x : 0;
    int right_cnt = count_black_in_region(bin, right_x, 0, right_w, bin->h);

    if (SDL_MUSTLOCK(bin)) SDL_UnlockSurface(bin);

    // Choose margin with most ink
    int wl_x, wl_y, wl_w, wl_h;
    if (left_cnt > right_cnt && left_w > 0) {
        wl_x = 0; wl_y = 0; wl_w = left_w; wl_h = bin->h;
        printf("[extract_wordlist] Using left margin: %d pixels of ink\n", left_cnt);
    } else if (right_w > 0) {
        wl_x = right_x; wl_y = 0; wl_w = right_w; wl_h = bin->h;
        printf("[extract_wordlist] Using right margin: %d pixels of ink\n", right_cnt);
    } else {
        fprintf(stderr, "[extract_wordlist] ERROR: no suitable word list margin found\n");
        return NULL;
    }

    SDL_Rect src_rect = { wl_x, wl_y, wl_w, wl_h };
    SDL_Surface* wl = SDL_CreateRGBSurfaceWithFormat(0, wl_w, wl_h, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_Rect dst_rect = { 0, 0, 0, 0 };
    SDL_BlitSurface(bin, &src_rect, wl, &dst_rect);

    if (out_x) *out_x = wl_x;
    if (out_y) *out_y = wl_y;
    if (out_w) *out_w = wl_w;
    if (out_h) *out_h = wl_h;

    return wl;
}
