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
    int y_start, y_end;
} BoundingBox;

// Flood fill to label connected pixels (iterative to avoid stack overflow)
static void flood_fill(int** labels, SDL_Surface* img, int x, int y, int label) {
    int W = img->w;
    int H = img->h;
    
    int* stack_x = (int*)malloc(W * H * sizeof(int));
    int* stack_y = (int*)malloc(W * H * sizeof(int));
    int stack_top = 0;
    
    stack_x[stack_top] = x;
    stack_y[stack_top] = y;
    stack_top++;
    
    uint8_t* base = (uint8_t*)img->pixels;
    int pitch = img->pitch;
    
    while (stack_top > 0) {
        stack_top--;
        int cx = stack_x[stack_top];
        int cy = stack_y[stack_top];
        
        if (cx < 0 || cx >= W || cy < 0 || cy >= H) continue;
        if (labels[cy][cx] != 0) continue;
        
        uint32_t* row = (uint32_t*)(base + cy * pitch);
        if (!is_black(row[cx])) continue;
        
        labels[cy][cx] = label;
        
        // Add 8-connected neighbors
        if (cx + 1 < W) { stack_x[stack_top] = cx + 1; stack_y[stack_top] = cy; stack_top++; }
        if (cx - 1 >= 0) { stack_x[stack_top] = cx - 1; stack_y[stack_top] = cy; stack_top++; }
        if (cy + 1 < H) { stack_x[stack_top] = cx; stack_y[stack_top] = cy + 1; stack_top++; }
        if (cy - 1 >= 0) { stack_x[stack_top] = cx; stack_y[stack_top] = cy - 1; stack_top++; }
        if (cx + 1 < W && cy + 1 < H) { stack_x[stack_top] = cx + 1; stack_y[stack_top] = cy + 1; stack_top++; }
        if (cx - 1 >= 0 && cy + 1 < H) { stack_x[stack_top] = cx - 1; stack_y[stack_top] = cy + 1; stack_top++; }
        if (cx + 1 < W && cy - 1 >= 0) { stack_x[stack_top] = cx + 1; stack_y[stack_top] = cy - 1; stack_top++; }
        if (cx - 1 >= 0 && cy - 1 >= 0) { stack_x[stack_top] = cx - 1; stack_y[stack_top] = cy - 1; stack_top++; }
    }
    
    free(stack_x);
    free(stack_y);
}

// Find all connected components
static BoundingBox* find_connected_components(SDL_Surface* img, int* out_count) {
    int W = img->w;
    int H = img->h;
    
    if (SDL_MUSTLOCK(img)) SDL_LockSurface(img);
    
    int** labels = (int**)malloc(H * sizeof(int*));
    for (int y = 0; y < H; y++) {
        labels[y] = (int*)calloc(W, sizeof(int));
    }
    
    int current_label = 1;
    
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            if (labels[y][x] == 0) {
                uint8_t* base = (uint8_t*)img->pixels;
                int pitch = img->pitch;
                uint32_t* row = (uint32_t*)(base + y * pitch);
                
                if (is_black(row[x])) {
                    flood_fill(labels, img, x, y, current_label);
                    current_label++;
                }
            }
        }
    }
    
    int num_components = current_label - 1;
    //printf("[CONNECTED_COMPONENTS] Found %d components\n", num_components);
    
    if (num_components == 0) {
        if (SDL_MUSTLOCK(img)) SDL_UnlockSurface(img);
        for (int y = 0; y < H; y++) free(labels[y]);
        free(labels);
        *out_count = 0;
        return NULL;
    }
    
    BoundingBox* boxes = (BoundingBox*)malloc(num_components * sizeof(BoundingBox));
    
    for (int i = 0; i < num_components; i++) {
        boxes[i].x_start = W;
        boxes[i].x_end = 0;
        boxes[i].y_start = H;
        boxes[i].y_end = 0;
    }
    
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int label = labels[y][x];
            if (label > 0) {
                int idx = label - 1;
                if (x < boxes[idx].x_start) boxes[idx].x_start = x;
                if (x > boxes[idx].x_end) boxes[idx].x_end = x;
                if (y < boxes[idx].y_start) boxes[idx].y_start = y;
                if (y > boxes[idx].y_end) boxes[idx].y_end = y;
            }
        }
    }
    
    for (int i = 0; i < num_components; i++) {
        boxes[i].x_end++;
        boxes[i].y_end++;
    }
    
    for (int y = 0; y < H; y++) free(labels[y]);
    free(labels);
    
    if (SDL_MUSTLOCK(img)) SDL_UnlockSurface(img);
    
    *out_count = num_components;
    return boxes;
}

// Find split points in a wide component using vertical projection
static int* find_split_points_in_component(SDL_Surface* img, BoundingBox box, int* out_splits) {
    int width = box.x_end - box.x_start;
    int height = box.y_end - box.y_start;
    
    uint8_t* base = (uint8_t*)img->pixels;
    int pitch = img->pitch;
    
    int* col_black = (int*)calloc(width, sizeof(int));
    
    for (int x = 0; x < width; x++) {
        for (int y = box.y_start; y < box.y_end; y++) {
            uint32_t* row = (uint32_t*)(base + y * pitch);
            if (is_black(row[box.x_start + x])) {
                col_black[x]++;
            }
        }
    }
    
    int* splits = (int*)malloc(10 * sizeof(int));
    int split_count = 0;
    
    int min_valley_depth = height / 4;
    
    for (int x = 1; x < width - 1; x++) {
        if (col_black[x] < col_black[x-1] && col_black[x] < col_black[x+1]) {
            if (col_black[x] < min_valley_depth) {
                splits[split_count] = box.x_start + x;
                split_count++;
                if (split_count >= 10) break;
            }
        }
    }
    
    free(col_black);
    *out_splits = split_count;
    return splits;
}

// Split a wide component into sub-components
static BoundingBox* split_wide_component(SDL_Surface* img, BoundingBox box, int* out_count) {
    //int width = box.x_end - box.x_start;
    
    int split_count;
    int* split_points = find_split_points_in_component(img, box, &split_count);
    
    //printf("[SPLIT] Component width=%d, found %d split points\n", width, split_count);
    
    if (split_count == 0) {
        free(split_points);
        BoundingBox* result = (BoundingBox*)malloc(sizeof(BoundingBox));
        result[0] = box;
        *out_count = 1;
        return result;
    }
    
    int num_boxes = split_count + 1;
    BoundingBox* boxes = (BoundingBox*)malloc(num_boxes * sizeof(BoundingBox));
    
    int prev_x = box.x_start;
    for (int i = 0; i < split_count; i++) {
        boxes[i].x_start = prev_x;
        boxes[i].x_end = split_points[i];
        boxes[i].y_start = box.y_start;
        boxes[i].y_end = box.y_end;
        prev_x = split_points[i];
    }
    
    boxes[split_count].x_start = prev_x;
    boxes[split_count].x_end = box.x_end;
    boxes[split_count].y_start = box.y_start;
    boxes[split_count].y_end = box.y_end;
    
    free(split_points);
    *out_count = num_boxes;
    return boxes;
}

// Sort boxes by x position
static int compare_boxes(const void* a, const void* b) {
    BoundingBox* box_a = (BoundingBox*)a;
    BoundingBox* box_b = (BoundingBox*)b;
    return box_a->x_start - box_b->x_start;
}

// Main segmentation function
static BoundingBox* segment_word(SDL_Surface* word_img, int* out_count) {
    BoundingBox* boxes = find_connected_components(word_img, out_count);
    
    if (!boxes || *out_count == 0) {
        return boxes;
    }
    
    qsort(boxes, *out_count, sizeof(BoundingBox), compare_boxes);
    
    BoundingBox* final_boxes = (BoundingBox*)malloc(100 * sizeof(BoundingBox));
    int final_count = 0;
    
    int MAX_LETTER_WIDTH = 25;
    
    for (int i = 0; i < *out_count; i++) {
        int width = boxes[i].x_end - boxes[i].x_start;
        int height = boxes[i].y_end - boxes[i].y_start;
        
        if (width < 3 || height < 5) {
            continue;
        }
        
        if (width > MAX_LETTER_WIDTH) {
            //printf("[SEGMENT] Component too wide (%d px), attempting split...\n", width);
            
            int sub_count;
            BoundingBox* sub_boxes = split_wide_component(word_img, boxes[i], &sub_count);
            
            for (int j = 0; j < sub_count && final_count < 100; j++) {
                final_boxes[final_count] = sub_boxes[j];
                final_count++;
            }
            
            free(sub_boxes);
        } else {
            final_boxes[final_count] = boxes[i];
            final_count++;
        }
    }
    
    free(boxes);
    
    //printf("[SEGMENT] Final count: %d letters\n", final_count);
    
    *out_count = final_count;
    return final_boxes;
}

// Count word images
static int count_word_images(const char* words_dir) {
    int count = 0;
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

// Main function
int slice_word_letters(const char* words_dir, const char* output_dir) {
    printf("\n[6/6] Slicing word letters (Connected Components + Splitting)...\n");
    printf("[WORD_LETTERS] Input: %s\n", words_dir);
    printf("[WORD_LETTERS] Output: %s\n", output_dir);
    
    struct stat st = {0};
    if (stat(output_dir, &st) == -1) {
        mkdir(output_dir, 0755);
    }
    
    int word_count = count_word_images(words_dir);
    if (word_count == 0) {
        fprintf(stderr, "[WORD_LETTERS] No word images found\n");
        return -1;
    }
    
    //printf("[WORD_LETTERS] Found %d word images\n", word_count);
    
    int total_letters = 0;
    
    for (int w = 0; w < word_count; w++) {
        char word_path[512];
        snprintf(word_path, sizeof(word_path), "%s/w_%02d.bmp", words_dir, w);
        
        SDL_Surface* word_img = SDL_LoadBMP(word_path);
        if (!word_img) {
            fprintf(stderr, "[WORD_LETTERS] Failed to load: %s\n", word_path);
            continue;
        }
        
        //printf("\n[WORD_LETTERS] Processing word %02d...\n", w);
        
        int letter_count;
        BoundingBox* boxes = segment_word(word_img, &letter_count);
        
        //printf("[WORD_LETTERS] Word %02d: %d letters detected\n", w, letter_count);
        
        if (boxes) {
            for (int l = 0; l < letter_count; l++) {
                int x = boxes[l].x_start;
                int y = boxes[l].y_start;
                int width = boxes[l].x_end - boxes[l].x_start;
                int height = boxes[l].y_end - boxes[l].y_start;
                
                int margin = 2;
                x = (x > margin) ? x - margin : 0;
                y = (y > margin) ? y - margin : 0;
                width += 2 * margin;
                height += 2 * margin;
                
                if (x + width > word_img->w) width = word_img->w - x;
                if (y + height > word_img->h) height = word_img->h - y;
                
                SDL_Rect src = {x, y, width, height};
                SDL_Surface* letter_surf = SDL_CreateRGBSurfaceWithFormat(
                    0, width, height, 32, SDL_PIXELFORMAT_ARGB8888);
                
                SDL_Rect dst = {0, 0, 0, 0};
                SDL_BlitSurface(word_img, &src, letter_surf, &dst);
                
                char letter_path[512];
                snprintf(letter_path, sizeof(letter_path), 
                         "%s/word_%02d_letter_%02d.bmp", output_dir, w, l);
                
                SDL_SaveBMP(letter_surf, letter_path);
                SDL_FreeSurface(letter_surf);
                
                total_letters++;
            }
            
            free(boxes);
        }
        
        SDL_FreeSurface(word_img);
    }
    
    printf("\n[WORD_LETTERS] ✓ Extracted %d letters from %d words\n", 
           total_letters, word_count);
    
    return 0;
}
