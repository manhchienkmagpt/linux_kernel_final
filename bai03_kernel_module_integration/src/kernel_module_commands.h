#ifndef KERNEL_MODULE_COMMANDS_H
#define KERNEL_MODULE_COMMANDS_H

#include <gtk/gtk.h>

#define MODULE_NAME "kfile_manager"
#define DEVICE_PATH "/dev/kfile_manager"
#define MODULE_KO_PATH "src/kfile_manager.ko"

typedef void (*CommandDoneCallback)(gboolean ok, const char *output, gpointer user_data);

typedef struct {
    char *root;
    char *last_command;
    char *last_result;
    int total_commands;
    char *raw;
} KFileStatus;

gboolean module_is_loaded(void);
gboolean device_exists(void);
char *kernel_version_string(void);
char *run_command_sync(const char *command, gboolean *ok);
void run_command_async(const char *command, CommandDoneCallback callback, gpointer user_data);
void run_command_async_sudo(GtkWindow *parent, const char *command, CommandDoneCallback callback, gpointer user_data);
char *read_device_data(gboolean *ok);
char *read_device_data_sudo(GtkWindow *parent, gboolean *ok);
char *write_device_data(const char *text, gboolean *ok);
char *write_device_data_sudo(GtkWindow *parent, const char *text, gboolean *ok);
char *send_kfile_command(GtkWindow *parent, const char *command, gboolean *ok);
KFileStatus *read_kfile_status(GtkWindow *parent, gboolean *ok);
void kfile_status_free(KFileStatus *status);
char *last_kfile_event(void);
char *read_kernel_log(gboolean module_only, const char *filter, int *line_count);
char *read_kernel_log_sudo(GtkWindow *parent, gboolean module_only, const char *filter, int *line_count);

#endif
