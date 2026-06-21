#include "ui_device_io_page.h"
#include "kernel_module_commands.h"

typedef struct {
    AppContext *ctx;
    GtkWidget *root_entry;
    GtkWidget *filename_entry;
    GtkWidget *target_entry;
    GtkWidget *content_view;
    GtkWidget *output_view;
} FileManagerPage;

static const char *entry_text(GtkWidget *entry) {
    return gtk_editable_get_text(GTK_EDITABLE(entry));
}

static char *text_view_text(GtkWidget *view) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    return gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
}

static void set_output(FileManagerPage *page, const char *text) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(page->output_view));
    gtk_text_buffer_set_text(buffer, text ? text : "", -1);
}

static void run_command(FileManagerPage *page, const char *command) {
    gboolean ok = FALSE;
    char *result = send_kfile_command(GTK_WINDOW(page->ctx->window), command, &ok);
    set_output(page, result);
    g_free(result);
}

static void on_set_root(GtkButton *button, gpointer user_data) {
    (void)button;
    FileManagerPage *page = user_data;
    char *cmd = g_strdup_printf("SET_ROOT %s", entry_text(page->root_entry));
    run_command(page, cmd);
    g_free(cmd);
}

static void on_simple_file(GtkButton *button, gpointer user_data) {
    FileManagerPage *page = g_object_get_data(G_OBJECT(button), "page");
    const char *verb = g_object_get_data(G_OBJECT(button), "verb");
    char *cmd = g_strdup_printf("%s %s", verb, entry_text(page->filename_entry));
    run_command(page, cmd);
    g_free(cmd);
}

static void on_write_append(GtkButton *button, gpointer user_data) {
    (void)user_data;
    FileManagerPage *page = g_object_get_data(G_OBJECT(button), "page");
    const char *verb = g_object_get_data(G_OBJECT(button), "verb");
    char *content = text_view_text(page->content_view);
    char *cmd = g_strdup_printf("%s %s %s", verb, entry_text(page->filename_entry), content);
    run_command(page, cmd);
    g_free(cmd);
    g_free(content);
}

static void confirm_delete_response(GtkDialog *dialog, int response, gpointer user_data) {
    FileManagerPage *page = user_data;
    if (response == GTK_RESPONSE_ACCEPT) {
        char *cmd = g_strdup_printf("DELETE %s", entry_text(page->filename_entry));
        run_command(page, cmd);
        g_free(cmd);
    }
    gtk_window_destroy(GTK_WINDOW(dialog));
}

static void on_delete(GtkButton *button, gpointer user_data) {
    (void)button;
    FileManagerPage *page = user_data;
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Confirm Delete",
        GTK_WINDOW(page->ctx->window), GTK_DIALOG_MODAL,
        "Cancel", GTK_RESPONSE_CANCEL, "Delete", GTK_RESPONSE_ACCEPT, NULL);
    gtk_box_append(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
        gtk_label_new("Delete this file from the managed root directory?"));
    g_signal_connect(dialog, "response", G_CALLBACK(confirm_delete_response), page);
    gtk_window_present(GTK_WINDOW(dialog));
}

static void on_pair(GtkButton *button, gpointer user_data) {
    (void)user_data;
    FileManagerPage *page = g_object_get_data(G_OBJECT(button), "page");
    const char *verb = g_object_get_data(G_OBJECT(button), "verb");
    char *cmd = g_strdup_printf("%s %s %s", verb,
        entry_text(page->filename_entry), entry_text(page->target_entry));
    run_command(page, cmd);
    g_free(cmd);
}

static void add_button(GtkWidget *box, const char *label, GCallback cb, FileManagerPage *page, const char *verb) {
    GtkWidget *btn = gtk_button_new_with_label(label);
    g_object_set_data(G_OBJECT(btn), "page", page);
    if (verb) g_object_set_data(G_OBJECT(btn), "verb", (gpointer)verb);
    g_signal_connect(btn, "clicked", cb, page);
    gtk_box_append(GTK_BOX(box), btn);
}

GtkWidget *ui_device_io_page_new(AppContext *ctx) {
    FileManagerPage *page = g_new0(FileManagerPage, 1);
    page->ctx = ctx;

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(root, 14);
    gtk_widget_set_margin_bottom(root, 14);
    gtk_widget_set_margin_start(root, 14);
    gtk_widget_set_margin_end(root, 14);
    g_object_set_data_full(G_OBJECT(root), "page-data", page, (GDestroyNotify)g_free);

    GtkWidget *title = gtk_label_new("File Manager");
    gtk_widget_add_css_class(title, "title-2");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(root), title);

    page->root_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(page->root_entry), "/tmp/kfile_manager_root");
    gtk_editable_set_text(GTK_EDITABLE(page->root_entry), "/tmp/kfile_manager_root");
    GtkWidget *set_root = gtk_button_new_with_label("Set Root");
    g_signal_connect(set_root, "clicked", G_CALLBACK(on_set_root), page);
    GtkWidget *root_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_hexpand(page->root_entry, TRUE);
    gtk_box_append(GTK_BOX(root_row), page->root_entry);
    gtk_box_append(GTK_BOX(root_row), set_root);
    gtk_box_append(GTK_BOX(root), root_row);

    page->filename_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(page->filename_entry), "Filename, e.g. test.txt");
    page->target_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(page->target_entry), "Target filename for rename/copy");
    gtk_box_append(GTK_BOX(root), page->filename_entry);
    gtk_box_append(GTK_BOX(root), page->target_entry);

    page->content_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(page->content_view), GTK_WRAP_WORD_CHAR);
    GtkWidget *content_scroll = gtk_scrolled_window_new();
    gtk_widget_set_size_request(content_scroll, -1, 120);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(content_scroll), page->content_view);
    gtk_box_append(GTK_BOX(root), content_scroll);

    GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    add_button(actions, "Create", G_CALLBACK(on_simple_file), page, "CREATE");
    add_button(actions, "Write", G_CALLBACK(on_write_append), page, "WRITE");
    add_button(actions, "Append", G_CALLBACK(on_write_append), page, "APPEND");
    add_button(actions, "Read", G_CALLBACK(on_simple_file), page, "READ");
    add_button(actions, "Delete", G_CALLBACK(on_delete), page, NULL);
    add_button(actions, "Rename", G_CALLBACK(on_pair), page, "RENAME");
    add_button(actions, "Copy", G_CALLBACK(on_pair), page, "COPY");
    add_button(actions, "Info", G_CALLBACK(on_simple_file), page, "INFO");
    add_button(actions, "List", G_CALLBACK(on_simple_file), page, "LIST");
    gtk_box_append(GTK_BOX(root), actions);

    page->output_view = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(page->output_view), TRUE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(page->output_view), FALSE);
    GtkWidget *output_scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(output_scroll, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(output_scroll), page->output_view);
    gtk_box_append(GTK_BOX(root), output_scroll);
    return root;
}
