#ifndef SOLVING_H
#define SOLVING_H

#include <gtk/gtk.h>

typedef struct {
    GtkWidget *window;
    GtkWidget *drawing_area;
    GtkWidget *scale;
    GtkWidget *angle_entry;
    GdkPixbuf *pixbuf;        /* original pixbuf */
    int pixbuf_width;
    int pixbuf_height;
    double angle_deg;
    GtkWidget *message_label;
} AppData;

extern int g_grid_rows, g_grid_cols;
extern int g_highlight_marks[100][100];

#endif
