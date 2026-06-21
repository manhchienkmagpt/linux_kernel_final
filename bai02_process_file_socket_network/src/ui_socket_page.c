#include "ui_socket_page.h"
#include "socket_demo.h"
#include <stdlib.h>

typedef struct {
    AppContext *ctx;
    GtkWidget *server_radio;
    GtkWidget *client_radio;
    GtkWidget *ip_entry;
    GtkWidget *port_entry;
    GtkWidget *connection_log;
    GtkWidget *message_entry;
} SocketPage;

typedef struct {
    SocketPage *page;
    char *message;
} SocketLogEvent;

static void append_connection_log(SocketPage *page, const char *message) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(page->connection_log));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, message, -1);
    gtk_text_buffer_insert(buffer, &end, "\n", -1);
}

static gboolean socket_log_idle(gpointer user_data) {
    SocketLogEvent *event = user_data;
    append_connection_log(event->page, event->message);
    log_page_append(event->page->ctx->log_page, "INFO", event->message);
    g_free(event->message);
    g_free(event);
    return FALSE;
}

static void socket_log_callback(const char *message, void *user_data) {
    SocketLogEvent *event = g_new0(SocketLogEvent, 1);
    event->page = user_data;
    event->message = g_strdup(message);
    g_idle_add(socket_log_idle, event);
}

static GtkWidget *labeled_entry(const char *label, GtkWidget **entry) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *lbl = gtk_label_new(label);
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    *entry = gtk_entry_new();
    gtk_box_append(GTK_BOX(box), lbl);
    gtk_box_append(GTK_BOX(box), *entry);
    return box;
}

static void on_start(GtkButton *button, gpointer user_data) {
    (void)button;
    SocketPage *page = user_data;
    int port = atoi(gtk_editable_get_text(GTK_EDITABLE(page->port_entry)));
    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(page->server_radio))) {
        char *msg = NULL;
        int ok = socket_server_start(port, socket_log_callback, page, &msg);
        append_connection_log(page, msg);
        log_page_append(page->ctx->log_page, ok == 0 ? "OK" : "ERROR", msg);
        g_free(msg);
    } else {
        const char *host = gtk_editable_get_text(GTK_EDITABLE(page->ip_entry));
        char *msg = NULL;
        int ok = socket_client_connect(host, port, socket_log_callback, page, &msg);
        append_connection_log(page, msg);
        log_page_append(page->ctx->log_page, ok == 0 ? "OK" : "ERROR", msg);
        g_free(msg);
    }
}

static void on_stop(GtkButton *button, gpointer user_data) {
    (void)button;
    SocketPage *page = user_data;
    socket_server_stop(socket_log_callback, page);
    socket_client_disconnect();
    append_connection_log(page, "Stop requested.");
    log_page_append(page->ctx->log_page, "INFO", "Socket stop requested.");
}

static void on_send(GtkButton *button, gpointer user_data) {
    (void)button;
    SocketPage *page = user_data;
    int port = atoi(gtk_editable_get_text(GTK_EDITABLE(page->port_entry)));
    const char *message = gtk_editable_get_text(GTK_EDITABLE(page->message_entry));
    if (port <= 0 || !message || !*message) {
        append_connection_log(page, "Port and message are required.");
        log_page_append(page->ctx->log_page, "ERROR", "Socket send input is incomplete.");
        return;
    }

    char *status = NULL;
    int ok = 0;
    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(page->server_radio))) {
        ok = socket_server_broadcast_message(message, &status);
    } else {
        ok = socket_client_send_message(message, &status);
    }

    if (ok != 0) append_connection_log(page, status);
    log_page_append(page->ctx->log_page, ok == 0 ? "INFO" : "ERROR", status);
    g_free(status);
}

GtkWidget *ui_socket_page_new(AppContext *ctx) {
    SocketPage *page = g_new0(SocketPage, 1);
    page->ctx = ctx;
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(root, 14);
    gtk_widget_set_margin_bottom(root, 14);
    gtk_widget_set_margin_start(root, 14);
    gtk_widget_set_margin_end(root, 14);
    g_object_set_data_full(G_OBJECT(root), "page-data", page, (GDestroyNotify)g_free);

    GtkWidget *title = gtk_label_new("Socket");
    gtk_widget_add_css_class(title, "title-2");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(root), title);

    GtkWidget *mode_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    page->server_radio = gtk_check_button_new_with_label("Server");
    page->client_radio = gtk_check_button_new_with_label("Client");
    gtk_check_button_set_group(GTK_CHECK_BUTTON(page->client_radio), GTK_CHECK_BUTTON(page->server_radio));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(page->server_radio), TRUE);
    gtk_box_append(GTK_BOX(mode_row), gtk_label_new("Mode:"));
    gtk_box_append(GTK_BOX(mode_row), page->server_radio);
    gtk_box_append(GTK_BOX(mode_row), page->client_radio);
    gtk_box_append(GTK_BOX(root), mode_row);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_grid_attach(GTK_GRID(grid), labeled_entry("IP", &page->ip_entry), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), labeled_entry("Port", &page->port_entry), 1, 0, 1, 1);
    gtk_editable_set_text(GTK_EDITABLE(page->ip_entry), "127.0.0.1");
    gtk_editable_set_text(GTK_EDITABLE(page->port_entry), "9090");
    gtk_box_append(GTK_BOX(root), grid);

    GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *start = gtk_button_new_with_label("Start");
    GtkWidget *stop = gtk_button_new_with_label("Stop");
    gtk_box_append(GTK_BOX(actions), start);
    gtk_box_append(GTK_BOX(actions), stop);
    gtk_box_append(GTK_BOX(root), actions);

    page->connection_log = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(page->connection_log), TRUE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(page->connection_log), FALSE);
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), page->connection_log);
    gtk_box_append(GTK_BOX(root), scroll);

    GtkWidget *message_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    page->message_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(page->message_entry), "Hello TCP server");
    gtk_widget_set_hexpand(page->message_entry, TRUE);
    GtkWidget *send = gtk_button_new_with_label("Send");
    gtk_box_append(GTK_BOX(message_row), page->message_entry);
    gtk_box_append(GTK_BOX(message_row), send);
    gtk_box_append(GTK_BOX(root), message_row);

    g_signal_connect(start, "clicked", G_CALLBACK(on_start), page);
    g_signal_connect(stop, "clicked", G_CALLBACK(on_stop), page);
    g_signal_connect(send, "clicked", G_CALLBACK(on_send), page);
    return root;
}
