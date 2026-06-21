#include "ui_dashboard_page.h"
#include "kernel_module_commands.h"

typedef struct {
    GtkWidget *module_status;
    GtkWidget *device_file;
    GtkWidget *device_status;
    GtkWidget *kernel_version;
    GtkWidget *last_action;
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
    char *kernel = kernel_version_string();
    gtk_label_set_text(GTK_LABEL(page->module_status), module_is_loaded() ? "Loaded" : "Not Loaded");
    gtk_label_set_text(GTK_LABEL(page->device_file), DEVICE_PATH);
    gtk_label_set_text(GTK_LABEL(page->device_status), device_exists() ? "Exists" : "Missing");
    gtk_label_set_text(GTK_LABEL(page->kernel_version), kernel);
    gtk_label_set_text(GTK_LABEL(page->last_action), last_action ? last_action : "Refreshed status");
    g_free(kernel);
}

static void on_refresh(GtkButton *button, gpointer user_data) {
    (void)button;
    refresh_dashboard(user_data, "Dashboard refreshed");
}

GtkWidget *ui_dashboard_page_new(AppContext *ctx) {
    (void)ctx;
    DashboardPage *page = g_new0(DashboardPage, 1);
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
    gtk_grid_attach(GTK_GRID(grid), card("Device File", &page->device_file), 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), card("Device Status", &page->device_status), 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), card("Kernel Version", &page->kernel_version), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), card("Last Action", &page->last_action), 1, 1, 2, 1);
    gtk_box_append(GTK_BOX(root), grid);

    GtkWidget *refresh = gtk_button_new_with_label("Refresh");
    gtk_widget_set_halign(refresh, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(root), refresh);
    g_signal_connect(refresh, "clicked", G_CALLBACK(on_refresh), page);
    refresh_dashboard(page, "Application started");
    return root;
}
