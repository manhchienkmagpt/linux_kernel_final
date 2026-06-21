#include "ui_kernel_log_page.h"
#include "kernel_module_commands.h"

typedef struct {
    GtkWidget *log_view;
    GtkWidget *filter_entry;
    GtkWidget *count_label;
} KernelLogPage;

static void set_log(KernelLogPage *page, const char *text, int count) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(page->log_view));
    gtk_text_buffer_set_text(buffer, text ? text : "", -1);
    char *label = g_strdup_printf("%d log lines shown", count);
    gtk_label_set_text(GTK_LABEL(page->count_label), label);
    g_free(label);
}

static void load_logs(KernelLogPage *page, gboolean module_only) {
    int count = 0;
    const char *filter = gtk_editable_get_text(GTK_EDITABLE(page->filter_entry));
    char *logs = read_kernel_log(module_only, filter, &count);
    set_log(page, logs, count);
    g_free(logs);
}

static void on_refresh(GtkButton *button, gpointer user_data) {
    (void)button;
    load_logs(user_data, FALSE);
}

static void on_filter_module(GtkButton *button, gpointer user_data) {
    (void)button;
    load_logs(user_data, TRUE);
}

static void on_clear(GtkButton *button, gpointer user_data) {
    (void)button;
    set_log(user_data, "", 0);
}

static void on_search_changed(GtkEditable *editable, gpointer user_data) {
    (void)editable;
    load_logs(user_data, FALSE);
}

GtkWidget *ui_kernel_log_page_new(AppContext *ctx) {
    (void)ctx;
    KernelLogPage *page = g_new0(KernelLogPage, 1);
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(root, 14);
    gtk_widget_set_margin_bottom(root, 14);
    gtk_widget_set_margin_start(root, 14);
    gtk_widget_set_margin_end(root, 14);
    g_object_set_data_full(G_OBJECT(root), "page-data", page, (GDestroyNotify)g_free);

    GtkWidget *title = gtk_label_new("Kernel Log");
    gtk_widget_add_css_class(title, "title-2");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(root), title);

    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *refresh = gtk_button_new_with_label("Refresh dmesg");
    GtkWidget *filter_module = gtk_button_new_with_label("Filter Module Log");
    GtkWidget *clear = gtk_button_new_with_label("Clear View");
    page->filter_entry = gtk_search_entry_new();
    gtk_widget_set_hexpand(page->filter_entry, TRUE);
    gtk_box_append(GTK_BOX(toolbar), refresh);
    gtk_box_append(GTK_BOX(toolbar), filter_module);
    gtk_box_append(GTK_BOX(toolbar), clear);
    gtk_box_append(GTK_BOX(toolbar), page->filter_entry);
    gtk_box_append(GTK_BOX(root), toolbar);

    page->log_view = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(page->log_view), TRUE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(page->log_view), FALSE);
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), page->log_view);
    gtk_box_append(GTK_BOX(root), scroll);

    page->count_label = gtk_label_new("0 log lines shown");
    gtk_label_set_xalign(GTK_LABEL(page->count_label), 0.0f);
    gtk_box_append(GTK_BOX(root), page->count_label);

    g_signal_connect(refresh, "clicked", G_CALLBACK(on_refresh), page);
    g_signal_connect(filter_module, "clicked", G_CALLBACK(on_filter_module), page);
    g_signal_connect(clear, "clicked", G_CALLBACK(on_clear), page);
    g_signal_connect(page->filter_entry, "search-changed", G_CALLBACK(on_search_changed), page);
    load_logs(page, TRUE);
    return root;
}
