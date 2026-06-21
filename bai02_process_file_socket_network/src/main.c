#include <gtk/gtk.h>
#include <stdlib.h>
#include "file_syscall.h"
#include "network_info.h"
#include "process_utils.h"
#include "socket_demo.h"

typedef struct {
    GtkWidget *window;
    GtkWidget *pid_entry;
    GtkWidget *file_entry;
    GtkWidget *file_text;
    GtkWidget *host_entry;
    GtkWidget *port_entry;
    GtkWidget *msg_entry;
    GtkWidget *log_view;
} App;

static gboolean append_log_idle(gpointer data) {
    char **pack = data;
    App *app = (App *)pack[0];
    char *msg = pack[1];
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_view));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buf, &end);
    gtk_text_buffer_insert(buf, &end, msg, -1);
    gtk_text_buffer_insert(buf, &end, "\n", -1);
    g_free(msg);
    g_free(pack);
    return FALSE;
}

static void append_log(App *app, const char *msg) {
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_view));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buf, &end);
    gtk_text_buffer_insert(buf, &end, msg, -1);
    gtk_text_buffer_insert(buf, &end, "\n", -1);
}

static void socket_log(const char *message, void *user_data) {
    char **pack = g_new(char *, 2);
    pack[0] = user_data;
    pack[1] = g_strdup(message);
    g_idle_add(append_log_idle, pack);
}

static void show_error(App *app, const char *msg) {
    GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(app->window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", msg);
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
}

static GtkWidget *labeled_entry(const char *label, GtkWidget **entry) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(box), gtk_label_new(label), FALSE, FALSE, 0);
    *entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(box), *entry, TRUE, TRUE, 0);
    return box;
}

static char *get_text_view_content(GtkWidget *view) {
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buf, &start, &end);
    return gtk_text_buffer_get_text(buf, &start, &end, FALSE);
}

static void set_text_view_content(GtkWidget *view, const char *text) {
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    gtk_text_buffer_set_text(buf, text, -1);
}

static void on_refresh_process(GtkButton *b, gpointer data) {
    (void)b;
    char *list = get_process_list();
    append_log(data, list);
    g_free(list);
}

static void on_kill_pid(GtkButton *b, gpointer data) {
    (void)b;
    App *app = data;
    int pid = atoi(gtk_entry_get_text(GTK_ENTRY(app->pid_entry)));
    char *msg = NULL;
    if (kill_process_by_pid(pid, &msg) != 0) show_error(app, msg);
    append_log(app, msg);
    g_free(msg);
}

static void on_fork_demo(GtkButton *b, gpointer data) {
    (void)b;
    char *msg = fork_demo();
    append_log(data, msg);
    g_free(msg);
}

static void on_choose_file(GtkButton *b, gpointer data) {
    (void)b;
    App *app = data;
    GtkWidget *dlg = gtk_file_chooser_dialog_new("Choose File", GTK_WINDOW(app->window),
        GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);
    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
        char *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
        gtk_entry_set_text(GTK_ENTRY(app->file_entry), path);
        g_free(path);
    }
    gtk_widget_destroy(dlg);
}

static void on_read_file(GtkButton *b, gpointer data) {
    (void)b;
    App *app = data;
    const char *path = gtk_entry_get_text(GTK_ENTRY(app->file_entry));
    char *content = read_file_syscall(path);
    set_text_view_content(app->file_text, content);
    append_log(app, "Read file with open/read/close.");
    g_free(content);
}

static void on_write_file(GtkButton *b, gpointer data) {
    (void)b;
    App *app = data;
    const char *path = gtk_entry_get_text(GTK_ENTRY(app->file_entry));
    char *content = get_text_view_content(app->file_text);
    char *msg = NULL;
    if (write_file_syscall(path, content, &msg) != 0) show_error(app, msg);
    append_log(app, msg);
    g_free(content); g_free(msg);
}

static void on_start_server(GtkButton *b, gpointer data) {
    (void)b;
    App *app = data;
    int port = atoi(gtk_entry_get_text(GTK_ENTRY(app->port_entry)));
    char *msg = NULL;
    if (socket_server_start(port, socket_log, app, &msg) != 0) show_error(app, msg);
    append_log(app, msg);
    g_free(msg);
}

static void on_stop_server(GtkButton *b, gpointer data) {
    (void)b;
    socket_server_stop(socket_log, data);
}

static void on_client_send(GtkButton *b, gpointer data) {
    (void)b;
    App *app = data;
    const char *host = gtk_entry_get_text(GTK_ENTRY(app->host_entry));
    int port = atoi(gtk_entry_get_text(GTK_ENTRY(app->port_entry)));
    const char *msg = gtk_entry_get_text(GTK_ENTRY(app->msg_entry));
    char *reply = socket_client_send(host, port, msg);
    append_log(app, reply);
    g_free(reply);
}

static void on_network_info(GtkButton *b, gpointer data) {
    (void)b;
    char *info = get_network_info();
    append_log(data, info);
    g_free(info);
}

static GtkWidget *build_process_tab(App *app) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_pack_start(GTK_BOX(box), labeled_entry("PID:", &app->pid_entry), FALSE, FALSE, 0);
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *refresh = gtk_button_new_with_label("Refresh Process");
    GtkWidget *kill = gtk_button_new_with_label("Kill PID");
    GtkWidget *fork_btn = gtk_button_new_with_label("Fork Demo");
    gtk_box_pack_start(GTK_BOX(row), refresh, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), kill, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), fork_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), row, FALSE, FALSE, 0);
    g_signal_connect(refresh, "clicked", G_CALLBACK(on_refresh_process), app);
    g_signal_connect(kill, "clicked", G_CALLBACK(on_kill_pid), app);
    g_signal_connect(fork_btn, "clicked", G_CALLBACK(on_fork_demo), app);
    return box;
}

static GtkWidget *build_file_tab(App *app) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    GtkWidget *row = labeled_entry("File:", &app->file_entry);
    GtkWidget *choose = gtk_button_new_with_label("Choose");
    gtk_box_pack_start(GTK_BOX(row), choose, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), row, FALSE, FALSE, 0);
    GtkWidget *buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *read_btn = gtk_button_new_with_label("Read File");
    GtkWidget *write_btn = gtk_button_new_with_label("Write File");
    gtk_box_pack_start(GTK_BOX(buttons), read_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(buttons), write_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), buttons, FALSE, FALSE, 0);
    app->file_text = gtk_text_view_new();
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), app->file_text);
    gtk_widget_set_size_request(scroll, -1, 260);
    gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 0);
    g_signal_connect(choose, "clicked", G_CALLBACK(on_choose_file), app);
    g_signal_connect(read_btn, "clicked", G_CALLBACK(on_read_file), app);
    g_signal_connect(write_btn, "clicked", G_CALLBACK(on_write_file), app);
    return box;
}

static GtkWidget *build_socket_tab(App *app) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_pack_start(GTK_BOX(box), labeled_entry("Host:", &app->host_entry), FALSE, FALSE, 0);
    gtk_entry_set_text(GTK_ENTRY(app->host_entry), "127.0.0.1");
    gtk_box_pack_start(GTK_BOX(box), labeled_entry("Port:", &app->port_entry), FALSE, FALSE, 0);
    gtk_entry_set_text(GTK_ENTRY(app->port_entry), "9090");
    gtk_box_pack_start(GTK_BOX(box), labeled_entry("Message:", &app->msg_entry), FALSE, FALSE, 0);
    gtk_entry_set_text(GTK_ENTRY(app->msg_entry), "Hello TCP server");
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *start = gtk_button_new_with_label("Start Server");
    GtkWidget *stop = gtk_button_new_with_label("Stop Server");
    GtkWidget *send = gtk_button_new_with_label("Client Send");
    gtk_box_pack_start(GTK_BOX(row), start, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), stop, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), send, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), row, FALSE, FALSE, 0);
    g_signal_connect(start, "clicked", G_CALLBACK(on_start_server), app);
    g_signal_connect(stop, "clicked", G_CALLBACK(on_stop_server), app);
    g_signal_connect(send, "clicked", G_CALLBACK(on_client_send), app);
    return box;
}

static GtkWidget *build_network_tab(App *app) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    GtkWidget *btn = gtk_button_new_with_label("Network Info");
    gtk_box_pack_start(GTK_BOX(box), btn, FALSE, FALSE, 0);
    g_signal_connect(btn, "clicked", G_CALLBACK(on_network_info), app);
    return box;
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    App app = {0};
    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app.window), "Bai 02 - Process File Socket Network");
    gtk_window_set_default_size(GTK_WINDOW(app.window), 900, 680);
    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(root), 10);
    GtkWidget *tabs = gtk_notebook_new();
    gtk_notebook_append_page(GTK_NOTEBOOK(tabs), build_process_tab(&app), gtk_label_new("Process"));
    gtk_notebook_append_page(GTK_NOTEBOOK(tabs), build_file_tab(&app), gtk_label_new("File"));
    gtk_notebook_append_page(GTK_NOTEBOOK(tabs), build_socket_tab(&app), gtk_label_new("Socket"));
    gtk_notebook_append_page(GTK_NOTEBOOK(tabs), build_network_tab(&app), gtk_label_new("Network"));
    gtk_box_pack_start(GTK_BOX(root), tabs, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(root), gtk_label_new("Log / Output"), FALSE, FALSE, 0);
    app.log_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app.log_view), FALSE);
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), app.log_view);
    gtk_widget_set_size_request(scroll, -1, 220);
    gtk_box_pack_start(GTK_BOX(root), scroll, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(app.window), root);
    gtk_widget_show_all(app.window);
    gtk_main();
    socket_server_stop(socket_log, &app);
    return 0;
}
