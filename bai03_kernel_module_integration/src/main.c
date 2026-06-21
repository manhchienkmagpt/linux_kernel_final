#include <gtk/gtk.h>
#include "ui_main_window.h"

static void on_activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;
    GtkWidget *window = ui_main_window_new(app);
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("vn.edu.linux.kernel_module_control_center", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
