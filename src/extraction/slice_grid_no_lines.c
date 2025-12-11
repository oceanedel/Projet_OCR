#include "slice_grid_no_lines.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

// --- Outils de comparaison pour le tri (nécessaire pour la médiane) ---
static int compare_ints(const void* a, const void* b) {
    return (*(int*)a - *(int*)b);
}

// --- Détection de pixel noir ---
static inline bool is_black(uint32_t px) {
    uint8_t r = (px >> 16) & 0xFF;
    uint8_t g = (px >> 8) & 0xFF;
    uint8_t b = px & 0xFF;
    // On est tolérant : tout ce qui n'est pas presque blanc est considéré comme de l'encre
    return (r < 200 || g < 200 || b < 200); 
}

// --- Construction des histogrammes ---
static void build_profile_rows(SDL_Surface* s, int* H) {
    uint8_t* base = (uint8_t*)s->pixels;
    for (int y = 0; y < s->h; y++) {
        int cnt = 0;
        for (int x = 0; x < s->w; x++) {
            uint32_t px = ((uint32_t*)(base + y * s->pitch))[x];
            if (is_black(px)) cnt++;
        }
        H[y] = cnt;
    }
}

static void build_profile_cols(SDL_Surface* s, int* V) {
    uint8_t* base = (uint8_t*)s->pixels;
    for (int x = 0; x < s->w; x++) {
        int cnt = 0;
        for (int y = 0; y < s->h; y++) {
            uint32_t px = ((uint32_t*)(base + y * s->pitch))[x];
            if (is_black(px)) cnt++;
        }
        V[x] = cnt;
    }
}

// --- Algorithme Intelligent de Découpe ---

// Cette structure stocke les limites d'une case potentielle
typedef struct {
    int start;
    int end;
} Range;

// Fonction principale de découpe d'un axe (X ou Y)
// 1. Trouve les zones de densité.
// 2. Calcule la médiane.
// 3. Découpe les zones trop larges.
static Range* get_smart_cuts(int* profile, int length, int threshold, int* out_count) {
    // Étape 1 : Trouver les segments "bruts" (là où il y a du noir)
    Range* raw_ranges = malloc(sizeof(Range) * length); // Allocation large
    int raw_count = 0;
    
    int start = -1;
    for (int i = 0; i < length; i++) {
        if (profile[i] > threshold) {
            if (start == -1) start = i;
        } else {
            if (start != -1) {
                // Fin d'un segment
                // On ignore les bruits minuscules (< 2 pixels)
                if ((i - start) > 2) {
                    raw_ranges[raw_count].start = start;
                    raw_ranges[raw_count].end = i;
                    raw_count++;
                }
                start = -1;
            }
        }
    }
    if (start != -1) {
        raw_ranges[raw_count].start = start;
        raw_ranges[raw_count].end = length;
        raw_count++;
    }

    if (raw_count == 0) {
        free(raw_ranges);
        *out_count = 0;
        return NULL;
    }

    // Étape 2 : Calculer la taille médiane d'un segment (une lettre)
    int* sizes = malloc(sizeof(int) * raw_count);
    for (int i = 0; i < raw_count; i++) {
        sizes[i] = raw_ranges[i].end - raw_ranges[i].start;
    }
    qsort(sizes, raw_count, sizeof(int), compare_ints);
    int median_size = sizes[raw_count / 2];
    free(sizes);

    printf("   [SmartCut] Median size detected: %d px\n", median_size);

    // Étape 3 : Raffiner les découpes
    // Si un segment est ~2x la médiane, c'est que deux lettres se touchent. On coupe au milieu.
    Range* final_ranges = malloc(sizeof(Range) * 200); // Max 200 cases/lignes
    int final_count = 0;

    // Tolérance pour dire "c'est une seule lettre" (ex: 1.5x la médiane max)
    double split_threshold = median_size * 1.6; 

    for (int i = 0; i < raw_count; i++) {
        int w = raw_ranges[i].end - raw_ranges[i].start;
        
        if (w > split_threshold) {
            // Le bloc est trop gros ! Il faut le découper.
            // On estime combien de lettres sont collées (arrondi à l'entier le plus proche)
            int num_items = (int)round((double)w / (double)median_size);
            if (num_items < 2) num_items = 2; // Sécurité

            double step = (double)w / num_items;
            
            // On crée les sous-segments
            for (int k = 0; k < num_items; k++) {
                int s = raw_ranges[i].start + (int)(k * step);
                int e = raw_ranges[i].start + (int)((k + 1) * step);
                
                // Petit ajustement pour laisser un pixel de marge si possible
                if (k > 0) s++; 
                if (k < num_items - 1) e--;

                final_ranges[final_count].start = s;
                final_ranges[final_count].end = e;
                final_count++;
            }
        } else {
            // Taille normale, on garde tel quel
            final_ranges[final_count++] = raw_ranges[i];
        }
    }

    free(raw_ranges);
    *out_count = final_count;
    return final_ranges;
}


int slice_grid_no_lines(SDL_Surface* grid, const char* output_dir) {
    if (!grid || !output_dir) return -1;

    if (SDL_MUSTLOCK(grid)) SDL_LockSurface(grid);

    int W = grid->w;
    int H = grid->h;
    
    // 1. Profils
    int* Hprof = (int*)calloc(H, sizeof(int));
    int* Vprof = (int*)calloc(W, sizeof(int));
    build_profile_rows(grid, Hprof);
    build_profile_cols(grid, Vprof);

    // 2. Découpe intelligente
    // Seuil à 0 ou 1 : permet d'ignorer le micro bruit blanc, mais attrape le moindre bout de lettre
    int nb_rows = 0;
    int nb_cols = 0;
    
    printf("[No-Line] Analyzing Rows...\n");
    Range* rows = get_smart_cuts(Hprof, H, 0, &nb_rows);
    
    printf("[No-Line] Analyzing Cols...\n");
    Range* cols = get_smart_cuts(Vprof, W, 0, &nb_cols);

    free(Hprof);
    free(Vprof);
    if (SDL_MUSTLOCK(grid)) SDL_UnlockSurface(grid);

    if (nb_rows == 0 || nb_cols == 0) {
        fprintf(stderr, "[No-Line] Error: Could not define grid structure.\n");
        if(rows) free(rows);
        if(cols) free(cols);
        return -1;
    }

    printf("[No-Line] Structure detected: %d rows x %d cols\n", nb_rows, nb_cols);

    // 3. Extraction
    char path[512];
    int saved = 0;
    
    // Marge de sécurité (padding) autour de la lettre découpée
    // Pour éviter de couper un bout de pixel au bord
    int pad = 1; 

    for (int r = 0; r < nb_rows; r++) {
        for (int c = 0; c < nb_cols; c++) {
            
            // Calculer coordonnées
            int y0 = rows[r].start - pad;
            int y1 = rows[r].end + pad;
            int x0 = cols[c].start - pad;
            int x1 = cols[c].end + pad;

            // Clamp (rester dans l'image)
            if (y0 < 0) y0 = 0;
            if (y1 > H) y1 = H;
            if (x0 < 0) x0 = 0;
            if (x1 > W) x1 = W;

            int w = x1 - x0;
            int h = y1 - y0;

            if (w <= 0 || h <= 0) continue;

            SDL_Rect src = { x0, y0, w, h };
            SDL_Surface* cell = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
            
            // Remplir de blanc (important si le padding dépasse la zone noire)
            SDL_FillRect(cell, NULL, SDL_MapRGB(cell->format, 255, 255, 255));
            SDL_BlitSurface(grid, &src, cell, NULL);

            snprintf(path, sizeof(path), "%s/c_%02d_%02d.bmp", output_dir, r, c);
            SDL_SaveBMP(cell, path);
            SDL_FreeSurface(cell);
            saved++;
        }
    }

    free(rows);
    free(cols);
    printf("[No-Line] Saved %d cells.\n", saved);
    return 0;
}
