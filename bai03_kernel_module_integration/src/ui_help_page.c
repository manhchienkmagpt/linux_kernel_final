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
        "3. Check Status\n"
        "4. Write data to /dev/simple_kmod\n"
        "5. Read data from /dev/simple_kmod\n"
        "6. View dmesg log\n"
        "7. Unload Module\n\n"
        "Linux commands:\n"
        "make\n"
        "sudo insmod src/simple_kmod.ko\n"
        "lsmod | grep simple_kmod\n"
        "echo \"hello\" > /dev/simple_kmod\n"
        "cat /dev/simple_kmod\n"
        "dmesg | tail\n"
        "sudo rmmod simple_kmod\n\n"
        "Notes:\n"
        "- Load/Unload usually requires sudo/root or pkexec.\n"
        "- If /dev/simple_kmod is missing, load the module first.\n"
        "- If permission is denied, run scripts/setup_device_permission.sh with sudo.";

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
