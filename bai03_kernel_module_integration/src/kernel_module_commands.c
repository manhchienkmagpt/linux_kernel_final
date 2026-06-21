#include "kernel_module_commands.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>

typedef struct {
    char *command;
    CommandDoneCallback callback;
    gpointer user_data;
} AsyncCommand;

typedef struct {
    gboolean ok;
    char *output;
    CommandDoneCallback callback;
    gpointer user_data;
} AsyncResult;

static gboolean command_done_idle(gpointer idle_data) {
    AsyncResult *r = idle_data;
    if (r->callback) r->callback(r->ok, r->output, r->user_data);
    g_free(r->output);
    g_free(r);
    return FALSE;
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

char *run_command_sync(const char *command, gboolean *ok) {
    char *out = NULL, *err = NULL;
    int status = 0;
    GError *error = NULL;
    gboolean spawn_ok = g_spawn_command_line_sync(command, &out, &err, &status, &error);

    GString *result = g_string_new("");
    g_string_append_printf(result, "$ %s\n", command);
    if (!spawn_ok) {
        g_string_append(result, error->message);
        g_error_free(error);
        if (ok) *ok = FALSE;
    } else {
        if (out && *out) g_string_append(result, out);
        if (err && *err) g_string_append(result, err);
        if (ok) *ok = (status == 0);
    }
    g_free(out);
    g_free(err);
    return g_string_free(result, FALSE);
}

static gpointer command_worker(gpointer data) {
    AsyncCommand *cmd = data;
    AsyncResult *result = g_new0(AsyncResult, 1);
    result->output = run_command_sync(cmd->command, &result->ok);
    result->callback = cmd->callback;
    result->user_data = cmd->user_data;
    g_free(cmd->command);
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

char *read_kernel_log(gboolean module_only, const char *filter, int *line_count) {
    gboolean ok = FALSE;
    char *raw = run_command_sync("bash -lc 'dmesg | tail -200'", &ok);
    GString *filtered = g_string_new("");
    char **lines = g_strsplit(raw, "\n", -1);
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
    g_free(raw);
    return g_string_free(filtered, FALSE);
}
