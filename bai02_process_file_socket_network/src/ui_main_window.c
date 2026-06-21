#include "ui_main_window.h"
#include "app_context.h"
#include "socket_demo.h"
#include "ui_dashboard_page.h"
#include "ui_file_page.h"
#include "ui_log_page.h"
#include "ui_network_page.h"
#include "ui_process_page.h"
#include "ui_socket_page.h"

static void on_window_destroy(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AppContext *ctx = user_data;
    socket_server_stop(NULL, NULL);
    g_free(ctx->log_page);
    g_free(ctx);
}

GtkWidget *ui_main_window_new(GtkApplication *app) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Linux Process, File, Socket & Network Manager");
    gtk_window_set_default_size(GTK_WINDOW(window), 1150, 720);

    GtkWidget *header = gtk_header_bar_new();
    GtkWidget *title = gtk_label_new("Linux Process, File, Socket & Network Manager");
    gtk_widget_add_css_class(title, "title-3");
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(header), title);
    gtk_window_set_titlebar(GTK_WINDOW(window), header);

    AppContext *ctx = g_new0(AppContext, 1);
    ctx->window = window;
    ctx->log_page = ui_log_page_new();

    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_window_set_child(GTK_WINDOW(window), paned);

    GtkWidget *stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    GtkWidget *sidebar = gtk_stack_sidebar_new();
    gtk_stack_sidebar_set_stack(GTK_STACK_SIDEBAR(sidebar), GTK_STACK(stack));
    gtk_widget_set_size_request(sidebar, 190, -1);

    gtk_paned_set_start_child(GTK_PANED(paned), sidebar);
    gtk_paned_set_end_child(GTK_PANED(paned), stack);
    gtk_paned_set_resize_start_child(GTK_PANED(paned), FALSE);
    gtk_paned_set_shrink_start_child(GTK_PANED(paned), FALSE);

    gtk_stack_add_titled(GTK_STACK(stack), ui_dashboard_page_new(ctx), "dashboard", "Dashboard");
    gtk_stack_add_titled(GTK_STACK(stack), ui_process_page_new(ctx), "processes", "Processes");
    gtk_stack_add_titled(GTK_STACK(stack), ui_file_page_new(ctx), "files", "Files");
    gtk_stack_add_titled(GTK_STACK(stack), ui_socket_page_new(ctx), "socket", "Socket");
    gtk_stack_add_titled(GTK_STACK(stack), ui_network_page_new(ctx), "network", "Network");
    gtk_stack_add_titled(GTK_STACK(stack), ctx->log_page->root, "log", "Log");
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "dashboard");

    log_page_append(ctx->log_page, "INFO", "Application started.");
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), ctx);
    return window;
}
