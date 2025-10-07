// main.c
// Orchestrates the full extraction pipeline for word-search puzzle images

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include all extraction module headers
#include "src/extraction/preprocess.h"
#include "src/extraction/extract_grid.h"
#include "src/extraction/extract_wordlist.h"
#include "src/extraction/slice_grid.h"
#include "src/extraction/slice_words.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.bmp>\n", argv[0]);
        fprintf(stderr, "Example: %s data/test1.bmp\n", argv[0]);
        return 1;
    }

    const char* input_path = argv[1];
    printf("========================================\n");
    printf("Word Search OCR Extraction Pipeline\n");
    printf("========================================\n");
    printf("Input: %s\n\n", input_path);

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    // ============================
    // STAGE 1: Binarization
    // ============================
    printf("[1/5] Binarizing image...\n");
    SDL_Surface* binary = binarize_image(input_path);
    if (!binary) {
        fprintf(stderr, "ERROR: Binarization failed\n");
        SDL_Quit();
        return 1;
    }
    
    // Save binary for inspection
    if (SDL_SaveBMP(binary, "output/binary.bmp") != 0) {
        fprintf(stderr, "WARNING: Could not save binary.bmp: %s\n", SDL_GetError());
    } else {
        printf("  ✓ Saved: output/binary.bmp\n");
    }

    // ============================
    // STAGE 2: Extract Grid
    // ============================
    printf("\n[2/5] Extracting grid region...\n");
    int grid_x, grid_y, grid_w, grid_h;
    SDL_Surface* grid = extract_grid(binary, &grid_x, &grid_y, &grid_w, &grid_h);
    if (!grid) {
        fprintf(stderr, "ERROR: Grid extraction failed\n");
        SDL_FreeSurface(binary);
        SDL_Quit();
        return 1;
    }

    // Save grid
    if (SDL_SaveBMP(grid, "output/grid.bmp") != 0) {
        fprintf(stderr, "WARNING: Could not save grid.bmp: %s\n", SDL_GetError());
    } else {
        printf("  ✓ Saved: output/grid.bmp\n");
    }

    // ============================
    // STAGE 3: Extract Word List
    // ============================
    printf("\n[3/5] Extracting word list region...\n");
    int wl_x, wl_y, wl_w, wl_h;
    SDL_Surface* wordlist = extract_wordlist(binary, grid_x, grid_y, grid_w, grid_h,
                                             &wl_x, &wl_y, &wl_w, &wl_h);
    
    // Binary no longer needed after extraction
    SDL_FreeSurface(binary);

    if (!wordlist) {
        fprintf(stderr, "ERROR: Word list extraction failed\n");
        SDL_FreeSurface(grid);
        SDL_Quit();
        return 1;
    }

    // Save word list
    if (SDL_SaveBMP(wordlist, "output/solvingwords.bmp") != 0) {
        fprintf(stderr, "WARNING: Could not save solvingwords.bmp: %s\n", SDL_GetError());
    } else {
        printf("  ✓ Saved: output/solvingwords.bmp\n");
    }

    // ============================
    // STAGE 4: Slice Grid into Cells
    // ============================
    printf("\n[4/5] Slicing grid into cells...\n");
    int ret_grid = slice_grid(grid, "output/cells");
    
    // Grid surface no longer needed
    SDL_FreeSurface(grid);

    if (ret_grid != 0) {
        fprintf(stderr, "ERROR: Grid slicing failed\n");
        SDL_FreeSurface(wordlist);
        SDL_Quit();
        return 1;
    }
    printf("  ✓ Grid cells saved to: output/cells/\n");

    // ============================
    // STAGE 5: Slice Words into Lines
    // ============================
    printf("\n[5/5] Slicing word list into lines...\n");
    int ret_words = slice_words(wordlist, "output/words");
    
    // Word list surface no longer needed
    SDL_FreeSurface(wordlist);

    if (ret_words != 0) {
        fprintf(stderr, "ERROR: Word list slicing failed\n");
        SDL_Quit();
        return 1;
    }
    printf("  ✓ Word lines saved to: output/words/\n");

    // ============================
    // Complete
    // ============================
    printf("\n========================================\n");
    printf("✓ Extraction pipeline complete!\n");
    printf("========================================\n");
    printf("Outputs:\n");
    printf("  - Binary image:    output/binary.bmp\n");
    printf("  - Grid region:     output/grid.bmp\n");
    printf("  - Word list:       output/solvingwords.bmp\n");
    printf("  - Grid cells:      output/cells/c_XX_YY.bmp\n");
    printf("  - Word lines:      output/words/w_XX.bmp\n");
    printf("\nNext steps:\n");
    printf("  1. Run OCR on cells and words\n");
    printf("  2. Pass recognized grid + word list to solver\n");
    printf("========================================\n");

    SDL_Quit();
    return 0;
}
