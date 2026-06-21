#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include <gtk/gtk.h>
#include "ui_log_page.h"

typedef struct {
    GtkWidget *window;
    LogPage *log_page;
} AppContext;

#endif
