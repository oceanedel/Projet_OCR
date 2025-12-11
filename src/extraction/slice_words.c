#include "slice_words.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// --- Configuration ---
// Espace minimum de pixels vides pour considérer qu'un mot est fini.
// Garde "H E L L O" ensemble.
#define MIN_WORD_GAP 25 

// Marge blanche (en pixels) à ajouter autour de chaque mot extrait
#define PADDING 5

// --- Fonctions utilitaires ---

static inline bool is_black(uint32_t px) {
    uint8_t r = (px >> 16) & 0xFF;
    uint8_t g = (px >> 8) & 0xFF;
    uint8_t b = px & 0xFF;
    return (r < 200 && g < 200 && b < 200);
}

typedef struct {
    int y_start;
    int y_end;
} LineSegment;

// --- Détection des lignes (Axe Y) ---

static LineSegment* find_text_lines(SDL_Surface* img, int* out_count) {
    int W = img->w;
    int H = img->h;
    uint8_t* base = (uint8_t*)img->pixels;
    int pitch = img->pitch;

    int* proj_y = (int*)calloc(H, sizeof(int));
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            uint32_t px = ((uint32_t*)(base + y * pitch))[x];
            if (is_black(px)) {
                proj_y[y]++;
            }
        }
    }

    LineSegment* lines = (LineSegment*)malloc(sizeof(LineSegment) * 100);
    int count = 0;
    
    bool inside_line = false;
    int start_y = 0;
    int empty_gap = 0;
    int min_gap_tolerance = 1;

    for (int y = 0; y < H; y++) {
        bool has_pixels = (proj_y[y] > 0);

        if (has_pixels) {
            if (!inside_line) {
                start_y = y;
                inside_line = true;
            }
            empty_gap = 0;
        } else {
            if (inside_line) {
                empty_gap++;
                if (empty_gap > min_gap_tolerance) {
                    int end_y = y - empty_gap + 1;
                    if ((end_y - start_y) > 5) {
                        lines[count].y_start = start_y;
                        lines[count].y_end = end_y;
                        count++;
                        if (count >= 100) break;
                    }
                    inside_line = false;
                }
            }
        }
    }
    if (inside_line) {
        lines[count].y_start = start_y;
        lines[count].y_end = H;
        count++;
    }

    free(proj_y);
    *out_count = count;
    return lines;
}

// --- Extraction des mots (Axe X) avec PADDING ---

static int slice_row_into_words(SDL_Surface* img, LineSegment line, int word_index_start, const char* output_dir) {
    int W = img->w;
    uint8_t* base = (uint8_t*)img->pixels;
    int pitch = img->pitch;

    int* proj_x = (int*)calloc(W, sizeof(int));
    for (int x = 0; x < W; x++) {
        for (int y = line.y_start; y < line.y_end; y++) {
            uint32_t px = ((uint32_t*)(base + y * pitch))[x];
            if (is_black(px)) {
                proj_x[x]++;
            }
        }
    }

    int saved_count = 0;
    bool inside_word = false;
    int start_x = 0;
    int white_gap = 0;

    // Helper pour sauvegarder un mot avec marge
    void save_word(int sx, int ex) {
        int w = ex - sx;
        int h = line.y_end - line.y_start;
        
        if (w > 5) {
            // Rectangle source (l'image originale)
            SDL_Rect src = { sx, line.y_start, w, h };
            
            // Dimensions de la destination (taille originale + marge blanche autour)
            int final_w = w + (PADDING * 2);
            int final_h = h + (PADDING * 2);

            SDL_Surface* word_s = SDL_CreateRGBSurfaceWithFormat(0, final_w, final_h, 32, SDL_PIXELFORMAT_ARGB8888);
            
            // Remplir tout en blanc d'abord
            SDL_FillRect(word_s, NULL, SDL_MapRGB(word_s->format, 255, 255, 255));
            
            // Position où coller le mot (décalé par le padding)
            SDL_Rect dst_offset = { PADDING, PADDING, w, h };
            
            // Copier le mot au centre
            SDL_BlitSurface(img, &src, word_s, &dst_offset);
            
            char path[512];
            snprintf(path, sizeof(path), "%s/w_%02d.bmp", output_dir, word_index_start + saved_count);
            SDL_SaveBMP(word_s, path);
            SDL_FreeSurface(word_s);
            
            saved_count++;
        }
    }

    for (int x = 0; x < W; x++) {
        bool col_has_black = (proj_x[x] > 0);

        if (col_has_black) {
            if (!inside_word) {
                start_x = x;
                inside_word = true;
            }
            white_gap = 0;
        } else {
            if (inside_word) {
                white_gap++;
                if (white_gap > MIN_WORD_GAP) {
                    int end_x = x - white_gap + 1;
                    save_word(start_x, end_x);
                    inside_word = false;
                }
            }
        }
    }
    // Dernier mot
    if (inside_word) {
        save_word(start_x, W);
    }

    free(proj_x);
    return saved_count;
}

// --- Fonction Principale ---

int slice_words(SDL_Surface* wordlist, const char* output_dir) {
    if (!wordlist || !output_dir) return -1;
    
    printf("[slice_words] Processing list %dx%d (Padding: %dpx)...\n", wordlist->w, wordlist->h, PADDING);

    if (SDL_MUSTLOCK(wordlist)) SDL_LockSurface(wordlist);

    int line_count = 0;
    LineSegment* lines = find_text_lines(wordlist, &line_count);

    if (line_count == 0) {
        fprintf(stderr, "[slice_words] ✗ No text lines detected.\n");
        if (SDL_MUSTLOCK(wordlist)) SDL_UnlockSurface(wordlist);
        free(lines);
        return -1;
    }

    printf("[slice_words] Detected %d rows of text.\n", line_count);

    int total_words = 0;
    for (int i = 0; i < line_count; i++) {
        int added = slice_row_into_words(wordlist, lines[i], total_words, output_dir);
        total_words += added;
    }

    if (SDL_MUSTLOCK(wordlist)) SDL_UnlockSurface(wordlist);
    free(lines);

    printf("[slice_words] ✓ Extraction complete. %d words saved to %s\n", total_words, output_dir);
    
    return 0;
}
