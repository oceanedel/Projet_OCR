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
 *  - Display is updated immediately and drawing area is resized to fit rotated image
 */
#include <gtk/gtk.h>
#include <math.h>

typedef struct {
    GtkWidget *window;
    GtkWidget *drawing_area;
    GtkWidget *scale;
    GtkWidget *angle_entry;
    GdkPixbuf *pixbuf;        /* original pixbuf */
    int pixbuf_width;
    int pixbuf_height;
    double angle_deg;
} AppData;

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

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    AppData *app = (AppData*)user_data;

    if (!app->pixbuf) {
        /* clear background */
        GtkAllocation alloc;
        gtk_widget_get_allocation(widget, &alloc);
        cairo_set_source_rgb(cr, 0.92, 0.92, 0.92);
        cairo_rectangle(cr, 0, 0, alloc.width, alloc.height);
        cairo_fill(cr);
        return FALSE;
    }

    int da_w, da_h;
    gtk_widget_get_allocated_width(widget);
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    da_w = alloc.width;
    da_h = alloc.height;

    /* fill background */
    cairo_set_source_rgb(cr, 0.95, 0.95, 0.95);
    cairo_paint(cr);

    /* center */
    double cx = da_w / 2.0;
    double cy = da_h / 2.0;

    /* prepare transform: translate to center, rotate, then draw pixbuf centered */
    cairo_save(cr);
    cairo_translate(cr, cx, cy);
    cairo_rotate(cr, app->angle_deg * G_PI / 180.0);

    /* draw pixbuf centered at (0,0) */
    double px = -app->pixbuf_width / 2.0;
    double py = -app->pixbuf_height / 2.0;
    gdk_cairo_set_source_pixbuf(cr, app->pixbuf, px, py);
    cairo_paint(cr);

    cairo_restore(cr);

    return FALSE;
}

static void update_drawing_size(AppData *app) {
    if (!app->pixbuf) return;
    int new_w, new_h;
    compute_rotated_size(app->pixbuf_width, app->pixbuf_height, app->angle_deg, &new_w, &new_h);

    /* set a reasonable minimum to avoid zero-size */
    if (new_w < 1) new_w = 1;
    if (new_h < 1) new_h = 1;

    /* size request so the drawing area (inside scrolled window) fits the rotated image */
    gtk_widget_set_size_request(app->drawing_area, new_w, new_h);
    gtk_widget_queue_draw(app->drawing_area);
}

static void on_scale_value_changed(GtkRange *range, gpointer user_data) {
    AppData *app = (AppData*)user_data;
    app->angle_deg = gtk_range_get_value(range);
    /* update entry to reflect */
    char buf[64];
    g_snprintf(buf, sizeof(buf), "%.2f", app->angle_deg);
    gtk_entry_set_text(GTK_ENTRY(app->angle_entry), buf);

    update_drawing_size(app);
}

static void rotate_by(AppData *app, double delta) {
    double new_angle = fmod(app->angle_deg + delta, 360.0);
    if (new_angle < 0) new_angle += 360.0;
    app->angle_deg = new_angle;
    gtk_range_set_value(GTK_RANGE(app->scale), app->angle_deg); /* will trigger update */
}

static void on_button_left(GtkButton *btn, gpointer user_data) {
    AppData *app = (AppData*)user_data;
    rotate_by(app, -5.0);
}

static void on_button_right(GtkButton *btn, gpointer user_data) {
    AppData *app = (AppData*)user_data;
    rotate_by(app, 5.0);
}

static void on_apply_angle(GtkButton *btn, gpointer user_data) {
    AppData *app = (AppData*)user_data;
    const char *txt = gtk_entry_get_text(GTK_ENTRY(app->angle_entry));
    if (!txt) return;
    char *endptr = NULL;
    double val = g_ascii_strtod(txt, &endptr);
    if (endptr == txt) return; /* invalid */
    /* normalize */
    val = fmod(val, 360.0);
    if (val < 0) val += 360.0;
    app->angle_deg = val;
    gtk_range_set_value(GTK_RANGE(app->scale), app->angle_deg); /* will trigger update */
}

/* Open file chooser and load image */
static void on_open(GtkButton *btn, gpointer user_data) {
    AppData *app = (AppData*)user_data;
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Ouvrir une image",
                                                    GTK_WINDOW(app->window),
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    "_Annuler", GTK_RESPONSE_CANCEL,
                                                    "_Ouvrir", GTK_RESPONSE_ACCEPT,
                                                    NULL);
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.png");
    gtk_file_filter_add_pattern(filter, "*.jpg");
    gtk_file_filter_add_pattern(filter, "*.jpeg");
    gtk_file_filter_set_name(filter, "Images (PNG, JPG)");
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
                                                    "Erreur lors du chargement de l'image:\n%s",
                                                    error ? error->message : "Inconnue");
            gtk_dialog_run(GTK_DIALOG(msg));
            gtk_widget_destroy(msg);
            if (error) g_error_free(error);
            g_free(filename);
            gtk_widget_destroy(dialog);
            return;
        }

        /* replace old */
        if (app->pixbuf) g_object_unref(app->pixbuf);
        app->pixbuf = pix;
        app->pixbuf_width = gdk_pixbuf_get_width(pix);
        app->pixbuf_height = gdk_pixbuf_get_height(pix);
        /* reset angle */
        app->angle_deg = 0.0;
        gtk_range_set_value(GTK_RANGE(app->scale), 0.0);
        update_drawing_size(app);
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    AppData app;
    memset(&app, 0, sizeof(app));
    app.angle_deg = 0.0;
    app.pixbuf = NULL;

    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app.window), "Image Rotator - GTK3 (C)");
    gtk_window_set_default_size(GTK_WINDOW(app.window), 800, 600);
    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    /* top controls */
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
    gtk_container_add(GTK_CONTAINER(app.window), vbox);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    GtkWidget *open_btn = gtk_button_new_with_label("Ouvrir...");
    g_signal_connect(open_btn, "clicked", G_CALLBACK(on_open), &app);
    gtk_box_pack_start(GTK_BOX(hbox), open_btn, FALSE, FALSE, 0);

    GtkWidget *left_btn = gtk_button_new_with_label("⟲ -5°");
    g_signal_connect(left_btn, "clicked", G_CALLBACK(on_button_left), &app);
    gtk_box_pack_start(GTK_BOX(hbox), left_btn, FALSE, FALSE, 0);

    GtkWidget *right_btn = gtk_button_new_with_label("+5° ⟳");
    g_signal_connect(right_btn, "clicked", G_CALLBACK(on_button_right), &app);
    gtk_box_pack_start(GTK_BOX(hbox), right_btn, FALSE, FALSE, 0);

    app.angle_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app.angle_entry), "Angle (°)");
    gtk_widget_set_size_request(app.angle_entry, 80, -1);
    gtk_box_pack_start(GTK_BOX(hbox), app.angle_entry, FALSE, FALSE, 0);

    GtkWidget *apply_btn = gtk_button_new_with_label("Appliquer");
    g_signal_connect(apply_btn, "clicked", G_CALLBACK(on_apply_angle), &app);
    gtk_box_pack_start(GTK_BOX(hbox), apply_btn, FALSE, FALSE, 0);

    /* scale */
    app.scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 360.0, 1.0);
    gtk_scale_set_draw_value(GTK_SCALE(app.scale), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), app.scale, FALSE, FALSE, 0);
    g_signal_connect(app.scale, "value-changed", G_CALLBACK(on_scale_value_changed), &app);

    /* drawing area inside scrolled window (so large rotated images can be scrolled) */
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_widget_set_hexpand(scrolled, TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

    app.drawing_area = gtk_drawing_area_new();
    /* initial minimal size */
    gtk_widget_set_size_request(app.drawing_area, 400, 300);
    gtk_container_add(GTK_CONTAINER(scrolled), app.drawing_area);
    g_signal_connect(app.drawing_area, "draw", G_CALLBACK(on_draw), &app);

    gtk_widget_show_all(app.window);
    gtk_main();

    if (app.pixbuf) g_object_unref(app.pixbuf);
    return 0;
}
