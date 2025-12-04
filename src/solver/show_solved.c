#include "solver.h"

/* ANSI underline codes (for a “line” highlight) */
#define YELLOW_BG "\x1b[43m"
#define RESET    "\x1b[0m"

static void mark_word_segment(int x0, int y0, int x1, int y1,
                              int rows, int cols, int **mark)
{

    int dx = (x1 > x0) ? 1 : (x1 < x0 ? -1 : 0);
    int dy = (y1 > y0) ? 1 : (y1 < y0 ? -1 : 0);

    int x = x0;
    int y = y0;

    while (1) {
        if (x >= 0 && x < cols && y >= 0 && y < rows)
            mark[y][x] = 1;

        if (x == x1 && y == y1)
            break;

        x += dx;
        y += dy;
    }
}


int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <grid_file> <words_file>\n", argv[0]);
        return 1;
    }

    const char *grid_file  = argv[1];
    const char *words_file = argv[2];

    /* 1) Load grid into global grid[][], rows, cols */
    read_grid(grid_file);

    /* 2) Allocate mark[y][x] = 1 if cell is in any found word */
    int **mark = malloc(rows * sizeof(int *));
    if (!mark) {
        fprintf(stderr, "Out of memory\n");
        return 1;
    }
    for (int y = 0; y < rows; y++) {
        mark[y] = calloc(cols, sizeof(int));
        if (!mark[y]) {
            fprintf(stderr, "Out of memory\n");
            for (int k = 0; k < y; k++) free(mark[k]);
            free(mark);
            return 1;
        }
    }

    /* 3) Read list of words we want to highlight */
    FILE *wf = fopen(words_file, "r");
    if (!wf) {
        fprintf(stderr, "Error opening words file: %s\n", words_file);
        perror("fopen");
        for (int y = 0; y < rows; y++) free(mark[y]);
        free(mark);
        return 1;
    }

    char line[256];


    while (fgets(line, sizeof(line), wf)) 
    {
        // Strip newline(s)
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';

        char cmd[512]; 
        // Pass trimmed word to solver
        snprintf(cmd, sizeof(cmd), "./solver %s \"%s\"", grid_file, line);
        
        FILE *p = popen(cmd, "r");
        if (p) {
            char out[32]; 
            if (fgets(out, sizeof(out), p) && strncmp(out, "Not found", 8) != 0) {
                int x0,y0,x1,y1;
                // Parse output like (x0,y0)(x1,y1)
                if (sscanf(out, "(%d,%d)(%d,%d)", &x0, &y0, &x1, &y1) == 4) {
                    printf("Found %s: %s", line, out);
                    mark_word_segment(x0, y0, x1, y1, rows, cols, mark);
                } else {
                    printf("Malformed output for %s: %s\n", line, out);
                }
            } else {
                printf("Not found: %s\n", line);
            }
            pclose(p);
        }
    }


    /* 4) Print a replica of the grid, with found words underlined */
    printf("\nSolved grid:\n\n");
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            char c = grid[y][x];
            if (mark[y][x])
                 printf("%s %c %s", YELLOW_BG, c, RESET);  /* Highlight found letters */
            else
                printf("%c ", c);
        }
        printf("\n");
    }

    /* 5) Cleanup */
    for (int y = 0; y < rows; y++)
        free(mark[y]);
    free(mark);

    return 0;
}
