#include "letter_recognition.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dirent.h>

#define MAX_TEMPLATES_PER_LETTER 20
#define TEMPLATE_SIZE 32

typedef struct {
    SDL_Surface* templates[MAX_TEMPLATES_PER_LETTER];
    int count;
} LetterTemplates;

static LetterTemplates letter_templates[26];  // A-Z
static int total_templates = 0;
static double last_confidence = 0.0;

// Normalize image to fixed size
static SDL_Surface* normalize_size(SDL_Surface* img, int target_w, int target_h) {
    SDL_Surface* normalized = SDL_CreateRGBSurfaceWithFormat(
        0, target_w, target_h, 32, SDL_PIXELFORMAT_ARGB8888);
    
    if (!normalized) return NULL;
    
    SDL_Rect src = {0, 0, img->w, img->h};
    SDL_Rect dst = {0, 0, target_w, target_h};
    SDL_BlitScaled(img, &src, normalized, &dst);
    
    return normalized;
}

// Calculate similarity between two images
static double calculate_similarity(SDL_Surface* img1, SDL_Surface* img2) {
    if (!img1 || !img2) return 0.0;
    if (img1->w != img2->w || img1->h != img2->h) return 0.0;
    
    int W = img1->w, H = img1->h;
    
    if (SDL_MUSTLOCK(img1)) SDL_LockSurface(img1);
    if (SDL_MUSTLOCK(img2)) SDL_LockSurface(img2);
    
    uint8_t* pixels1 = (uint8_t*)img1->pixels;
    uint8_t* pixels2 = (uint8_t*)img2->pixels;
    int pitch1 = img1->pitch;
    int pitch2 = img2->pitch;
    
    int matching_pixels = 0;
    int total_pixels = W * H;
    
    for (int y = 0; y < H; y++) {
        uint32_t* row1 = (uint32_t*)(pixels1 + y * pitch1);
        uint32_t* row2 = (uint32_t*)(pixels2 + y * pitch2);
        
        for (int x = 0; x < W; x++) {
            int is_black1 = ((row1[x] & 0x00FFFFFF) == 0);
            int is_black2 = ((row2[x] & 0x00FFFFFF) == 0);
            
            if (is_black1 == is_black2) {
                matching_pixels++;
            }
        }
    }
    
    if (SDL_MUSTLOCK(img1)) SDL_UnlockSurface(img1);
    if (SDL_MUSTLOCK(img2)) SDL_UnlockSurface(img2);
    
    return (double)matching_pixels / total_pixels;
}

// Initialize with training templates
int letter_recognition_init(const char* training_dir) {
    printf("[OCR] Initializing letter recognition...\n");
    printf("[OCR] Loading templates from: %s\n", training_dir);
    
    // Initialize structures
    for (int i = 0; i < 26; i++) {
        letter_templates[i].count = 0;
        for (int j = 0; j < MAX_TEMPLATES_PER_LETTER; j++) {
            letter_templates[i].templates[j] = NULL;
        }
    }
    
    total_templates = 0;
    
    // Try multiple variants per letter
    for (int i = 0; i < 26; i++) {
        char letter = 'A' + i;
        
        // Try base template
        char path[512];
        snprintf(path, sizeof(path), "%s/%c.bmp", training_dir, letter);
        
        SDL_Surface* img = SDL_LoadBMP(path);
        if (img) {
            letter_templates[i].templates[0] = normalize_size(img, TEMPLATE_SIZE, TEMPLATE_SIZE);
            SDL_FreeSurface(img);
            letter_templates[i].count = 1;
            total_templates++;
            //printf("[OCR]   ✓ Loaded: %c.bmp\n", letter);
        }
        

        // Try additional variants
        for (int v = 1; v < MAX_TEMPLATES_PER_LETTER; v++) {
            snprintf(path, sizeof(path), "%s/%c_%d.bmp", training_dir, letter, v);
            
            img = SDL_LoadBMP(path);
            if (img) {
                letter_templates[i].templates[v] = normalize_size(img, TEMPLATE_SIZE, TEMPLATE_SIZE);
                SDL_FreeSurface(img);
                letter_templates[i].count++;
                total_templates++;
                //printf("[OCR]   ✓ Loaded: %c_%d.bmp\n", letter, v);
            }
        }
    }
    
    printf("[OCR] Loaded %d total templates across 26 letters\n", total_templates);
    
    
    if (total_templates == 0) {
        fprintf(stderr, "[OCR] ⚠️  No templates found!\n");
        return -1;
    }
    
    return 0;
}

// Recognize single letter
char recognize_letter(SDL_Surface* image) {
    if (!image || total_templates == 0) {
        last_confidence = 0.0;
        return '?';
    }
    
    // Normalize to template size
    SDL_Surface* normalized = normalize_size(image, TEMPLATE_SIZE, TEMPLATE_SIZE);
    if (!normalized) {
        last_confidence = 0.0;
        return '?';
    }
    
    // Compare against all templates for all letters
    double best_score = 0.0;
    char best_letter = '?';
    
    for (int i = 0; i < 26; i++) {
        char letter = 'A' + i;
        
        // Try all variants of this letter
        for (int v = 0; v < letter_templates[i].count; v++) {
            if (!letter_templates[i].templates[v]) continue;
            
            double score = calculate_similarity(normalized, letter_templates[i].templates[v]);
            
            if (score > best_score) {
                best_score = score;
                best_letter = letter;
            }
        }
    }
    
    SDL_FreeSurface(normalized);
    
    last_confidence = best_score;
    
    // Require at least 65% match
    if (best_score < 0.65) {
        return '?';
    }
    
    return best_letter;
}

// Get confidence of last recognition
double get_recognition_confidence() {
    return last_confidence;
}

// Cleanup
void letter_recognition_cleanup() {
    for (int i = 0; i < 26; i++) {
        for (int v = 0; v < letter_templates[i].count; v++) {
            if (letter_templates[i].templates[v]) {
                SDL_FreeSurface(letter_templates[i].templates[v]);
                letter_templates[i].templates[v] = NULL;
            }
        }
        letter_templates[i].count = 0;
    }
    total_templates = 0;
    printf("[OCR] Letter recognition cleanup complete\n");
}
