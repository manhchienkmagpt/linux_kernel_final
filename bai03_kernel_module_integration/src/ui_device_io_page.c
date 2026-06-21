#include "ui_device_io_page.h"
#include "kernel_module_commands.h"

typedef struct {
    AppContext *ctx;
    GtkWidget *path_entry;
    GtkWidget *output_view;
    GtkWidget *device_status_label;
} MonitorConfigPage;

static void append_output(MonitorConfigPage *page, const char *level, const char *message) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(page->output_view));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    char *line = g_strdup_printf("[%s] %s\n", level, message ? message : "");
    gtk_text_buffer_insert(buffer, &end, line, -1);
    g_free(line);
}

static const char *current_entry_path(MonitorConfigPage *page) {
    const char *path = gtk_editable_get_text(GTK_EDITABLE(page->path_entry));
    return (path && *path) ? path : "/tmp/protected";
}

static void refresh_device_status(MonitorConfigPage *page) {
    gtk_label_set_text(GTK_LABEL(page->device_status_label),
                       device_exists() ? "Config device: /dev/access_monitor exists" :
                                         "Config device missing. Load module first.");
}

static void on_set_path(GtkButton *button, gpointer user_data) {
    (void)button;
    MonitorConfigPage *page = user_data;
    gboolean ok = FALSE;
    char *msg = set_protected_path(GTK_WINDOW(page->ctx->window), current_entry_path(page), &ok);
    append_output(page, ok ? "OK" : "ERROR", msg);
    g_free(msg);
    refresh_device_status(page);
}

static void on_read_path(GtkButton *button, gpointer user_data) {
    (void)button;
    MonitorConfigPage *page = user_data;
    gboolean ok = FALSE;
    char *path = read_protected_path(GTK_WINDOW(page->ctx->window), &ok);
    if (ok) gtk_editable_set_text(GTK_EDITABLE(page->path_entry), path);
    append_output(page, ok ? "OK" : "ERROR", ok ? "Read current protected path." : path);
    g_free(path);
    refresh_device_status(page);
}

static void on_create_test(GtkButton *button, gpointer user_data) {
    (void)button;
    MonitorConfigPage *page = user_data;
    gboolean ok = FALSE;
    char *msg = create_test_file(GTK_WINDOW(page->ctx->window), current_entry_path(page), &ok);
    append_output(page, ok ? "OK" : "ERROR", msg);
    g_free(msg);
}

static void on_write_test(GtkButton *button, gpointer user_data) {
    (void)button;
    MonitorConfigPage *page = user_data;
    gboolean ok = FALSE;
    char *msg = write_test_file(GTK_WINDOW(page->ctx->window), current_entry_path(page), &ok);
    append_output(page, ok ? "OK" : "ERROR", msg);
    g_free(msg);
}

static void on_delete_test(GtkButton *button, gpointer user_data) {
    (void)button;
    MonitorConfigPage *page = user_data;
    gboolean ok = FALSE;
    char *msg = delete_test_file(GTK_WINDOW(page->ctx->window), current_entry_path(page), &ok);
    append_output(page, ok ? "OK" : "ERROR", msg);
    g_free(msg);
}

static void on_clear(GtkButton *button, gpointer user_data) {
    (void)button;
    MonitorConfigPage *page = user_data;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(page->output_view));
    gtk_text_buffer_set_text(buffer, "", -1);
}

GtkWidget *ui_device_io_page_new(AppContext *ctx) {
    MonitorConfigPage *page = g_new0(MonitorConfigPage, 1);
    page->ctx = ctx;
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(root, 14);
    gtk_widget_set_margin_bottom(root, 14);
    gtk_widget_set_margin_start(root, 14);
    gtk_widget_set_margin_end(root, 14);
    g_object_set_data_full(G_OBJECT(root), "page-data", page, (GDestroyNotify)g_free);

    GtkWidget *title = gtk_label_new("Monitor Config");
    gtk_widget_add_css_class(title, "title-2");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(root), title);

    page->device_status_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(page->device_status_label), 0.0f);
    gtk_box_append(GTK_BOX(root), page->device_status_label);

    GtkWidget *path_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(path_row), gtk_label_new("Protected path:"));
    page->path_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(page->path_entry), "/tmp/protected");
    gtk_widget_set_hexpand(page->path_entry, TRUE);
    gtk_box_append(GTK_BOX(path_row), page->path_entry);
    gtk_box_append(GTK_BOX(root), path_row);

    GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *set_btn = gtk_button_new_with_label("Set Protected Path");
    GtkWidget *read_btn = gtk_button_new_with_label("Read Current Path");
    GtkWidget *create_btn = gtk_button_new_with_label("Create Test File");
    GtkWidget *write_btn = gtk_button_new_with_label("Write Test File");
    GtkWidget *delete_btn = gtk_button_new_with_label("Delete Test File");
    GtkWidget *clear_btn = gtk_button_new_with_label("Clear Output");
    gtk_box_append(GTK_BOX(actions), set_btn);
    gtk_box_append(GTK_BOX(actions), read_btn);
    gtk_box_append(GTK_BOX(actions), create_btn);
    gtk_box_append(GTK_BOX(actions), write_btn);
    gtk_box_append(GTK_BOX(actions), delete_btn);
    gtk_box_append(GTK_BOX(actions), clear_btn);
    gtk_box_append(GTK_BOX(root), actions);

    page->output_view = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(page->output_view), TRUE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(page->output_view), FALSE);
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), page->output_view);
    gtk_box_append(GTK_BOX(root), scroll);

    g_signal_connect(set_btn, "clicked", G_CALLBACK(on_set_path), page);
    g_signal_connect(read_btn, "clicked", G_CALLBACK(on_read_path), page);
    g_signal_connect(create_btn, "clicked", G_CALLBACK(on_create_test), page);
    g_signal_connect(write_btn, "clicked", G_CALLBACK(on_write_test), page);
    g_signal_connect(delete_btn, "clicked", G_CALLBACK(on_delete_test), page);
    g_signal_connect(clear_btn, "clicked", G_CALLBACK(on_clear), page);
    refresh_device_status(page);
    return root;
}
