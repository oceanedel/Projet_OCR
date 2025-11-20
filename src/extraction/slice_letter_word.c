#include "slice_letter_word.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

static inline bool is_black(uint32_t px) {
    return ((px & 0x00FFFFFF) == 0);
}

typedef struct {
    int x_start, x_end;
} LetterSegment;

// Find letter boundaries in a word image by detecting vertical gaps
static LetterSegment* segment_word(SDL_Surface* word_img, int* out_count) {
    int W = word_img->w;
    int H = word_img->h;
    
    uint8_t* base = (uint8_t*)word_img->pixels;
    int pitch = word_img->pitch;
    
    if (SDL_MUSTLOCK(word_img)) SDL_LockSurface(word_img);
    
    // Build vertical projection (count black pixels per column)
    int* col_black = (int*)calloc(W, sizeof(int));
    
    for (int x = 0; x < W; x++) {
        int black_count = 0;
        for (int y = 0; y < H; y++) {
            uint32_t* row = (uint32_t*)(base + y * pitch);
            if (is_black(row[x])) {
                black_count++;
            }
        }
        col_black[x] = black_count;
    }
    
    // Find letter segments
    LetterSegment* segments = (LetterSegment*)malloc(sizeof(LetterSegment) * 50);
    int segment_count = 0;
    
    bool in_letter = false;
    int letter_start = 0;
    int gap_width = 0;
    int min_gap_width = 1;  // Gap of 1+ white columns = new letter
    
    for (int x = 0; x < W; x++) {
        if (col_black[x] > 0) {
            // Found ink
            gap_width = 0;
            if (!in_letter) {
                letter_start = x;
                in_letter = true;
            }
        } 
        else {
            // White column
            gap_width++;
            
            if (in_letter && gap_width >= min_gap_width) {
                // End of letter (significant gap found)
                int letter_end = x - gap_width;
                int width = letter_end - letter_start;
                
                // Filter: letters must be at least 5px wide
                if (width >= 5) {
                    segments[segment_count].x_start = letter_start;
                    segments[segment_count].x_end = letter_end;
                    segment_count++;
                    
                    if (segment_count >= 50) break;
                }
                
                in_letter = false;
            }
        }
    }
    
    // Handle last letter if word ends with ink
    if (in_letter) {
        int width = W - letter_start;
        if (width >= 5) {
            segments[segment_count].x_start = letter_start;
            segments[segment_count].x_end = W;
            segment_count++;
        }
    }
    
    if (SDL_MUSTLOCK(word_img)) SDL_UnlockSurface(word_img);
    free(col_black);
    
    *out_count = segment_count;
    return segments;
}

// Count number of word images in directory
static int count_word_images(const char* words_dir) {
    int count = 0;
    
    // Count w_XX.bmp files
    for (int i = 0; i < 100; i++) {
        char path[512];
        snprintf(path, sizeof(path), "%s/w_%02d.bmp", words_dir, i);
        
        struct stat st;
        if (stat(path, &st) == 0) {
            count++;
        } else {
            break;
        }
    }
    
    return count;
}

// Main function to slice all words into letters
int slice_word_letters(const char* words_dir, const char* output_dir) {
    printf("\n[6/6] Slicing word letters...\n");
    printf("[WORD_LETTERS] Input: %s\n", words_dir);
    printf("[WORD_LETTERS] Output: %s\n", output_dir);
    
    // Create output directory
    struct stat st = {0};
    if (stat(output_dir, &st) == -1) {
        mkdir(output_dir, 0755);
    }
    
    // Count word images
    int word_count = count_word_images(words_dir);
    if (word_count == 0) {
        fprintf(stderr, "[WORD_LETTERS] No word images found in %s\n", words_dir);
        return -1;
    }
    
    printf("[WORD_LETTERS] Found %d word images\n", word_count);
    
    int total_letters = 0;
    
    // Process each word
    for (int w = 0; w < word_count; w++) {
        char word_path[512];
        snprintf(word_path, sizeof(word_path), "%s/w_%02d.bmp", words_dir, w);
        
        SDL_Surface* word_img = SDL_LoadBMP(word_path);
        if (!word_img) {
            fprintf(stderr, "[WORD_LETTERS] Failed to load: %s\n", word_path);
            continue;
        }
        
        // Segment word into letters
        int letter_count;
        LetterSegment* segments = segment_word(word_img, &letter_count);
        
        printf("[WORD_LETTERS] Word %02d: %d letters found\n", w, letter_count);
        
        // Extract each letter
        for (int l = 0; l < letter_count; l++) {
            int x = segments[l].x_start;
            int width = segments[l].x_end - segments[l].x_start;
            
            // Add small margin
            int margin = 0;
            x = (x > margin) ? x - margin : 0;
            width += 2 * margin;
            if (x + width > word_img->w) {
                width = word_img->w - x;
            }
            
            // Create letter surface
            SDL_Rect src = {x, 0, width, word_img->h};
            SDL_Surface* letter_surf = SDL_CreateRGBSurfaceWithFormat(
                0, width, word_img->h, 32, SDL_PIXELFORMAT_ARGB8888);
            
            SDL_Rect dst = {0, 0, 0, 0};
            SDL_BlitSurface(word_img, &src, letter_surf, &dst);
            
            // Save letter
            char letter_path[512];
            snprintf(letter_path, sizeof(letter_path), 
                     "%s/word_%02d_letter_%02d.bmp", output_dir, w, l);
            
            SDL_SaveBMP(letter_surf, letter_path);
            SDL_FreeSurface(letter_surf);
            
            total_letters++;
        }
        
        free(segments);
        SDL_FreeSurface(word_img);
    }
    
    printf("[WORD_LETTERS] ✓ Extracted %d letters from %d words\n", 
           total_letters, word_count);
    printf("[WORD_LETTERS] ✓ Letters saved to: %s\n", output_dir);
    
    return 0;
}
