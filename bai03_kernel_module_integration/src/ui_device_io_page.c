#include "ui_device_io_page.h"
#include "kernel_module_commands.h"

typedef struct {
    AppContext *ctx;
    GtkWidget *input_entry;
    GtkWidget *read_view;
    GtkWidget *status_view;
    GtkWidget *device_status_label;
} DeviceIOPage;

static void set_text(GtkWidget *view, const char *text) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    gtk_text_buffer_set_text(buffer, text ? text : "", -1);
}

static void append_status(DeviceIOPage *page, const char *level, const char *message) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(page->status_view));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    char *line = g_strdup_printf("[%s] %s\n", level, message);
    gtk_text_buffer_insert(buffer, &end, line, -1);
    g_free(line);
}

static void refresh_device_status(DeviceIOPage *page) {
    gtk_label_set_text(GTK_LABEL(page->device_status_label), device_exists() ? "Device status: Exists" : "Device status: Missing");
    append_status(page, "INFO", device_exists() ? "Device file exists." : "Device file is missing. Load module first.");
}

static void on_refresh(GtkButton *button, gpointer user_data) {
    (void)button;
    refresh_device_status(user_data);
}

static void perform_write(DeviceIOPage *page) {
    const char *text = gtk_editable_get_text(GTK_EDITABLE(page->input_entry));
    if (!device_exists()) {
        append_status(page, "ERROR", DEVICE_PATH " does not exist.");
        return;
    }
    gboolean ok = FALSE;
    char *msg = write_device_data_sudo(GTK_WINDOW(page->ctx->window), text, &ok);
    append_status(page, ok ? "OK" : "ERROR", msg);
    g_free(msg);
}

static void on_empty_confirm(GtkDialog *dialog, int response, gpointer user_data) {
    DeviceIOPage *page = user_data;
    if (response == GTK_RESPONSE_ACCEPT) perform_write(page);
    gtk_window_destroy(GTK_WINDOW(dialog));
}

static void on_write(GtkButton *button, gpointer user_data) {
    (void)button;
    DeviceIOPage *page = user_data;
    const char *text = gtk_editable_get_text(GTK_EDITABLE(page->input_entry));
    if (!text || !*text) {
        GtkWidget *dialog = gtk_dialog_new_with_buttons("Confirm Empty Write", GTK_WINDOW(page->ctx->window),
            GTK_DIALOG_MODAL, "Cancel", GTK_RESPONSE_CANCEL, "Write Empty Data", GTK_RESPONSE_ACCEPT, NULL);
        gtk_box_append(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                       gtk_label_new("Input is empty. Write empty data to character device?"));
        g_signal_connect(dialog, "response", G_CALLBACK(on_empty_confirm), page);
        gtk_window_present(GTK_WINDOW(dialog));
        return;
    }
    perform_write(page);
}

static void on_read(GtkButton *button, gpointer user_data) {
    (void)button;
    DeviceIOPage *page = user_data;
    if (!device_exists()) {
        append_status(page, "ERROR", DEVICE_PATH " does not exist.");
        return;
    }
    gboolean ok = FALSE;
    char *data = read_device_data_sudo(GTK_WINDOW(page->ctx->window), &ok);
    set_text(page->read_view, data);
    append_status(page, ok ? "OK" : "ERROR", ok ? "Read data from device." : data);
    g_free(data);
}

static void on_clear(GtkButton *button, gpointer user_data) {
    (void)button;
    DeviceIOPage *page = user_data;
    gtk_editable_set_text(GTK_EDITABLE(page->input_entry), "");
    append_status(page, "INFO", "Input cleared.");
}

GtkWidget *ui_device_io_page_new(AppContext *ctx) {
    DeviceIOPage *page = g_new0(DeviceIOPage, 1);
    page->ctx = ctx;
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(root, 14);
    gtk_widget_set_margin_bottom(root, 14);
    gtk_widget_set_margin_start(root, 14);
    gtk_widget_set_margin_end(root, 14);
    g_object_set_data_full(G_OBJECT(root), "page-data", page, (GDestroyNotify)g_free);

    GtkWidget *title = gtk_label_new("Device I/O");
    gtk_widget_add_css_class(title, "title-2");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(root), title);

    GtkWidget *path = gtk_label_new("Device path: " DEVICE_PATH);
    gtk_label_set_xalign(GTK_LABEL(path), 0.0f);
    gtk_widget_add_css_class(path, "dim-label");
    gtk_box_append(GTK_BOX(root), path);

    page->device_status_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(page->device_status_label), 0.0f);
    gtk_box_append(GTK_BOX(root), page->device_status_label);

    GtkWidget *input_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    page->input_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(page->input_entry), "Hello from GTK user-space");
    gtk_widget_set_hexpand(page->input_entry, TRUE);
    GtkWidget *write_btn = gtk_button_new_with_label("Write to Device");
    GtkWidget *clear_btn = gtk_button_new_with_label("Clear Input");
    gtk_box_append(GTK_BOX(input_row), page->input_entry);
    gtk_box_append(GTK_BOX(input_row), write_btn);
    gtk_box_append(GTK_BOX(input_row), clear_btn);
    gtk_box_append(GTK_BOX(root), input_row);

    GtkWidget *read_btn = gtk_button_new_with_label("Read from Device");
    GtkWidget *refresh_btn = gtk_button_new_with_label("Refresh Device Status");
    GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(actions), read_btn);
    gtk_box_append(GTK_BOX(actions), refresh_btn);
    gtk_box_append(GTK_BOX(root), actions);

    page->read_view = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(page->read_view), TRUE);
    GtkWidget *read_scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(read_scroll, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(read_scroll), page->read_view);
    gtk_box_append(GTK_BOX(root), read_scroll);

    page->status_view = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(page->status_view), TRUE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(page->status_view), FALSE);
    GtkWidget *status_scroll = gtk_scrolled_window_new();
    gtk_widget_set_size_request(status_scroll, -1, 120);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(status_scroll), page->status_view);
    gtk_box_append(GTK_BOX(root), status_scroll);

    g_signal_connect(write_btn, "clicked", G_CALLBACK(on_write), page);
    g_signal_connect(read_btn, "clicked", G_CALLBACK(on_read), page);
    g_signal_connect(clear_btn, "clicked", G_CALLBACK(on_clear), page);
    g_signal_connect(refresh_btn, "clicked", G_CALLBACK(on_refresh), page);
    refresh_device_status(page);
    return root;
}
