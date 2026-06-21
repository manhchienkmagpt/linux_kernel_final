#include "ui_main_window.h"
#include "app_context.h"
#include "ui_dashboard_page.h"
#include "ui_device_io_page.h"
#include "ui_help_page.h"
#include "ui_kernel_log_page.h"
#include "ui_module_control_page.h"

static void on_destroy(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    g_free(user_data);
}

GtkWidget *ui_main_window_new(GtkApplication *app) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Linux Kernel Module Control Center");
    gtk_window_set_default_size(GTK_WINDOW(window), 1100, 700);

    GtkWidget *header = gtk_header_bar_new();
    GtkWidget *title = gtk_label_new("Linux Kernel Module Control Center");
    gtk_widget_add_css_class(title, "title-3");
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(header), title);
    gtk_window_set_titlebar(GTK_WINDOW(window), header);

    AppContext *ctx = g_new0(AppContext, 1);
    ctx->window = window;
    g_signal_connect(window, "destroy", G_CALLBACK(on_destroy), ctx);

    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_window_set_child(GTK_WINDOW(window), paned);

    GtkWidget *stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    GtkWidget *sidebar = gtk_stack_sidebar_new();
    gtk_stack_sidebar_set_stack(GTK_STACK_SIDEBAR(sidebar), GTK_STACK(stack));
    gtk_widget_set_size_request(sidebar, 210, -1);

    gtk_paned_set_start_child(GTK_PANED(paned), sidebar);
    gtk_paned_set_end_child(GTK_PANED(paned), stack);
    gtk_paned_set_resize_start_child(GTK_PANED(paned), FALSE);
    gtk_paned_set_shrink_start_child(GTK_PANED(paned), FALSE);

    gtk_stack_add_titled(GTK_STACK(stack), ui_dashboard_page_new(ctx), "dashboard", "Dashboard");
    gtk_stack_add_titled(GTK_STACK(stack), ui_module_control_page_new(ctx), "module", "Module Control");
    gtk_stack_add_titled(GTK_STACK(stack), ui_device_io_page_new(ctx), "device", "Device I/O");
    gtk_stack_add_titled(GTK_STACK(stack), ui_kernel_log_page_new(ctx), "logs", "Kernel Log");
    gtk_stack_add_titled(GTK_STACK(stack), ui_help_page_new(ctx), "help", "Help");
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "dashboard");
    return window;
}
