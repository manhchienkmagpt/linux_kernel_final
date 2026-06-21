#include "system_commands.h"

gboolean run_system_script(const char *args, char **output) {
    char *cmd = g_strdup_printf("bash scripts/system_ops.sh %s", args ? args : "");
    char *stdout_text = NULL;
    char *stderr_text = NULL;
    int status = 0;
    GError *error = NULL;
    gboolean ok = g_spawn_command_line_sync(cmd, &stdout_text, &stderr_text, &status, &error);

    GString *result = g_string_new("");
    g_string_append_printf(result, "$ %s\n", cmd);
    if (!ok) {
        g_string_append(result, error->message);
        g_error_free(error);
    } else {
        if (stdout_text && *stdout_text) g_string_append(result, stdout_text);
        if (stderr_text && *stderr_text) g_string_append(result, stderr_text);
    }

    *output = g_string_free(result, FALSE);
    g_free(stdout_text);
    g_free(stderr_text);
    g_free(cmd);
    return ok && status == 0;
}

char *quote_arg(const char *value) {
    return g_shell_quote(value ? value : "");
}

char *get_current_time_string(void) {
    GDateTime *now = g_date_time_new_now_local();
    char *text = g_date_time_format(now, "%Y-%m-%d %H:%M:%S %Z");
    g_date_time_unref(now);
    return text;
}
