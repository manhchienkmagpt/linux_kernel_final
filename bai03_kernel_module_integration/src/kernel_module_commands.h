#ifndef KERNEL_MODULE_COMMANDS_H
#define KERNEL_MODULE_COMMANDS_H

#include <gtk/gtk.h>

#define MODULE_NAME "usb_mouse_monitor"
#define DEVICE_PATH "/proc/usb_mouse_monitor"
#define MODULE_KO_PATH "src/usb_mouse_monitor.ko"

typedef void (*CommandDoneCallback)(gboolean ok, const char *output, gpointer user_data);

typedef struct {
    int connected;
    int left;
    int right;
    int middle;
    int dx;
    int dy;
    int wheel;
    char *raw;
} MouseStatus;

gboolean module_is_loaded(void);
gboolean device_exists(void);
char *kernel_version_string(void);
char *run_command_sync(const char *command, gboolean *ok);
void run_command_async(const char *command, CommandDoneCallback callback, gpointer user_data);
void run_command_async_sudo(GtkWindow *parent, const char *command, CommandDoneCallback callback, gpointer user_data);
char *read_device_data(gboolean *ok);
char *read_device_data_sudo(GtkWindow *parent, gboolean *ok);
MouseStatus *read_mouse_status(GtkWindow *parent, gboolean *ok, char **message);
void mouse_status_free(MouseStatus *status);
char *last_mouse_event(void);
char *read_kernel_log(gboolean module_only, const char *filter, int *line_count);
char *read_kernel_log_sudo(GtkWindow *parent, gboolean module_only, const char *filter, int *line_count);

#endif
