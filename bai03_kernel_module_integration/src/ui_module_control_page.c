#include "ui_module_control_page.h"
#include "kernel_module_commands.h"

typedef struct {
    AppContext *ctx;
    GtkWidget *output_view;
    GtkWidget *ko_label;
    GtkWidget *busy_label;
} ModuleControlPage;

static void append_output(ModuleControlPage *page, const char *text) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(page->output_view));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, text ? text : "", -1);
    gtk_text_buffer_insert(buffer, &end, "\n", -1);
}

static void command_done(gboolean ok, const char *output, gpointer user_data) {
    ModuleControlPage *page = user_data;
    gtk_label_set_text(GTK_LABEL(page->busy_label), ok ? "Last command finished successfully." : "Last command failed. See output.");
    append_output(page, output);
}

static void run_page_command(ModuleControlPage *page, const char *command, const char *label, gboolean needs_sudo) {
    gtk_label_set_text(GTK_LABEL(page->busy_label), label);
    append_output(page, label);
    if (needs_sudo) {
        run_command_async_sudo(GTK_WINDOW(page->ctx->window), command, command_done, page);
    } else {
        run_command_async(command, command_done, page);
    }
}

static void on_build(GtkButton *button, gpointer user_data) {
    (void)button;
    run_page_command(user_data, "make module", "Running: make module", FALSE);
}

static void on_clean(GtkButton *button, gpointer user_data) {
    (void)button;
    run_page_command(user_data, "make clean", "Running: make clean", FALSE);
}

static void on_status(GtkButton *button, gpointer user_data) {
    (void)button;
    run_page_command(user_data, "bash scripts/module_control.sh status", "Checking module status...", FALSE);
}

static void confirm_response(GtkDialog *dialog, int response, gpointer user_data) {
    ModuleControlPage *page = g_object_get_data(G_OBJECT(dialog), "page");
    const char *command = user_data;
    if (response == GTK_RESPONSE_ACCEPT) {
        run_page_command(page, command, g_strrstr(command, "unload") ? "Unloading module..." : "Loading module...", TRUE);
    }
    gtk_window_destroy(GTK_WINDOW(dialog));
}

static void confirm_command(ModuleControlPage *page, const char *title, const char *message, const char *command) {
    if (g_strcmp0(command, "bash scripts/module_control.sh load") == 0 && !g_file_test(MODULE_KO_PATH, G_FILE_TEST_EXISTS)) {
        append_output(page, "WARNING: module .ko does not exist. Build Module first.");
    }
    GtkWidget *dialog = gtk_dialog_new_with_buttons(title, GTK_WINDOW(page->ctx->window),
        GTK_DIALOG_MODAL, "Cancel", GTK_RESPONSE_CANCEL, "Continue", GTK_RESPONSE_ACCEPT, NULL);
    gtk_box_append(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), gtk_label_new(message));
    g_object_set_data(G_OBJECT(dialog), "page", page);
    g_signal_connect(dialog, "response", G_CALLBACK(confirm_response), (gpointer)command);
    gtk_window_present(GTK_WINDOW(dialog));
}

static void on_load(GtkButton *button, gpointer user_data) {
    (void)button;
    confirm_command(user_data, "Confirm Load Module", "Load module requires sudo/root. Continue?", "bash scripts/module_control.sh load");
}

static void on_unload(GtkButton *button, gpointer user_data) {
    (void)button;
    confirm_command(user_data, "Confirm Unload Module", "Unload kfile_manager and remove /dev/kfile_manager. Continue?", "bash scripts/module_control.sh unload");
}

GtkWidget *ui_module_control_page_new(AppContext *ctx) {
    ModuleControlPage *page = g_new0(ModuleControlPage, 1);
    page->ctx = ctx;
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(root, 14);
    gtk_widget_set_margin_bottom(root, 14);
    gtk_widget_set_margin_start(root, 14);
    gtk_widget_set_margin_end(root, 14);
    g_object_set_data_full(G_OBJECT(root), "page-data", page, (GDestroyNotify)g_free);

    GtkWidget *title = gtk_label_new("Module Control");
    gtk_widget_add_css_class(title, "title-2");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(root), title);

    page->ko_label = gtk_label_new("Module path: " MODULE_KO_PATH);
    gtk_label_set_xalign(GTK_LABEL(page->ko_label), 0.0f);
    gtk_widget_add_css_class(page->ko_label, "dim-label");
    gtk_box_append(GTK_BOX(root), page->ko_label);

    GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    const char *labels[] = {"Build Module", "Load Module", "Unload Module", "Check Status", "Clean Build"};
    GCallback callbacks[] = {G_CALLBACK(on_build), G_CALLBACK(on_load), G_CALLBACK(on_unload), G_CALLBACK(on_status), G_CALLBACK(on_clean)};
    for (int i = 0; i < 5; i++) {
        GtkWidget *btn = gtk_button_new_with_label(labels[i]);
        g_signal_connect(btn, "clicked", callbacks[i], page);
        gtk_box_append(GTK_BOX(actions), btn);
    }
    gtk_box_append(GTK_BOX(root), actions);

    page->busy_label = gtk_label_new("Ready.");
    gtk_label_set_xalign(GTK_LABEL(page->busy_label), 0.0f);
    gtk_box_append(GTK_BOX(root), page->busy_label);

    page->output_view = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(page->output_view), TRUE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(page->output_view), FALSE);
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), page->output_view);
    gtk_box_append(GTK_BOX(root), scroll);
    return root;
}
