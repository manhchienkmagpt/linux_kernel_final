#include "ui_file_page.h"
#include <glib/gstdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>

typedef struct {
    GtkWidget *root;
    GtkWidget *parent_window;
    GtkWidget *path_label;
    GtkWidget *search_entry;
    GtkWidget *list_box;
    char *current_dir;
    LogPage *log;
} FilePage;

typedef struct {
    FilePage *page;
    GtkWidget *entry;
    GtkWidget *check_dir;
} CreateDialogData;

typedef struct {
    FilePage *page;
    GtkWidget *entry;
    char *old_name;
} RenameDialogData;

static void refresh_file_list(FilePage *page);

static void file_page_free(FilePage *page) {
    if (!page) return;
    g_free(page->current_dir);
    g_free(page);
}

static void on_close_dialog(GtkDialog *dialog, int response, gpointer user_data) {
    (void)response;
    (void)user_data;
    gtk_window_destroy(GTK_WINDOW(dialog));
}

static void show_simple_dialog(GtkWidget *parent, const char *title, const char *message) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(title, GTK_WINDOW(parent), GTK_DIALOG_MODAL,
                                                    "OK", GTK_RESPONSE_OK, NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *label = gtk_label_new(message);
    gtk_label_set_wrap(GTK_LABEL(label), TRUE);
    gtk_box_append(GTK_BOX(content), label);
    g_signal_connect(dialog, "response", G_CALLBACK(on_close_dialog), NULL);
    gtk_window_present(GTK_WINDOW(dialog));
}

static char *selected_file_name(FilePage *page) {
    GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(page->list_box));
    if (!row) return NULL;
    const char *name = g_object_get_data(G_OBJECT(row), "file-name");
    return name ? g_strdup(name) : NULL;
}

static char *build_current_path(FilePage *page, const char *name) {
    return g_build_filename(page->current_dir, name, NULL);
}

static GtkWidget *cell_label(const char *text, int width) {
    GtkWidget *label = gtk_label_new(text);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
    gtk_widget_set_size_request(label, width, -1);
    return label;
}

static void add_file_row(FilePage *page, const char *name) {
    char *path = build_current_path(page, name);
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
    gtk_box_append(GTK_BOX(box), cell_label(name, 250));
    gtk_box_append(GTK_BOX(box), cell_label(S_ISDIR(st.st_mode) ? "Folder" : "File", 90));
    gtk_box_append(GTK_BOX(box), cell_label(size, 100));
    gtk_box_append(GTK_BOX(box), cell_label(mode, 80));
    gtk_box_append(GTK_BOX(box), cell_label(mtime, 170));
    g_object_set_data_full(G_OBJECT(row), "file-name", g_strdup(name), g_free);
    gtk_list_box_append(GTK_LIST_BOX(page->list_box), row);
    g_free(path);
}

static gboolean name_matches_filter(FilePage *page, const char *name) {
    const char *filter = gtk_editable_get_text(GTK_EDITABLE(page->search_entry));
    return !filter || !*filter || g_strrstr(name, filter) != NULL;
}

static void refresh_file_list(FilePage *page) {
    GtkWidget *child = gtk_widget_get_first_child(page->list_box);
    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_list_box_remove(GTK_LIST_BOX(page->list_box), child);
        child = next;
    }

    gtk_label_set_text(GTK_LABEL(page->path_label), page->current_dir);
    GDir *dir = g_dir_open(page->current_dir, 0, NULL);
    if (!dir) {
        append_log(page->log, "ERROR", "Cannot open selected folder.");
        return;
    }

    const char *name;
    while ((name = g_dir_read_name(dir)) != NULL) {
        if (name_matches_filter(page, name)) add_file_row(page, name);
    }
    g_dir_close(dir);
    append_log(page->log, "INFO", "File list refreshed.");
}

static void on_folder_response(GtkNativeDialog *dialog, int response, gpointer user_data) {
    FilePage *page = user_data;
    if (response == GTK_RESPONSE_ACCEPT) {
        GFile *file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
        char *path = g_file_get_path(file);
        g_free(page->current_dir);
        page->current_dir = path;
        g_object_unref(file);
        refresh_file_list(page);
    }
    g_object_unref(dialog);
}

static void on_choose_folder(GtkButton *button, gpointer user_data) {
    (void)button;
    FilePage *page = user_data;
    GtkFileChooserNative *dialog = gtk_file_chooser_native_new("Chon thu muc", GTK_WINDOW(page->parent_window),
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, "Chon", "Huy");
    g_signal_connect(dialog, "response", G_CALLBACK(on_folder_response), page);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));
}

static void on_create_response(GtkDialog *dialog, int response, gpointer user_data) {
    CreateDialogData *data = user_data;
    if (response == GTK_RESPONSE_ACCEPT) {
        const char *name = gtk_editable_get_text(GTK_EDITABLE(data->entry));
        if (!name || !*name) {
            append_log(data->page->log, "ERROR", "Create failed: empty name.");
        } else {
            char *path = build_current_path(data->page, name);
            gboolean make_dir = gtk_check_button_get_active(GTK_CHECK_BUTTON(data->check_dir));
            gboolean success = FALSE;
            if (make_dir) {
                success = (g_mkdir(path, 0755) == 0);
            } else {
                success = g_file_set_contents(path, "", 0, NULL);
            }
            if (success) append_log(data->page->log, "OK", "Created item.");
            else append_log(data->page->log, "ERROR", g_strerror(errno));
            g_free(path);
            refresh_file_list(data->page);
        }
    }
    g_free(data);
    gtk_window_destroy(GTK_WINDOW(dialog));
}

static void on_create(GtkButton *button, gpointer user_data) {
    (void)button;
    FilePage *page = user_data;
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Tao file/thu muc", GTK_WINDOW(page->parent_window),
        GTK_DIALOG_MODAL, "Huy", GTK_RESPONSE_CANCEL, "Tao", GTK_RESPONSE_ACCEPT, NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *entry = gtk_entry_new();
    GtkWidget *check = gtk_check_button_new_with_label("Tao thu muc thay vi file");
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Nhap ten file hoac thu muc");
    gtk_box_append(GTK_BOX(content), entry);
    gtk_box_append(GTK_BOX(content), check);

    CreateDialogData *data = g_new0(CreateDialogData, 1);
    data->page = page;
    data->entry = entry;
    data->check_dir = check;
    g_signal_connect(dialog, "response", G_CALLBACK(on_create_response), data);
    gtk_window_present(GTK_WINDOW(dialog));
}

static void on_delete_response(GtkDialog *dialog, int response, gpointer user_data) {
    FilePage *page = g_object_get_data(G_OBJECT(dialog), "page");
    char *name = user_data;
    if (response == GTK_RESPONSE_ACCEPT) {
        char *path = build_current_path(page, name);
        if (g_remove(path) == 0 || g_rmdir(path) == 0) append_log(page->log, "OK", "Deleted selected item.");
        else append_log(page->log, "ERROR", g_strerror(errno));
        g_free(path);
        refresh_file_list(page);
    }
    g_free(name);
    gtk_window_destroy(GTK_WINDOW(dialog));
}

static void on_delete(GtkButton *button, gpointer user_data) {
    (void)button;
    FilePage *page = user_data;
    char *name = selected_file_name(page);
    if (!name) return show_simple_dialog(page->parent_window, "Thong bao", "Hay chon mot file/thu muc.");
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Xac nhan xoa", GTK_WINDOW(page->parent_window),
        GTK_DIALOG_MODAL, "Huy", GTK_RESPONSE_CANCEL, "Xoa", GTK_RESPONSE_ACCEPT, NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    char *msg = g_strdup_printf("Ban co chac muon xoa '%s'?", name);
    gtk_box_append(GTK_BOX(content), gtk_label_new(msg));
    g_free(msg);
    g_object_set_data(G_OBJECT(dialog), "page", page);
    g_signal_connect(dialog, "response", G_CALLBACK(on_delete_response), name);
    gtk_window_present(GTK_WINDOW(dialog));
}

static void on_rename_response(GtkDialog *dialog, int response, gpointer user_data) {
    RenameDialogData *data = user_data;
    if (response == GTK_RESPONSE_ACCEPT) {
        const char *new_name = gtk_editable_get_text(GTK_EDITABLE(data->entry));
        char *old_path = build_current_path(data->page, data->old_name);
        char *new_path = build_current_path(data->page, new_name);
        if (new_name && *new_name && g_rename(old_path, new_path) == 0) append_log(data->page->log, "OK", "Renamed item.");
        else append_log(data->page->log, "ERROR", g_strerror(errno));
        g_free(old_path);
        g_free(new_path);
        refresh_file_list(data->page);
    }
    g_free(data->old_name);
    g_free(data);
    gtk_window_destroy(GTK_WINDOW(dialog));
}

static void on_rename(GtkButton *button, gpointer user_data) {
    (void)button;
    FilePage *page = user_data;
    char *name = selected_file_name(page);
    if (!name) return show_simple_dialog(page->parent_window, "Thong bao", "Hay chon file can doi ten.");
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Doi ten", GTK_WINDOW(page->parent_window),
        GTK_DIALOG_MODAL, "Huy", GTK_RESPONSE_CANCEL, "Doi ten", GTK_RESPONSE_ACCEPT, NULL);
    GtkWidget *entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(entry), name);
    gtk_box_append(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), entry);
    RenameDialogData *data = g_new0(RenameDialogData, 1);
    data->page = page;
    data->entry = entry;
    data->old_name = name;
    g_signal_connect(dialog, "response", G_CALLBACK(on_rename_response), data);
    gtk_window_present(GTK_WINDOW(dialog));
}

static void copy_or_move_file(FilePage *page, const char *name, GFile *dest_dir, gboolean move) {
    char *src_path = build_current_path(page, name);
    char *dest_base = g_file_get_path(dest_dir);
    char *dest_path = g_build_filename(dest_base, name, NULL);
    GFile *src = g_file_new_for_path(src_path);
    GFile *dst = g_file_new_for_path(dest_path);
    GError *error = NULL;
    gboolean ok = move ? g_file_move(src, dst, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &error)
                       : g_file_copy(src, dst, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &error);
    if (ok) append_log(page->log, "OK", move ? "Moved selected file." : "Copied selected file.");
    else {
        append_log(page->log, "ERROR", error->message);
        g_error_free(error);
    }
    g_object_unref(src);
    g_object_unref(dst);
    g_free(src_path);
    g_free(dest_base);
    g_free(dest_path);
    refresh_file_list(page);
}

static void on_dest_response(GtkNativeDialog *dialog, int response, gpointer user_data) {
    FilePage *page = g_object_get_data(G_OBJECT(dialog), "page");
    char *name = g_object_get_data(G_OBJECT(dialog), "file-name");
    gboolean move = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dialog), "move"));
    if (response == GTK_RESPONSE_ACCEPT) {
        GFile *dest = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
        copy_or_move_file(page, name, dest, move);
        g_object_unref(dest);
    }
    g_free(name);
    g_object_unref(dialog);
    (void)user_data;
}

static void choose_destination(FilePage *page, gboolean move) {
    char *name = selected_file_name(page);
    if (!name) return show_simple_dialog(page->parent_window, "Thong bao", "Hay chon file truoc.");
    GtkFileChooserNative *dialog = gtk_file_chooser_native_new("Chon thu muc dich", GTK_WINDOW(page->parent_window),
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, "Chon", "Huy");
    g_object_set_data(G_OBJECT(dialog), "page", page);
    g_object_set_data(G_OBJECT(dialog), "file-name", name);
    g_object_set_data(G_OBJECT(dialog), "move", GINT_TO_POINTER(move));
    g_signal_connect(dialog, "response", G_CALLBACK(on_dest_response), NULL);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));
}

static void on_copy(GtkButton *button, gpointer user_data) { (void)button; choose_destination(user_data, FALSE); }
static void on_move(GtkButton *button, gpointer user_data) { (void)button; choose_destination(user_data, TRUE); }
static void on_refresh(GtkButton *button, gpointer user_data) { (void)button; refresh_file_list(user_data); }
static void on_search_changed(GtkEditable *editable, gpointer user_data) { (void)editable; refresh_file_list(user_data); }

static void on_info(GtkButton *button, gpointer user_data) {
    (void)button;
    FilePage *page = user_data;
    char *name = selected_file_name(page);
    if (!name) return show_simple_dialog(page->parent_window, "Thong tin", "Hay chon file/thu muc.");
    char *path = build_current_path(page, name);
    GStatBuf st;
    if (g_stat(path, &st) == 0) {
        char mtime[64];
        strftime(mtime, sizeof(mtime), "%Y-%m-%d %H:%M:%S", localtime(&st.st_mtime));
        char *info = g_strdup_printf("Ten: %s\nDuong dan: %s\nLoai: %s\nKich thuoc: %ld bytes\nQuyen: %o\nSua doi: %s",
            name, path, S_ISDIR(st.st_mode) ? "Thu muc" : "File", (long)st.st_size, st.st_mode & 0777, mtime);
        show_simple_dialog(page->parent_window, "Thong tin file", info);
        append_log(page->log, "INFO", "Displayed file information.");
        g_free(info);
    }
    g_free(path);
    g_free(name);
}

GtkWidget *ui_file_page_new(GtkWidget *parent_window, LogPage *log_page) {
    FilePage *page = g_new0(FilePage, 1);
    page->parent_window = parent_window;
    page->log = log_page;
    page->current_dir = g_get_current_dir();
    page->root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(page->root, 14);
    gtk_widget_set_margin_bottom(page->root, 14);
    gtk_widget_set_margin_start(page->root, 14);
    gtk_widget_set_margin_end(page->root, 14);
    g_object_set_data_full(G_OBJECT(page->root), "page-data", page, (GDestroyNotify)file_page_free);

    GtkWidget *title = gtk_label_new("File Manager");
    gtk_widget_add_css_class(title, "title-3");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(page->root), title);

    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *choose = gtk_button_new_with_label("Chon thu muc");
    GtkWidget *refresh = gtk_button_new_with_label("Refresh");
    page->search_entry = gtk_search_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(page->search_entry), "");
    gtk_box_append(GTK_BOX(toolbar), choose);
    gtk_box_append(GTK_BOX(toolbar), refresh);
    gtk_box_append(GTK_BOX(toolbar), page->search_entry);
    gtk_box_append(GTK_BOX(page->root), toolbar);

    page->path_label = gtk_label_new(page->current_dir);
    gtk_label_set_xalign(GTK_LABEL(page->path_label), 0.0f);
    gtk_widget_add_css_class(page->path_label, "dim-label");
    gtk_box_append(GTK_BOX(page->root), page->path_label);

    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_add_css_class(header, "heading");
    gtk_box_append(GTK_BOX(header), cell_label("Ten", 250));
    gtk_box_append(GTK_BOX(header), cell_label("Loai", 90));
    gtk_box_append(GTK_BOX(header), cell_label("Kich thuoc", 100));
    gtk_box_append(GTK_BOX(header), cell_label("Quyen", 80));
    gtk_box_append(GTK_BOX(header), cell_label("Thoi gian sua doi", 170));
    gtk_box_append(GTK_BOX(page->root), header);

    page->list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(page->list_box), GTK_SELECTION_SINGLE);
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), page->list_box);
    gtk_box_append(GTK_BOX(page->root), scroll);

    GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    const char *labels[] = {"Tao", "Xoa", "Doi ten", "Copy", "Move", "Thong tin"};
    GCallback callbacks[] = {G_CALLBACK(on_create), G_CALLBACK(on_delete), G_CALLBACK(on_rename),
                             G_CALLBACK(on_copy), G_CALLBACK(on_move), G_CALLBACK(on_info)};
    for (int i = 0; i < 6; i++) {
        GtkWidget *btn = gtk_button_new_with_label(labels[i]);
        g_signal_connect(btn, "clicked", callbacks[i], page);
        gtk_box_append(GTK_BOX(actions), btn);
    }
    gtk_box_append(GTK_BOX(page->root), actions);

    g_signal_connect(choose, "clicked", G_CALLBACK(on_choose_folder), page);
    g_signal_connect(refresh, "clicked", G_CALLBACK(on_refresh), page);
    g_signal_connect(page->search_entry, "changed", G_CALLBACK(on_search_changed), page);
    refresh_file_list(page);
    return page->root;
}
