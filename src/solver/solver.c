#include "solver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <err.h>

#define MAX_ROWS 100
#define MAX_COLS 100

char grid[MAX_ROWS][MAX_COLS];
int rows, cols;


void read_grid(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        errx(EXIT_FAILURE, "Error opening file");
    }

    char line[MAX_COLS + 2];
    rows = 0;
    cols = 0;
    
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';
        
        // Skip empty lines
        if (strlen(line) == 0) continue;
        
        strcpy(grid[rows], line);
        if (cols == 0)
            cols = strlen(line);
        rows++;
        
        if (rows >= MAX_ROWS) break;
    }
    fclose(file);
}

int check_word_in_direction(const char *word, int sx, int sy, int dx, int dy) {
    int len = strlen(word);
    for (int i = 0; i < len; i++) {
        int x = sx + i * dx;
        int y = sy + i * dy;
        if (x < 0 || x >= rows || y < 0 || y >= cols || toupper(grid[x][y]) != toupper(word[i]))
            return 0;
    }
    return 1;
}

int find_word(char word[], int *x, int *y, int *x1, int *y1) {
    int directions[8][2] = {
        {0, 1}, {0, -1}, {1, 0}, {-1, 0},
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
    };
    
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            if (toupper(grid[r][c]) == toupper(word[0])) {
                for (int d = 0; d < 8; d++) {
                    int dx = directions[d][0], dy = directions[d][1];
                    if (check_word_in_direction(word, r, c, dx, dy)) {
                        *x = c; *y = r;
                        *x1 = c + (strlen(word)-1) * dy;
                        *y1 = r + (strlen(word)-1) * dx;
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}


/*int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s grid.txt WORD\n", argv[0]);
        return 1;
    }
    
    read_grid(argv[1]);
    char word[256]; strcpy(word, argv[2]);
    
    int dirs[8][2] = {{0,1},{0,-1},{1,0},{-1,0},{1,1},{1,-1},{-1,1},{-1,-1}};
    
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            if (toupper(grid[r][c]) == toupper(word[0])) {
                for (int d = 0; d < 8; d++) {
                    int dx = dirs[d][0], dy = dirs[d][1];
                    if (check_word_in_direction(word, r, c, dx, dy)) {
                        int len = strlen(word) - 1;
                        printf("(%d,%d)(%d,%d)\n", c, r, c + len * dy, r + len * dx);
                        return 0;
                    }
                }
            }
        }
    }
    printf("Not found\n");
    return 0;
}*/