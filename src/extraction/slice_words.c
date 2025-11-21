#include "slice_words.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static inline bool is_black(uint32_t px) {
    return ((px & 0x00FFFFFF) == 0);
}

typedef struct {
    int y_start, y_end;
} TextRow;

typedef struct {
    int x, y, w, h;
} Word;

// Forward declaration
static TextRow* split_large_block(int* row_black, int start_y, int end_y, int threshold, int* out_count);

// Recursive splitting function for large blocks
static TextRow* split_large_block(int* row_black, int start_y, int end_y, int threshold, int* out_count) {
    int height = end_y - start_y;
    
    TextRow* rows = (TextRow*)malloc(sizeof(TextRow) * 50);
    int row_count = 0;
    
    printf("[slice_words] Splitting block y=%d-%d (height=%d, threshold=%d)\n", 
           start_y, end_y, height, threshold);
    
    bool in_text = false;
    int row_start = start_y;
    int gap_height = 0;
    
    for (int y = start_y; y < end_y; y++) {
        if (row_black[y] <= threshold) {
            gap_height++;
            
            if (in_text && gap_height >= 1) {
                int row_end = y;
                int h = row_end - row_start;
                
                if (h >= 8 && h <= 30) {
                    // Good size - accept
                    rows[row_count++] = (TextRow){ row_start, row_end };
                    printf("[slice_words]   ✓ Sub-row: y=%d-%d (h=%d)\n", row_start, row_end, h);
                } else if (h > 30 && threshold < 150) {
                    // Still too large - split recursively with higher threshold
                    printf("[slice_words]   ↻ Re-splitting y=%d-%d (h=%d)\n", row_start, row_end, h);
                    int sub_count;
                    TextRow* sub_rows = split_large_block(row_black, row_start, row_end, threshold + 30, &sub_count);
                    
                    for (int i = 0; i < sub_count && row_count < 50; i++) {
                        rows[row_count++] = sub_rows[i];
                    }
                    free(sub_rows);
                } else if (h < 8) {
                    printf("[slice_words]   ✗ Sub-row: y=%d-%d (h=%d, too small)\n", row_start, row_end, h);
                } else {
                    printf("[slice_words]   ✗ Sub-row: y=%d-%d (h=%d, too large, skipping)\n", row_start, row_end, h);
                }
                in_text = false;
            }
        } else {
            gap_height = 0;
            if (!in_text) {
                row_start = y;
                in_text = true;
            }
        }
    }
    
    // Handle last sub-block
    if (in_text) {
        int h = end_y - row_start;
        if (h >= 8 && h <= 30) {
            rows[row_count++] = (TextRow){ row_start, end_y };
            printf("[slice_words]   ✓ Sub-row: y=%d-%d (h=%d)\n", row_start, end_y, h);
        } else if (h > 30 && threshold < 150) {
            printf("[slice_words]   ↻ Re-splitting y=%d-%d (h=%d)\n", row_start, end_y, h);
            int sub_count;
            TextRow* sub_rows = split_large_block(row_black, row_start, end_y, threshold + 30, &sub_count);
            
            for (int i = 0; i < sub_count && row_count < 50; i++) {
                rows[row_count++] = sub_rows[i];
            }
            free(sub_rows);
        } else if (h < 8) {
            printf("[slice_words]   ✗ Final sub-row: y=%d-%d (h=%d, too small)\n", row_start, end_y, h);
        } else {
            printf("[slice_words]   ✗ Final sub-row: y=%d-%d (h=%d, too large, skipping)\n", row_start, end_y, h);
        }
    }
    
    *out_count = row_count;
    return rows;
}

// Main row detection function
static TextRow* find_text_rows(SDL_Surface* img, int* out_count) {
    int W = img->w, H = img->h;
    uint8_t* base = (uint8_t*)img->pixels;
    int pitch = img->pitch;
    
    if (SDL_MUSTLOCK(img)) SDL_LockSurface(img);
    
    int* row_black = (int*)calloc(H, sizeof(int));
    
    for (int y = 0; y < H; y++) {
        uint32_t* row = (uint32_t*)(base + y * pitch);
        int black_count = 0;
        for (int x = 0; x < W; x++) {
            if (is_black(row[x])) black_count++;
        }
        row_black[y] = black_count;
    }
    
    TextRow* rows = (TextRow*)malloc(sizeof(TextRow) * 50);
    int row_count = 0;
    
    bool in_text = false;
    int row_start = 0;
    int gap_height = 0;
    int min_gap_height = 2;
    
    for (int y = 0; y < H; y++) {
        if (row_black[y] <= 2) {
            gap_height++;
            
            if (in_text && gap_height >= min_gap_height) {
                int row_end = y - gap_height + 1;
                int height = row_end - row_start;
                
                if (height >= 8 && height <= 30) {
                    // Normal text row
                    rows[row_count++] = (TextRow){ row_start, row_end };
                    if (row_count >= 50) break;
                } else if (height > 30) {
                    // Large block - split recursively
                    int sub_count;
                    TextRow* sub_rows = split_large_block(row_black, row_start, row_end, 40, &sub_count);
                    
                    for (int i = 0; i < sub_count && row_count < 50; i++) {
                        rows[row_count++] = sub_rows[i];
                    }
                    free(sub_rows);
                }
                in_text = false;
            }
        } else {
            gap_height = 0;
            if (!in_text) {
                row_start = y;
                in_text = true;
            }
        }
    }
    
    // Handle final block
    if (in_text) {
        int height = H - row_start;
        if (height >= 8 && height <= 30) {
            rows[row_count++] = (TextRow){ row_start, H };
        } else if (height > 30) {
            int sub_count;
            TextRow* sub_rows = split_large_block(row_black, row_start, H, 40, &sub_count);
            
            for (int i = 0; i < sub_count && row_count < 50; i++) {
                rows[row_count++] = sub_rows[i];
            }
            free(sub_rows);
        }
    }
    
    printf("[slice_words] Accepted %d text rows total\n", row_count);
    
    if (SDL_MUSTLOCK(img)) SDL_UnlockSurface(img);
    free(row_black);
    
    *out_count = row_count;
    return rows;
}

// Extract individual words from a text row
static Word* extract_words_from_row(SDL_Surface* img, TextRow row, int* out_count) {
    int W = img->w;
    uint8_t* base = (uint8_t*)img->pixels;
    int pitch = img->pitch;
    
    if (SDL_MUSTLOCK(img)) SDL_LockSurface(img);
    
    Word* words = (Word*)malloc(sizeof(Word) * 20);
    int word_count = 0;
    
    bool in_word = false;
    int word_start = 0;
    int consecutive_white = 0;
    int max_letter_gap = 6;
    
    for (int x = 0; x < W; x++) {
        int black_in_column = 0;
        for (int y = row.y_start; y < row.y_end; y++) {
            uint32_t* row_pixels = (uint32_t*)(base + y * pitch);
            if (is_black(row_pixels[x])) {
                black_in_column++;
            }
        }
        
        if (black_in_column > 0) {
            if (!in_word) {
                word_start = x;
                in_word = true;
            }
            consecutive_white = 0;
        } else {
            consecutive_white++;
            
            if (in_word && consecutive_white > max_letter_gap) {
                int word_end = x - consecutive_white;
                int width = word_end - word_start;
                
                // Count black pixels for density check
                int total_black = 0;
                for (int yy = row.y_start; yy < row.y_end; yy++) {
                    uint32_t* rr = (uint32_t*)(base + yy * pitch);
                    for (int xx = word_start; xx < word_end; xx++) {
                        if (is_black(rr[xx])) total_black++;
                    }
                }
                
                int area = width * (row.y_end - row.y_start);
                double density = (area > 0) ? (double)total_black / area : 0;
                
                // Filter: reasonable size and density
                if (width >= 15 && width <= 250 && density >= 0.1 && density <= 0.7) {
                    words[word_count++] = (Word){ 
                        word_start, 
                        row.y_start, 
                        width, 
                        row.y_end - row.y_start 
                    };
                    if (word_count >= 20) break;
                }
                
                in_word = false;
                consecutive_white = 0;
            }
        }
    }
    
    // Handle last word in row
    if (in_word) {
        int width = W - word_start;
        if (width >= 15 && width <= 250) {
            words[word_count++] = (Word){ 
                word_start, 
                row.y_start, 
                width, 
                row.y_end - row.y_start 
            };
        }
    }
    
    if (SDL_MUSTLOCK(img)) SDL_UnlockSurface(img);
    
    *out_count = word_count;
    return words;
}

// Main function
int slice_words(SDL_Surface* wordlist, const char* output_dir) {
    if (!wordlist || !output_dir) return -1;

    printf("[slice_words] Analyzing word list: %dx%d\n", wordlist->w, wordlist->h);
    
    // Find all text rows
    int row_count;
    TextRow* rows = find_text_rows(wordlist, &row_count);
    
    if (row_count == 0) {
        fprintf(stderr, "[slice_words] ERROR: No text rows detected\n");
        free(rows);
        return -1;
    }
    
    int total_words = 0;
    
    // Extract words from each row
    for (int r = 0; r < row_count; r++) {
        int word_count;
        Word* words = extract_words_from_row(wordlist, rows[r], &word_count);
        
        // Save each word
        for (int w = 0; w < word_count; w++) {
            int margin = 1;
            int x = (words[w].x > margin) ? words[w].x - margin : 0;
            int y = (words[w].y > margin) ? words[w].y - margin : 0;
            int width = words[w].w + 2 * margin;
            int height = words[w].h + 2 * margin;
            
            if (x + width > wordlist->w) width = wordlist->w - x;
            if (y + height > wordlist->h) height = wordlist->h - y;
            
            SDL_Rect src = { x, y, width, height };
            SDL_Surface* word_surf = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_ARGB8888);
            SDL_Rect dst = { 0, 0, 0, 0 };
            SDL_BlitSurface(wordlist, &src, word_surf, &dst);
            
            char path[512];
            snprintf(path, sizeof(path), "%s/w_%02d.bmp", output_dir, total_words);
            SDL_SaveBMP(word_surf, path);
            SDL_FreeSurface(word_surf);
            
            total_words++;
        }
        
        free(words);
    }
    
    free(rows);
    
    printf("[slice_words] ✓ Saved %d words to %s\n", total_words, output_dir);
    return 0;
}
