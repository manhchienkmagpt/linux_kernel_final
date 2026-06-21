#include "ui_dashboard_page.h"
#include "kernel_module_commands.h"

typedef struct {
    AppContext *ctx;
    GtkWidget *module_status;
    GtkWidget *device_file;
    GtkWidget *root_dir;
    GtkWidget *last_command;
    GtkWidget *last_result;
    GtkWidget *total_commands;
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

static void refresh_dashboard(DashboardPage *page) {
    gboolean loaded = module_is_loaded();
    gboolean ok = FALSE;
    KFileStatus *status = loaded ? read_kfile_status(GTK_WINDOW(page->ctx->window), &ok) : NULL;

    gtk_label_set_text(GTK_LABEL(page->module_status), loaded ? "Loaded" : "Not Loaded");
    gtk_label_set_text(GTK_LABEL(page->device_file), DEVICE_PATH);

    if (ok && status) {
        char total[32];
        g_snprintf(total, sizeof(total), "%d", status->total_commands);
        gtk_label_set_text(GTK_LABEL(page->root_dir), status->root);
        gtk_label_set_text(GTK_LABEL(page->last_command), status->last_command);
        gtk_label_set_text(GTK_LABEL(page->last_result), status->last_result);
        gtk_label_set_text(GTK_LABEL(page->total_commands), total);
    } else {
        gtk_label_set_text(GTK_LABEL(page->root_dir), "/tmp/kfile_manager_root");
        gtk_label_set_text(GTK_LABEL(page->last_command), "-");
        gtk_label_set_text(GTK_LABEL(page->last_result), loaded ? "Cannot read status" : "-");
        gtk_label_set_text(GTK_LABEL(page->total_commands), "0");
    }

    kfile_status_free(status);
}

static void on_refresh(GtkButton *button, gpointer user_data) {
    (void)button;
    refresh_dashboard(user_data);
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
    gtk_grid_attach(GTK_GRID(grid), card("Device File", &page->device_file), 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), card("Root Directory", &page->root_dir), 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), card("Last Command", &page->last_command), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), card("Last Result", &page->last_result), 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), card("Total Commands", &page->total_commands), 2, 1, 1, 1);
    gtk_box_append(GTK_BOX(root), grid);

    GtkWidget *refresh = gtk_button_new_with_label("Refresh");
    gtk_widget_set_halign(refresh, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(root), refresh);
    g_signal_connect(refresh, "clicked", G_CALLBACK(on_refresh), page);
    refresh_dashboard(page);
    return root;
}
