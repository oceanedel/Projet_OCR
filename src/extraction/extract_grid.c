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

static int median_gap(const int* lines, int count) {
    if (count < 2) return 0;
    int sum = 0;
    for (int i = 0; i < count - 1; i++) {
        sum += (lines[i + 1] - lines[i]);
    }
    return sum / (count - 1);
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

    printf("[extract_grid] Image size: %dx%d\n", W, H);

    int thr_row = (int)(0.5 * W);
    int thr_col = (int)(0.5 * H);
    
    int row_line_count = 0, col_line_count = 0;
    int* row_lines = find_line_positions(Hprof, H, thr_row, &row_line_count);
    int* col_lines = find_line_positions(Vprof, W, thr_col, &col_line_count);
    
    printf("[extract_grid] Detected %d row lines, %d col lines\n", row_line_count, col_line_count);
    
    if (row_line_count < 2 || col_line_count < 2) {
        printf("[extract_grid] Insufficient grid lines, switching to fallback\n");
        free(Hprof); free(Vprof); free(row_lines); free(col_lines);
        if (SDL_MUSTLOCK(bin)) SDL_UnlockSurface(bin);
        return NULL;
    }
    
    int avg_cell_height = median_gap(row_lines, row_line_count);
    int avg_cell_width = median_gap(col_lines, col_line_count);
    
    printf("[extract_grid] Average cell size: %dx%d\n", avg_cell_width, avg_cell_height);
    
    int y0 = row_lines[0];
    int y1 = row_lines[row_line_count - 1];
    int x0 = col_lines[0];
    int x1 = col_lines[col_line_count - 1];
    
    printf("[extract_grid] Initial bounds: y=[%d,%d] x=[%d,%d]\n", y0, y1, x0, x1);
    
    // Validate using regularity (trim irregular columns)
    int valid_x1 = x0;
    double tolerance = 0.25;
    
    for (int i = 1; i < col_line_count; i++) {
        int gap = col_lines[i] - col_lines[i - 1];
        double ratio = (double)gap / avg_cell_width;
        
        if (ratio >= (1.0 - tolerance) && ratio <= (1.0 + tolerance)) {
            valid_x1 = col_lines[i];
        } else {
            printf("[extract_grid] Irregular gap at col %d: %d px (expected ~%d)\n", i, gap, avg_cell_width);
            break;
        }
    }
    
    x1 = valid_x1;
    
    free(Hprof); free(Vprof); free(row_lines); free(col_lines);

    if (SDL_MUSTLOCK(bin)) SDL_UnlockSurface(bin);

    int gw = x1 - x0 + 1;
    int gh = y1 - y0 + 1;

    if (gw <= 0 || gh <= 0) {
        fprintf(stderr, "[extract_grid] ERROR: invalid dimensions\n");
        return NULL;
    }

    printf("[extract_grid] Final grid at (%d, %d) size %dx%d\n", x0, y0, gw, gh);

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
