#include <gtk/gtk.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define DEVICE_PATH "/dev/simple_kmod"

typedef struct {
    GtkWidget *window;
    GtkWidget *input_entry;
    GtkWidget *output_view;
} App;

static void append_output(App *app, const char *text) {
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->output_view));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buf, &end);
    gtk_text_buffer_insert(buf, &end, text, -1);
    gtk_text_buffer_insert(buf, &end, "\n", -1);
}

static void show_error(App *app, const char *message) {
    GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(app->window), GTK_DIALOG_MODAL,
        GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", message);
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
}

static void run_script(App *app, const char *arg) {
    char *cmd = g_strdup_printf("bash scripts/module_control.sh %s", arg);
    char *out = NULL, *err = NULL;
    int status = 0;
    GError *error = NULL;
    gboolean ok = g_spawn_command_line_sync(cmd, &out, &err, &status, &error);
    append_output(app, cmd);
    if (!ok) {
        show_error(app, error->message);
        append_output(app, error->message);
        g_error_free(error);
    } else {
        if (out && *out) append_output(app, out);
        if (err && *err) append_output(app, err);
        if (status != 0) show_error(app, "Command failed. See output.");
    }
    g_free(cmd);
    g_free(out);
    g_free(err);
}

static void write_device(App *app) {
    const char *text = gtk_entry_get_text(GTK_ENTRY(app->input_entry));
    int fd = open(DEVICE_PATH, O_WRONLY);
    if (fd < 0) {
        show_error(app, strerror(errno));
        return;
    }
    ssize_t n = write(fd, text, strlen(text));
    close(fd);
    if (n < 0) {
        show_error(app, strerror(errno));
        return;
    }
    char *msg = g_strdup_printf("Wrote %zd bytes to %s", n, DEVICE_PATH);
    append_output(app, msg);
    g_free(msg);
}

static void read_device(App *app) {
    int fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        show_error(app, strerror(errno));
        return;
    }
    char buffer[1025] = {0};
    ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);
    if (n < 0) {
        show_error(app, strerror(errno));
        return;
    }
    buffer[n] = '\0';
    append_output(app, "Data from device:");
    append_output(app, buffer);
}

static void on_load(GtkButton *b, gpointer data) { (void)b; run_script(data, "load"); }
static void on_unload(GtkButton *b, gpointer data) { (void)b; run_script(data, "unload"); }
static void on_status(GtkButton *b, gpointer data) { (void)b; run_script(data, "status"); }
static void on_dmesg(GtkButton *b, gpointer data) { (void)b; run_script(data, "dmesg"); }
static void on_write(GtkButton *b, gpointer data) { (void)b; write_device(data); }
static void on_read(GtkButton *b, gpointer data) { (void)b; read_device(data); }

static GtkWidget *button(const char *label, GCallback cb, App *app) {
    GtkWidget *b = gtk_button_new_with_label(label);
    g_signal_connect(b, "clicked", cb, app);
    return b;
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    App app = {0};

    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app.window), "Bai 03 - Kernel Module Integration");
    gtk_window_set_default_size(GTK_WINDOW(app.window), 820, 560);
    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(root), 10);

    GtkWidget *module_box = gtk_frame_new("Module Control");
    GtkWidget *module_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_container_add(GTK_CONTAINER(module_box), module_row);
    gtk_box_pack_start(GTK_BOX(module_row), button("Load Module", G_CALLBACK(on_load), &app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(module_row), button("Unload Module", G_CALLBACK(on_unload), &app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(module_row), button("Check Status", G_CALLBACK(on_status), &app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(module_row), button("Kernel Logs", G_CALLBACK(on_dmesg), &app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root), module_box, FALSE, FALSE, 0);

    GtkWidget *device_box = gtk_frame_new("Device /dev/simple_kmod");
    GtkWidget *device_col = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add(GTK_CONTAINER(device_box), device_col);
    GtkWidget *input_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(input_row), gtk_label_new("Data:"), FALSE, FALSE, 0);
    app.input_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(app.input_entry), "Hello from GTK user-space");
    gtk_box_pack_start(GTK_BOX(input_row), app.input_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(device_col), input_row, FALSE, FALSE, 0);

    GtkWidget *rw_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(rw_row), button("Write Device", G_CALLBACK(on_write), &app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(rw_row), button("Read Device", G_CALLBACK(on_read), &app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(device_col), rw_row, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root), device_box, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(root), gtk_label_new("Output"), FALSE, FALSE, 0);
    app.output_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app.output_view), FALSE);
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), app.output_view);
    gtk_box_pack_start(GTK_BOX(root), scroll, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(app.window), root);
    gtk_widget_show_all(app.window);
    gtk_main();
    return 0;
}
