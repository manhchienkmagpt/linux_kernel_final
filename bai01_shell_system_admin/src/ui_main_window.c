#include "ui_main_window.h"
#include "ui_apt_page.h"
#include "ui_cron_page.h"
#include "ui_file_page.h"
#include "ui_log_page.h"
#include "ui_time_page.h"

static void on_sidebar_clicked(GtkButton *btn, gpointer data) {
    (void)data;
    GtkStack *s = g_object_get_data(G_OBJECT(btn), "stack");
    const char *name = g_object_get_data(G_OBJECT(btn), "page-name");
    gtk_stack_set_visible_child_name(s, name);
}

static GtkWidget *sidebar_button(const char *label, GtkStack *stack, const char *page_name) {
    GtkWidget *button = gtk_button_new_with_label(label);
    gtk_widget_set_hexpand(button, TRUE);
    g_object_set_data(G_OBJECT(button), "stack", stack);
    g_object_set_data_full(G_OBJECT(button), "page-name", g_strdup(page_name), g_free);
    g_signal_connect(button, "clicked", G_CALLBACK(on_sidebar_clicked), NULL);
    return button;
}

GtkWidget *ui_main_window_new(GtkApplication *app) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Linux System Admin Tool");
    gtk_window_set_default_size(GTK_WINDOW(window), 1050, 700);

    GtkWidget *header = gtk_header_bar_new();
    GtkWidget *title = gtk_label_new("Linux System Admin Tool");
    gtk_widget_add_css_class(title, "title-2");
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(header), title);
    gtk_window_set_titlebar(GTK_WINDOW(window), header);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), main_box);

    GtkWidget *sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_size_request(sidebar, 210, -1);
    gtk_widget_set_margin_top(sidebar, 12);
    gtk_widget_set_margin_bottom(sidebar, 12);
    gtk_widget_set_margin_start(sidebar, 12);
    gtk_widget_set_margin_end(sidebar, 12);
    gtk_box_append(GTK_BOX(main_box), sidebar);

    GtkWidget *stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_widget_set_hexpand(stack, TRUE);
    gtk_widget_set_vexpand(stack, TRUE);
    gtk_box_append(GTK_BOX(main_box), stack);

    LogPage *log_page = ui_log_page_create(window);
    GtkWidget *file_page = ui_file_page_new(window, log_page);
    GtkWidget *cron_page = ui_cron_page_new(window, log_page);
    GtkWidget *time_page = ui_time_page_new(window, log_page);
    GtkWidget *apt_page = ui_apt_page_new(window, log_page);

    gtk_stack_add_named(GTK_STACK(stack), file_page, "file_page");
    gtk_stack_add_named(GTK_STACK(stack), cron_page, "cron_page");
    gtk_stack_add_named(GTK_STACK(stack), time_page, "time_page");
    gtk_stack_add_named(GTK_STACK(stack), apt_page, "apt_page");
    gtk_stack_add_named(GTK_STACK(stack), log_page->root, "log_page");

    gtk_box_append(GTK_BOX(sidebar), sidebar_button("File Manager", GTK_STACK(stack), "file_page"));
    gtk_box_append(GTK_BOX(sidebar), sidebar_button("Cron Scheduler", GTK_STACK(stack), "cron_page"));
    gtk_box_append(GTK_BOX(sidebar), sidebar_button("Time System", GTK_STACK(stack), "time_page"));
    gtk_box_append(GTK_BOX(sidebar), sidebar_button("Apt Manager", GTK_STACK(stack), "apt_page"));
    gtk_box_append(GTK_BOX(sidebar), sidebar_button("Log", GTK_STACK(stack), "log_page"));

    gtk_stack_set_visible_child_name(GTK_STACK(stack), "file_page");
    append_log(log_page, "INFO", "Application started.");
    return window;
}
