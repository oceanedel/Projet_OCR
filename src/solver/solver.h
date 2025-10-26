#ifndef SOLVER_H
#define SOLVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_ROWS 100
#define MAX_COLS 100

extern char grid[MAX_ROWS][MAX_COLS];
extern int rows;
extern int cols;

void read_grid(const char *filename);

int check_word_in_direction(char word[], int x, int y, int dx, int dy);

int find_word(char word[], int *x0, int *y0, int *x1, int *y1);

#endif

