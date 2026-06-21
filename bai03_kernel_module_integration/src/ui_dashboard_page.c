#include "ui_dashboard_page.h"
#include "kernel_module_commands.h"

typedef struct {
    AppContext *ctx;
    GtkWidget *module_status;
    GtkWidget *mouse_connected;
    GtkWidget *last_action;
    GtkWidget *device_interface;
} DashboardPage;

static GtkWidget *card(const char *title, GtkWidget **value) {
    GtkWidget *frame = gtk_frame_new(NULL);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_top(box, 12);
    gtk_widget_set_margin_bottom(box, 12);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);
    gtk_frame_set_child(GTK_FRAME(frame), box);

    GtkWidget *label = gtk_label_new(title);
    gtk_widget_add_css_class(label, "dim-label");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), label);

    *value = gtk_label_new("...");
    gtk_widget_add_css_class(*value, "title-3");
    gtk_widget_set_halign(*value, GTK_ALIGN_START);
    gtk_label_set_wrap(GTK_LABEL(*value), TRUE);
    gtk_box_append(GTK_BOX(box), *value);
    return frame;
}

static void refresh_dashboard(DashboardPage *page, const char *last_action) {
    gboolean ok = FALSE;
    char *message = NULL;
    MouseStatus *status = module_is_loaded() ? read_mouse_status(GTK_WINDOW(page->ctx->window), &ok, &message) : NULL;
    char *event = last_mouse_event();

    gtk_label_set_text(GTK_LABEL(page->module_status), module_is_loaded() ? "Loaded" : "Not Loaded");
    gtk_label_set_text(GTK_LABEL(page->mouse_connected), (ok && status && status->connected) ? "Connected" : "Not Connected");
    gtk_label_set_text(GTK_LABEL(page->last_action), event);
    gtk_label_set_text(GTK_LABEL(page->device_interface), DEVICE_PATH);
    (void)last_action;
    mouse_status_free(status);
    g_free(message);
    g_free(event);
}

static void on_refresh(GtkButton *button, gpointer user_data) {
    (void)button;
    refresh_dashboard(user_data, "Dashboard refreshed");
}

GtkWidget *ui_dashboard_page_new(AppContext *ctx) {
    DashboardPage *page = g_new0(DashboardPage, 1);
    page->ctx = ctx;
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_top(root, 14);
    gtk_widget_set_margin_bottom(root, 14);
    gtk_widget_set_margin_start(root, 14);
    gtk_widget_set_margin_end(root, 14);
    g_object_set_data_full(G_OBJECT(root), "page-data", page, (GDestroyNotify)g_free);

    GtkWidget *title = gtk_label_new("Dashboard");
    gtk_widget_add_css_class(title, "title-2");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(root), title);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_grid_attach(GTK_GRID(grid), card("Module Status", &page->module_status), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), card("Mouse Connected", &page->mouse_connected), 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), card("Device Interface", &page->device_interface), 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), card("Last Event", &page->last_action), 0, 1, 3, 1);
    gtk_box_append(GTK_BOX(root), grid);

    GtkWidget *refresh = gtk_button_new_with_label("Refresh");
    gtk_widget_set_halign(refresh, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(root), refresh);
    g_signal_connect(refresh, "clicked", G_CALLBACK(on_refresh), page);
    refresh_dashboard(page, "Application started");
    return root;
}
