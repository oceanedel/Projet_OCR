// include/m2_grid.h
#pragma once
#include <SDL2/SDL.h>

typedef struct {
    int *rows; int n_rows;   // divider line y-coordinates (includes top and bottom)
    int *cols; int n_cols;   // divider line x-coordinates (includes left and right)
} GridLines;

typedef struct {
    int R, C;         // number of cell rows/cols
    SDL_Rect **cells; // cells[r][c] rectangles inside the source image
} CellMap;

// Detect horizontal/vertical divider lines from a binary ARGB8888 image.
// row_alpha and col_alpha are initial thresholds as fraction of width/height (e.g., 0.5).
int detect_grid_lines(SDL_Surface* bin, GridLines* out, float row_alpha, float col_alpha);

// Build per-cell rectangles from divider lines; trim is pixels removed from borders (e.g., 1 or 2).
int build_cells(const GridLines* gl, CellMap* out, int trim);

// Export each cell as BMPs into folder (must exist).
int export_cells(SDL_Surface* src, const CellMap* cm, const char* folder);

// Free helpers
void free_grid_lines(GridLines* gl);
void free_cell_map(CellMap* cm);
