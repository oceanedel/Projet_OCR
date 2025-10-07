#include "slice_grid.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

static inline bool is_black(uint32_t px) {
    return ((px & 0x00FFFFFF) == 0);
}

static int* malloc_int(int n) {
    int* p = (int*)malloc(sizeof(int) * n);
    if (!p) { fprintf(stderr, "alloc fail\n"); exit(1); }
    return p;
}

static void find_bands_int(const int* prof, int N, int thr, int** outBands, int* outCount) {
    int cap = 16, cnt = 0;
    int* bands = (int*)malloc(sizeof(int) * cap * 2);
    int i = 0;
    while (i < N) {
        if (prof[i] >= thr) {
            int s = i;
            while (i < N && prof[i] >= thr) i++;
            int e = i - 1;
            if (cnt >= cap) {
                cap *= 2;
                bands = (int*)realloc(bands, sizeof(int) * cap * 2);
            }
            bands[2 * cnt + 0] = s;
            bands[2 * cnt + 1] = e;
            cnt++;
        } else {
            i++;
        }
    }
    *outBands = bands;
    *outCount = cnt;
}

static void bands_to_centers(const int* bands, int nBands, int** outCenters, int* outN) {
    int* centers = malloc_int(nBands);
    for (int i = 0; i < nBands; i++) {
        int s = bands[2 * i + 0], e = bands[2 * i + 1];
        centers[i] = (s + e) / 2;
    }
    *outCenters = centers;
    *outN = nBands;
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

int slice_grid(SDL_Surface* grid, const char* output_dir) {
    if (!grid || !output_dir) return -1;

    if (SDL_MUSTLOCK(grid)) SDL_LockSurface(grid);

    int W = grid->w, H = grid->h;
    int* Hprof = malloc_int(H);
    int* Vprof = malloc_int(W);

    build_profile_rows(grid, Hprof);
    build_profile_cols(grid, Vprof);

    int thr_row = (int)(0.5 * W);
    int thr_col = (int)(0.5 * H);

    int *rbands = NULL, *cbands = NULL;
    int nr = 0, nc = 0;
    find_bands_int(Hprof, H, thr_row, &rbands, &nr);
    find_bands_int(Vprof, W, thr_col, &cbands, &nc);

    int *rcent = NULL, *ccent = NULL;
    int nrl = 0, ncl = 0;
    bands_to_centers(rbands, nr, &rcent, &nrl);
    bands_to_centers(cbands, nc, &ccent, &ncl);

    free(rbands); free(cbands);
    free(Hprof); free(Vprof);

    if (SDL_MUSTLOCK(grid)) SDL_UnlockSurface(grid);

    if (nrl < 2 || ncl < 2) {
        fprintf(stderr, "[slice_grid] ERROR: not enough dividers rows=%d cols=%d\n", nrl, ncl);
        free(rcent); free(ccent);
        return -2;
    }

    int R = nrl - 1, C = ncl - 1;
    printf("[slice_grid] Detected %dx%d grid\n", R, C);

    char path[512];
    int trim = 2;
    for (int r = 0; r < R; r++) {
        for (int c = 0; c < C; c++) {
            int x0 = ccent[c] + trim;
            int x1 = ccent[c + 1] - trim;
            int y0 = rcent[r] + trim;
            int y1 = rcent[r + 1] - trim;

            if (x1 <= x0 || y1 <= y0) continue;

            SDL_Rect rc = { x0, y0, x1 - x0, y1 - y0 };
            SDL_Surface* cell = SDL_CreateRGBSurfaceWithFormat(0, rc.w, rc.h, 32, SDL_PIXELFORMAT_ARGB8888);
            SDL_Rect dst = { 0, 0, 0, 0 };
            SDL_BlitSurface(grid, &rc, cell, &dst);

            snprintf(path, sizeof(path), "%s/c_%02d_%02d.bmp", output_dir, r, c);
            SDL_SaveBMP(cell, path);
            SDL_FreeSurface(cell);
        }
    }

    free(rcent); free(ccent);
    printf("[slice_grid] Saved %d cells to %s\n", R * C, output_dir);
    return 0;
}
