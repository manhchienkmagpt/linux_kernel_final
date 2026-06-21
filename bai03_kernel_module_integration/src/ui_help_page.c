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
        "Linux Kernel File Manager\n\n"
        "Module: kfile_manager\n"
        "Device: /dev/kfile_manager\n"
        "Default root: /tmp/kfile_manager_root\n\n"
        "Commands:\n"
        "SET_ROOT /tmp/kfile_manager_root\n"
        "CREATE test.txt\n"
        "WRITE test.txt hello world\n"
        "APPEND test.txt new line\n"
        "READ test.txt\n"
        "INFO test.txt\n"
        "COPY test.txt copy.txt\n"
        "RENAME copy.txt renamed.txt\n"
        "DELETE renamed.txt\n"
        "LIST\n"
        "STATUS\n"
        "HELP\n\n"
        "GUI File Manager flow:\n"
        "1. Choose Root or type root path, then Set Root\n"
        "2. File list appears on the left\n"
        "3. Select a file to fill Filename\n"
        "4. Read loads content into editor\n"
        "5. Write/Append saves editor content through the kernel module\n"
        "6. Delete/Rename/Copy/List refresh the file list\n\n"
        "Terminal demo:\n"
        "sudo mkdir -p /tmp/kfile_manager_root\n"
        "make\n"
        "sudo insmod src/kfile_manager.ko\n"
        "echo \"CREATE test.txt\" | sudo tee /dev/kfile_manager\n"
        "echo \"WRITE test.txt hello world\" | sudo tee /dev/kfile_manager\n"
        "echo \"READ test.txt\" | sudo tee /dev/kfile_manager\n"
        "cat /dev/kfile_manager\n"
        "echo \"INFO test.txt\" | sudo tee /dev/kfile_manager\n"
        "cat /dev/kfile_manager\n"
        "dmesg | grep kfile_manager\n"
        "sudo rmmod kfile_manager\n\n"
        "Security notes:\n"
        "- Root directory must be inside /tmp or /home.\n"
        "- Filenames cannot be absolute paths and cannot contain '..'.\n"
        "- File operations are built under the configured root directory only.";

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
