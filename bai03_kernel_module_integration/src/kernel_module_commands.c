#include "kernel_module_commands.h"
#include <gio/gio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>

typedef struct {
    char *command;
    char *password;
    gboolean use_sudo;
    CommandDoneCallback callback;
    gpointer user_data;
} AsyncCommand;

typedef struct {
    gboolean ok;
    char *output;
    CommandDoneCallback callback;
    gpointer user_data;
} AsyncResult;

typedef struct {
    GMainLoop *loop;
    GtkWidget *entry;
    char *password;
    gboolean accepted;
} PasswordDialogData;

static gboolean command_done_idle(gpointer idle_data) {
    AsyncResult *r = idle_data;
    if (r->callback) r->callback(r->ok, r->output, r->user_data);
    g_free(r->output);
    g_free(r);
    return FALSE;
}

static void password_dialog_response(GtkDialog *dialog, int response, gpointer user_data) {
    PasswordDialogData *data = user_data;
    if (response == GTK_RESPONSE_ACCEPT) {
        data->accepted = TRUE;
        data->password = g_strdup(gtk_editable_get_text(GTK_EDITABLE(data->entry)));
    }
    gtk_window_destroy(GTK_WINDOW(dialog));
    g_main_loop_quit(data->loop);
}

static char *prompt_sudo_password(GtkWindow *parent) {
    PasswordDialogData data = {0};
    data.loop = g_main_loop_new(NULL, FALSE);

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Enter sudo password", parent,
        GTK_DIALOG_MODAL, "Cancel", GTK_RESPONSE_CANCEL, "Run sudo", GTK_RESPONSE_ACCEPT, NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_top(box, 12);
    gtk_widget_set_margin_bottom(box, 12);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);

    GtkWidget *label = gtk_label_new("This command requires sudo. Enter the current user's password:");
    gtk_label_set_wrap(GTK_LABEL(label), TRUE);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), label);

    data.entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(data.entry), FALSE);
    gtk_entry_set_input_purpose(GTK_ENTRY(data.entry), GTK_INPUT_PURPOSE_PASSWORD);
    gtk_entry_set_placeholder_text(GTK_ENTRY(data.entry), "sudo password");
    gtk_box_append(GTK_BOX(box), data.entry);

    gtk_box_append(GTK_BOX(content), box);
    g_signal_connect(dialog, "response", G_CALLBACK(password_dialog_response), &data);
    g_signal_connect_swapped(data.entry, "activate", G_CALLBACK(gtk_widget_activate_default), dialog);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
    gtk_window_present(GTK_WINDOW(dialog));
    gtk_widget_grab_focus(data.entry);

    g_main_loop_run(data.loop);
    g_main_loop_unref(data.loop);
    return data.accepted ? data.password : NULL;
}

static gboolean sudo_has_cached_credentials(void) {
    int status = 0;
    return g_spawn_command_line_sync("sudo -n true", NULL, NULL, &status, NULL) && status == 0;
}

gboolean module_is_loaded(void) {
    char *content = NULL;
    gboolean loaded = FALSE;
    if (g_file_get_contents("/proc/modules", &content, NULL, NULL)) {
        char *needle = g_strdup_printf("%s ", MODULE_NAME);
        loaded = g_strstr_len(content, -1, needle) != NULL;
        g_free(needle);
    }
    g_free(content);
    return loaded;
}

gboolean device_exists(void) {
    return g_file_test(DEVICE_PATH, G_FILE_TEST_EXISTS);
}

char *kernel_version_string(void) {
    struct utsname uts;
    if (uname(&uts) == 0) return g_strdup(uts.release);
    return g_strdup("Unknown");
}

static char *run_command_sync_internal(const char *command, gboolean use_sudo, const char *password, gboolean *ok) {
    char *full_command = use_sudo ? g_strdup_printf("sudo -S -p '' %s", command) : g_strdup(command);
    char *stdin_text = (use_sudo && password) ? g_strdup_printf("%s\n", password) : NULL;
    GError *error = NULL;
    GSubprocess *proc = g_subprocess_new(G_SUBPROCESS_FLAGS_STDIN_PIPE |
                                         G_SUBPROCESS_FLAGS_STDOUT_PIPE |
                                         G_SUBPROCESS_FLAGS_STDERR_PIPE,
                                         &error, "bash", "-lc", full_command, NULL);

    if (!proc) {
        if (ok) *ok = FALSE;
        char *message = g_strdup(error->message);
        g_clear_error(&error);
        g_free(full_command);
        g_free(stdin_text);
        return message;
    }

    char *out = NULL;
    char *err = NULL;
    gboolean communicate_ok = g_subprocess_communicate_utf8(proc, stdin_text, NULL, &out, &err, &error);
    gboolean status_ok = communicate_ok && g_subprocess_get_successful(proc);

    GString *result = g_string_new("");
    g_string_append_printf(result, "$ %s\n", full_command);
    if (!communicate_ok) {
        g_string_append(result, error->message);
        g_clear_error(&error);
    } else {
        if (out && *out) g_string_append(result, out);
        if (err && *err) g_string_append(result, err);
    }

    if (ok) *ok = status_ok;
    g_free(out);
    g_free(err);
    g_object_unref(proc);
    g_free(full_command);
    g_free(stdin_text);
    return g_string_free(result, FALSE);
}

char *run_command_sync(const char *command, gboolean *ok) {
    return run_command_sync_internal(command, FALSE, NULL, ok);
}

static gpointer command_worker(gpointer data) {
    AsyncCommand *cmd = data;
    AsyncResult *result = g_new0(AsyncResult, 1);
    result->output = run_command_sync_internal(cmd->command, cmd->use_sudo, cmd->password, &result->ok);
    result->callback = cmd->callback;
    result->user_data = cmd->user_data;
    g_free(cmd->command);
    g_free(cmd->password);
    g_free(cmd);
    g_idle_add(command_done_idle, result);
    return NULL;
}

void run_command_async(const char *command, CommandDoneCallback callback, gpointer user_data) {
    AsyncCommand *cmd = g_new0(AsyncCommand, 1);
    cmd->command = g_strdup(command);
    cmd->callback = callback;
    cmd->user_data = user_data;
    GThread *thread = g_thread_new("kernel-command", command_worker, cmd);
    g_thread_unref(thread);
}

void run_command_async_sudo(GtkWindow *parent, const char *command, CommandDoneCallback callback, gpointer user_data) {
    if (geteuid() == 0) {
        run_command_async(command, callback, user_data);
        return;
    }

    char *password = NULL;
    if (!sudo_has_cached_credentials()) {
        password = prompt_sudo_password(parent);
        if (!password) {
            if (callback) callback(FALSE, "Sudo password prompt was cancelled.", user_data);
            return;
        }
    }

    AsyncCommand *cmd = g_new0(AsyncCommand, 1);
    cmd->command = g_strdup(command);
    cmd->password = password;
    cmd->use_sudo = TRUE;
    cmd->callback = callback;
    cmd->user_data = user_data;
    GThread *thread = g_thread_new("kernel-command-sudo", command_worker, cmd);
    g_thread_unref(thread);
}

char *read_device_data(gboolean *ok) {
    int fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        if (ok) *ok = FALSE;
        return g_strdup_printf("open %s failed: %s", DEVICE_PATH, strerror(errno));
    }
    char buffer[1025] = {0};
    ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);
    if (n < 0) {
        if (ok) *ok = FALSE;
        return g_strdup_printf("read failed: %s", strerror(errno));
    }
    buffer[n] = '\0';
    if (ok) *ok = TRUE;
    return g_strdup(buffer);
}

char *write_device_data(const char *text, gboolean *ok) {
    int fd = open(DEVICE_PATH, O_WRONLY);
    if (fd < 0) {
        if (ok) *ok = FALSE;
        return g_strdup_printf("open %s failed: %s", DEVICE_PATH, strerror(errno));
    }
    ssize_t n = write(fd, text ? text : "", strlen(text ? text : ""));
    close(fd);
    if (n < 0) {
        if (ok) *ok = FALSE;
        return g_strdup_printf("write failed: %s", strerror(errno));
    }
    if (ok) *ok = TRUE;
    return g_strdup_printf("Wrote %zd bytes to %s", n, DEVICE_PATH);
}

static gboolean message_is_permission_denied(const char *message) {
    return message && g_strrstr(message, "Permission denied") != NULL;
}

static char *strip_command_echo(char *text) {
    if (!text) return g_strdup("");
    if (g_str_has_prefix(text, "$ ")) {
        char *newline = strchr(text, '\n');
        if (newline) return g_strdup(newline + 1);
    }
    return g_strdup(text);
}

static char *sudo_password_for_device(GtkWindow *parent, gboolean *cancelled);

static char *run_sudo_command_with_prompt(GtkWindow *parent, const char *command, gboolean *ok) {
    gboolean cancelled = FALSE;
    char *password = sudo_password_for_device(parent, &cancelled);
    if (cancelled) {
        if (ok) *ok = FALSE;
        return g_strdup("Sudo password prompt was cancelled.");
    }

    char *raw = run_command_sync_internal(command, TRUE, password, ok);
    g_free(password);
    return raw;
}

static char *sudo_password_for_device(GtkWindow *parent, gboolean *cancelled) {
    if (cancelled) *cancelled = FALSE;
    if (geteuid() == 0 || sudo_has_cached_credentials()) return NULL;

    char *password = prompt_sudo_password(parent);
    if (!password && cancelled) *cancelled = TRUE;
    return password;
}

char *read_device_data_sudo(GtkWindow *parent, gboolean *ok) {
    gboolean normal_ok = FALSE;
    char *normal = read_device_data(&normal_ok);
    if (normal_ok || !message_is_permission_denied(normal)) {
        if (ok) *ok = normal_ok;
        return normal;
    }
    g_free(normal);

    gboolean cancelled = FALSE;
    char *password = sudo_password_for_device(parent, &cancelled);
    if (cancelled) {
        if (ok) *ok = FALSE;
        return g_strdup("Sudo password prompt was cancelled.");
    }

    gboolean command_ok = FALSE;
    char *raw = run_command_sync_internal("cat " DEVICE_PATH, TRUE, password, &command_ok);
    char *data = command_ok ? strip_command_echo(raw) : g_strdup(raw);
    if (ok) *ok = command_ok;
    g_free(raw);
    g_free(password);
    return data;
}

char *write_device_data_sudo(GtkWindow *parent, const char *text, gboolean *ok) {
    gboolean normal_ok = FALSE;
    char *normal = write_device_data(text, &normal_ok);
    if (normal_ok || !message_is_permission_denied(normal)) {
        if (ok) *ok = normal_ok;
        return normal;
    }
    g_free(normal);

    gboolean cancelled = FALSE;
    char *password = sudo_password_for_device(parent, &cancelled);
    if (cancelled) {
        if (ok) *ok = FALSE;
        return g_strdup("Sudo password prompt was cancelled.");
    }

    char *quoted_text = g_shell_quote(text ? text : "");
    char *inner = g_strdup_printf("printf %%s %s > %s", quoted_text, DEVICE_PATH);
    char *quoted_inner = g_shell_quote(inner);
    char *command = g_strdup_printf("bash -lc %s", quoted_inner);
    gboolean command_ok = FALSE;
    char *raw = run_command_sync_internal(command, TRUE, password, &command_ok);

    char *message = command_ok
        ? g_strdup_printf("Wrote %zu bytes to %s using sudo", strlen(text ? text : ""), DEVICE_PATH)
        : g_strdup(raw);
    if (ok) *ok = command_ok;

    g_free(raw);
    g_free(command);
    g_free(quoted_inner);
    g_free(inner);
    g_free(quoted_text);
    g_free(password);
    return message;
}

char *read_protected_path(GtkWindow *parent, gboolean *ok) {
    char *path = read_device_data_sudo(parent, ok);
    g_strstrip(path);
    return path;
}

char *set_protected_path(GtkWindow *parent, const char *path, gboolean *ok) {
    if (!path || !*path || path[0] != '/') {
        if (ok) *ok = FALSE;
        return g_strdup("Protected path must be an absolute path.");
    }
    return write_device_data_sudo(parent, path, ok);
}

static char *test_file_path(const char *protected_path) {
    const char *base = (protected_path && *protected_path) ? protected_path : "/tmp/protected";
    return g_build_filename(base, "test.txt", NULL);
}

char *create_test_file(GtkWindow *parent, const char *protected_path, gboolean *ok) {
    char *path = test_file_path(protected_path);
    char *dir = g_path_get_dirname(path);
    char *qdir = g_shell_quote(dir);
    char *qpath = g_shell_quote(path);
    char *command = g_strdup_printf("mkdir -p %s && touch %s", qdir, qpath);
    char *raw = run_sudo_command_with_prompt(parent, command, ok);
    char *result = (*ok) ? g_strdup_printf("Created test file: %s", path) : g_strdup(raw);
    g_free(raw);
    g_free(command);
    g_free(qpath);
    g_free(qdir);
    g_free(dir);
    g_free(path);
    return result;
}

char *write_test_file(GtkWindow *parent, const char *protected_path, gboolean *ok) {
    char *path = test_file_path(protected_path);
    char *text = g_shell_quote("hello from access_monitor GUI\n");
    char *qpath = g_shell_quote(path);
    char *command = g_strdup_printf("printf %%s %s >> %s", text, qpath);
    char *raw = run_sudo_command_with_prompt(parent, command, ok);
    char *result = (*ok) ? g_strdup_printf("Wrote test data to: %s", path) : g_strdup(raw);
    g_free(raw);
    g_free(command);
    g_free(qpath);
    g_free(text);
    g_free(path);
    return result;
}

char *delete_test_file(GtkWindow *parent, const char *protected_path, gboolean *ok) {
    char *path = test_file_path(protected_path);
    char *qpath = g_shell_quote(path);
    char *command = g_strdup_printf("rm -f %s", qpath);
    char *raw = run_sudo_command_with_prompt(parent, command, ok);
    char *result = (*ok) ? g_strdup_printf("Deleted test file: %s", path) : g_strdup(raw);
    g_free(raw);
    g_free(command);
    g_free(qpath);
    g_free(path);
    return result;
}

char *last_access_event(int *total_events) {
    gboolean ok = FALSE;
    char *raw = run_command_sync("dmesg | grep access_monitor | tail -50", &ok);
    char **lines = g_strsplit(raw ? raw : "", "\n", -1);
    char *last = g_strdup("No events yet");
    int count = 0;

    for (int i = 0; lines[i]; i++) {
        if (!g_strstr_len(lines[i], -1, "[access_monitor]")) continue;
        g_free(last);
        last = g_strdup(lines[i]);
        count++;
    }
    if (total_events) *total_events = count;
    g_strfreev(lines);
    g_free(raw);
    return last;
}

static char *filter_kernel_log(const char *raw, gboolean module_only, const char *filter, int *line_count) {
    GString *filtered = g_string_new("");
    char **lines = g_strsplit(raw ? raw : "", "\n", -1);
    int count = 0;
    for (int i = 0; lines[i]; i++) {
        if (!lines[i][0] || lines[i][0] == '$') continue;
        if (module_only && !g_strrstr(lines[i], MODULE_NAME)) continue;
        if (filter && *filter && !g_strrstr(lines[i], filter)) continue;
        g_string_append_printf(filtered, "%s\n", lines[i]);
        count++;
    }
    if (line_count) *line_count = count;
    g_strfreev(lines);
    return g_string_free(filtered, FALSE);
}

char *read_kernel_log(gboolean module_only, const char *filter, int *line_count) {
    gboolean ok = FALSE;
    char *raw = run_command_sync("dmesg | tail -200", &ok);
    char *result = filter_kernel_log(raw, module_only, filter, line_count);
    g_free(raw);
    return result;
}

char *read_kernel_log_sudo(GtkWindow *parent, gboolean module_only, const char *filter, int *line_count) {
    gboolean ok = FALSE;
    char *raw = run_command_sync("dmesg | tail -200", &ok);

    if (!ok && geteuid() != 0) {
        g_free(raw);
        char *password = NULL;
        if (!sudo_has_cached_credentials()) {
            password = prompt_sudo_password(parent);
            if (!password) {
                if (line_count) *line_count = 0;
                return g_strdup("Sudo password prompt was cancelled.");
            }
        }
        raw = run_command_sync_internal("dmesg | tail -200", TRUE, password, &ok);
        g_free(password);
    }

    char *result = filter_kernel_log(raw, module_only, filter, line_count);
    g_free(raw);
    return result;
}
