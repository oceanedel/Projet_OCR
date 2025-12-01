

#include "slice_grid.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

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

static int* find_line_positions(const int* prof, int N, int thr, int* out_count) {
    int* bands = (int*)malloc(sizeof(int) * 100);
    int count = 0;
    int in_band = 0, band_start = 0;
    
    for (int i = 0; i < N; i++) {
        if (prof[i] >= thr) {
            if (!in_band) { 
                band_start = i; 
                in_band = 1; 
            }
        } else {
            if (in_band) {
                bands[count++] = (band_start + i - 1) / 2;
                in_band = 0;
            }
        }
    }
    if (in_band) {
        bands[count++] = (band_start + N - 1) / 2;
    }
    
    *out_count = count;
    return bands;
}

int slice_grid(SDL_Surface* grid, const char* output_dir) {
    if (!grid || !output_dir) return -1;

    if (SDL_MUSTLOCK(grid)) SDL_LockSurface(grid);

    int W = grid->w, H = grid->h;
    int* Hprof = (int*)calloc(H, sizeof(int));
    int* Vprof = (int*)calloc(W, sizeof(int));

    build_profile_rows(grid, Hprof);
    build_profile_cols(grid, Vprof);

    int thr_row = (int)(0.5 * W);
    int thr_col = (int)(0.5 * H);

    int nr = 0, nc = 0;
    int* rbands = find_line_positions(Hprof, H, thr_row, &nr);
    int* cbands = find_line_positions(Vprof, W, thr_col, &nc);

    free(Hprof); free(Vprof);

    printf("[slice_grid] Detected %d row lines, %d col lines\n", nr, nc);

    if (nr < 2 || nc < 2) {
        fprintf(stderr, "[slice_grid] ERROR: not enough divider lines\n");
        free(rbands); free(cbands);
        if (SDL_MUSTLOCK(grid)) SDL_UnlockSurface(grid);
        return -2;
    }

    int R = nr - 1, C = nc - 1;
    printf("[slice_grid] Detected %dx%d grid\n", R, C);

    if (SDL_MUSTLOCK(grid)) SDL_UnlockSurface(grid);

    char path[512];
    int trim = 3;
    int saved = 0;

    for (int r = 0; r < R; r++) {
        for (int c = 0; c < C; c++) {
            int x0 = cbands[c] + trim;
            int x1 = cbands[c + 1] - trim;
            int y0 = rbands[r] + trim;
            int y1 = rbands[r + 1] - trim;

            if (x1 <= x0 || y1 <= y0) continue;

            SDL_Rect rc = { x0, y0, x1 - x0, y1 - y0 };
            SDL_Surface* cell = SDL_CreateRGBSurfaceWithFormat(0, rc.w, rc.h, 32, SDL_PIXELFORMAT_ARGB8888);
            SDL_Rect dst = { 0, 0, 0, 0 };
            SDL_BlitSurface(grid, &rc, cell, &dst);

            snprintf(path, sizeof(path), "%s/c_%02d_%02d.bmp", output_dir, r, c);
            SDL_SaveBMP(cell, path);
            SDL_FreeSurface(cell);
            saved++;
        }
    }

    free(rbands); free(cbands);
    printf("[slice_grid] Saved %d cells to %s\n", saved, output_dir);
    return 0;
}
