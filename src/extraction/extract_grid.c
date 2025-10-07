#include "extract_grid.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static inline bool is_black(uint32_t px) {
    return ((px & 0x00FFFFFF) == 0);
}

static void build_profile_rows(SDL_Surface* s, int* H) {
    uint8_t* base = (uint8_t*)s->pixels;
    int pitch = s->pitch;
    for (int y = 0; y < s->h; y++) {
        uint32_t* row = (uint32_t*)(base + y * pitch);
        int cnt = 0;
        for (int x = 0; x < s->w; x++) {
            if (is_black(row[x])) cnt++;
        }
        H[y] = cnt;
    }
}

static void build_profile_cols(SDL_Surface* s, int* V) {
    uint8_t* base = (uint8_t*)s->pixels;
    int pitch = s->pitch;
    for (int x = 0; x < s->w; x++) {
        int cnt = 0;
        for (int y = 0; y < s->h; y++) {
            uint32_t* row = (uint32_t*)(base + y * pitch);
            if (is_black(row[x])) cnt++;
        }
        V[x] = cnt;
    }
}

static void find_grid_bounds(const int* prof, int N, int thr, int* start, int* end) {
    *start = -1; *end = -1;
    for (int i = 0; i < N; i++) {
        if (prof[i] >= thr) {
            if (*start == -1) *start = i;
            *end = i;
        }
    }
}

SDL_Surface* extract_grid(SDL_Surface* bin, int* out_x, int* out_y, int* out_w, int* out_h) {
    if (!bin) return NULL;
    if (bin->format->format != SDL_PIXELFORMAT_ARGB8888) {
        fprintf(stderr, "[extract_grid] ERROR: expected ARGB8888\n");
        return NULL;
    }

    if (SDL_MUSTLOCK(bin)) SDL_LockSurface(bin);

    int W = bin->w, H = bin->h;
    int* Hprof = (int*)calloc(H, sizeof(int));
    int* Vprof = (int*)calloc(W, sizeof(int));

    build_profile_rows(bin, Hprof);
    build_profile_cols(bin, Vprof);

    // Use threshold: significant ink density
    int thr_row = (int)(0.3 * W);
    int thr_col = (int)(0.3 * H);

    int y0, y1, x0, x1;
    find_grid_bounds(Hprof, H, thr_row, &y0, &y1);
    find_grid_bounds(Vprof, W, thr_col, &x0, &x1);

    free(Hprof); free(Vprof);

    if (SDL_MUSTLOCK(bin)) SDL_UnlockSurface(bin);

    if (y0 == -1 || x0 == -1 || y1 <= y0 || x1 <= x0) {
        fprintf(stderr, "[extract_grid] ERROR: could not find grid bounds\n");
        return NULL;
    }

    int gw = x1 - x0 + 1;
    int gh = y1 - y0 + 1;

    printf("[extract_grid] Grid found at (%d, %d) size %dx%d\n", x0, y0, gw, gh);

    SDL_Rect src_rect = { x0, y0, gw, gh };
    SDL_Surface* grid = SDL_CreateRGBSurfaceWithFormat(0, gw, gh, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_Rect dst_rect = { 0, 0, 0, 0 };
    SDL_BlitSurface(bin, &src_rect, grid, &dst_rect);

    if (out_x) *out_x = x0;
    if (out_y) *out_y = y0;
    if (out_w) *out_w = gw;
    if (out_h) *out_h = gh;

    return grid;
}
