#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "result.h"

#define MAX_ROWS 100
#define MAX_COLS 100
#define MAX_WORDS 100

// Structure pour stocker l'état de la grille
typedef struct {
    char grid[MAX_ROWS][MAX_COLS];
    int rows;
    int cols;
    char *words[MAX_WORDS];
    int word_count;
    GtkWidget *drawing_area;
} ResultData;

static ResultData data;

// --- Fonctions utilitaires de lecture ---

static void load_grid_from_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return;
    
    char line[256];
    data.rows = 0;
    while (fgets(line, sizeof(line), f) && data.rows < MAX_ROWS) {
        line[strcspn(line, "\r\n")] = 0;
        if (strlen(line) == 0) continue;
        
        int len = strlen(line);
        int c = 0;
        for(int i=0; i<len; i++) {
            if(!isspace(line[i])) {
                data.grid[data.rows][c++] = line[i];
            }
        }
        data.cols = c;
        data.rows++;
    }
    fclose(f);
}

static void load_words_from_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return;
    
    char line[256];
    data.word_count = 0;
    while (fgets(line, sizeof(line), f) && data.word_count < MAX_WORDS) {
        line[strcspn(line, "\r\n")] = 0;
        if (strlen(line) > 0) {
            data.words[data.word_count] = strdup(line);
            data.word_count++;
        }
    }
    fclose(f);
}

static int check_word(int r, int c, int dr, int dc, const char *word) {
    int len = strlen(word);
    for (int i = 0; i < len; i++) {
        int nr = r + i * dr;
        int nc = c + i * dc;
        if (nr < 0 || nr >= data.rows || nc < 0 || nc >= data.cols || 
            data.grid[nr][nc] != word[i]) {
            return 0;
        }
    }
    return 1;
}

// --- Dessin avec Cairo ---

// Palette (identique à solving.c)
static const double colors[8][3] = {
    {1.0, 0.6, 0.6}, {0.6, 1.0, 0.6}, {0.6, 0.6, 1.0}, {1.0, 1.0, 0.6},
    {1.0, 0.6, 1.0}, {0.6, 1.0, 1.0}, {1.0, 0.8, 0.6}, {0.5, 0.5, 0.5}
};

static void draw_puzzle(cairo_t *cr, int width, int height) {
    // 1. Fond blanc global
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    // Définition des zones : 75% pour la grille, 25% pour la liste
    int grid_area_w = width * 0.75;
    int list_area_w = width - grid_area_w;
    (void)list_area_w;
    
    // --- PARTIE GAUCHE : LA GRILLE ---
    
    if (data.rows > 0 && data.cols > 0) {
        // Calcul taille cellule
        double cell_w = (double)grid_area_w / data.cols;
        double cell_h = (double)height / data.rows;
        double cell_size = (cell_w < cell_h) ? cell_w : cell_h;
        
        // Marge de sécurité
        cell_size *= 0.95; 

        // Centrage dans la zone de gauche
        double offset_x = (grid_area_w - (cell_size * data.cols)) / 2.0;
        double offset_y = (height - (cell_size * data.rows)) / 2.0;

        cairo_save(cr);
        cairo_translate(cr, offset_x, offset_y);

        // Dessin des surlignages (Mots trouvés)
        int color_idx = 0;
        for (int w = 0; w < data.word_count; w++) {
            char *word = data.words[w];
            int found = 0;
            for (int r = 0; r < data.rows && !found; r++) {
                for (int c = 0; c < data.cols && !found; c++) {
                    int dirs[8][2] = {{0,1},{0,-1},{1,0},{-1,0},{1,1},{1,-1},{-1,1},{-1,-1}};
                    for (int d = 0; d < 8; d++) {
                        if (check_word(r, c, dirs[d][0], dirs[d][1], word)) {
                            double start_x = c * cell_size + cell_size/2;
                            double start_y = r * cell_size + cell_size/2;
                            double end_x = (c + dirs[d][1] * (strlen(word)-1)) * cell_size + cell_size/2;
                            double end_y = (r + dirs[d][0] * (strlen(word)-1)) * cell_size + cell_size/2;

                            cairo_set_source_rgba(cr, colors[color_idx][0], colors[color_idx][1], colors[color_idx][2], 0.6);
                            cairo_set_line_width(cr, cell_size * 0.8);
                            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
                            
                            cairo_move_to(cr, start_x, start_y);
                            cairo_line_to(cr, end_x, end_y);
                            cairo_stroke(cr);
                            found = 1;
                            break;
                        }
                    }
                }
            }
            color_idx = (color_idx + 1) % 8;
        }

        // Dessin des lettres et de la grille
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, cell_size * 0.6);

        for (int r = 0; r < data.rows; r++) {
            for (int c = 0; c < data.cols; c++) {
                double x = c * cell_size;
                double y = r * cell_size;
                char str[2] = { data.grid[r][c], '\0' };
                
                cairo_text_extents_t extents;
                cairo_text_extents(cr, str, &extents);
                cairo_move_to(cr, x + (cell_size - extents.width)/2 - extents.x_bearing, 
                                  y + (cell_size - extents.height)/2 - extents.y_bearing);
                cairo_show_text(cr, str);
            }
        }
        cairo_restore(cr);
    }

    // --- PARTIE DROITE : LA LISTE DES MOTS ---

    // Ligne de séparation verticale
    cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
    cairo_set_line_width(cr, 2.0);
    cairo_move_to(cr, grid_area_w, 10);
    cairo_line_to(cr, grid_area_w, height - 10);
    cairo_stroke(cr);

    // Titre "MOTS"
    double text_x = grid_area_w + 20;
    double text_y = 40;
    
    cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 20);
    cairo_move_to(cr, text_x, text_y);
    cairo_show_text(cr, "Word list");
    
    text_y += 30;

    // Liste des mots
    cairo_set_font_size(cr, 16);
    int color_idx = 0;
    
    for (int i = 0; i < data.word_count; i++) {
        // Carré de couleur (Légende)
        cairo_set_source_rgb(cr, colors[color_idx][0], colors[color_idx][1], colors[color_idx][2]);
        cairo_rectangle(cr, text_x, text_y - 12, 12, 12);
        cairo_fill(cr);
        
        // Le mot
        cairo_set_source_rgb(cr, 0, 0, 0); // Texte en noir
        cairo_move_to(cr, text_x + 20, text_y);
        cairo_show_text(cr, data.words[i]);
        
        text_y += 25;
        color_idx = (color_idx + 1) % 8;

        // Si la liste dépasse la hauteur, on arrête (ou on pourrait faire une 2ème colonne)
        if (text_y > height - 20) break; 
    }
}

static gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    (void)user_data;
    int w = gtk_widget_get_allocated_width(widget);
    int h = gtk_widget_get_allocated_height(widget);
    draw_puzzle(cr, w, h);
    return FALSE;
}

static void on_save_clicked(GtkWidget *widget, gpointer window) {
    GtkWidget *dialog;
    (void)widget;
    dialog = gtk_file_chooser_dialog_new("Save as...",
                                         GTK_WINDOW(window),
                                         GTK_FILE_CHOOSER_ACTION_SAVE,
                                         "Cancel", GTK_RESPONSE_CANCEL,
                                         "Save", GTK_RESPONSE_ACCEPT,
                                         NULL);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "result_puzzle.png");
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        
        // On crée une image large pour contenir grille + mots
        int save_w = 1000; 
        int save_h = 800; 

        cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, save_w, save_h);
        cairo_t *cr = cairo_create(surface);
        draw_puzzle(cr, save_w, save_h);
        cairo_surface_write_to_png(surface, filename);
        cairo_destroy(cr);
        cairo_surface_destroy(surface);
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

// --- Point d'entrée ---

void show_result_window() {
    load_grid_from_file("./output/grid.txt");
    load_words_from_file("./output/words.txt");

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Result");
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 700); // Plus large pour la liste
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    
    // On n'ajoute PAS de gtk_main_quit au signal destroy
    // La fenêtre se détruit juste quand on la ferme.

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_container_set_border_width(GTK_CONTAINER(window), 12); 

    // Zone de dessin
    data.drawing_area = gtk_drawing_area_new();
    gtk_widget_set_vexpand(data.drawing_area, TRUE);
    g_signal_connect(G_OBJECT(data.drawing_area), "draw", G_CALLBACK(on_draw_event), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), data.drawing_area, TRUE, TRUE, 0);

    // Bouton sauvegarder
    GtkWidget *save_btn = gtk_button_new_with_label("Save as...");
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_save_clicked), window);
    gtk_box_pack_start(GTK_BOX(vbox), save_btn, FALSE, FALSE, 10);

    // --- APPLICATION DU CSS ---
    // Correction : On applique le style SEULEMENT à cette fenêtre (context), pas à l'écran.
    // Cela évite de corrompre le style de la fenêtre principale.
    
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        // Style spécifique pour CETTE fenêtre et ses enfants
        "window { background-color: #5C5656; }"
        "button { background: white; color: #5C5656; border-radius: 8px; padding: 6px 12px; font-weight: bold; border: none; }"
        "button:hover { background: #B5AEAE; }"
        , -1, NULL);

    // On récupère le contexte de la fenêtre créée
    GtkStyleContext *context = gtk_widget_get_style_context(window);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

    // Astuce pour que les enfants (Boutons) héritent du provider sans polluer l'écran global :
    // On applique récursivement le provider aux widgets déjà créés n'est pas nécessaire car
    // le sélecteur "window" affecte le fond, et "button" affectera les boutons à l'intérieur
    // car le provider est attaché à la racine de cette hiérarchie (window).
    // Mais pour être sûr que le CSS s'applique aux boutons, on peut l'ajouter à l'écran 
    // MAIS avec des sélecteurs très précis, ou utiliser la méthode ci-dessus qui suffit souvent pour le fond.
    
    // Pour être sûr que les boutons soient stylés sans toucher au main, on boucle :
    gtk_style_context_add_provider(gtk_widget_get_style_context(save_btn), 
                                   GTK_STYLE_PROVIDER(provider), 
                                   GTK_STYLE_PROVIDER_PRIORITY_USER);

    gtk_widget_show_all(window);
}
