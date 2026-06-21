#include "ui_kernel_log_page.h"
#include "kernel_module_commands.h"

typedef struct {
    AppContext *ctx;
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

static char *extract_field(const char *line, const char *key) {
    char *start = g_strstr_len(line, -1, key);
    if (!start) return g_strdup("N/A");
    start += strlen(key);
    char *end = strchr(start, ' ');
    if (!end) return g_strdup(start);
    return g_strndup(start, end - start);
}

static char *extract_time_field(const char *line) {
    const char *left = strchr(line, '[');
    const char *right = strchr(line, ']');
    if (left && right && right > left) return g_strndup(left + 1, right - left - 1);
    return g_strdup("N/A");
}

static char *format_event_lines(const char *logs, int *count) {
    GString *out = g_string_new("Time | PID | Process | Action | Path\n");
    g_string_append(out, "-----|-----|---------|--------|-----\n");
    char **lines = g_strsplit(logs ? logs : "", "\n", -1);
    int rows = 0;

    for (int i = 0; lines[i]; i++) {
        if (!g_strstr_len(lines[i], -1, "[access_monitor]")) continue;
        char *time = extract_time_field(lines[i]);
        char *pid = extract_field(lines[i], "PID=");
        char *comm = extract_field(lines[i], "COMM=");
        char *action = extract_field(lines[i], "ACTION=");
        char *path_start = g_strstr_len(lines[i], -1, "PATH=");
        const char *path = path_start ? path_start + 5 : "N/A";
        g_string_append_printf(out, "%s | %s | %s | %s | %s\n", time, pid, comm, action, path);
        rows++;
        g_free(time);
        g_free(pid);
        g_free(comm);
        g_free(action);
    }

    g_strfreev(lines);
    if (count) *count = rows;
    return g_string_free(out, FALSE);
}

static void load_logs(KernelLogPage *page, gboolean module_only) {
    int raw_count = 0;
    int count = 0;
    const char *filter = gtk_editable_get_text(GTK_EDITABLE(page->filter_entry));
    char *logs = read_kernel_log_sudo(GTK_WINDOW(page->ctx->window), module_only, filter, &raw_count);
    char *formatted = format_event_lines(logs, &count);
    set_log(page, formatted, count);
    g_free(formatted);
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
    KernelLogPage *page = g_new0(KernelLogPage, 1);
    page->ctx = ctx;
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(root, 14);
    gtk_widget_set_margin_bottom(root, 14);
    gtk_widget_set_margin_start(root, 14);
    gtk_widget_set_margin_end(root, 14);
    g_object_set_data_full(G_OBJECT(root), "page-data", page, (GDestroyNotify)g_free);

    GtkWidget *title = gtk_label_new("Event Log");
    gtk_widget_add_css_class(title, "title-2");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(root), title);

    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *refresh = gtk_button_new_with_label("Refresh dmesg");
    GtkWidget *filter_module = gtk_button_new_with_label("Filter access_monitor");
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
