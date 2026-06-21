#include "ui_apt_page.h"
#include "system_commands.h"

typedef struct {
    GtkWidget *root;
    GtkWidget *parent_window;
    GtkWidget *package_entry;
    GtkWidget *output_view;
    LogPage *log;
} AptPage;

static void set_output(AptPage *page, const char *text) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(page->output_view));
    gtk_text_buffer_set_text(buffer, text ? text : "", -1);
}

static gboolean package_valid(AptPage *page) {
    const char *pkg = gtk_editable_get_text(GTK_EDITABLE(page->package_entry));
    if (!pkg || !*pkg) {
        append_log(page->log, "ERROR", "Package name is required.");
        set_output(page, "Package name is required.");
        return FALSE;
    }
    return TRUE;
}

static void run_apt_action(AptPage *page, const char *action) {
    const char *pkg = gtk_editable_get_text(GTK_EDITABLE(page->package_entry));
    char *qpkg = quote_arg(pkg);
    char *args = g_strdup_printf("%s %s", action, qpkg);
    char *output = NULL;
    gboolean needs_sudo = g_strcmp0(action, "apt-install") == 0 || g_strcmp0(action, "apt-remove") == 0;
    gboolean ok = needs_sudo
        ? run_system_script_sudo(GTK_WINDOW(page->parent_window), args, &output)
        : run_system_script(args, &output);
    set_output(page, output);
    append_log(page->log, ok ? "OK" : "ERROR", output);
    g_free(output);
    g_free(args);
    g_free(qpkg);
}

static void on_confirm_response(GtkDialog *dialog, int response, gpointer user_data) {
    AptPage *page = g_object_get_data(G_OBJECT(dialog), "page");
    const char *action = user_data;
    if (response == GTK_RESPONSE_ACCEPT) run_apt_action(page, action);
    gtk_window_destroy(GTK_WINDOW(dialog));
}

static void confirm_apt(AptPage *page, const char *action, const char *title) {
    if (!package_valid(page)) return;
    GtkWidget *dialog = gtk_dialog_new_with_buttons(title, GTK_WINDOW(page->parent_window),
        GTK_DIALOG_MODAL, "Huy", GTK_RESPONSE_CANCEL, "Dong y", GTK_RESPONSE_ACCEPT, NULL);
    gtk_box_append(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                   gtk_label_new("Lenh apt can quyen sudo/root. Ban co chac muon tiep tuc?"));
    g_object_set_data(G_OBJECT(dialog), "page", page);
    g_signal_connect(dialog, "response", G_CALLBACK(on_confirm_response), (gpointer)action);
    gtk_window_present(GTK_WINDOW(dialog));
}

static void on_install(GtkButton *button, gpointer user_data) {
    (void)button;
    confirm_apt(user_data, "apt-install", "Cai dat package");
}

static void on_remove(GtkButton *button, gpointer user_data) {
    (void)button;
    confirm_apt(user_data, "apt-remove", "Go bo package");
}

static void on_check(GtkButton *button, gpointer user_data) {
    (void)button;
    AptPage *page = user_data;
    if (package_valid(page)) run_apt_action(page, "apt-check");
}

GtkWidget *ui_apt_page_new(GtkWidget *parent_window, LogPage *log_page) {
    AptPage *page = g_new0(AptPage, 1);
    page->parent_window = parent_window;
    page->log = log_page;
    page->root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(page->root, 14);
    gtk_widget_set_margin_bottom(page->root, 14);
    gtk_widget_set_margin_start(page->root, 14);
    gtk_widget_set_margin_end(page->root, 14);
    g_object_set_data_full(G_OBJECT(page->root), "page-data", page, (GDestroyNotify)g_free);

    GtkWidget *title = gtk_label_new("Apt Manager");
    gtk_widget_add_css_class(title, "title-3");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(page->root), title);

    GtkWidget *input_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(input_row), gtk_label_new("Package:"));
    page->package_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(page->package_entry), "vi du: htop");
    gtk_widget_set_hexpand(page->package_entry, TRUE);
    gtk_box_append(GTK_BOX(input_row), page->package_entry);
    gtk_box_append(GTK_BOX(page->root), input_row);

    GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *install = gtk_button_new_with_label("Cai dat");
    GtkWidget *remove = gtk_button_new_with_label("Go bo");
    GtkWidget *check = gtk_button_new_with_label("Kiem tra da cai chua");
    gtk_box_append(GTK_BOX(actions), install);
    gtk_box_append(GTK_BOX(actions), remove);
    gtk_box_append(GTK_BOX(actions), check);
    gtk_box_append(GTK_BOX(page->root), actions);

    page->output_view = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(page->output_view), TRUE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(page->output_view), FALSE);
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), page->output_view);
    gtk_box_append(GTK_BOX(page->root), scroll);

    g_signal_connect(install, "clicked", G_CALLBACK(on_install), page);
    g_signal_connect(remove, "clicked", G_CALLBACK(on_remove), page);
    g_signal_connect(check, "clicked", G_CALLBACK(on_check), page);
    return page->root;
}
