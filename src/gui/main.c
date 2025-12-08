/* main.c
 * Simple image viewer with manual rotation control (GTK3)
 *
 * Compile:
 *   gcc `pkg-config --cflags gtk+-3.0` -o image_rotator main.c `pkg-config --libs gtk+-3.0`
 *
 * Dependencies:
 *   GTK+ 3 development headers (libgtk-3-dev on Debian/Ubuntu)
 *
 * Features:
 *  - Open JPG/PNG from disk (FileChooser)
 *  - Display image in a drawing area
 *  - Slider (0..360°) to set angle
 *  - Buttons to rotate left/right (by step)
 *  - Entry + Apply for arbitrary angle
 *  - Display is updated immediately with scaling to fit
 */

#include <gtk/gtk.h>
#include <math.h>
#include "../solver/solver.h"
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <stdlib.h>
#include <stdio.h>
#include "solving.c"
#include "../autorotation/rotation.h"
#include "solving.h"
#include <SDL2/SDL.h>

// Extraction modules
#include "../extraction/preprocess.h"
#include "../extraction/extract_grid.h"
#include "../extraction/slice_grid.h"
#include "../extraction/extract_wordlist.h"
#include "../extraction/slice_words.h"
#include "../extraction/slice_letter_word.h"
#include "../extraction/trim_cells.h"
#include "../extraction/trim_word_letters.h"
#include "../extraction/denoiser.h"


// Solver wrapper
#include "../solver/solver.h"


// OCR modules
#include "../ocr/letter_recognition.h"
#include "../ocr/grid_processor.h"
#include "../ocr/word_processor.h"




#include <dirent.h>




/* Compute bounding box of rotated rectangle */
static void compute_rotated_size(int w, int h, double angle_deg, int *out_w, int *out_h) {
    double rad = angle_deg * G_PI / 180.0;
    double c = fabs(cos(rad));
    double s = fabs(sin(rad));
    double new_w = w * c + h * s;
    double new_h = w * s + h * c;
    *out_w = (int)ceil(new_w);
    *out_h = (int)ceil(new_h);
}



/* Draw function – image always fits inside fixed drawing area */
static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    AppData *app = (AppData*)user_data;

    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    int da_w = alloc.width;
    int da_h = alloc.height;

    /* fond gris clair */
    cairo_set_source_rgb(cr, 0.95, 0.95, 0.95);
    cairo_paint(cr);
    
    if (!app->pixbuf)
        return FALSE;


    int rot_w, rot_h;
    compute_rotated_size(app->pixbuf_width, app->pixbuf_height, app->angle_deg, &rot_w, &rot_h);

    /* calcul du facteur d’échelle pour tout faire tenir */
    double scale = fmin((double)da_w / rot_w, (double)da_h / rot_h);


    /* centre de la zone de dessin */
    double cx = da_w / 2.0;
    double cy = da_h / 2.0;

    cairo_save(cr);
    cairo_translate(cr, cx, cy);
    cairo_scale(cr, scale, scale);
    cairo_rotate(cr, app->angle_deg * G_PI / 180.0);

    /* dessin centré */
    double px = -app->pixbuf_width / 2.0;
    double py = -app->pixbuf_height / 2.0;
    gdk_cairo_set_source_pixbuf(cr, app->pixbuf, px, py);
    cairo_paint(cr);

    cairo_restore(cr);

    return FALSE;
}



/* Slider change handler */
static void on_scale_value_changed(GtkRange *range, gpointer user_data) {
    AppData *app = (AppData*)user_data;
    app->angle_deg = gtk_range_get_value(range);
    char buf[64];
    g_snprintf(buf, sizeof(buf), "%.2f", app->angle_deg);
    gtk_entry_set_text(GTK_ENTRY(app->angle_entry), buf);
    gtk_widget_queue_draw(app->drawing_area);
}






/* Save rotated image to disk */
static void save_rotated_bmp(AppData *app) {
    if (!app->pixbuf) return;

    int w = app->pixbuf_width;
    int h = app->pixbuf_height;
    double angle = app->angle_deg;

    int rot_w, rot_h;
    compute_rotated_size(w, h, angle, &rot_w, &rot_h);

    /* Create cairo surface to draw rotated image with white background */
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, rot_w, rot_h);
    cairo_t *cr = cairo_create(surface);

    /* Fill with white */
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);

    /* Rotate around center */
    cairo_translate(cr, rot_w / 2.0, rot_h / 2.0);
    cairo_rotate(cr, angle * G_PI / 180.0);
    cairo_translate(cr, -w / 2.0, -h / 2.0);

    gdk_cairo_set_source_pixbuf(cr, app->pixbuf, 0, 0);
    cairo_paint(cr);

    cairo_destroy(cr);

    GdkPixbuf *dest = gdk_pixbuf_get_from_surface(surface, 0, 0, rot_w, rot_h);
    cairo_surface_destroy(surface);

    if (dest) {
        GError *err = NULL;
        /* Save to ../../output/image.bmp */
        if (!gdk_pixbuf_save(dest, "../../output/image.bmp", "bmp", &err, NULL)) {
             g_printerr("Failed to save rotated image: %s\n", err ? err->message : "Unknown error");
             if (err) g_error_free(err);
        } else {
             /* Success */
        }
        g_object_unref(dest);
    }
}

/* Rotation helpers */
static void rotate_by(AppData *app, double delta) {
    double new_angle = fmod(app->angle_deg + delta, 360.0);
    if (new_angle < 0) new_angle += 360.0;
    app->angle_deg = new_angle;
    gtk_range_set_value(GTK_RANGE(app->scale), app->angle_deg);
    save_rotated_bmp(app);
}

static void on_button_left(GtkButton *btn, gpointer user_data) {
    (void )btn;
    rotate_by((AppData*)user_data, -5.0);
}

static void on_button_right(GtkButton *btn, gpointer user_data) {
    (void )btn;
    rotate_by((AppData*)user_data, 5.0);
}

static void on_apply_angle(GtkButton *btn, gpointer user_data) {
    (void )btn;
    AppData *app = (AppData*)user_data;
    const char *txt = gtk_entry_get_text(GTK_ENTRY(app->angle_entry));
    if (!txt) return;
    char *endptr = NULL;
    double val = g_ascii_strtod(txt, &endptr);
    if (endptr == txt) return;
    val = fmod(val, 360.0);
    if (val < 0) val += 360.0;
    app->angle_deg = val;
    gtk_range_set_value(GTK_RANGE(app->scale), app->angle_deg);
    save_rotated_bmp(app);
}

static void on_solve(GtkButton *btn, gpointer user_data) {
    (void)btn;
    AppData *app = (AppData*)user_data;
    gtk_label_set_text(GTK_LABEL(app->message_label), "Grid solving in progress...");
    gtk_widget_set_name(app->message_label, "message_error"); 

    while (gtk_events_pending()) {
        gtk_main_iteration();
    }

    launch_solving("../../output/image.bmp", user_data);

    
}


static void denoiser(GtkButton *btn, gpointer user_data)
{
    (void)btn; 
    AppData *app = (AppData *)user_data;

    if (!app->pixbuf) {
        gtk_label_set_text(GTK_LABEL(app->message_label), "No image to denoise");
        gtk_widget_set_name(app->message_label, "message_error");
        return;
    }

    save_rotated_bmp(app); 
    const char *filepath = "../../output/image.bmp";

    SDL_Surface *src_surface = SDL_LoadBMP(filepath);
    if (!src_surface) {
        g_printerr("SDL Load Error: %s\n", SDL_GetError());
        gtk_label_set_text(GTK_LABEL(app->message_label), "Error loading for denoise");
        gtk_widget_set_name(app->message_label, "message_error");
        return;
    }

    SDL_Surface *denoised_surface = denoise_image(src_surface);
    SDL_FreeSurface(src_surface);

    if (!denoised_surface) {
        gtk_label_set_text(GTK_LABEL(app->message_label), "Denoising failed");
        gtk_widget_set_name(app->message_label, "message_error");
        return;
    }

    if (SDL_SaveBMP(denoised_surface, filepath) != 0) {
        g_printerr("SDL Save Error: %s\n", SDL_GetError());
        SDL_FreeSurface(denoised_surface);
        return;
    }
    SDL_FreeSurface(denoised_surface);

    GError *err = NULL;
    GdkPixbuf *new_pix = gdk_pixbuf_new_from_file(filepath, &err);
    if (!new_pix) {
        g_printerr("Reload Pixbuf Error: %s\n", err->message);
        g_error_free(err);
        return;
    }

    if (app->pixbuf) g_object_unref(app->pixbuf);
    app->pixbuf = new_pix;
    app->pixbuf_width = gdk_pixbuf_get_width(new_pix);
    app->pixbuf_height = gdk_pixbuf_get_height(new_pix);

    app->angle_deg = 0.0;
    gtk_range_set_value(GTK_RANGE(app->scale), 0.0);
    gtk_entry_set_text(GTK_ENTRY(app->angle_entry), "0");

    gtk_widget_queue_draw(app->drawing_area);
    gtk_label_set_text(GTK_LABEL(app->message_label), "Denoise applied successfully!");
    gtk_widget_set_name(app->message_label, "message_label");
    
    printf("Denoise applied and reloaded.\n");
}



static void on_auto_rotation(GtkButton *btn, gpointer user_data)
{
    (void)btn;
    AppData *app = (AppData*)user_data;

    if (!app->pixbuf) return;

    const char *temp_input_path = "../../output/grid_before_autorotate.bmp";
    
    // On boucle 2 fois pour affiner la rotation
    for (int i = 0; i < 2; i++) 
    {
        GError *error = NULL;

        // 1. IMPORTANT : On sauvegarde l'état ACTUEL de l'image sur le disque
        // Cela permet à la 2ème itération de travailler sur le résultat de la 1ère.
        if (!gdk_pixbuf_save(app->pixbuf, temp_input_path, "bmp", &error, NULL)) {
            g_printerr("Error saving temp image for rotation loop %d: %s\n", i, error->message);
            g_error_free(error);
            break; // On arrête si on ne peut pas sauvegarder
        }

        // 2. Appel de la fonction de rotation (détection + correction)
        char *output_path = rotate_original_auto((char *)temp_input_path);
        
        if (!output_path) {
            // Si output_path est NULL, c'est que la rotation a échoué ou n'était pas nécessaire
            g_print("Auto-rotation step %d skipped or failed.\n", i + 1);
            continue; 
        }

        // 3. Chargement de l'image corrigée
        GdkPixbuf *rot = gdk_pixbuf_new_from_file(output_path, &error);
        
        // On libère le chemin retourné par la fonction
        free(output_path);

        if (!rot) {
            g_printerr("Error loading rotated image loop %d: %s\n", i, error->message);
            g_error_free(error);
            break;
        }

        // 4. Nettoyage des bords noirs (Artefacts de rotation)
        int w = gdk_pixbuf_get_width(rot);
        int h = gdk_pixbuf_get_height(rot);
        int n_channels = gdk_pixbuf_get_n_channels(rot);
        int rowstride = gdk_pixbuf_get_rowstride(rot);
        guchar *pixels = gdk_pixbuf_get_pixels(rot);

        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                guchar *p = pixels + y * rowstride + x * n_channels;
                // Si le pixel est presque noir (artefact) -> on le met en blanc
                if (p[0] < 20 && p[1] < 20 && p[2] < 20) {
                    p[0] = 255; // R
                    p[1] = 255; // G
                    p[2] = 255; // B
                }
            }
        }

        // 5. Mise à jour de l'image de l'application pour la prochaine itération
        if (app->pixbuf)
            g_object_unref(app->pixbuf);

        app->pixbuf = rot;
        app->pixbuf_width = w;
        app->pixbuf_height = h;
        
        g_print("Auto-rotation pass %d applied.\n", i + 1);
    }

    // Reset des contrôles UI (curseurs à 0 car l'image est maintenant "droite" en interne)
    app->angle_deg = 0.0;
    gtk_range_set_value(GTK_RANGE(app->scale), 0.0);
    gtk_entry_set_text(GTK_ENTRY(app->angle_entry), "0");

    // Sauvegarde finale pour que le Solveur/OCR utilise la dernière version
    save_rotated_bmp(app); 

    gtk_widget_queue_draw(app->drawing_area);
}



/* Open file chooser and load image */
static void on_open(GtkButton *btn, gpointer user_data) {
    (void )btn;
    AppData *app = (AppData*)user_data;
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Open an image",
                                                    GTK_WINDOW(app->window),
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Open", GTK_RESPONSE_ACCEPT,
                                                    NULL);
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.png");
    gtk_file_filter_add_pattern(filter, "*.jpg");
    gtk_file_filter_add_pattern(filter, "*.jpeg");
    gtk_file_filter_add_pattern(filter, "*.bmp");
    gtk_file_filter_set_name(filter, "Images (PNG, JPG, BMP)");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        GError *error = NULL;
        GdkPixbuf *pix = gdk_pixbuf_new_from_file(filename, &error);
        if (!pix) {
            GtkWidget *msg = gtk_message_dialog_new(GTK_WINDOW(app->window),
                                                    GTK_DIALOG_MODAL,
                                                    GTK_MESSAGE_ERROR,
                                                    GTK_BUTTONS_OK,
                                                    "Error loading image:\n%s",
                                                    error ? error->message : "Unknown");
            gtk_dialog_run(GTK_DIALOG(msg));
            gtk_widget_destroy(msg);
            if (error) g_error_free(error);
            g_free(filename);
            gtk_widget_destroy(dialog);
            return;
        }

        if (app->pixbuf) g_object_unref(app->pixbuf);
        app->pixbuf = pix;
        app->pixbuf_width = gdk_pixbuf_get_width(pix);
        app->pixbuf_height = gdk_pixbuf_get_height(pix);
        app->angle_deg = 0.0;
        gtk_range_set_value(GTK_RANGE(app->scale), 0.0);


        gtk_widget_queue_draw(app->drawing_area);

        //save as image.bmp
        SDL_Surface *surface = SDL_LoadBMP(filename);

        if (surface == NULL) {
            printf("Erreur lors du chargement : %s\n", SDL_GetError());
            return;
        }
        if (SDL_SaveBMP(surface,"../../output/image.bmp" ) != 0) {
            printf("Erreur lors de la sauvegarde : %s\n", SDL_GetError());
        } 
        else {
            printf("Image copiée avec succès vers : %s\n", "../../output/image.bmp");
        }
        SDL_FreeSurface(surface);

        g_free(filename);
    }
    gtk_label_set_text(GTK_LABEL(app->message_label), "");

    gtk_widget_set_name(app->message_label, "message_error"); 
    gtk_widget_destroy(dialog);

    
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    AppData app = {0};
    app.angle_deg = 0.0;

    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app.window), "OCR Word Search Solver");
    gtk_window_set_default_size(GTK_WINDOW(app.window), 950, 700);
    gtk_container_set_border_width(GTK_CONTAINER(app.window), 12);
    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_set_name(app.window, "main_window");

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(app.window), vbox);

    GtkWidget *header_frame = gtk_frame_new(NULL);
    gtk_widget_set_name(header_frame, "header_frame");
    gtk_frame_set_shadow_type(GTK_FRAME(header_frame), GTK_SHADOW_NONE);
    gtk_box_pack_start(GTK_BOX(vbox), header_frame, FALSE, FALSE, 0);

    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(header_box), 8);
    gtk_container_add(GTK_CONTAINER(header_frame), header_box);
    gtk_widget_set_halign(header_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(header_box, GTK_ALIGN_CENTER);

    GtkWidget *open_btn = gtk_button_new_with_label("Open Image");
    g_signal_connect(open_btn, "clicked", G_CALLBACK(on_open), &app);
    gtk_box_pack_start(GTK_BOX(header_box), open_btn, FALSE, FALSE, 0);

    GtkWidget *left_btn = gtk_button_new_with_label("⟲ -5°");
    g_signal_connect(left_btn, "clicked", G_CALLBACK(on_button_left), &app);
    gtk_box_pack_start(GTK_BOX(header_box), left_btn, FALSE, FALSE, 0);

    GtkWidget *right_btn = gtk_button_new_with_label("+5° ⟳");
    g_signal_connect(right_btn, "clicked", G_CALLBACK(on_button_right), &app);
    gtk_box_pack_start(GTK_BOX(header_box), right_btn, FALSE, FALSE, 0);

    app.angle_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app.angle_entry), "Angle (°)");
    gtk_widget_set_size_request(app.angle_entry, 50, -1);
    gtk_box_pack_start(GTK_BOX(header_box), app.angle_entry, FALSE, FALSE, 0);

    GtkWidget *apply_btn = gtk_button_new_with_label("Apply");
    g_signal_connect(apply_btn, "clicked", G_CALLBACK(on_apply_angle), &app);
    gtk_box_pack_start(GTK_BOX(header_box), apply_btn, FALSE, FALSE, 0);

    GtkWidget *auto_btn = gtk_button_new_with_label("⟳ Auto-Rotate");
    g_signal_connect(auto_btn, "clicked", G_CALLBACK(on_auto_rotation), &app);
    gtk_box_pack_start(GTK_BOX(header_box), auto_btn, FALSE, FALSE, 0);

    GtkWidget *denoise_btn = gtk_button_new_with_label("Denoise");
    g_signal_connect(denoise_btn, "clicked", G_CALLBACK(denoiser), &app);
    gtk_box_pack_start(GTK_BOX(header_box), denoise_btn, FALSE, FALSE,0);




    app.scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 360.0, 1.0);
    gtk_scale_set_draw_value(GTK_SCALE(app.scale), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), app.scale, FALSE, FALSE, 5);
    g_signal_connect(app.scale, "value-changed", G_CALLBACK(on_scale_value_changed), &app);

    GtkWidget *image_frame = gtk_frame_new("");
    gtk_frame_set_shadow_type(GTK_FRAME(image_frame), GTK_SHADOW_NONE);
    gtk_box_pack_start(GTK_BOX(vbox), image_frame, TRUE, TRUE, 5);

    app.drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(app.drawing_area, 800, 600);
    gtk_container_add(GTK_CONTAINER(image_frame), app.drawing_area);
    g_signal_connect(app.drawing_area, "draw", G_CALLBACK(on_draw), &app);

    app.message_label = gtk_label_new("");
    gtk_widget_set_name(app.message_label, "message_label");
    
    gtk_box_pack_end(GTK_BOX(vbox), app.message_label, FALSE, FALSE, 5);


    GtkWidget *solve_btn = gtk_button_new_with_label("Solve Grid");
    g_signal_connect(solve_btn, "clicked", G_CALLBACK(on_solve), &app);
    gtk_box_pack_end(GTK_BOX(vbox), solve_btn, FALSE, FALSE, 10);


    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "button { background: white ;color:#5C5656 ; border-radius: 8px; padding: 6px 12px; font-weight: bold; }"
        "button:hover { background:#B5AEAE ; }"
        "frame#header_frame {border:none;}"
        "window#main_window { background-color: #5C5656 ;}"
        "entry { background: white ;color: #5C5656;border-radius: 6px; padding: 4px; font-weight: bold;}"
        "scale { margin-top: 5px; font-weight: bold; color :white;}"
        "frame label { font-weight: bold; color:#5C5656 ;}"

        "label#message_error { color: white; font-weight: bold; font-size: 20px; margin: 6px; }"
        "label#message_label { color: white; font-weight: bold; font-size: 20px; margin: 6px; }",
        -1, NULL);

    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

    gtk_widget_show_all(app.window);
    gtk_main();

    if (app.pixbuf) g_object_unref(app.pixbuf);
    return 0;
}

