#include "system_commands.h"
#include <gio/gio.h>
#include <unistd.h>

typedef struct {
    GMainLoop *loop;
    GtkWidget *entry;
    char *password;
    gboolean accepted;
} PasswordDialogData;

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

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Nhap mat khau sudo", parent,
        GTK_DIALOG_MODAL, "Huy", GTK_RESPONSE_CANCEL, "Chay sudo", GTK_RESPONSE_ACCEPT, NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_top(box, 12);
    gtk_widget_set_margin_bottom(box, 12);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);

    GtkWidget *label = gtk_label_new("Lenh nay can quyen sudo. Hay nhap mat khau cua user hien tai:");
    gtk_label_set_wrap(GTK_LABEL(label), TRUE);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), label);

    data.entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(data.entry), FALSE);
    gtk_entry_set_input_purpose(GTK_ENTRY(data.entry), GTK_INPUT_PURPOSE_PASSWORD);
    gtk_entry_set_placeholder_text(GTK_ENTRY(data.entry), "Mat khau sudo");
    gtk_box_append(GTK_BOX(box), data.entry);

    gtk_box_append(GTK_BOX(content), box);
    g_signal_connect(dialog, "response", G_CALLBACK(password_dialog_response), &data);
    g_signal_connect_swapped(data.entry, "activate", G_CALLBACK(gtk_window_activate_default), dialog);
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

static gboolean run_command_with_optional_sudo(GtkWindow *parent, const char *command, gboolean use_sudo, char **output) {
    char *password = NULL;
    char *full_command = NULL;
    char *stdin_text = NULL;

    if (use_sudo && geteuid() != 0 && !sudo_has_cached_credentials()) {
        password = prompt_sudo_password(parent);
        if (!password) {
            *output = g_strdup("Sudo password prompt was cancelled.");
            return FALSE;
        }
    }

    if (use_sudo && geteuid() != 0) {
        full_command = g_strdup_printf("sudo -S -p '' %s", command);
        if (password) stdin_text = g_strdup_printf("%s\n", password);
    } else {
        full_command = g_strdup(command);
    }

    GError *error = NULL;
    GSubprocess *proc = g_subprocess_new(G_SUBPROCESS_FLAGS_STDIN_PIPE |
                                         G_SUBPROCESS_FLAGS_STDOUT_PIPE |
                                         G_SUBPROCESS_FLAGS_STDERR_PIPE,
                                         &error, "bash", "-lc", full_command, NULL);
    if (!proc) {
        *output = g_strdup(error->message);
        g_clear_error(&error);
        g_free(password);
        g_free(full_command);
        g_free(stdin_text);
        return FALSE;
    }

    char *stdout_text = NULL;
    char *stderr_text = NULL;
    gboolean ok = g_subprocess_communicate_utf8(proc, stdin_text, NULL, &stdout_text, &stderr_text, &error);
    gboolean status_ok = ok && g_subprocess_get_successful(proc);

    GString *result = g_string_new("");
    g_string_append_printf(result, "$ %s\n", full_command);
    if (!ok) {
        g_string_append(result, error->message);
        g_clear_error(&error);
    } else {
        if (stdout_text && *stdout_text) g_string_append(result, stdout_text);
        if (stderr_text && *stderr_text) g_string_append(result, stderr_text);
    }

    *output = g_string_free(result, FALSE);
    g_free(stdout_text);
    g_free(stderr_text);
    g_object_unref(proc);
    g_free(password);
    g_free(full_command);
    g_free(stdin_text);
    return status_ok;
}

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

gboolean run_system_script_sudo(GtkWindow *parent, const char *args, char **output) {
    char *cmd = g_strdup_printf("bash scripts/system_ops.sh %s", args ? args : "");
    gboolean ok = run_command_with_optional_sudo(parent, cmd, TRUE, output);
    g_free(cmd);
    return ok;
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
