// src/grid.c  (a.k.a. m2_grid.c)
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "grid.h"

static inline bool is_black(uint32_t px){
    // ARGB8888: 0xAARRGGBB with ink as black from M1
    return ((px & 0x00FFFFFFu) == 0u);
}

static int* malloc_int(int n){
    int* p = (int*)malloc(sizeof(int)*n);
    if(!p){ fprintf(stderr,"alloc fail\n"); exit(1); }
    return p;
}

static void find_bands_int(const int* prof, int N, int thr, int** outBands, int* outCount){
    // bands as pairs [start,end] inclusive, packed as s0,e0,s1,e1...
    int cap = 16, cnt = 0;
    int* bands = (int*)malloc(sizeof(int)*cap*2);
    int i = 0;
    while(i < N){
        if(prof[i] >= thr){
            int s = i;
            while(i < N && prof[i] >= thr) i++;
            int e = i-1;
            if(cnt >= cap){
                cap *= 2;
                bands = (int*)realloc(bands, sizeof(int)*cap*2);
            }
            bands[2*cnt+0] = s;
            bands[2*cnt+1] = e;
            cnt++;
        }else{
            i++;
        }
    }
    *outBands = bands;
    *outCount = cnt;
}

static void bands_to_centers(const int* bands, int nBands, int** outCenters, int* outN){
    int* centers = malloc_int(nBands);
    for(int i=0;i<nBands;i++){
        int s = bands[2*i+0], e = bands[2*i+1];
        centers[i] = (s + e) / 2;
    }
    *outCenters = centers;
    *outN = nBands;
}

static double gaps_cv(const int* centers, int n){
    if(n < 2) return 1.0;
    int m = n-1;
    double sum=0;
    for(int i=0;i<m;i++) sum += (double)(centers[i+1] - centers[i]);
    double mean = sum / m;
    double var=0;
    for(int i=0;i<m;i++){
        double d = (centers[i+1] - centers[i]) - mean;
        var += d*d;
    }
    var /= m>1? m-1:1;
    double std = sqrt(var);
    return (mean>0)? std/mean : 1.0;
}

static void build_profile_rows(SDL_Surface* s, int* H){
    uint8_t* base = (uint8_t*)s->pixels;
    int pitch = s->pitch;
    for(int y=0;y<s->h;y++){
        uint32_t* row = (uint32_t*)(base + y*pitch);
        int cnt=0;
        for(int x=0;x<s->w;x++){
            if(is_black(row[x])) cnt++;
        }
        H[y]=cnt;
    }
}

static void build_profile_cols(SDL_Surface* s, int* V){
    uint8_t* base = (uint8_t*)s->pixels;
    int pitch = s->pitch;
    for(int x=0;x<s->w;x++){
        int cnt=0;
        for(int y=0;y<s->h;y++){
            uint32_t* row = (uint32_t*)(base + y*pitch);
            if(is_black(row[x])) cnt++;
        }
        V[x]=cnt;
    }
}

static int auto_threshold_bands(const int* prof, int N, int baseThr, int wantMin, int wantMax){
    // Nudge threshold to get bands in [wantMin, wantMax], step by ~3% of range.
    int thr = baseThr;
    for(int iter=0; iter<20; iter++){
        int *bands=NULL; int nb=0;
        find_bands_int(prof, N, thr, &bands, &nb);
        free(bands);
        if(nb < wantMin){ thr = (int)(thr*0.97); }      // too few bands -> lower thr
        else if(nb > wantMax){ thr = (int)(thr*1.03); } // too many -> raise thr
        else return thr;
        if(thr < 1) return 1;
        if(thr > (N*2)) return thr;
    }
    return thr;
}

int detect_grid_lines(SDL_Surface* bin, GridLines* out, float row_alpha, float col_alpha){
    if(!bin || !out) return -1;
    if(SDL_MUSTLOCK(bin)) SDL_LockSurface(bin);

    int W=bin->w, H=bin->h;
    int* Hprof = malloc_int(H);
    int* Vprof = malloc_int(W);

    build_profile_rows(bin, Hprof);   // sum black per row
    build_profile_cols(bin, Vprof);   // sum black per col

    // Initial thresholds proportional to dimension
    int thr_row = (int)(row_alpha * W);
    int thr_col = (int)(col_alpha * H);

    // Expect at least 2 bands (top/bottom or left/right). Upper bound: many bands but less than H/3, W/3.
    thr_row = auto_threshold_bands(Hprof, H, thr_row, 2, H/3);
    thr_col = auto_threshold_bands(Vprof, W, thr_col, 2, W/3);

    // Extract bands and collapse to centers
    int *rbands=NULL,*cbands=NULL; int nr=0,nc=0;
    find_bands_int(Hprof, H, thr_row, &rbands, &nr);
    find_bands_int(Vprof, W, thr_col, &cbands, &nc);

    int *rcent=NULL,*ccent=NULL; int nrl=0,ncl=0;
    bands_to_centers(rbands, nr, &rcent, &nrl);
    bands_to_centers(cbands, nc, &ccent, &ncl);
    free(rbands); free(cbands);

    // Basic sanity
    if(nrl < 2 || ncl < 2){
        fprintf(stderr,"grid: not enough divider lines (rows=%d, cols=%d)\n", nrl, ncl);
        free(Hprof); free(Vprof);
        free(rcent); free(ccent);
        if(SDL_MUSTLOCK(bin)) SDL_UnlockSurface(bin);
        return -2;
    }

    // Optionally sort centers (robust to scanning order)
    // but they already come in ascending order from scan.

    // Validate spacing regularity; warn if high CV
    double cvR = gaps_cv(rcent, nrl);
    double cvC = gaps_cv(ccent, ncl);
    if(cvR > 0.25 || cvC > 0.25){
        fprintf(stderr,"grid: irregular spacing (CV rows=%.2f cols=%.2f)\n", cvR, cvC);
    }

    out->rows = rcent; out->n_rows = nrl;
    out->cols = ccent; out->n_cols = ncl;

    free(Hprof); free(Vprof);
    if(SDL_MUSTLOCK(bin)) SDL_UnlockSurface(bin);
    return 0;
}

int build_cells(const GridLines* gl, CellMap* out, int trim){
    if(!gl || !out) return -1;
    if(gl->n_rows < 2 || gl->n_cols < 2) return -2;
    int R = gl->n_rows - 1;
    int C = gl->n_cols - 1;
    SDL_Rect **cells = (SDL_Rect**)malloc(sizeof(SDL_Rect*)*R);
    for(int r=0;r<R;r++){
        cells[r] = (SDL_Rect*)malloc(sizeof(SDL_Rect)*C);
        for(int c=0;c<C;c++){
            int x0 = gl->cols[c]   + trim;
            int x1 = gl->cols[c+1] - trim;
            int y0 = gl->rows[r]   + trim;
            int y1 = gl->rows[r+1] - trim;
            if(x1 < x0) x1 = x0;
            if(y1 < y0) y1 = y0;
            cells[r][c].x = x0;
            cells[r][c].y = y0;
            cells[r][c].w = x1 - x0 + 1;
            cells[r][c].h = y1 - y0 + 1;
        }
    }
    out->R = R; out->C = C; out->cells = cells;
    return 0;
}

int export_cells(SDL_Surface* src, const CellMap* cm, const char* folder){
    if(!src || !cm || !folder) return -1;
    char path[256];
    for(int r=0;r<cm->R;r++){
        for(int c=0;c<cm->C;c++){
            SDL_Rect rc = cm->cells[r][c];
            SDL_Surface* dst = SDL_CreateRGBSurfaceWithFormat(0, rc.w, rc.h, 32, SDL_PIXELFORMAT_ARGB8888);
            if(!dst){ fprintf(stderr,"create dst fail\n"); return -2; }
            SDL_Rect dstpos = {0,0,0,0};
            if(SDL_BlitSurface(src, &rc, dst, &dstpos) != 0){
                fprintf(stderr,"blit fail: %s\n", SDL_GetError());
                SDL_FreeSurface(dst);
                return -3;
            }
            snprintf(path, sizeof(path), "%s/r%02d_c%02d.bmp", folder, r, c);
            if(SDL_SaveBMP(dst, path) != 0){
                fprintf(stderr,"save fail: %s\n", SDL_GetError());
                SDL_FreeSurface(dst);
                return -4;
            }
            SDL_FreeSurface(dst);
        }
    }
    return 0;
}

void free_grid_lines(GridLines* gl){
    if(!gl) return;
    free(gl->rows); free(gl->cols);
    gl->rows=NULL; gl->cols=NULL; gl->n_rows=gl->n_cols=0;
}

void free_cell_map(CellMap* cm){
    if(!cm) return;
    if(cm->cells){
        for(int r=0;r<cm->R;r++) free(cm->cells[r]);
        free(cm->cells);
    }
    cm->cells=NULL; cm->R=cm->C=0;
}
