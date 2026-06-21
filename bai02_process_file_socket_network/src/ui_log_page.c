#include "ui_log_page.h"

static void on_clear(GtkButton *button, gpointer user_data) {
    (void)button;
    LogPage *page = user_data;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(page->text_view));
    gtk_text_buffer_set_text(buffer, "", -1);
}

static void on_save(GtkButton *button, gpointer user_data) {
    (void)button;
    LogPage *page = user_data;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(page->text_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    char *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    GError *error = NULL;
    if (g_file_set_contents("process_file_socket_network_log.txt", text, -1, &error)) {
        log_page_append(page, "OK", "Saved log to process_file_socket_network_log.txt");
    } else {
        log_page_append(page, "ERROR", error->message);
        g_error_free(error);
    }
    g_free(text);
}

LogPage *ui_log_page_new(void) {
    LogPage *page = g_new0(LogPage, 1);
    page->root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(page->root, 14);
    gtk_widget_set_margin_bottom(page->root, 14);
    gtk_widget_set_margin_start(page->root, 14);
    gtk_widget_set_margin_end(page->root, 14);

    GtkWidget *title = gtk_label_new("Program Log");
    gtk_widget_add_css_class(title, "title-3");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(page->root), title);

    page->text_view = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(page->text_view), TRUE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(page->text_view), FALSE);
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), page->text_view);
    gtk_box_append(GTK_BOX(page->root), scroll);

    GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *clear = gtk_button_new_with_label("Clear Log");
    GtkWidget *save = gtk_button_new_with_label("Save Log");
    gtk_box_append(GTK_BOX(actions), clear);
    gtk_box_append(GTK_BOX(actions), save);
    gtk_box_append(GTK_BOX(page->root), actions);

    g_signal_connect(clear, "clicked", G_CALLBACK(on_clear), page);
    g_signal_connect(save, "clicked", G_CALLBACK(on_save), page);
    return page;
}

void log_page_append(LogPage *page, const char *level, const char *message) {
    if (!page || !page->text_view) return;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(page->text_view));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    char **lines = g_strsplit(message ? message : "", "\n", -1);
    for (int i = 0; lines[i]; i++) {
        if (lines[i][0] == '\0' && lines[i + 1] == NULL) continue;
        char *line = g_strdup_printf("[%s] %s\n", level, lines[i]);
        gtk_text_buffer_insert(buffer, &end, line, -1);
        g_free(line);
    }
    g_strfreev(lines);
}
