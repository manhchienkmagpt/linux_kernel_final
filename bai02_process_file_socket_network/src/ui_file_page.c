#include "ui_file_page.h"
#include "file_syscall.h"
#include <errno.h>
#include <fcntl.h>
#include <glib/gstdio.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    AppContext *ctx;
    GtkWidget *path_label;
    GtkWidget *list_box;
    char *current_dir;
} FilePage;

typedef struct {
    FilePage *page;
    GtkWidget *text_view;
    char *path;
} FileDialogData;

static void refresh_files(FilePage *page);

static void file_page_free(FilePage *page) {
    if (!page) return;
    g_free(page->current_dir);
    g_free(page);
}

static GtkWidget *cell_label(const char *text, int width) {
    GtkWidget *label = gtk_label_new(text);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
    gtk_widget_set_size_request(label, width, -1);
    return label;
}

static char *selected_name(FilePage *page) {
    GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(page->list_box));
    const char *name = row ? g_object_get_data(G_OBJECT(row), "file-name") : NULL;
    return name ? g_strdup(name) : NULL;
}

static char *selected_path(FilePage *page) {
    char *name = selected_name(page);
    if (!name) return NULL;
    char *path = g_build_filename(page->current_dir, name, NULL);
    g_free(name);
    return path;
}

static void add_file_row(FilePage *page, const char *name) {
    char *path = g_build_filename(page->current_dir, name, NULL);
    GStatBuf st;
    if (g_stat(path, &st) != 0) {
        g_free(path);
        return;
    }

    char size[32], mode[16], mtime[64];
    g_snprintf(size, sizeof(size), "%ld", (long)st.st_size);
    g_snprintf(mode, sizeof(mode), "%o", st.st_mode & 0777);
    strftime(mtime, sizeof(mtime), "%Y-%m-%d %H:%M", localtime(&st.st_mtime));

    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_top(box, 5);
    gtk_widget_set_margin_bottom(box, 5);
    gtk_widget_set_margin_start(box, 8);
    gtk_widget_set_margin_end(box, 8);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), box);
    gtk_box_append(GTK_BOX(box), cell_label(name, 260));
    gtk_box_append(GTK_BOX(box), cell_label(S_ISDIR(st.st_mode) ? "Folder" : "File", 90));
    gtk_box_append(GTK_BOX(box), cell_label(size, 100));
    gtk_box_append(GTK_BOX(box), cell_label(mode, 90));
    gtk_box_append(GTK_BOX(box), cell_label(mtime, 170));
    g_object_set_data_full(G_OBJECT(row), "file-name", g_strdup(name), g_free);
    gtk_list_box_append(GTK_LIST_BOX(page->list_box), row);
    g_free(path);
}

static void refresh_files(FilePage *page) {
    GtkWidget *child = gtk_widget_get_first_child(page->list_box);
    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_list_box_remove(GTK_LIST_BOX(page->list_box), child);
        child = next;
    }
    gtk_label_set_text(GTK_LABEL(page->path_label), page->current_dir);

    GDir *dir = g_dir_open(page->current_dir, 0, NULL);
    if (!dir) {
        log_page_append(page->ctx->log_page, "ERROR", "Cannot open current folder.");
        return;
    }
    const char *name;
    while ((name = g_dir_read_name(dir)) != NULL) add_file_row(page, name);
    g_dir_close(dir);
    log_page_append(page->ctx->log_page, "INFO", "File list refreshed.");
}

static void on_folder_response(GtkNativeDialog *dialog, int response, gpointer user_data) {
    FilePage *page = user_data;
    if (response == GTK_RESPONSE_ACCEPT) {
        GFile *file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
        char *path = g_file_get_path(file);
        g_free(page->current_dir);
        page->current_dir = path;
        g_object_unref(file);
        refresh_files(page);
    }
    g_object_unref(dialog);
}

static void on_choose_folder(GtkButton *button, gpointer user_data) {
    (void)button;
    FilePage *page = user_data;
    GtkFileChooserNative *dialog = gtk_file_chooser_native_new("Choose Folder", GTK_WINDOW(page->ctx->window),
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, "Choose", "Cancel");
    g_signal_connect(dialog, "response", G_CALLBACK(on_folder_response), page);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));
}

static void on_refresh(GtkButton *button, gpointer user_data) {
    (void)button;
    refresh_files(user_data);
}

static void on_open(GtkButton *button, gpointer user_data) {
    (void)button;
    FilePage *page = user_data;
    char *path = selected_path(page);
    if (!path) {
        log_page_append(page->ctx->log_page, "ERROR", "Select a file or folder first.");
        return;
    }
    char *uri = g_filename_to_uri(path, NULL, NULL);
    GError *error = NULL;
    if (!uri || !g_app_info_launch_default_for_uri(uri, NULL, &error)) {
        log_page_append(page->ctx->log_page, "ERROR", error ? error->message : "Cannot open selected item.");
    } else {
        log_page_append(page->ctx->log_page, "OK", "Open requested for selected item.");
    }
    if (error) g_error_free(error);
    g_free(uri);
    g_free(path);
}

static void dialog_data_free(FileDialogData *data) {
    if (!data) return;
    g_free(data->path);
    g_free(data);
}

static char *text_view_content(GtkWidget *view) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    return gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
}

static void on_read_close(GtkDialog *dialog, int response, gpointer user_data) {
    (void)response;
    dialog_data_free(user_data);
    gtk_window_destroy(GTK_WINDOW(dialog));
}

static void on_read(GtkButton *button, gpointer user_data) {
    (void)button;
    FilePage *page = user_data;
    char *path = selected_path(page);
    if (!path) {
        log_page_append(page->ctx->log_page, "ERROR", "Select a file before reading.");
        return;
    }
    char *content = read_file_syscall(path);
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Read File", GTK_WINDOW(page->ctx->window),
        GTK_DIALOG_MODAL, "Close", GTK_RESPONSE_CLOSE, NULL);
    GtkWidget *view = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(view), TRUE);
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)), content, -1);
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_size_request(scroll, 650, 420);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), view);
    gtk_box_append(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), scroll);
    FileDialogData *data = g_new0(FileDialogData, 1);
    data->page = page;
    data->path = path;
    log_page_append(page->ctx->log_page, "OK", "Read file with open/read/close.");
    g_signal_connect(dialog, "response", G_CALLBACK(on_read_close), data);
    gtk_window_present(GTK_WINDOW(dialog));
    g_free(content);
}

static void on_write_response(GtkDialog *dialog, int response, gpointer user_data) {
    FileDialogData *data = user_data;
    if (response == GTK_RESPONSE_ACCEPT) {
        char *content = text_view_content(data->text_view);
        char *msg = NULL;
        int ok = write_file_syscall(data->path, content, &msg);
        log_page_append(data->page->ctx->log_page, ok == 0 ? "OK" : "ERROR", msg);
        g_free(content);
        g_free(msg);
        refresh_files(data->page);
    }
    dialog_data_free(data);
    gtk_window_destroy(GTK_WINDOW(dialog));
}

static void on_write(GtkButton *button, gpointer user_data) {
    (void)button;
    FilePage *page = user_data;
    char *path = selected_path(page);
    if (!path) {
        log_page_append(page->ctx->log_page, "ERROR", "Select a file before writing.");
        return;
    }
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Write File", GTK_WINDOW(page->ctx->window),
        GTK_DIALOG_MODAL, "Cancel", GTK_RESPONSE_CANCEL, "Write", GTK_RESPONSE_ACCEPT, NULL);
    GtkWidget *view = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(view), TRUE);
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_size_request(scroll, 650, 420);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), view);
    gtk_box_append(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), scroll);
    FileDialogData *data = g_new0(FileDialogData, 1);
    data->page = page;
    data->text_view = view;
    data->path = path;
    g_signal_connect(dialog, "response", G_CALLBACK(on_write_response), data);
    gtk_window_present(GTK_WINDOW(dialog));
}

static void on_delete_response(GtkDialog *dialog, int response, gpointer user_data) {
    FilePage *page = user_data;
    char *path = g_object_get_data(G_OBJECT(dialog), "path");
    if (response == GTK_RESPONSE_ACCEPT) {
        if (g_remove(path) == 0) log_page_append(page->ctx->log_page, "OK", "Deleted selected file.");
        else log_page_append(page->ctx->log_page, "ERROR", g_strerror(errno));
        refresh_files(page);
    }
    g_free(path);
    gtk_window_destroy(GTK_WINDOW(dialog));
}

static void on_delete(GtkButton *button, gpointer user_data) {
    (void)button;
    FilePage *page = user_data;
    char *path = selected_path(page);
    if (!path) {
        log_page_append(page->ctx->log_page, "ERROR", "Select a file before deleting.");
        return;
    }
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Confirm Delete", GTK_WINDOW(page->ctx->window),
        GTK_DIALOG_MODAL, "Cancel", GTK_RESPONSE_CANCEL, "Delete", GTK_RESPONSE_ACCEPT, NULL);
    gtk_box_append(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                   gtk_label_new("Delete selected file? This cannot be undone."));
    g_object_set_data(G_OBJECT(dialog), "path", path);
    g_signal_connect(dialog, "response", G_CALLBACK(on_delete_response), page);
    gtk_window_present(GTK_WINDOW(dialog));
}

GtkWidget *ui_file_page_new(AppContext *ctx) {
    FilePage *page = g_new0(FilePage, 1);
    page->ctx = ctx;
    page->current_dir = g_get_current_dir();

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(root, 14);
    gtk_widget_set_margin_bottom(root, 14);
    gtk_widget_set_margin_start(root, 14);
    gtk_widget_set_margin_end(root, 14);
    g_object_set_data_full(G_OBJECT(root), "page-data", page, (GDestroyNotify)file_page_free);

    GtkWidget *title = gtk_label_new("Files");
    gtk_widget_add_css_class(title, "title-2");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(root), title);

    page->path_label = gtk_label_new(page->current_dir);
    gtk_label_set_xalign(GTK_LABEL(page->path_label), 0.0f);
    gtk_widget_add_css_class(page->path_label, "dim-label");
    gtk_box_append(GTK_BOX(root), page->path_label);

    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *choose = gtk_button_new_with_label("Choose Folder");
    GtkWidget *refresh = gtk_button_new_with_label("Refresh");
    gtk_box_append(GTK_BOX(toolbar), choose);
    gtk_box_append(GTK_BOX(toolbar), refresh);
    gtk_box_append(GTK_BOX(root), toolbar);

    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_add_css_class(header, "heading");
    gtk_box_append(GTK_BOX(header), cell_label("Name", 260));
    gtk_box_append(GTK_BOX(header), cell_label("Type", 90));
    gtk_box_append(GTK_BOX(header), cell_label("Size", 100));
    gtk_box_append(GTK_BOX(header), cell_label("Permission", 90));
    gtk_box_append(GTK_BOX(header), cell_label("Modified Time", 170));
    gtk_box_append(GTK_BOX(root), header);

    page->list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(page->list_box), GTK_SELECTION_SINGLE);
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), page->list_box);
    gtk_box_append(GTK_BOX(root), scroll);

    GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *open = gtk_button_new_with_label("Open");
    GtkWidget *read = gtk_button_new_with_label("Read");
    GtkWidget *write = gtk_button_new_with_label("Write");
    GtkWidget *del = gtk_button_new_with_label("Delete");
    gtk_box_append(GTK_BOX(actions), open);
    gtk_box_append(GTK_BOX(actions), read);
    gtk_box_append(GTK_BOX(actions), write);
    gtk_box_append(GTK_BOX(actions), del);
    gtk_box_append(GTK_BOX(root), actions);

    g_signal_connect(choose, "clicked", G_CALLBACK(on_choose_folder), page);
    g_signal_connect(refresh, "clicked", G_CALLBACK(on_refresh), page);
    g_signal_connect(open, "clicked", G_CALLBACK(on_open), page);
    g_signal_connect(read, "clicked", G_CALLBACK(on_read), page);
    g_signal_connect(write, "clicked", G_CALLBACK(on_write), page);
    g_signal_connect(del, "clicked", G_CALLBACK(on_delete), page);
    refresh_files(page);
    return root;
}
