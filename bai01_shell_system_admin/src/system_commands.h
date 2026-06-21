#ifndef SYSTEM_COMMANDS_H
#define SYSTEM_COMMANDS_H

#include <glib.h>

gboolean run_system_script(const char *args, char **output);
char *quote_arg(const char *value);
char *get_current_time_string(void);

#endif
