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

int check_word_in_direction(char word[], int x, int y, int x1, int y1) 
{
    int len = strlen(word);
    for (int i = 0; i < len; i++) 
    {
        int x2 = x + i * x1;
        int y2 = y + i * y1;

        if (x2 < 0 || x2 >= rows || y2 < 0 || y2 >= cols)
        {
            return 0;
        }
        if (grid[x2][y2] != word[i])
        {    
             return 0;
        }
    }
    return 1;
}

int find_word(char word[], int *x, int *y, int *x1, int *y1) 
{
    int directions[8][2] = {
        {0, 1}, {0, -1}, {1, 0}, {-1, 0},
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
    };

    for (int x2 = 0; x2 < rows; x++) 
    {
        for (int y2 = 0; y2 < cols; y++) 
        {
            if (grid[x2][y2] == word[0]) 
            {
                for (int d = 0; d < 8; d++) 
                {
                    if (check_word_in_direction(word, x, y, directions[d][0], directions[d][1])) {
                        *x = y2;
                        *y = x2;
                        *x1 = y2 + (strlen(word) - 1) * directions[d][1];
                        *y1 = x2 + (strlen(word) - 1) * directions[d][0];
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

