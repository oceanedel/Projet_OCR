#include "grid_processor.h"
#include "letter_recognition.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

static int detect_grid_col_size(const char* cells_dir) {
    int max_col = -1;
    
    for (int c = 0; c < 50; c++) {
        char path[512];
        snprintf(path, sizeof(path), "%s/c_00_%02d.bmp", cells_dir, c);
        
        struct stat st;
        if (stat(path, &st) == 0) {
            max_col = c;
        }
    }
    
    return max_col + 1;
}

static int detect_grid_row_size(const char* cells_dir) {
    int max_row = -1;
    
    for (int r = 0; r < 50; r++) {
        char path[512];
        snprintf(path, sizeof(path), "%s/c_%02d_00.bmp", cells_dir, r);
        
        struct stat st;
        if (stat(path, &st) == 0) {
            max_row = r;
        }
    }
    
    return max_row + 1;
}


int process_grid(const char* cells_dir, const char* output_file) {
    printf("\n========================================\n");
    printf("Grid Processing\n");
    printf("========================================\n");
    printf("[GRID] Input: %s\n", cells_dir);
    printf("[GRID] Output: %s\n", output_file);
    

    // Detect grid dimensions
    int grid_cols = detect_grid_col_size(cells_dir);
    int grid_rows = detect_grid_row_size(cells_dir);
    
    if (grid_cols == 0 || grid_rows == 0) {
        fprintf(stderr, "[GRID] ✗ No cells found in %s\n", cells_dir);
        return -1;
    }
    
    printf("[GRID] Detected grid size: %d rows × %d columns\n", grid_rows, grid_cols);
    
    // Allocate grid (rows × columns)
    char** grid = (char**)malloc(grid_rows * sizeof(char*));
    for (int i = 0; i < grid_rows; i++) {
        grid[i] = (char*)malloc((grid_cols + 1) * sizeof(char));
        grid[i][grid_cols] = '\0';  // Null terminate each row
    }
    
    // Process each cell
    int total_cells = grid_rows * grid_cols;
    int recognized = 0;
    int uncertain = 0;
    
    printf("[GRID] Processing %d cells...\n", total_cells);
    
    for (int row = 0; row < grid_rows; row++) {
        for (int col = 0; col < grid_cols; col++) {
            char path[512];
            snprintf(path, sizeof(path), "%s/c_%02d_%02d.bmp", cells_dir, row, col);
            
            char letter = recognize_letter(path);
            grid[row][col] = letter;
            
            if (letter != '?') {
                recognized++;
            } else {
                uncertain++;
            }
        }
    }
    
    printf("[GRID] Recognition complete:\n");
    printf("[GRID]   ✓ Recognized: %d/%d (%.1f%%)\n", 
           recognized, total_cells, 100.0 * recognized / total_cells);
    printf("[GRID]   ? Uncertain:  %d/%d (%.1f%%)\n", 
           uncertain, total_cells, 100.0 * uncertain / total_cells);
    
    // Write to file
    FILE* f = fopen(output_file, "w");
    if (!f) {
        fprintf(stderr, "[GRID] ✗ Cannot create output file: %s\n", output_file);
        for (int i = 0; i < grid_rows; i++) free(grid[i]);
        free(grid);
        return -1;
    }

    for (int i = 0; i < grid_rows; i++) {
        printf( "%s\n", grid[i]);
        fprintf(f, "%s\n", grid[i]);
    }
    
    fclose(f);
/*
    //Conversion du tableau en SDL_surface
    if (output_file) {
        if (SDL_Init(0) != 0) {
            fprintf(stderr, "[GRID] ✗ SDL_Init failed: %s\n", SDL_GetError());
            
            for (int i = 0; i < grid_rows; i++) free(grid[i]);
            free(grid);
            return -1;
        }
        
        SDL_Surface *surf = SDL_CreateRGBSurface(0, grid_cols, grid_rows, 32,
                                                 0x00FF0000,
                                                 0x0000FF00,
                                                 0x000000FF,
                                                 0xFF000000);
        if (!surf) {
            fprintf(stderr, "[GRID] ✗ SDL_CreateRGBSurface failed: %s\n", SDL_GetError());
            SDL_Quit();
            for (int i = 0; i < grid_rows; i++) free(grid[i]);
            free(grid);
            return -1;
        }
        
       // écriture en tenant compte du pitch (padding) //
        Uint32 *pixels = (Uint32*)surf->pixels;
        int pitch_pixels = surf->pitch / 4; // nombre d'Uint32 par ligne //
        for (int y = 0; y < grid_rows; y++) {
            for (int x = 0; x < grid_cols; x++) {
                char ch = grid[y][x];
                Uint8 v;
                if (ch == '?') {
                    v = 32; // sombre pour incertain //
                } else {
                    // exemple : lettre reconnue -> valeur lumineuse //
                    v = 220;
                }
                Uint32 color = SDL_MapRGBA(surf->format, v, v, v, 255);
                pixels[y * pitch_pixels + x] = color;
            }
        }
        
        if (SDL_SaveBMP(surf, output_file) != 0) {
            fprintf(stderr, "[GRID] ✗ SDL_SaveBMP failed: %s\n", SDL_GetError());
            SDL_FreeSurface(surf);
            SDL_Quit();
            for (int i = 0; i < grid_rows; i++) free(grid[i]);
            free(grid);
            return -1;
        }
        
        SDL_FreeSurface(surf);
        SDL_Quit();
    }
*/
    // Cleanup
    for (int i = 0; i < grid_rows; i++) free(grid[i]);
    free(grid);
    
    printf("[GRID] ✓ Grid saved to: %s\n", output_file);
    
    if (uncertain > total_cells / 4) {
        printf("[GRID] ⚠️  Warning: >25%% uncertain letters. Check training templates!\n");
    }
    
    return 0;
}
