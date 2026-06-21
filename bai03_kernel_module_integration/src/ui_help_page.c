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
        "2. Load Module with protected_path=/tmp/protected\n"
        "3. Create /tmp/protected\n"
        "4. Write / delete /tmp/protected/test.txt\n"
        "5. View Event Log\n"
        "6. Unload Module\n\n"
        "Linux commands:\n"
        "make\n"
        "sudo insmod src/access_monitor.ko protected_path=/tmp/protected\n"
        "mkdir -p /tmp/protected\n"
        "echo \"hello\" > /tmp/protected/test.txt\n"
        "rm /tmp/protected/test.txt\n"
        "dmesg | grep access_monitor\n"
        "sudo rmmod access_monitor\n\n"
        "Notes:\n"
        "- access_monitor uses kprobes on vfs_write and vfs_unlink.\n"
        "- It does not hook the syscall table directly.\n"
        "- /dev/access_monitor lets the GUI read/write protected_path.\n"
        "- Device access and module load/unload may require sudo/root.";

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
