#ifndef UI_LOG_PAGE_H
#define UI_LOG_PAGE_H

#include <gtk/gtk.h>

typedef struct {
    GtkWidget *root;
    GtkWidget *text_view;
    GtkWidget *parent_window;
} LogPage;

LogPage *ui_log_page_create(GtkWidget *parent_window);
void append_log(LogPage *page, const char *level, const char *message);

#endif
