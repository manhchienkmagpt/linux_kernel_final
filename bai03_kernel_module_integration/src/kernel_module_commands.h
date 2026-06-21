#ifndef KERNEL_MODULE_COMMANDS_H
#define KERNEL_MODULE_COMMANDS_H

#include <gtk/gtk.h>

#define MODULE_NAME "access_monitor"
#define DEVICE_PATH "/dev/access_monitor"
#define MODULE_KO_PATH "src/access_monitor.ko"

typedef void (*CommandDoneCallback)(gboolean ok, const char *output, gpointer user_data);

gboolean module_is_loaded(void);
gboolean device_exists(void);
char *kernel_version_string(void);
char *run_command_sync(const char *command, gboolean *ok);
void run_command_async(const char *command, CommandDoneCallback callback, gpointer user_data);
void run_command_async_sudo(GtkWindow *parent, const char *command, CommandDoneCallback callback, gpointer user_data);
char *read_device_data(gboolean *ok);
char *write_device_data(const char *text, gboolean *ok);
char *read_device_data_sudo(GtkWindow *parent, gboolean *ok);
char *write_device_data_sudo(GtkWindow *parent, const char *text, gboolean *ok);
char *read_protected_path(GtkWindow *parent, gboolean *ok);
char *set_protected_path(GtkWindow *parent, const char *path, gboolean *ok);
char *create_test_file(GtkWindow *parent, const char *protected_path, gboolean *ok);
char *write_test_file(GtkWindow *parent, const char *protected_path, gboolean *ok);
char *delete_test_file(GtkWindow *parent, const char *protected_path, gboolean *ok);
char *last_access_event(int *total_events);
char *read_kernel_log(gboolean module_only, const char *filter, int *line_count);
char *read_kernel_log_sudo(GtkWindow *parent, gboolean module_only, const char *filter, int *line_count);

#endif
