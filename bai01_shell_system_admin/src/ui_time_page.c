#include "ui_time_page.h"
#include "system_commands.h"

typedef struct {
    GtkWidget *root;
    GtkWidget *parent_window;
    GtkWidget *current_time_label;
    GtkWidget *date_entry;
    GtkWidget *time_entry;
    LogPage *log;
} TimePage;

static void refresh_time(TimePage *page) {
    char *text = get_current_time_string();
    gtk_label_set_text(GTK_LABEL(page->current_time_label), text);
    append_log(page->log, "INFO", text);
    g_free(text);
}

static void on_refresh(GtkButton *button, gpointer user_data) {
    (void)button;
    refresh_time(user_data);
}

static void on_set_time_response(GtkDialog *dialog, int response, gpointer user_data) {
    TimePage *page = user_data;
    if (response == GTK_RESPONSE_ACCEPT) {
        const char *date = gtk_editable_get_text(GTK_EDITABLE(page->date_entry));
        const char *time = gtk_editable_get_text(GTK_EDITABLE(page->time_entry));
        if (!date || !*date || !time || !*time) {
            append_log(page->log, "ERROR", "Date and time are required.");
        } else {
            char *value = g_strdup_printf("%s %s", date, time);
            char *qvalue = quote_arg(value);
            char *args = g_strdup_printf("time-set %s", qvalue);
            char *output = NULL;
            gboolean ok = run_system_script_sudo(GTK_WINDOW(page->parent_window), args, &output);
            append_log(page->log, ok ? "OK" : "ERROR", output);
            refresh_time(page);
            g_free(output);
            g_free(args);
            g_free(qvalue);
            g_free(value);
        }
    }
    gtk_window_destroy(GTK_WINDOW(dialog));
}

static void on_set_time(GtkButton *button, gpointer user_data) {
    (void)button;
    TimePage *page = user_data;
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Xac nhan doi thoi gian", GTK_WINDOW(page->parent_window),
        GTK_DIALOG_MODAL, "Huy", GTK_RESPONSE_CANCEL, "Doi thoi gian", GTK_RESPONSE_ACCEPT, NULL);
    gtk_box_append(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                   gtk_label_new("Chuc nang nay can quyen sudo/root. Ban co muon tiep tuc?"));
    g_signal_connect(dialog, "response", G_CALLBACK(on_set_time_response), page);
    gtk_window_present(GTK_WINDOW(dialog));
}

GtkWidget *ui_time_page_new(GtkWidget *parent_window, LogPage *log_page) {
    TimePage *page = g_new0(TimePage, 1);
    page->parent_window = parent_window;
    page->log = log_page;
    page->root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_top(page->root, 14);
    gtk_widget_set_margin_bottom(page->root, 14);
    gtk_widget_set_margin_start(page->root, 14);
    gtk_widget_set_margin_end(page->root, 14);
    g_object_set_data_full(G_OBJECT(page->root), "page-data", page, (GDestroyNotify)g_free);

    GtkWidget *title = gtk_label_new("Time System");
    gtk_widget_add_css_class(title, "title-3");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(page->root), title);

    page->current_time_label = gtk_label_new("");
    gtk_widget_add_css_class(page->current_time_label, "title-4");
    gtk_widget_set_halign(page->current_time_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(page->root), page->current_time_label);

    GtkWidget *refresh = gtk_button_new_with_label("Refresh");
    gtk_widget_set_halign(refresh, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(page->root), refresh);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Ngay (YYYY-MM-DD)"), 0, 0, 1, 1);
    page->date_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(page->date_entry), "2026-06-21");
    gtk_grid_attach(GTK_GRID(grid), page->date_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Gio (HH:MM:SS)"), 0, 1, 1, 1);
    page->time_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(page->time_entry), "08:30:00");
    gtk_grid_attach(GTK_GRID(grid), page->time_entry, 1, 1, 1, 1);
    gtk_box_append(GTK_BOX(page->root), grid);

    GtkWidget *warning = gtk_label_new("Chuc nang doi thoi gian can quyen sudo");
    gtk_widget_add_css_class(warning, "error");
    gtk_widget_set_halign(warning, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(page->root), warning);

    GtkWidget *set_btn = gtk_button_new_with_label("Doi thoi gian he thong");
    gtk_widget_set_halign(set_btn, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(page->root), set_btn);

    g_signal_connect(refresh, "clicked", G_CALLBACK(on_refresh), page);
    g_signal_connect(set_btn, "clicked", G_CALLBACK(on_set_time), page);
    refresh_time(page);
    return page->root;
}
