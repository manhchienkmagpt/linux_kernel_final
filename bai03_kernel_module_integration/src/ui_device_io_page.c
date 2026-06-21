#include "ui_device_io_page.h"
#include "kernel_module_commands.h"

typedef struct {
    AppContext *ctx;
    GtkWidget *root_entry;
    GtkWidget *filename_entry;
    GtkWidget *target_entry;
    GtkWidget *content_view;
    GtkWidget *output_view;
    GtkWidget *file_list;
    GtkWidget *status_label;
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

static void set_text_view(GtkWidget *view, const char *text) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    gtk_text_buffer_set_text(buffer, text ? text : "", -1);
}

static void set_output(FileManagerPage *page, const char *text) {
    set_text_view(page->output_view, text);
}

static void clear_file_list(FileManagerPage *page) {
    GtkWidget *child = gtk_widget_get_first_child(page->file_list);
    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_list_box_remove(GTK_LIST_BOX(page->file_list), child);
        child = next;
    }
}

static void add_file_row(FileManagerPage *page, const char *filename) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *label = gtk_label_new(filename);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    gtk_widget_set_margin_top(label, 6);
    gtk_widget_set_margin_bottom(label, 6);
    gtk_widget_set_margin_start(label, 8);
    gtk_widget_set_margin_end(label, 8);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
    g_object_set_data_full(G_OBJECT(row), "filename", g_strdup(filename), g_free);
    gtk_list_box_append(GTK_LIST_BOX(page->file_list), row);
}

static void populate_file_list(FileManagerPage *page, const char *list_result) {
    clear_file_list(page);

    char **lines = g_strsplit(list_result ? list_result : "", "\n", -1);
    int count = 0;
    for (int i = 0; lines[i]; i++) {
        if (!lines[i][0]) continue;
        if (g_str_has_prefix(lines[i], "LIST ")) continue;
        if (g_str_has_prefix(lines[i], "ERROR ")) continue;
        add_file_row(page, lines[i]);
        count++;
    }
    g_strfreev(lines);

    char *status = g_strdup_printf("%d file(s)", count);
    gtk_label_set_text(GTK_LABEL(page->status_label), status);
    g_free(status);
}

static char *run_command(FileManagerPage *page, const char *command, gboolean refresh_after) {
    gboolean ok = FALSE;
    char *result = send_kfile_command(GTK_WINDOW(page->ctx->window), command, &ok);
    set_output(page, result);

    if (refresh_after) {
        gboolean list_ok = FALSE;
        char *list = send_kfile_command(GTK_WINDOW(page->ctx->window), "LIST", &list_ok);
        if (list_ok) populate_file_list(page, list);
        g_free(list);
    }

    return result;
}

static void refresh_file_list(FileManagerPage *page) {
    char *result = run_command(page, "LIST", FALSE);
    populate_file_list(page, result);
    g_free(result);
}

static void on_file_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    (void)box;
    FileManagerPage *page = user_data;
    if (!row) return;

    const char *filename = g_object_get_data(G_OBJECT(row), "filename");
    if (filename) {
        gtk_editable_set_text(GTK_EDITABLE(page->filename_entry), filename);
        gtk_label_set_text(GTK_LABEL(page->status_label), filename);
    }
}

static void on_set_root(GtkButton *button, gpointer user_data) {
    (void)button;
    FileManagerPage *page = user_data;
    char *cmd = g_strdup_printf("SET_ROOT %s", entry_text(page->root_entry));
    char *result = run_command(page, cmd, TRUE);
    g_free(result);
    g_free(cmd);
}

static void folder_dialog_response(GtkNativeDialog *dialog, int response, gpointer user_data) {
    FileManagerPage *page = user_data;
    if (response == GTK_RESPONSE_ACCEPT) {
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        GFile *folder = gtk_file_chooser_get_file(chooser);
        if (folder) {
            char *path = g_file_get_path(folder);
            if (path) {
                gtk_editable_set_text(GTK_EDITABLE(page->root_entry), path);
                char *cmd = g_strdup_printf("SET_ROOT %s", path);
                char *result = run_command(page, cmd, TRUE);
                g_free(result);
                g_free(cmd);
                g_free(path);
            }
            g_object_unref(folder);
        }
    }
    g_object_unref(dialog);
}

static void on_choose_root(GtkButton *button, gpointer user_data) {
    (void)button;
    FileManagerPage *page = user_data;
    GtkFileChooserNative *dialog = gtk_file_chooser_native_new("Choose Root Directory",
        GTK_WINDOW(page->ctx->window), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        "Choose", "Cancel");
    g_signal_connect(dialog, "response", G_CALLBACK(folder_dialog_response), page);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));
}

static void on_refresh(GtkButton *button, gpointer user_data) {
    (void)button;
    refresh_file_list(user_data);
}

static void on_create(GtkButton *button, gpointer user_data) {
    (void)button;
    FileManagerPage *page = user_data;
    char *cmd = g_strdup_printf("CREATE %s", entry_text(page->filename_entry));
    char *result = run_command(page, cmd, TRUE);
    g_free(result);
    g_free(cmd);
}

static void on_read(GtkButton *button, gpointer user_data) {
    (void)button;
    FileManagerPage *page = user_data;
    char *cmd = g_strdup_printf("READ %s", entry_text(page->filename_entry));
    char *result = run_command(page, cmd, FALSE);

    char *body = strchr(result ? result : "", '\n');
    if (body) set_text_view(page->content_view, body + 1);

    g_free(result);
    g_free(cmd);
}

static void on_write_append(GtkButton *button, gpointer user_data) {
    FileManagerPage *page = g_object_get_data(G_OBJECT(button), "page");
    const char *verb = g_object_get_data(G_OBJECT(button), "verb");
    char *content = text_view_text(page->content_view);
    char *cmd = g_strdup_printf("%s %s %s", verb, entry_text(page->filename_entry), content);
    char *result = run_command(page, cmd, TRUE);
    g_free(result);
    g_free(cmd);
    g_free(content);
    (void)user_data;
}

static void confirm_delete_response(GtkDialog *dialog, int response, gpointer user_data) {
    FileManagerPage *page = user_data;
    if (response == GTK_RESPONSE_ACCEPT) {
        char *cmd = g_strdup_printf("DELETE %s", entry_text(page->filename_entry));
        char *result = run_command(page, cmd, TRUE);
        g_free(result);
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
        gtk_label_new("Delete selected file from the managed root directory?"));
    g_signal_connect(dialog, "response", G_CALLBACK(confirm_delete_response), page);
    gtk_window_present(GTK_WINDOW(dialog));
}

static void on_pair(GtkButton *button, gpointer user_data) {
    FileManagerPage *page = g_object_get_data(G_OBJECT(button), "page");
    const char *verb = g_object_get_data(G_OBJECT(button), "verb");
    char *cmd = g_strdup_printf("%s %s %s", verb,
        entry_text(page->filename_entry), entry_text(page->target_entry));
    char *result = run_command(page, cmd, TRUE);
    g_free(result);
    g_free(cmd);
    (void)user_data;
}

static void on_info(GtkButton *button, gpointer user_data) {
    (void)button;
    FileManagerPage *page = user_data;
    char *cmd = g_strdup_printf("INFO %s", entry_text(page->filename_entry));
    char *result = run_command(page, cmd, FALSE);
    g_free(result);
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

    GtkWidget *root_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    page->root_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(page->root_entry), "/tmp/kfile_manager_root");
    gtk_editable_set_text(GTK_EDITABLE(page->root_entry), "/tmp/kfile_manager_root");
    gtk_widget_set_hexpand(page->root_entry, TRUE);
    GtkWidget *choose_root = gtk_button_new_with_label("Choose Root");
    GtkWidget *set_root = gtk_button_new_with_label("Set Root");
    GtkWidget *refresh = gtk_button_new_with_label("Refresh");
    g_signal_connect(choose_root, "clicked", G_CALLBACK(on_choose_root), page);
    g_signal_connect(set_root, "clicked", G_CALLBACK(on_set_root), page);
    g_signal_connect(refresh, "clicked", G_CALLBACK(on_refresh), page);
    gtk_box_append(GTK_BOX(root_row), page->root_entry);
    gtk_box_append(GTK_BOX(root_row), choose_root);
    gtk_box_append(GTK_BOX(root_row), set_root);
    gtk_box_append(GTK_BOX(root_row), refresh);
    gtk_box_append(GTK_BOX(root), root_row);

    GtkWidget *main = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_vexpand(main, TRUE);
    gtk_box_append(GTK_BOX(root), main);

    GtkWidget *left = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_end(left, 8);
    page->status_label = gtk_label_new("Choose or set a root directory");
    gtk_label_set_xalign(GTK_LABEL(page->status_label), 0.0f);
    gtk_box_append(GTK_BOX(left), page->status_label);
    page->file_list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(page->file_list), GTK_SELECTION_SINGLE);
    g_signal_connect(page->file_list, "row-selected", G_CALLBACK(on_file_selected), page);
    GtkWidget *list_scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(list_scroll, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(list_scroll), page->file_list);
    gtk_box_append(GTK_BOX(left), list_scroll);
    gtk_paned_set_start_child(GTK_PANED(main), left);

    GtkWidget *right = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    page->filename_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(page->filename_entry), "Selected or new filename, e.g. test.txt");
    page->target_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(page->target_entry), "Target filename for rename/copy");
    gtk_box_append(GTK_BOX(right), page->filename_entry);
    gtk_box_append(GTK_BOX(right), page->target_entry);

    page->content_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(page->content_view), GTK_WRAP_WORD_CHAR);
    GtkWidget *content_scroll = gtk_scrolled_window_new();
    gtk_widget_set_size_request(content_scroll, -1, 160);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(content_scroll), page->content_view);
    gtk_box_append(GTK_BOX(right), content_scroll);

    GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    add_button(actions, "Create", G_CALLBACK(on_create), page, NULL);
    add_button(actions, "Read", G_CALLBACK(on_read), page, NULL);
    add_button(actions, "Write", G_CALLBACK(on_write_append), page, "WRITE");
    add_button(actions, "Append", G_CALLBACK(on_write_append), page, "APPEND");
    add_button(actions, "Delete", G_CALLBACK(on_delete), page, NULL);
    add_button(actions, "Rename", G_CALLBACK(on_pair), page, "RENAME");
    add_button(actions, "Copy", G_CALLBACK(on_pair), page, "COPY");
    add_button(actions, "Info", G_CALLBACK(on_info), page, NULL);
    gtk_box_append(GTK_BOX(right), actions);

    page->output_view = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(page->output_view), TRUE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(page->output_view), FALSE);
    GtkWidget *output_scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(output_scroll, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(output_scroll), page->output_view);
    gtk_box_append(GTK_BOX(right), output_scroll);
    gtk_paned_set_end_child(GTK_PANED(main), right);
    gtk_paned_set_position(GTK_PANED(main), 320);

    return root;
}
