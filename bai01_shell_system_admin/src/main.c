#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>

typedef struct {
    GtkWidget *window;
    GtkWidget *folder_entry;
    GtkWidget *name_entry;
    GtkWidget *new_name_entry;
    GtkWidget *dest_entry;
    GtkWidget *search_entry;
    GtkWidget *file_view;
    GtkListStore *file_store;
    GtkWidget *file_info;
    GtkWidget *cron_expr_entry;
    GtkWidget *cron_cmd_entry;
    GtkWidget *cron_line_entry;
    GtkWidget *time_entry;
    GtkWidget *pkg_entry;
    GtkWidget *log_view;
} App;

static void append_log(App *app, const char *msg) {
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_view));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buf, &end);
    gtk_text_buffer_insert(buf, &end, msg, -1);
    gtk_text_buffer_insert(buf, &end, "\n", -1);
}

static void show_message(App *app, GtkMessageType type, const char *msg) {
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL, type, GTK_BUTTONS_OK, "%s", msg);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static char *shell_quote(const char *s) {
    return g_shell_quote(s ? s : "");
}

static gboolean run_script(App *app, const char *args) {
    char *cmd = g_strdup_printf("bash scripts/system_ops.sh %s", args);
    char *out = NULL, *err = NULL;
    int status = 0;
    GError *error = NULL;
    gboolean ok = g_spawn_command_line_sync(cmd, &out, &err, &status, &error);
    append_log(app, cmd);
    if (!ok) {
        append_log(app, error->message);
        show_message(app, GTK_MESSAGE_ERROR, error->message);
        g_error_free(error);
    } else {
        if (out && *out) append_log(app, out);
        if (err && *err) append_log(app, err);
        if (status != 0) show_message(app, GTK_MESSAGE_ERROR, "Command failed. See log.");
    }
    g_free(out); g_free(err); g_free(cmd);
    return ok && status == 0;
}

static char *current_folder(App *app) {
    const char *folder = gtk_entry_get_text(GTK_ENTRY(app->folder_entry));
    if (!folder || !*folder) return g_get_current_dir();
    return g_strdup(folder);
}

static void add_file_row(App *app, const char *folder, const char *name) {
    char *path = g_build_filename(folder, name, NULL);
    GStatBuf st;
    if (g_stat(path, &st) == 0) {
        GtkTreeIter it;
        char mode[16], size[32], mtime[64];
        strftime(mtime, sizeof(mtime), "%Y-%m-%d %H:%M", localtime(&st.st_mtime));
        g_snprintf(mode, sizeof(mode), "%o", st.st_mode & 0777);
        g_snprintf(size, sizeof(size), "%ld", (long)st.st_size);
        gtk_list_store_append(app->file_store, &it);
        gtk_list_store_set(app->file_store, &it, 0, name, 1, S_ISDIR(st.st_mode) ? "dir" : "file",
                           2, size, 3, mode, 4, mtime, -1);
    }
    g_free(path);
}

static void refresh_file_list(App *app) {
    char *folder = current_folder(app);
    GDir *dir = g_dir_open(folder, 0, NULL);
    gtk_list_store_clear(app->file_store);
    if (!dir) {
        show_message(app, GTK_MESSAGE_ERROR, "Cannot open folder.");
        g_free(folder);
        return;
    }
    const char *name;
    while ((name = g_dir_read_name(dir)) != NULL) add_file_row(app, folder, name);
    g_dir_close(dir);
    append_log(app, "File list refreshed.");
    g_free(folder);
}

static char *path_from_name(App *app, const char *name) {
    char *folder = current_folder(app);
    char *path = g_build_filename(folder, name, NULL);
    g_free(folder);
    return path;
}

static void set_file_info(App *app, const char *text) {
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->file_info));
    gtk_text_buffer_set_text(buf, text, -1);
}

static void show_file_info(App *app, const char *name) {
    char *path = path_from_name(app, name);
    GStatBuf st;
    if (g_stat(path, &st) != 0) {
        show_message(app, GTK_MESSAGE_ERROR, g_strerror(errno));
        g_free(path);
        return;
    }
    char mtime[64];
    strftime(mtime, sizeof(mtime), "%Y-%m-%d %H:%M:%S", localtime(&st.st_mtime));
    char *info = g_strdup_printf("Name: %s\nPath: %s\nSize: %ld bytes\nPermission: %o\nModified: %s\nType: %s",
        name, path, (long)st.st_size, st.st_mode & 0777, mtime, S_ISDIR(st.st_mode) ? "Directory" : "File");
    set_file_info(app, info);
    g_free(info); g_free(path);
}

static void on_selection_changed(GtkTreeSelection *sel, gpointer data) {
    App *app = data;
    GtkTreeIter it;
    gchar *name = NULL;
    if (gtk_tree_selection_get_selected(sel, NULL, &it)) {
        gtk_tree_model_get(GTK_TREE_MODEL(app->file_store), &it, 0, &name, -1);
        show_file_info(app, name);
        g_free(name);
    }
}

static void on_browse_clicked(GtkButton *btn, gpointer data) {
    (void)btn;
    App *app = data;
    GtkWidget *dlg = gtk_file_chooser_dialog_new("Choose Folder", GTK_WINDOW(app->window),
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);
    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
        char *folder = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
        gtk_entry_set_text(GTK_ENTRY(app->folder_entry), folder);
        g_free(folder);
        refresh_file_list(app);
    }
    gtk_widget_destroy(dlg);
}

static void on_refresh_clicked(GtkButton *btn, gpointer data) { (void)btn; refresh_file_list(data); }

static void on_create_file_clicked(GtkButton *btn, gpointer data) {
    (void)btn;
    App *app = data;
    const char *name = gtk_entry_get_text(GTK_ENTRY(app->name_entry));
    if (!*name) return show_message(app, GTK_MESSAGE_ERROR, "Enter a file name.");
    char *path = path_from_name(app, name);
    GError *err = NULL;
    if (!g_file_set_contents(path, "", 0, &err)) {
        show_message(app, GTK_MESSAGE_ERROR, err->message);
        g_error_free(err);
    } else append_log(app, "Created file.");
    g_free(path);
    refresh_file_list(app);
}

static void on_create_dir_clicked(GtkButton *btn, gpointer data) {
    (void)btn;
    App *app = data;
    const char *name = gtk_entry_get_text(GTK_ENTRY(app->name_entry));
    char *path = path_from_name(app, name);
    if (!*name || g_mkdir(path, 0755) != 0) show_message(app, GTK_MESSAGE_ERROR, g_strerror(errno));
    else append_log(app, "Created directory.");
    g_free(path);
    refresh_file_list(app);
}

static void on_delete_clicked(GtkButton *btn, gpointer data) {
    (void)btn;
    App *app = data;
    const char *name = gtk_entry_get_text(GTK_ENTRY(app->name_entry));
    char *path = path_from_name(app, name);
    if (!*name || (g_remove(path) != 0 && g_rmdir(path) != 0)) show_message(app, GTK_MESSAGE_ERROR, g_strerror(errno));
    else append_log(app, "Deleted item.");
    g_free(path);
    refresh_file_list(app);
}

static void on_rename_clicked(GtkButton *btn, gpointer data) {
    (void)btn;
    App *app = data;
    const char *name = gtk_entry_get_text(GTK_ENTRY(app->name_entry));
    const char *new_name = gtk_entry_get_text(GTK_ENTRY(app->new_name_entry));
    char *oldp = path_from_name(app, name);
    char *newp = path_from_name(app, new_name);
    if (!*name || !*new_name || g_rename(oldp, newp) != 0) show_message(app, GTK_MESSAGE_ERROR, g_strerror(errno));
    else append_log(app, "Renamed item.");
    g_free(oldp); g_free(newp);
    refresh_file_list(app);
}

static void on_copy_clicked(GtkButton *btn, gpointer data) {
    (void)btn;
    App *app = data;
    const char *name = gtk_entry_get_text(GTK_ENTRY(app->name_entry));
    const char *dest = gtk_entry_get_text(GTK_ENTRY(app->dest_entry));
    char *src_path = path_from_name(app, name);
    GFile *src = g_file_new_for_path(src_path);
    GFile *dst = g_file_new_for_path(dest);
    GError *err = NULL;
    if (!*name || !*dest || !g_file_copy(src, dst, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &err)) {
        show_message(app, GTK_MESSAGE_ERROR, err ? err->message : "Copy failed.");
        if (err) g_error_free(err);
    } else append_log(app, "Copied file.");
    g_object_unref(src); g_object_unref(dst); g_free(src_path);
    refresh_file_list(app);
}

static void on_move_clicked(GtkButton *btn, gpointer data) {
    (void)btn;
    App *app = data;
    const char *name = gtk_entry_get_text(GTK_ENTRY(app->name_entry));
    const char *dest = gtk_entry_get_text(GTK_ENTRY(app->dest_entry));
    char *src = path_from_name(app, name);
    if (!*name || !*dest || g_rename(src, dest) != 0) show_message(app, GTK_MESSAGE_ERROR, g_strerror(errno));
    else append_log(app, "Moved item.");
    g_free(src);
    refresh_file_list(app);
}

static void on_search_clicked(GtkButton *btn, gpointer data) {
    (void)btn;
    App *app = data;
    const char *key = gtk_entry_get_text(GTK_ENTRY(app->search_entry));
    char *folder = current_folder(app);
    GDir *dir = g_dir_open(folder, 0, NULL);
    gtk_list_store_clear(app->file_store);
    if (dir) {
        const char *name;
        while ((name = g_dir_read_name(dir)) != NULL)
            if (g_strrstr(name, key)) add_file_row(app, folder, name);
        g_dir_close(dir);
    }
    g_free(folder);
}

static void on_cron_list_clicked(GtkButton *btn, gpointer data) { (void)btn; run_script(data, "cron-list"); }
static void on_time_now_clicked(GtkButton *btn, gpointer data) { (void)btn; run_script(data, "time-now"); }

static void on_cron_add_clicked(GtkButton *btn, gpointer data) {
    (void)btn;
    App *app = data;
    char *expr = shell_quote(gtk_entry_get_text(GTK_ENTRY(app->cron_expr_entry)));
    char *cmd = shell_quote(gtk_entry_get_text(GTK_ENTRY(app->cron_cmd_entry)));
    char *args = g_strdup_printf("cron-add %s %s", expr, cmd);
    run_script(app, args);
    g_free(expr); g_free(cmd); g_free(args);
}

static void on_cron_remove_clicked(GtkButton *btn, gpointer data) {
    (void)btn;
    App *app = data;
    char *line = shell_quote(gtk_entry_get_text(GTK_ENTRY(app->cron_line_entry)));
    char *args = g_strdup_printf("cron-remove-line %s", line);
    run_script(app, args);
    g_free(line); g_free(args);
}

static void on_time_set_clicked(GtkButton *btn, gpointer data) {
    (void)btn;
    App *app = data;
    char *t = shell_quote(gtk_entry_get_text(GTK_ENTRY(app->time_entry)));
    char *args = g_strdup_printf("time-set %s", t);
    run_script(app, args);
    g_free(t); g_free(args);
}

static void on_apt_install_clicked(GtkButton *btn, gpointer data) {
    (void)btn;
    App *app = data;
    char *pkg = shell_quote(gtk_entry_get_text(GTK_ENTRY(app->pkg_entry)));
    char *args = g_strdup_printf("apt-install %s", pkg);
    run_script(app, args);
    g_free(pkg); g_free(args);
}

static void on_apt_remove_clicked(GtkButton *btn, gpointer data) {
    (void)btn;
    App *app = data;
    char *pkg = shell_quote(gtk_entry_get_text(GTK_ENTRY(app->pkg_entry)));
    char *args = g_strdup_printf("apt-remove %s", pkg);
    run_script(app, args);
    g_free(pkg); g_free(args);
}

static GtkWidget *labeled_entry(const char *label, GtkWidget **entry) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(box), gtk_label_new(label), FALSE, FALSE, 0);
    *entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(box), *entry, TRUE, TRUE, 0);
    return box;
}

static GtkWidget *build_file_tab(App *app) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    GtkWidget *row = labeled_entry("Folder:", &app->folder_entry);
    GtkWidget *browse = gtk_button_new_with_label("Browse");
    GtkWidget *refresh = gtk_button_new_with_label("Refresh");
    gtk_box_pack_start(GTK_BOX(row), browse, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), refresh, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), row, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box), labeled_entry("Name:", &app->name_entry), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), labeled_entry("New name:", &app->new_name_entry), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), labeled_entry("Destination path:", &app->dest_entry), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), labeled_entry("Search:", &app->search_entry), FALSE, FALSE, 0);

    GtkWidget *buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    const char *labels[] = {"Create File", "Create Dir", "Delete", "Rename", "Copy", "Move", "Search"};
    GCallback callbacks[] = {G_CALLBACK(on_create_file_clicked), G_CALLBACK(on_create_dir_clicked), G_CALLBACK(on_delete_clicked),
        G_CALLBACK(on_rename_clicked), G_CALLBACK(on_copy_clicked), G_CALLBACK(on_move_clicked), G_CALLBACK(on_search_clicked)};
    for (int i = 0; i < 7; i++) {
        GtkWidget *b = gtk_button_new_with_label(labels[i]);
        g_signal_connect(b, "clicked", callbacks[i], app);
        gtk_box_pack_start(GTK_BOX(buttons), b, FALSE, FALSE, 0);
    }
    gtk_box_pack_start(GTK_BOX(box), buttons, FALSE, FALSE, 0);

    app->file_store = gtk_list_store_new(5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    app->file_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(app->file_store));
    const char *cols[] = {"Name", "Type", "Size", "Perm", "Modified"};
    for (int i = 0; i < 5; i++) {
        GtkCellRenderer *r = gtk_cell_renderer_text_new();
        gtk_tree_view_append_column(GTK_TREE_VIEW(app->file_view), gtk_tree_view_column_new_with_attributes(cols[i], r, "text", i, NULL));
    }
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(app->file_view));
    g_signal_connect(sel, "changed", G_CALLBACK(on_selection_changed), app);
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), app->file_view);
    gtk_widget_set_size_request(scroll, -1, 230);
    gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 0);

    app->file_info = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app->file_info), FALSE);
    gtk_widget_set_size_request(app->file_info, -1, 90);
    gtk_box_pack_start(GTK_BOX(box), app->file_info, FALSE, FALSE, 0);

    g_signal_connect(browse, "clicked", G_CALLBACK(on_browse_clicked), app);
    g_signal_connect(refresh, "clicked", G_CALLBACK(on_refresh_clicked), app);
    char *cwd = g_get_current_dir();
    gtk_entry_set_text(GTK_ENTRY(app->folder_entry), cwd);
    g_free(cwd);
    return box;
}

static GtkWidget *build_cron_tab(App *app) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_pack_start(GTK_BOX(box), labeled_entry("Cron expr:", &app->cron_expr_entry), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), labeled_entry("Command:", &app->cron_cmd_entry), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), labeled_entry("Remove line:", &app->cron_line_entry), FALSE, FALSE, 0);
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *add = gtk_button_new_with_label("Add Cron Job");
    GtkWidget *list = gtk_button_new_with_label("List Cron Jobs");
    GtkWidget *remove = gtk_button_new_with_label("Remove Cron Line");
    gtk_box_pack_start(GTK_BOX(row), add, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), list, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), remove, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), row, FALSE, FALSE, 0);
    g_signal_connect(add, "clicked", G_CALLBACK(on_cron_add_clicked), app);
    g_signal_connect(list, "clicked", G_CALLBACK(on_cron_list_clicked), app);
    g_signal_connect(remove, "clicked", G_CALLBACK(on_cron_remove_clicked), app);
    return box;
}

static GtkWidget *build_system_tab(App *app) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_pack_start(GTK_BOX(box), labeled_entry("New time:", &app->time_entry), FALSE, FALSE, 0);
    gtk_entry_set_placeholder_text(GTK_ENTRY(app->time_entry), "YYYY-MM-DD HH:MM:SS");
    gtk_box_pack_start(GTK_BOX(box), labeled_entry("Package:", &app->pkg_entry), FALSE, FALSE, 0);
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    const char *labels[] = {"Current Time", "Set Time", "Apt Install", "Apt Remove"};
    GCallback callbacks[] = {G_CALLBACK(on_time_now_clicked), G_CALLBACK(on_time_set_clicked), G_CALLBACK(on_apt_install_clicked), G_CALLBACK(on_apt_remove_clicked)};
    for (int i = 0; i < 4; i++) {
        GtkWidget *b = gtk_button_new_with_label(labels[i]);
        g_signal_connect(b, "clicked", callbacks[i], app);
        gtk_box_pack_start(GTK_BOX(row), b, FALSE, FALSE, 0);
    }
    gtk_box_pack_start(GTK_BOX(box), row, FALSE, FALSE, 0);
    return box;
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    App app = {0};
    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app.window), "Bai 01 - Shell System Admin");
    gtk_window_set_default_size(GTK_WINDOW(app.window), 920, 720);
    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(root), 10);
    GtkWidget *tabs = gtk_notebook_new();
    gtk_notebook_append_page(GTK_NOTEBOOK(tabs), build_file_tab(&app), gtk_label_new("File Manager"));
    gtk_notebook_append_page(GTK_NOTEBOOK(tabs), build_cron_tab(&app), gtk_label_new("Cron"));
    gtk_notebook_append_page(GTK_NOTEBOOK(tabs), build_system_tab(&app), gtk_label_new("System"));
    gtk_box_pack_start(GTK_BOX(root), tabs, TRUE, TRUE, 0);

    app.log_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app.log_view), FALSE);
    GtkWidget *log_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(log_scroll), app.log_view);
    gtk_widget_set_size_request(log_scroll, -1, 140);
    gtk_box_pack_start(GTK_BOX(root), gtk_label_new("Log / Output"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root), log_scroll, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(app.window), root);
    refresh_file_list(&app);
    gtk_widget_show_all(app.window);
    gtk_main();
    return 0;
}
