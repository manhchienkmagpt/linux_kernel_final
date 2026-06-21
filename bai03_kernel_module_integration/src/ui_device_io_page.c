#include "ui_device_io_page.h"
#include "kernel_module_commands.h"

typedef struct {
    AppContext *ctx;
    GtkWidget *connected_label;
    GtkWidget *left_label;
    GtkWidget *right_label;
    GtkWidget *middle_label;
    GtkWidget *dx_label;
    GtkWidget *dy_label;
    GtkWidget *wheel_label;
    GtkWidget *raw_view;
} MouseStatusPage;

static void set_label_int(GtkWidget *label, int value) {
    char text[32];
    g_snprintf(text, sizeof(text), "%d", value);
    gtk_label_set_text(GTK_LABEL(label), text);
}

static GtkWidget *status_row(const char *title, GtkWidget **value) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *left = gtk_label_new(title);
    gtk_widget_set_size_request(left, 150, -1);
    gtk_label_set_xalign(GTK_LABEL(left), 0.0f);
    *value = gtk_label_new("0");
    gtk_label_set_xalign(GTK_LABEL(*value), 0.0f);
    gtk_widget_set_hexpand(*value, TRUE);
    gtk_box_append(GTK_BOX(row), left);
    gtk_box_append(GTK_BOX(row), *value);
    return row;
}

static void set_raw(MouseStatusPage *page, const char *text) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(page->raw_view));
    gtk_text_buffer_set_text(buffer, text ? text : "", -1);
}

static void refresh_status(MouseStatusPage *page) {
    gboolean ok = FALSE;
    char *message = NULL;
    MouseStatus *status = read_mouse_status(GTK_WINDOW(page->ctx->window), &ok, &message);
    if (!ok || !status) {
        set_raw(page, message ? message : "Cannot read /proc/mouse_monitor. Load module first.");
        gtk_label_set_text(GTK_LABEL(page->connected_label), "0");
        g_free(message);
        return;
    }

    set_label_int(page->connected_label, status->connected);
    set_label_int(page->left_label, status->left);
    set_label_int(page->right_label, status->right);
    set_label_int(page->middle_label, status->middle);
    set_label_int(page->dx_label, status->dx);
    set_label_int(page->dy_label, status->dy);
    set_label_int(page->wheel_label, status->wheel);
    set_raw(page, status->raw);

    mouse_status_free(status);
    g_free(message);
}

static void on_refresh(GtkButton *button, gpointer user_data) {
    (void)button;
    refresh_status(user_data);
}

GtkWidget *ui_device_io_page_new(AppContext *ctx) {
    MouseStatusPage *page = g_new0(MouseStatusPage, 1);
    page->ctx = ctx;
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(root, 14);
    gtk_widget_set_margin_bottom(root, 14);
    gtk_widget_set_margin_start(root, 14);
    gtk_widget_set_margin_end(root, 14);
    g_object_set_data_full(G_OBJECT(root), "page-data", page, (GDestroyNotify)g_free);

    GtkWidget *title = gtk_label_new("Mouse Status");
    gtk_widget_add_css_class(title, "title-2");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(root), title);

    GtkWidget *refresh = gtk_button_new_with_label("Refresh Status");
    gtk_widget_set_halign(refresh, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(root), refresh);

    GtkWidget *grid = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_box_append(GTK_BOX(grid), status_row("Connected", &page->connected_label));
    gtk_box_append(GTK_BOX(grid), status_row("Left Button", &page->left_label));
    gtk_box_append(GTK_BOX(grid), status_row("Right Button", &page->right_label));
    gtk_box_append(GTK_BOX(grid), status_row("Middle Button", &page->middle_label));
    gtk_box_append(GTK_BOX(grid), status_row("dx", &page->dx_label));
    gtk_box_append(GTK_BOX(grid), status_row("dy", &page->dy_label));
    gtk_box_append(GTK_BOX(grid), status_row("Wheel", &page->wheel_label));
    gtk_box_append(GTK_BOX(root), grid);

    page->raw_view = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(page->raw_view), TRUE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(page->raw_view), FALSE);
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), page->raw_view);
    gtk_box_append(GTK_BOX(root), scroll);

    g_signal_connect(refresh, "clicked", G_CALLBACK(on_refresh), page);
    refresh_status(page);
    return root;
}
