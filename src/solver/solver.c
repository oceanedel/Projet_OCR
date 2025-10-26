#include "solver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <err.h>

#define MAX_ROWS 100
#define MAX_COLS 100

char grid[MAX_ROWS][MAX_COLS];
int rows = 0, cols = 0;

void read_grid(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) 
    {
        errx(EXIT_FAILURE,"Error opening file");
    }

    char line[MAX_COLS + 2];
    while (fgets(line, sizeof(line), file)) 
    {
        line[strcspn(line, "\n")] = '\0';
        strcpy(grid[rows], line);
        if (cols == 0)
            cols = strlen(line);
        rows++;
    }
    fclose(file);
}

int check_word_in_direction(char word[], int x, int y, int dx, int dy) 
{
    int len = strlen(word);
    for (int i = 0; i < len; i++) 
    {
        int x1 = x + i * dx;
        int y1 = y + i * dy;

        if (x1 < 0 || x1 >= rows || y1 < 0 || y1 >= cols)
        {
            return 0;
        }
        if (grid[x1][y1] != word[i])
        {    
             return 0;
        }
    }
    return 1;
}

int find_word(char word[], int *x0, int *y0, int *x1, int *y1) 
{
    int directions[8][2] = {
        {0, 1}, {0, -1}, {1, 0}, {-1, 0},
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
    };

    for (int x = 0; x < rows; x++) {
        for (int y = 0; y < cols; y++) {
            if (grid[x][y] == word[0]) {
                for (int d = 0; d < 8; d++) {
                    if (check_word_in_direction(word, x, y, directions[d][0], directions[d][1])) {
                        *x0 = y;
                        *y0 = x;
                        *x1 = y + (strlen(word) - 1) * directions[d][1];
                        *y1 = x + (strlen(word) - 1) * directions[d][0];
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s grid.txt WORD\n", argv[0]);
        return EXIT_FAILURE;
    }

    read_grid(argv[1]);

    char word[100];
    strcpy(word, argv[2]);
    for (int i = 0; word[i]; i++)
        word[i] = toupper(word[i]);

    int x0, y0, x1, y1;
    if (find_word(word, &x0, &y0, &x1, &y1))
        printf("(%d,%d)(%d,%d)\n", x0, y0, x1, y1);
    else
        printf("Not Found\n");

    return EXIT_SUCCESS;
}

