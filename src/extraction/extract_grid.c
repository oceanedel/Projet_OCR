#include "extract_grid.h"
#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---------- Basic pixel helpers ----------

static inline bool is_black(uint32_t px) {
    return ((px & 0x00FFFFFF) == 0);
}

static inline uint32_t get_pixel(SDL_Surface* s, int x, int y) {
    if (x < 0 || y < 0 || x >= s->w || y >= s->h)
        return 0xFFFFFFFF; // white background
    uint8_t* base = (uint8_t*)s->pixels;
    uint32_t* row = (uint32_t*)(base + y * s->pitch);
    return row[x];
}

static inline void put_pixel(SDL_Surface* s, int x, int y, uint32_t px) {
    if (x < 0 || y < 0 || x >= s->w || y >= s->h) return;
    uint8_t* base = (uint8_t*)s->pixels;
    uint32_t* row = (uint32_t*)(base + y * s->pitch);
    row[x] = px;
}

// ---------- Manual rotation ----------

static SDL_Surface* rotate_surface(SDL_Surface* src, double angle_deg) {
    if (fabs(angle_deg) < 0.01) {
        SDL_Surface* dup = SDL_CreateRGBSurfaceWithFormat(0, src->w, src->h, 32, SDL_PIXELFORMAT_ARGB8888);
        SDL_BlitSurface(src, NULL, dup, NULL);
        return dup;
    }

    double rad = -angle_deg * M_PI / 180.0;
    double cs = cos(rad);
    double sn = sin(rad);

    double corners_x[4] = {0, (double)src->w, (double)src->w, 0};
    double corners_y[4] = {0, 0, (double)src->h, (double)src->h};

    double min_x = 0, max_x = 0, min_y = 0, max_y = 0;
    for (int i = 0; i < 4; i++) {
        double rx = corners_x[i] * cs - corners_y[i] * sn;
        double ry = corners_x[i] * sn + corners_y[i] * cs;
        if (i == 0 || rx < min_x) min_x = rx;
        if (i == 0 || rx > max_x) max_x = rx;
        if (i == 0 || ry < min_y) min_y = ry;
        if (i == 0 || ry > max_y) max_y = ry;
    }

    int new_w = (int)(max_x - min_x + 1);
    int new_h = (int)(max_y - min_y + 1);

    SDL_Surface* dst = SDL_CreateRGBSurfaceWithFormat(0, new_w, new_h, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_FillRect(dst, NULL, 0xFFFFFFFF);

    if (SDL_MUSTLOCK(src)) SDL_LockSurface(src);
    if (SDL_MUSTLOCK(dst)) SDL_LockSurface(dst);

    double cx_src = src->w / 2.0;
    double cy_src = src->h / 2.0;
    double cx_dst = new_w / 2.0;
    double cy_dst = new_h / 2.0;

    for (int y = 0; y < new_h; y++) {
        for (int x = 0; x < new_w; x++) {
            double dx = x - cx_dst;
            double dy = y - cy_dst;
            double sx = dx * cs + dy * sn + cx_src;
            double sy = -dx * sn + dy * cs + cy_src;

            int ix = (int)(sx + 0.5);
            int iy = (int)(sy + 0.5);

            uint32_t px = get_pixel(src, ix, iy);
            put_pixel(dst, x, y, px);
        }
    }

    if (SDL_MUSTLOCK(src)) SDL_UnlockSurface(src);
    if (SDL_MUSTLOCK(dst)) SDL_UnlockSurface(dst);

    return dst;
}

// ---------- Projection profiles ----------

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

static void smooth_profile(int* P, int N) {
    if (N < 3) return;
    int* temp = (int*)malloc(sizeof(int) * N);
    memcpy(temp, P, sizeof(int) * N);
    
    for (int i = 1; i < N - 1; i++) {
        P[i] = (temp[i-1] + temp[i] + temp[i+1]) / 3;
    }
    free(temp);
}

// ---------- Band/line detection ----------

static int* find_line_positions(const int* prof, int N, int thr, int* out_count) {
    int cap = 128;
    int* bands = (int*)malloc(sizeof(int) * cap);
    int count = 0;
    int in_band = 0, band_start = 0;

    for (int i = 0; i < N; i++) {
        if (prof[i] >= thr) {
            if (!in_band) {
                band_start = i;
                in_band = 1;
            }
        } else if (in_band) {
            int pos = (band_start + i - 1) / 2;
            if (count == cap) {
                cap *= 2;
                bands = (int*)realloc(bands, sizeof(int) * cap);
            }
            bands[count++] = pos;
            in_band = 0;
        }
    }

    if (in_band) {
        int pos = (band_start + N - 1) / 2;
        if (count == cap) {
            cap *= 2;
            bands = (int*)realloc(bands, sizeof(int) * cap);
        }
        bands[count++] = pos;
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

// ---------- Skew detection ----------

static double projection_variance_at_angle(SDL_Surface* bin, double angle_deg) {
    int step = 4;
    int dsW = bin->w / step;
    int dsH = bin->h / step;
    if (dsW <= 0 || dsH <= 0) return 0.0;

    double rad = angle_deg * M_PI / 180.0;
    double cs = cos(rad);
    double sn = sin(rad);

    int* prof = (int*)calloc(dsH, sizeof(int));

    double cx = dsW / 2.0;
    double cy = dsH / 2.0;

    for (int y = 0; y < dsH; y++) {
        for (int x = 0; x < dsW; x++) {
            double xr = x - cx;
            double yr = y - cy;

            double rx = xr * cs - yr * sn + cx;
            double ry = xr * sn + yr * cs + cy;

            int ix = (int)(rx * step + 0.5);
            int iy = (int)(ry * step + 0.5);

            if (ix >= 0 && iy >= 0 && ix < bin->w && iy < bin->h) {
                uint32_t px = get_pixel(bin, ix, iy);
                if (is_black(px)) prof[y]++;
            }
        }
    }

    double mean = 0.0;
    for (int i = 0; i < dsH; i++) mean += prof[i];
    mean /= (double)dsH;

    double var = 0.0;
    for (int i = 0; i < dsH; i++) {
        double d = prof[i] - mean;
        var += d * d;
    }
    var /= (double)dsH;

    free(prof);
    return var;
}

static double detect_skew_angle(SDL_Surface* bin) {
    double best_angle = 0.0;
    double best_var = projection_variance_at_angle(bin, 0.0);
    double var_at_zero = best_var;

    for (double a = -10.0; a <= 10.0; a += 0.5) {
        if (fabs(a) < 0.1) continue;
        
        double v = projection_variance_at_angle(bin, a);
        if (v > best_var) {
            best_var = v;
            best_angle = a;
        }
    }

    double improvement = (best_var - var_at_zero) / var_at_zero;
    
    if (improvement < 0.05) {
        printf("[extract_grid] No significant skew detected (improvement: %.1f%%), skipping rotation\n", 
               improvement * 100.0);
        return 0.0;
    }

    printf("[extract_grid] Estimated skew angle: %.2f degrees (improvement: %.1f%%)\n", 
           best_angle, improvement * 100.0);
    return best_angle;
}

// ---------- Main extraction with fallback ----------

SDL_Surface* extract_grid(SDL_Surface* bin, int* out_x, int* out_y, int* out_w, int* out_h) {
    if (!bin) return NULL;
    if (bin->format->format != SDL_PIXELFORMAT_ARGB8888) {
        fprintf(stderr, "[extract_grid] ERROR: expected ARGB8888\n");
        return NULL;
    }

    if (SDL_MUSTLOCK(bin)) SDL_LockSurface(bin);

    // Detect skew
    double angle = detect_skew_angle(bin);

    // Rotate if needed
    SDL_Surface* deskewed = NULL;
    
    if (fabs(angle) > 0.5) {
        if (SDL_MUSTLOCK(bin)) SDL_UnlockSurface(bin);
        deskewed = rotate_surface(bin, angle);
        if (!deskewed) {
            fprintf(stderr, "[extract_grid] ERROR: rotation failed\n");
            return NULL;
        }
    } else {
        if (SDL_MUSTLOCK(bin)) SDL_UnlockSurface(bin);
        deskewed = SDL_CreateRGBSurfaceWithFormat(0, bin->w, bin->h, 32, SDL_PIXELFORMAT_ARGB8888);
        SDL_BlitSurface(bin, NULL, deskewed, NULL);
    }

    if (SDL_MUSTLOCK(deskewed)) SDL_LockSurface(deskewed);

    int W = deskewed->w;
    int H = deskewed->h;
    int* Hprof = (int*)calloc(H, sizeof(int));
    int* Vprof = (int*)calloc(W, sizeof(int));

    build_profile_rows(deskewed, Hprof);
    build_profile_cols(deskewed, Vprof);

    printf("[extract_grid] Processing image size: %dx%d\n", W, H);

    // TRY STRICT THRESHOLDS FIRST (for clean images like test1_1.jpg)
    int thr_row = (int)(0.5 * W);
    int thr_col = (int)(0.5 * H);

    int row_line_count = 0, col_line_count = 0;
    int* row_lines = find_line_positions(Hprof, H, thr_row, &row_line_count);
    int* col_lines = find_line_positions(Vprof, W, thr_col, &col_line_count);

    printf("[extract_grid] Strict (0.5): %d row lines, %d col lines\n", row_line_count, col_line_count);

    // FALLBACK: if too few lines, try relaxed thresholds with smoothing
    if (row_line_count < 8 || col_line_count < 8) {
        printf("[extract_grid] Insufficient lines, trying relaxed detection with smoothing\n");
        
        free(row_lines);
        free(col_lines);
        
        // Apply smoothing
        smooth_profile(Hprof, H);
        smooth_profile(Vprof, W);
        
        // Use relaxed thresholds
        thr_row = (int)(0.25 * W);
        thr_col = (int)(0.25 * H);
        
        row_lines = find_line_positions(Hprof, H, thr_row, &row_line_count);
        col_lines = find_line_positions(Vprof, W, thr_col, &col_line_count);
        
        printf("[extract_grid] Relaxed (0.25): %d row lines, %d col lines\n", row_line_count, col_line_count);
    }

    if (row_line_count < 2 || col_line_count < 2) {
        printf("[extract_grid] Insufficient grid lines\n");
        free(Hprof); free(Vprof);
        free(row_lines); free(col_lines);
        if (SDL_MUSTLOCK(deskewed)) SDL_UnlockSurface(deskewed);
        SDL_FreeSurface(deskewed);
        return NULL;
    }

    int avg_cell_height = median_gap(row_lines, row_line_count);
    int avg_cell_width  = median_gap(col_lines, col_line_count);

    printf("[extract_grid] Average cell size: %dx%d\n", avg_cell_width, avg_cell_height);

    int y0 = row_lines[0];
    int y1 = row_lines[row_line_count - 1];
    int x0 = col_lines[0];
    int x1 = col_lines[col_line_count - 1];

    printf("[extract_grid] Initial bounds: y=[%d,%d] x=[%d,%d]\n", y0, y1, x0, x1);

    // Validate using regularity
    int valid_x1 = x0;
    double tolerance = 0.25;

    for (int i = 1; i < col_line_count; i++) {
        int gap = col_lines[i] - col_lines[i - 1];
        double ratio = (double)gap / (double)avg_cell_width;

        if (ratio >= (1.0 - tolerance) && ratio <= (1.0 + tolerance)) {
            valid_x1 = col_lines[i];
        } else {
            printf("[extract_grid] Irregular gap at col %d: %d px (expected ~%d)\n",
                   i, gap, avg_cell_width);
            break;
        }
    }

    x1 = valid_x1;

    free(Hprof); free(Vprof);
    free(row_lines); free(col_lines);

    if (SDL_MUSTLOCK(deskewed)) SDL_UnlockSurface(deskewed);

    int gw = x1 - x0 + 1;
    int gh = y1 - y0 + 1;

    if (gw <= 0 || gh <= 0) {
        fprintf(stderr, "[extract_grid] ERROR: invalid dimensions\n");
        SDL_FreeSurface(deskewed);
        return NULL;
    }

    printf("[extract_grid] Final grid at (%d, %d) size %dx%d\n", x0, y0, gw, gh);

    SDL_Rect src_rect = { x0, y0, gw, gh };
    SDL_Surface* grid = SDL_CreateRGBSurfaceWithFormat(0, gw, gh, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!grid) {
        fprintf(stderr, "[extract_grid] ERROR: cannot create grid surface\n");
        SDL_FreeSurface(deskewed);
        return NULL;
    }

    SDL_Rect dst_rect = { 0, 0, 0, 0 };
    SDL_BlitSurface(deskewed, &src_rect, grid, &dst_rect);

    if (out_x) *out_x = x0;
    if (out_y) *out_y = y0;
    if (out_w) *out_w = gw;
    if (out_h) *out_h = gh;

    SDL_FreeSurface(deskewed);
    return grid;
}
