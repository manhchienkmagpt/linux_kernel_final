#include "ui_command_console_page.h"
#include "kernel_module_commands.h"

typedef struct {
    AppContext *ctx;
    GtkWidget *command_entry;
    GtkWidget *output_view;
} ConsolePage;

static void set_output(ConsolePage *page, const char *text) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(page->output_view));
    gtk_text_buffer_set_text(buffer, text ? text : "", -1);
}

static void on_send(GtkButton *button, gpointer user_data) {
    (void)button;
    ConsolePage *page = user_data;
    gboolean ok = FALSE;
    const char *command = gtk_editable_get_text(GTK_EDITABLE(page->command_entry));
    char *result = send_kfile_command(GTK_WINDOW(page->ctx->window), command, &ok);
    set_output(page, result);
    g_free(result);
}

static void on_read(GtkButton *button, gpointer user_data) {
    (void)button;
    ConsolePage *page = user_data;
    gboolean ok = FALSE;
    char *result = read_device_data_sudo(GTK_WINDOW(page->ctx->window), &ok);
    set_output(page, result);
    g_free(result);
}

GtkWidget *ui_command_console_page_new(AppContext *ctx) {
    ConsolePage *page = g_new0(ConsolePage, 1);
    page->ctx = ctx;

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(root, 14);
    gtk_widget_set_margin_bottom(root, 14);
    gtk_widget_set_margin_start(root, 14);
    gtk_widget_set_margin_end(root, 14);
    g_object_set_data_full(G_OBJECT(root), "page-data", page, (GDestroyNotify)g_free);

    GtkWidget *title = gtk_label_new("Command Console");
    gtk_widget_add_css_class(title, "title-2");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(root), title);

    page->command_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(page->command_entry), "CREATE a.txt / WRITE a.txt hello / READ a.txt");
    gtk_box_append(GTK_BOX(root), page->command_entry);

    GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *send = gtk_button_new_with_label("Send Command");
    GtkWidget *read = gtk_button_new_with_label("Read Result");
    g_signal_connect(send, "clicked", G_CALLBACK(on_send), page);
    g_signal_connect(read, "clicked", G_CALLBACK(on_read), page);
    gtk_box_append(GTK_BOX(actions), send);
    gtk_box_append(GTK_BOX(actions), read);
    gtk_box_append(GTK_BOX(root), actions);

    page->output_view = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(page->output_view), TRUE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(page->output_view), FALSE);
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), page->output_view);
    gtk_box_append(GTK_BOX(root), scroll);
    return root;
}
