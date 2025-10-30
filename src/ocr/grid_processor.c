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
            
            SDL_Surface* cell = SDL_LoadBMP(path);
            if (!cell) {
                grid[row][col] = '?';
                uncertain++;
                continue;
            }
            
            char letter = recognize_letter(cell);
            grid[row][col] = letter;
            SDL_FreeSurface(cell);
            
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
        fprintf(f, "%s\n", grid[i]);
    }
    
    fclose(f);
    
    // Cleanup
    for (int i = 0; i < grid_rows; i++) free(grid[i]);
    free(grid);
    
    printf("[GRID] ✓ Grid saved to: %s\n", output_file);
    
    if (uncertain > total_cells / 4) {
        printf("[GRID] ⚠️  Warning: >25%% uncertain letters. Check training templates!\n");
    }
    
    return 0;
}
