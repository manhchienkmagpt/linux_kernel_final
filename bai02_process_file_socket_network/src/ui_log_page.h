#ifndef UI_LOG_PAGE_H
#define UI_LOG_PAGE_H

#include <gtk/gtk.h>

typedef struct {
    GtkWidget *root;
    GtkWidget *text_view;
} LogPage;

LogPage *ui_log_page_new(void);
void log_page_append(LogPage *page, const char *level, const char *message);

#endif
