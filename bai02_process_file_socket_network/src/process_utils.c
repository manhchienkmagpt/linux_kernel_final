#include "process_utils.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

char *get_process_list(void) {
    char *out = NULL, *err = NULL;
    int status = 0;
    GError *error = NULL;
    gboolean ok = g_spawn_command_line_sync("ps -eo pid,comm,pcpu,pmem --sort=pid", &out, &err, &status, &error);
    if (!ok) {
        char *msg = g_strdup(error->message);
        g_error_free(error);
        return msg;
    }
    if (status != 0 && err && *err) {
        char *msg = g_strdup(err);
        g_free(out); g_free(err);
        return msg;
    }
    g_free(err);
    return out ? out : g_strdup("No process output.");
}

int kill_process_by_pid(pid_t pid, char **error_message) {
    if (pid <= 0) {
        *error_message = g_strdup("PID must be greater than 0.");
        return -1;
    }
    if (kill(pid, SIGTERM) == -1) {
        *error_message = g_strdup_printf("kill failed: %s", strerror(errno));
        return -1;
    }
    *error_message = g_strdup_printf("Sent SIGTERM to PID %d", pid);
    return 0;
}

char *fork_demo(void) {
    pid_t pid = fork();
    if (pid < 0) return g_strdup_printf("fork failed: %s", strerror(errno));
    if (pid == 0) {
        g_print("Child process running. PID=%d PPID=%d\n", getpid(), getppid());
        sleep(2);
        _exit(0);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    return g_strdup_printf("Parent PID=%d created child PID=%d. Child exited with status=%d", getpid(), pid, status);
}
