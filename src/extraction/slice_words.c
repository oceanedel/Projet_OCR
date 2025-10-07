#include "slice_words.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

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

int slice_words(SDL_Surface* wl, const char* output_dir) {
    if (!wl || !output_dir) return -1;

    if (SDL_MUSTLOCK(wl)) SDL_LockSurface(wl);

    int H = wl->h;
    int* Hprof = (int*)calloc(H, sizeof(int));
    build_profile_rows(wl, Hprof);

    // Find text lines: threshold for ink presence
    int thr = (int)(0.05 * wl->w); // At least 5% width has ink

    // Detect line bands
    typedef struct { int y0, y1; } Band;
    Band* lines = (Band*)malloc(sizeof(Band) * 64);
    int line_count = 0, cap = 64;

    int in_line = 0, y0 = 0;
    for (int y = 0; y < H; y++) {
        if (Hprof[y] >= thr) {
            if (!in_line) { y0 = y; in_line = 1; }
        } else {
            if (in_line) {
                if (line_count >= cap) {
                    cap *= 2;
                    lines = (Band*)realloc(lines, sizeof(Band) * cap);
                }
                lines[line_count++] = (Band){ y0, y - 1 };
                in_line = 0;
            }
        }
    }
    if (in_line) lines[line_count++] = (Band){ y0, H - 1 };

    free(Hprof);
    if (SDL_MUSTLOCK(wl)) SDL_UnlockSurface(wl);

    printf("[slice_words] Detected %d word lines\n", line_count);

    char path[512];
    for (int i = 0; i < line_count; i++) {
        int h = lines[i].y1 - lines[i].y0 + 1;
        if (h < 5) continue; // Skip noise

        SDL_Rect rc = { 0, lines[i].y0, wl->w, h };
        SDL_Surface* word = SDL_CreateRGBSurfaceWithFormat(0, wl->w, h, 32, SDL_PIXELFORMAT_ARGB8888);
        SDL_Rect dst = { 0, 0, 0, 0 };
        SDL_BlitSurface(wl, &rc, word, &dst);

        snprintf(path, sizeof(path), "%s/w_%02d.bmp", output_dir, i);
        SDL_SaveBMP(word, path);
        SDL_FreeSurface(word);
    }

    free(lines);
    printf("[slice_words] Saved %d words to %s\n", line_count, output_dir);
    return 0;
}
