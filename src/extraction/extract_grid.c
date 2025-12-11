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

// --- Fonctions de base ---

static inline bool is_black(uint32_t px) {
    uint8_t r = (px >> 16) & 0xFF;
    uint8_t g = (px >> 8) & 0xFF;
    uint8_t b = px & 0xFF;
    // Seuil de noir assez large pour capter les gris foncés
    return (r < 100 && g < 100 && b < 100);
}

static inline uint32_t get_pixel(SDL_Surface* s, int x, int y) {
    if (x < 0 || y < 0 || x >= s->w || y >= s->h) return 0xFFFFFFFF;
    return ((uint32_t*)((uint8_t*)s->pixels + y * s->pitch))[x];
}

static inline void put_pixel(SDL_Surface* s, int x, int y, uint32_t px) {
    if (x < 0 || y < 0 || x >= s->w || y >= s->h) return;
    ((uint32_t*)((uint8_t*)s->pixels + y * s->pitch))[x] = px;
}

// --- Rotation (Idem) ---

static double projection_variance_at_angle(SDL_Surface* bin, double angle_deg) {
    int step = 8; // On scanne moins de pixels pour aller plus vite
    int dsW = bin->w / step;
    int dsH = bin->h / step;
    if (dsW <= 0 || dsH <= 0) return 0.0;

    double rad = angle_deg * M_PI / 180.0;
    double cs = cos(rad);
    double sn = sin(rad);

    int* prof = (int*)calloc(dsH, sizeof(int));
    double cx = dsW / 2.0, cy = dsH / 2.0;

    for (int y = 0; y < dsH; y++) {
        for (int x = 0; x < dsW; x++) {
            double rx = (x - cx) * cs - (y - cy) * sn + cx;
            double ry = (x - cx) * sn + (y - cy) * cs + cy;
            int ix = (int)(rx * step + 0.5);
            int iy = (int)(ry * step + 0.5);
            if (ix >= 0 && iy >= 0 && ix < bin->w && iy < bin->h) {
                if (is_black(get_pixel(bin, ix, iy))) prof[y]++;
            }
        }
    }
    double mean = 0.0, var = 0.0;
    for (int i = 0; i < dsH; i++) mean += prof[i];
    mean /= (double)dsH;
    for (int i = 0; i < dsH; i++) var += (prof[i] - mean) * (prof[i] - mean);
    var /= (double)dsH;
    free(prof);
    return var;
}

static double detect_skew_angle(SDL_Surface* bin) {
    double best_angle = 0.0;
    double best_var = projection_variance_at_angle(bin, 0.0);
    
    // Recherche restreinte (-5 à +5 degrés suffisent souvent)
    for (double a = -5.0; a <= 5.0; a += 1.0) {
        if (a == 0.0) continue;
        double v = projection_variance_at_angle(bin, a);
        if (v > best_var) { best_var = v; best_angle = a; }
    }
    // Raffinement
    double center = best_angle;
    for (double a = center - 0.5; a <= center + 0.5; a += 0.1) {
        double v = projection_variance_at_angle(bin, a);
        if (v > best_var) { best_var = v; best_angle = a; }
    }
    printf("[extract_grid] Detected Skew: %.2f degrees\n", best_angle);
    return best_angle;
}

static SDL_Surface* rotate_surface(SDL_Surface* src, double angle_deg) {
    if (fabs(angle_deg) < 0.1) {
        SDL_Surface* dup = SDL_CreateRGBSurfaceWithFormat(0, src->w, src->h, 32, SDL_PIXELFORMAT_ARGB8888);
        SDL_BlitSurface(src, NULL, dup, NULL);
        return dup;
    }
    double rad = -angle_deg * M_PI / 180.0;
    double cs = cos(rad);
    double sn = sin(rad);
    int new_w = (int)(fabs(src->w * cs) + fabs(src->h * sn));
    int new_h = (int)(fabs(src->w * sn) + fabs(src->h * cs));
    
    SDL_Surface* dst = SDL_CreateRGBSurfaceWithFormat(0, new_w, new_h, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_FillRect(dst, NULL, 0xFFFFFFFF);

    if (SDL_MUSTLOCK(src)) SDL_LockSurface(src);
    if (SDL_MUSTLOCK(dst)) SDL_LockSurface(dst);

    double cx_src = src->w / 2.0, cy_src = src->h / 2.0;
    double cx_dst = new_w / 2.0, cy_dst = new_h / 2.0;

    for (int y = 0; y < new_h; y++) {
        for (int x = 0; x < new_w; x++) {
            double dx = x - cx_dst;
            double dy = y - cy_dst;
            double src_x = dx * cs + dy * sn + cx_src;
            double src_y = -dx * sn + dy * cs + cy_src;
            if (src_x >= 0 && src_x < src->w && src_y >= 0 && src_y < src->h) {
                put_pixel(dst, x, y, get_pixel(src, (int)src_x, (int)src_y));
            }
        }
    }
    if (SDL_MUSTLOCK(src)) SDL_UnlockSurface(src);
    if (SDL_MUSTLOCK(dst)) SDL_UnlockSurface(dst);
    return dst;
}

// --- NOUVELLE LOGIQUE : Smearing (Dilatation) ---

// Applique une "bavure" horizontale et verticale sur une copie de travail
// Cela permet de connecter les lettres entre elles pour former un gros bloc,
// sans abîmer l'image originale.
static SDL_Surface* create_smeared_map(SDL_Surface* src, int radius_x, int radius_y) {
    SDL_Surface* temp = SDL_CreateRGBSurfaceWithFormat(0, src->w, src->h, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_FillRect(temp, NULL, 0xFFFFFFFF);
    
    if (SDL_MUSTLOCK(src)) SDL_LockSurface(src);
    if (SDL_MUSTLOCK(temp)) SDL_LockSurface(temp);

    uint32_t black = SDL_MapRGB(temp->format, 0, 0, 0);

    // 1. Smear Horizontal
    for (int y = 0; y < src->h; y++) {
        int run = 0;
        for (int x = 0; x < src->w; x++) {
            if (is_black(get_pixel(src, x, y))) {
                run = radius_x; // On recharge le "crayon"
            }
            if (run > 0) {
                put_pixel(temp, x, y, black);
                run--;
            }
        }
        // Scan inverse (droite vers gauche) pour boucher les trous
        run = 0;
        for (int x = src->w - 1; x >= 0; x--) {
            if (is_black(get_pixel(src, x, y))) run = radius_x;
            if (run > 0) { put_pixel(temp, x, y, black); run--; }
        }
    }

    // 2. Smear Vertical (sur le résultat horizontal)
    // On fait une copie temporaire pour ne pas lire ce qu'on vient d'écrire
    SDL_Surface* res = SDL_CreateRGBSurfaceWithFormat(0, src->w, src->h, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_FillRect(res, NULL, 0xFFFFFFFF);
    if (SDL_MUSTLOCK(res)) SDL_LockSurface(res);

    for (int x = 0; x < src->w; x++) {
        int run = 0;
        for (int y = 0; y < src->h; y++) {
            if (is_black(get_pixel(temp, x, y))) run = radius_y;
            if (run > 0) { put_pixel(res, x, y, black); run--; }
        }
        run = 0;
        for (int y = src->h - 1; y >= 0; y--) {
            if (is_black(get_pixel(temp, x, y))) run = radius_y;
            if (run > 0) { put_pixel(res, x, y, black); run--; }
        }
    }

    if (SDL_MUSTLOCK(src)) SDL_UnlockSurface(src);
    if (SDL_MUSTLOCK(temp)) SDL_UnlockSurface(temp);
    if (SDL_MUSTLOCK(res)) SDL_UnlockSurface(res);
    
    SDL_FreeSurface(temp);
    return res;
}

// Analyse le profil d'une image "smeared" pour trouver le plus grand bloc
static void find_largest_block_range(int* prof, int N, int* start, int* end) {
    int max_len = 0;
    int cur_len = 0;
    int cur_start = -1;
    int best_start = 0;
    int best_end = N;

    // Seuil de détection: sur l'image smeared, c'est presque tout noir (haute densité)
    // Mais on prend > 0 pour attraper tout le bloc.
    for (int i = 0; i < N; i++) {
        if (prof[i] > 0) {
            if (cur_start == -1) cur_start = i;
            cur_len++;
        } else {
            if (cur_start != -1) {
                // Fin d'un bloc
                if (cur_len > max_len) {
                    max_len = cur_len;
                    best_start = cur_start;
                    best_end = i;
                }
                cur_start = -1;
                cur_len = 0;
            }
        }
    }
    // Check fin de tableau
    if (cur_start != -1 && cur_len > max_len) {
        max_len = cur_len;
        best_start = cur_start;
        best_end = N;
    }

    *start = best_start;
    *end = best_end;
}


SDL_Surface* extract_grid(SDL_Surface* bin, int* out_x, int* out_y, int* out_w, int* out_h) {
    if (!bin) return NULL;

    // 1. Rotation
    double angle = detect_skew_angle(bin);
    SDL_Surface* work;
    if (fabs(angle) > 0.5) work = rotate_surface(bin, angle);
    else {
        work = SDL_CreateRGBSurfaceWithFormat(0, bin->w, bin->h, 32, SDL_PIXELFORMAT_ARGB8888);
        SDL_BlitSurface(bin, NULL, work, NULL);
    }

    // 2. Création de la "Smeared Map"
    // On dilate fortement (ex: 20px) pour relier les lettres et le cadre
    // Cela transforme la grille en un gros rectangle noir solide
    // Les oiseaux restent des "taches" séparées si ils sont un peu éloignés
    printf("[extract_grid] Generating smeared map for detection...\n");
    int smear_radius = (work->w / 50); // environ 2% de la largeur
    if (smear_radius < 5) smear_radius = 5;
    
    SDL_Surface* smeared = create_smeared_map(work, smear_radius, smear_radius);
    
    // Décommenter pour debug : 
    // SDL_SaveBMP(smeared, "debug_smeared.bmp");

    // 3. Projection Verticale (X) sur la map smeared
    int* Vprof = (int*)calloc(work->w, sizeof(int));
    if (SDL_MUSTLOCK(smeared)) SDL_LockSurface(smeared);
    
    for (int x = 0; x < smeared->w; x++) {
        for (int y = 0; y < smeared->h; y++) {
            if (is_black(get_pixel(smeared, x, y))) Vprof[x]++;
        }
    }

    // Trouver la zone X la plus large (la grille est généralement le plus gros objet)
    int gx0, gx1;
    find_largest_block_range(Vprof, smeared->w, &gx0, &gx1);
    free(Vprof);
    
    printf("[extract_grid] Found X range: %d -> %d\n", gx0, gx1);

    // 4. Projection Horizontale (Y) MAIS restreinte à la zone X trouvée
    int* Hprof = (int*)calloc(work->h, sizeof(int));
    for (int y = 0; y < smeared->h; y++) {
        // On ne regarde que dans la colonne identifiée
        for (int x = gx0; x < gx1; x++) {
            if (is_black(get_pixel(smeared, x, y))) Hprof[y]++;
        }
    }
    if (SDL_MUSTLOCK(smeared)) SDL_UnlockSurface(smeared);

    // Trouver la zone Y la plus haute dans cette colonne
    int gy0, gy1;
    find_largest_block_range(Hprof, smeared->h, &gy0, &gy1);
    free(Hprof);

    printf("[extract_grid] Found Y range: %d -> %d\n", gy0, gy1);

    SDL_FreeSurface(smeared); // On n'a plus besoin de la map moche

    // 5. Raffinement et Crop
    // On a maintenant les coordonnées brutes du "plus gros bloc"
    // C'est souvent le cadre noir extérieur.
    
    // Si on veut supprimer le cadre noir lui-même pour ne garder que l'intérieur,
    // on peut scanner vers l'intérieur depuis les bords trouvés.
    // Pour l'instant, on prend le bloc tel quel, le découpage des cases gèrera le reste.
    
    // Petit padding de sécurité inverse (on rentre un peu dedans pour virer le flou du smear)
    int pad = 5;
    gx0 += pad; gy0 += pad;
    gx1 -= pad; gy1 -= pad;

    int w = gx1 - gx0;
    int h = gy1 - gy0;

    if (w < 50 || h < 50) {
        fprintf(stderr, "[extract_grid] ✗ Failed to detect valid grid block.\n");
        SDL_FreeSurface(work);
        return NULL;
    }

    SDL_Rect src_rect = { gx0, gy0, w, h };
    SDL_Surface* grid = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_FillRect(grid, NULL, 0xFFFFFFFF);
    SDL_BlitSurface(work, &src_rect, grid, NULL);

    if (out_x) *out_x = gx0;
    if (out_y) *out_y = gy0;
    if (out_w) *out_w = w;
    if (out_h) *out_h = h;

    SDL_FreeSurface(work);
    printf("[extract_grid] ✓ Grid extracted successfully (%dx%d)\n", w, h);
    return grid;
}
