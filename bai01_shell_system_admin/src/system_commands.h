#ifndef SYSTEM_COMMANDS_H
#define SYSTEM_COMMANDS_H

#include <gtk/gtk.h>

gboolean run_system_script(const char *args, char **output);
gboolean run_system_script_sudo(GtkWindow *parent, const char *args, char **output);
char *quote_arg(const char *value);
char *get_current_time_string(void);

#endif
