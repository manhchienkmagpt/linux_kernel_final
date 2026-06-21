#include "ui_cron_page.h"
#include "system_commands.h"

typedef struct {
    GtkWidget *root;
    GtkWidget *parent_window;
    GtkWidget *command_entry;
    GtkWidget *minute_entry;
    GtkWidget *hour_entry;
    GtkWidget *day_entry;
    GtkWidget *month_entry;
    GtkWidget *weekday_entry;
    GtkWidget *job_list;
    LogPage *log;
} CronPage;

static GtkWidget *labeled_entry(const char *label, GtkWidget **entry) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *lbl = gtk_label_new(label);
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    *entry = gtk_entry_new();
    gtk_box_append(GTK_BOX(box), lbl);
    gtk_box_append(GTK_BOX(box), *entry);
    return box;
}

static void set_cron(CronPage *page, const char *m, const char *h, const char *d, const char *mo, const char *w) {
    gtk_editable_set_text(GTK_EDITABLE(page->minute_entry), m);
    gtk_editable_set_text(GTK_EDITABLE(page->hour_entry), h);
    gtk_editable_set_text(GTK_EDITABLE(page->day_entry), d);
    gtk_editable_set_text(GTK_EDITABLE(page->month_entry), mo);
    gtk_editable_set_text(GTK_EDITABLE(page->weekday_entry), w);
}

static void clear_job_list(CronPage *page) {
    GtkWidget *child = gtk_widget_get_first_child(page->job_list);
    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_list_box_remove(GTK_LIST_BOX(page->job_list), child);
        child = next;
    }
}

static void add_job_row(CronPage *page, int index, const char *line) {
    GtkWidget *row = gtk_list_box_row_new();
    char *text = g_strdup_printf("%d. %s", index, line);
    GtkWidget *label = gtk_label_new(text);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(label), TRUE);
    gtk_widget_set_margin_top(label, 6);
    gtk_widget_set_margin_bottom(label, 6);
    gtk_widget_set_margin_start(label, 8);
    gtk_widget_set_margin_end(label, 8);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
    g_object_set_data(G_OBJECT(row), "line-index", GINT_TO_POINTER(index));
    gtk_list_box_append(GTK_LIST_BOX(page->job_list), row);
    g_free(text);
}

static void refresh_cron_jobs(CronPage *page) {
    char *output = NULL;
    gboolean ok = run_system_script("cron-list", &output);
    clear_job_list(page);
    if (output) {
        char **lines = g_strsplit(output, "\n", -1);
        int index = 1;
        for (int i = 0; lines[i]; i++) {
            if (lines[i][0] == '$' || lines[i][0] == '\0') continue;
            add_job_row(page, index++, lines[i]);
        }
        if (index == 1) add_job_row(page, 1, "(No cron job)");
        append_log(page->log, ok ? "OK" : "ERROR", output);
        g_strfreev(lines);
    }
    g_free(output);
}

static gboolean cron_input_valid(CronPage *page) {
    const char *cmd = gtk_editable_get_text(GTK_EDITABLE(page->command_entry));
    const char *fields[] = {
        gtk_editable_get_text(GTK_EDITABLE(page->minute_entry)),
        gtk_editable_get_text(GTK_EDITABLE(page->hour_entry)),
        gtk_editable_get_text(GTK_EDITABLE(page->day_entry)),
        gtk_editable_get_text(GTK_EDITABLE(page->month_entry)),
        gtk_editable_get_text(GTK_EDITABLE(page->weekday_entry)),
    };
    if (!cmd || !*cmd) return FALSE;
    for (int i = 0; i < 5; i++) if (!fields[i] || !*fields[i]) return FALSE;
    return TRUE;
}

static void on_add_cron(GtkButton *button, gpointer user_data) {
    (void)button;
    CronPage *page = user_data;
    if (!cron_input_valid(page)) {
        append_log(page->log, "ERROR", "Cron command and all time fields are required.");
        return;
    }

    char *expr = g_strdup_printf("%s %s %s %s %s",
        gtk_editable_get_text(GTK_EDITABLE(page->minute_entry)),
        gtk_editable_get_text(GTK_EDITABLE(page->hour_entry)),
        gtk_editable_get_text(GTK_EDITABLE(page->day_entry)),
        gtk_editable_get_text(GTK_EDITABLE(page->month_entry)),
        gtk_editable_get_text(GTK_EDITABLE(page->weekday_entry)));
    char *qexpr = quote_arg(expr);
    char *qcmd = quote_arg(gtk_editable_get_text(GTK_EDITABLE(page->command_entry)));
    char *args = g_strdup_printf("cron-add %s %s", qexpr, qcmd);
    char *output = NULL;
    gboolean ok = run_system_script(args, &output);
    append_log(page->log, ok ? "OK" : "ERROR", output);
    refresh_cron_jobs(page);
    g_free(output);
    g_free(args);
    g_free(qexpr);
    g_free(qcmd);
    g_free(expr);
}

static void on_view_cron(GtkButton *button, gpointer user_data) {
    (void)button;
    refresh_cron_jobs(user_data);
}

static void on_delete_response(GtkDialog *dialog, int response, gpointer user_data) {
    CronPage *page = user_data;
    if (response == GTK_RESPONSE_ACCEPT) {
        GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(page->job_list));
        if (!row) {
            append_log(page->log, "ERROR", "No cron job selected.");
        } else {
            int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "line-index"));
            char *args = g_strdup_printf("cron-remove-line %d", index);
            char *output = NULL;
            gboolean ok = run_system_script(args, &output);
            append_log(page->log, ok ? "OK" : "ERROR", output);
            g_free(output);
            g_free(args);
            refresh_cron_jobs(page);
        }
    }
    gtk_window_destroy(GTK_WINDOW(dialog));
}

static void on_delete_cron(GtkButton *button, gpointer user_data) {
    (void)button;
    CronPage *page = user_data;
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Xoa cron job", GTK_WINDOW(page->parent_window),
        GTK_DIALOG_MODAL, "Huy", GTK_RESPONSE_CANCEL, "Xoa", GTK_RESPONSE_ACCEPT, NULL);
    gtk_box_append(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                   gtk_label_new("Ban co chac muon xoa cron job dang chon?"));
    g_signal_connect(dialog, "response", G_CALLBACK(on_delete_response), page);
    gtk_window_present(GTK_WINDOW(dialog));
}

static void on_every_minute(GtkButton *button, gpointer user_data) { (void)button; set_cron(user_data, "*", "*", "*", "*", "*"); }
static void on_daily_8(GtkButton *button, gpointer user_data) { (void)button; set_cron(user_data, "0", "8", "*", "*", "*"); }
static void on_weekly(GtkButton *button, gpointer user_data) { (void)button; set_cron(user_data, "0", "8", "*", "*", "1"); }
static void on_custom(GtkButton *button, gpointer user_data) { (void)button; append_log(((CronPage *)user_data)->log, "INFO", "Custom mode: edit cron fields manually."); }

GtkWidget *ui_cron_page_new(GtkWidget *parent_window, LogPage *log_page) {
    CronPage *page = g_new0(CronPage, 1);
    page->parent_window = parent_window;
    page->log = log_page;
    page->root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(page->root, 14);
    gtk_widget_set_margin_bottom(page->root, 14);
    gtk_widget_set_margin_start(page->root, 14);
    gtk_widget_set_margin_end(page->root, 14);
    g_object_set_data_full(G_OBJECT(page->root), "page-data", page, (GDestroyNotify)g_free);

    GtkWidget *title = gtk_label_new("Cron Scheduler");
    gtk_widget_add_css_class(title, "title-3");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(page->root), title);

    gtk_box_append(GTK_BOX(page->root), labeled_entry("Lenh can chay", &page->command_entry));

    GtkWidget *cron_grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(cron_grid), 8);
    gtk_grid_attach(GTK_GRID(cron_grid), labeled_entry("Phut", &page->minute_entry), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(cron_grid), labeled_entry("Gio", &page->hour_entry), 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(cron_grid), labeled_entry("Ngay", &page->day_entry), 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(cron_grid), labeled_entry("Thang", &page->month_entry), 3, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(cron_grid), labeled_entry("Thu", &page->weekday_entry), 4, 0, 1, 1);
    gtk_box_append(GTK_BOX(page->root), cron_grid);
    set_cron(page, "*", "*", "*", "*", "*");

    GtkWidget *preset = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    const char *preset_labels[] = {"Moi phut", "Moi ngay luc 8h", "Moi tuan", "Custom"};
    GCallback preset_callbacks[] = {G_CALLBACK(on_every_minute), G_CALLBACK(on_daily_8), G_CALLBACK(on_weekly), G_CALLBACK(on_custom)};
    for (int i = 0; i < 4; i++) {
        GtkWidget *btn = gtk_button_new_with_label(preset_labels[i]);
        g_signal_connect(btn, "clicked", preset_callbacks[i], page);
        gtk_box_append(GTK_BOX(preset), btn);
    }
    gtk_box_append(GTK_BOX(page->root), preset);

    GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *add = gtk_button_new_with_label("Them cron job");
    GtkWidget *view = gtk_button_new_with_label("Xem cron");
    GtkWidget *del = gtk_button_new_with_label("Xoa cron da chon");
    gtk_box_append(GTK_BOX(actions), add);
    gtk_box_append(GTK_BOX(actions), view);
    gtk_box_append(GTK_BOX(actions), del);
    gtk_box_append(GTK_BOX(page->root), actions);

    page->job_list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(page->job_list), GTK_SELECTION_SINGLE);
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), page->job_list);
    gtk_box_append(GTK_BOX(page->root), scroll);

    g_signal_connect(add, "clicked", G_CALLBACK(on_add_cron), page);
    g_signal_connect(view, "clicked", G_CALLBACK(on_view_cron), page);
    g_signal_connect(del, "clicked", G_CALLBACK(on_delete_cron), page);
    refresh_cron_jobs(page);
    return page->root;
}
