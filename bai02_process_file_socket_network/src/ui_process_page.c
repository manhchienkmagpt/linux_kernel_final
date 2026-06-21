#include "ui_process_page.h"
#include "process_utils.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    AppContext *ctx;
    GtkWidget *search_entry;
    GtkWidget *list_box;
} ProcessPage;

static GtkWidget *cell_label(const char *text, int width) {
    GtkWidget *label = gtk_label_new(text);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
    gtk_widget_set_size_request(label, width, -1);
    return label;
}

static char process_state_for_pid(int pid) {
    char *path = g_strdup_printf("/proc/%d/stat", pid);
    char *content = NULL;
    char state = '?';
    if (g_file_get_contents(path, &content, NULL, NULL) && content) {
        char *close_paren = strrchr(content, ')');
        if (close_paren && close_paren[1] == ' ') state = close_paren[2];
    }
    g_free(content);
    g_free(path);
    return state;
}

static gboolean row_matches(ProcessPage *page, const char *pid, const char *name) {
    const char *filter = gtk_editable_get_text(GTK_EDITABLE(page->search_entry));
    return !filter || !*filter || g_strrstr(pid, filter) || g_strrstr(name, filter);
}

static void clear_process_list(ProcessPage *page) {
    GtkWidget *child = gtk_widget_get_first_child(page->list_box);
    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_list_box_remove(GTK_LIST_BOX(page->list_box), child);
        child = next;
    }
}

static void add_process_row(ProcessPage *page, const char *pid, const char *name, const char *cpu, const char *ram) {
    int pid_value = atoi(pid);
    char state_text[8];
    g_snprintf(state_text, sizeof(state_text), "%c", process_state_for_pid(pid_value));

    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_top(box, 5);
    gtk_widget_set_margin_bottom(box, 5);
    gtk_widget_set_margin_start(box, 8);
    gtk_widget_set_margin_end(box, 8);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), box);
    gtk_box_append(GTK_BOX(box), cell_label(pid, 90));
    gtk_box_append(GTK_BOX(box), cell_label(name, 260));
    gtk_box_append(GTK_BOX(box), cell_label(cpu, 90));
    gtk_box_append(GTK_BOX(box), cell_label(ram, 90));
    gtk_box_append(GTK_BOX(box), cell_label(state_text, 90));
    g_object_set_data(G_OBJECT(row), "pid", GINT_TO_POINTER(pid_value));
    gtk_list_box_append(GTK_LIST_BOX(page->list_box), row);
}

static void refresh_processes(ProcessPage *page) {
    clear_process_list(page);
    char *list = get_process_list();
    if (!list) return;

    char **lines = g_strsplit(list, "\n", -1);
    for (int i = 1; lines[i]; i++) {
        char **cols = g_strsplit_set(g_strstrip(lines[i]), " \t", 0);
        GPtrArray *parts = g_ptr_array_new();
        for (int j = 0; cols[j]; j++) if (cols[j][0]) g_ptr_array_add(parts, cols[j]);
        if (parts->len >= 4) {
            const char *pid = g_ptr_array_index(parts, 0);
            const char *name = g_ptr_array_index(parts, 1);
            const char *cpu = g_ptr_array_index(parts, 2);
            const char *ram = g_ptr_array_index(parts, 3);
            if (row_matches(page, pid, name)) add_process_row(page, pid, name, cpu, ram);
        }
        g_ptr_array_free(parts, TRUE);
        g_strfreev(cols);
    }
    g_strfreev(lines);
    g_free(list);
    log_page_append(page->ctx->log_page, "INFO", "Process list refreshed.");
}

static int selected_pid(ProcessPage *page) {
    GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(page->list_box));
    return row ? GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "pid")) : 0;
}

static void on_refresh(GtkButton *button, gpointer user_data) {
    (void)button;
    refresh_processes(user_data);
}

static void on_search_changed(GtkEditable *editable, gpointer user_data) {
    (void)editable;
    refresh_processes(user_data);
}

static void on_kill_response(GtkDialog *dialog, int response, gpointer user_data) {
    ProcessPage *page = user_data;
    if (response == GTK_RESPONSE_ACCEPT) {
        int pid = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dialog), "pid"));
        char *msg = NULL;
        int ok = kill_process_by_pid(pid, &msg);
        log_page_append(page->ctx->log_page, ok == 0 ? "OK" : "ERROR", msg);
        g_free(msg);
        refresh_processes(page);
    }
    gtk_window_destroy(GTK_WINDOW(dialog));
}

static void on_kill(GtkButton *button, gpointer user_data) {
    (void)button;
    ProcessPage *page = user_data;
    int pid = selected_pid(page);
    if (pid <= 0) {
        log_page_append(page->ctx->log_page, "ERROR", "Select a process before killing.");
        return;
    }
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Confirm Kill Process", GTK_WINDOW(page->ctx->window),
        GTK_DIALOG_MODAL, "Cancel", GTK_RESPONSE_CANCEL, "Kill", GTK_RESPONSE_ACCEPT, NULL);
    char *msg = g_strdup_printf("Send SIGTERM to PID %d?", pid);
    gtk_box_append(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), gtk_label_new(msg));
    g_free(msg);
    g_object_set_data(G_OBJECT(dialog), "pid", GINT_TO_POINTER(pid));
    g_signal_connect(dialog, "response", G_CALLBACK(on_kill_response), page);
    gtk_window_present(GTK_WINDOW(dialog));
}

static void on_child_response(GtkDialog *dialog, int response, gpointer user_data) {
    ProcessPage *page = user_data;
    GtkWidget *spin = g_object_get_data(G_OBJECT(dialog), "spin");
    if (response == GTK_RESPONSE_ACCEPT) {
        int count = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin));
        for (int i = 0; i < count; i++) {
            char *msg = fork_demo();
            log_page_append(page->ctx->log_page, "OK", msg);
            g_free(msg);
        }
    }
    gtk_window_destroy(GTK_WINDOW(dialog));
}

static void on_create_child(GtkButton *button, gpointer user_data) {
    (void)button;
    ProcessPage *page = user_data;
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Create Child Process", GTK_WINDOW(page->ctx->window),
        GTK_DIALOG_MODAL, "Cancel", GTK_RESPONSE_CANCEL, "Create", GTK_RESPONSE_ACCEPT, NULL);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *spin = gtk_spin_button_new_with_range(1, 10, 1);
    gtk_box_append(GTK_BOX(box), gtk_label_new("Number of child processes:"));
    gtk_box_append(GTK_BOX(box), spin);
    gtk_box_append(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), box);
    g_object_set_data(G_OBJECT(dialog), "spin", spin);
    g_signal_connect(dialog, "response", G_CALLBACK(on_child_response), page);
    gtk_window_present(GTK_WINDOW(dialog));
}

GtkWidget *ui_process_page_new(AppContext *ctx) {
    ProcessPage *page = g_new0(ProcessPage, 1);
    page->ctx = ctx;
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(root, 14);
    gtk_widget_set_margin_bottom(root, 14);
    gtk_widget_set_margin_start(root, 14);
    gtk_widget_set_margin_end(root, 14);
    g_object_set_data_full(G_OBJECT(root), "page-data", page, (GDestroyNotify)g_free);

    GtkWidget *title = gtk_label_new("Processes");
    gtk_widget_add_css_class(title, "title-2");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(root), title);

    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *refresh = gtk_button_new_with_label("Refresh");
    GtkWidget *kill = gtk_button_new_with_label("Kill Process");
    GtkWidget *child = gtk_button_new_with_label("Create Child Process");
    page->search_entry = gtk_search_entry_new();
    gtk_widget_set_hexpand(page->search_entry, TRUE);
    gtk_box_append(GTK_BOX(toolbar), refresh);
    gtk_box_append(GTK_BOX(toolbar), kill);
    gtk_box_append(GTK_BOX(toolbar), child);
    gtk_box_append(GTK_BOX(toolbar), page->search_entry);
    gtk_box_append(GTK_BOX(root), toolbar);

    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_add_css_class(header, "heading");
    gtk_box_append(GTK_BOX(header), cell_label("PID", 90));
    gtk_box_append(GTK_BOX(header), cell_label("Name", 260));
    gtk_box_append(GTK_BOX(header), cell_label("CPU %", 90));
    gtk_box_append(GTK_BOX(header), cell_label("RAM %", 90));
    gtk_box_append(GTK_BOX(header), cell_label("State", 90));
    gtk_box_append(GTK_BOX(root), header);

    page->list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(page->list_box), GTK_SELECTION_SINGLE);
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), page->list_box);
    gtk_box_append(GTK_BOX(root), scroll);

    g_signal_connect(refresh, "clicked", G_CALLBACK(on_refresh), page);
    g_signal_connect(kill, "clicked", G_CALLBACK(on_kill), page);
    g_signal_connect(child, "clicked", G_CALLBACK(on_create_child), page);
    g_signal_connect(page->search_entry, "changed", G_CALLBACK(on_search_changed), page);
    refresh_processes(page);
    return root;
}
