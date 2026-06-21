#include "ui_help_page.h"

GtkWidget *ui_help_page_new(AppContext *ctx) {
    (void)ctx;
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(root, 14);
    gtk_widget_set_margin_bottom(root, 14);
    gtk_widget_set_margin_start(root, 14);
    gtk_widget_set_margin_end(root, 14);

    GtkWidget *title = gtk_label_new("Help / Demo Guide");
    gtk_widget_add_css_class(title, "title-2");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(root), title);

    const char *guide =
        "Demo steps:\n"
        "1. Build Module\n"
        "2. Load Module\n"
        "3. Move/click/scroll the current Ubuntu mouse\n"
        "4. Refresh Mouse Status\n"
        "5. View Event Log\n"
        "6. Unload Module\n\n"
        "Linux commands:\n"
        "make\n"
        "sudo insmod src/usb_mouse_monitor.ko\n"
        "dmesg | grep usb_mouse_monitor\n"
        "cat /proc/usb_mouse_monitor\n"
        "sudo rmmod usb_mouse_monitor\n\n"
        "Notes:\n"
        "- dx/dy are relative movement deltas, not absolute screen coordinates.\n"
        "- This module registers an input_handler, not a USB driver.\n"
        "- It reads events from the current Ubuntu mouse/touchpad without unbind/bind.\n"
        "- Module load/unload usually requires sudo/root.";

    GtkWidget *view = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(view), TRUE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)), guide, -1);

    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), view);
    gtk_box_append(GTK_BOX(root), scroll);
    return root;
}
