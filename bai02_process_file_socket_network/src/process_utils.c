#include "process_utils.h"
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/prctl.h>
#include <time.h>
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

static const char *task_name(ChildTaskType task) {
    switch (task) {
        case CHILD_TASK_WRITE_DATE: return "write-date";
        case CHILD_TASK_WRITE_HEARTBEAT: return "heartbeat";
        case CHILD_TASK_IDLE: return "idle";
        default: return "unknown";
    }
}

static const char *process_name_for_task(ChildTaskType task) {
    switch (task) {
        case CHILD_TASK_WRITE_DATE: return "child_date";
        case CHILD_TASK_WRITE_HEARTBEAT: return "child_beat";
        case CHILD_TASK_IDLE: return "child_idle";
        default: return "child_task";
    }
}

static void append_line_to_file(const char *path, const char *line) {
    FILE *f = fopen(path, "a");
    if (!f) return;
    fprintf(f, "%s\n", line);
    fclose(f);
}

static void run_child_task(ChildTaskType task, const char *output_path, int interval_seconds) {
    const char *path = (output_path && *output_path) ? output_path : "child_process_output.txt";
    int interval = interval_seconds > 0 ? interval_seconds : 5;
    unsigned long counter = 1;

    prctl(PR_SET_NAME, process_name_for_task(task), 0, 0, 0);

    while (1) {
        if (task == CHILD_TASK_WRITE_DATE) {
            time_t now = time(NULL);
            struct tm tm_now;
            char time_text[64];
            localtime_r(&now, &tm_now);
            strftime(time_text, sizeof(time_text), "%Y-%m-%d %H:%M:%S", &tm_now);

            char line[256];
            snprintf(line, sizeof(line), "PID=%d PPID=%d date=%s", getpid(), getppid(), time_text);
            append_line_to_file(path, line);
        } else if (task == CHILD_TASK_WRITE_HEARTBEAT) {
            char line[256];
            snprintf(line, sizeof(line), "PID=%d PPID=%d heartbeat=%lu", getpid(), getppid(), counter++);
            append_line_to_file(path, line);
        }

        sleep((unsigned int)interval);
    }
}

char *fork_child_task(ChildTaskType task, const char *output_path, int interval_seconds) {
    signal(SIGCHLD, SIG_IGN);

    pid_t pid = fork();
    if (pid < 0) return g_strdup_printf("fork failed: %s", strerror(errno));
    if (pid == 0) {
        run_child_task(task, output_path, interval_seconds);
        _exit(0);
    }

    return g_strdup_printf("Parent PID=%d created child PID=%d with task=%s. Kill it from the Processes page when you want it to stop.",
                           getpid(), pid, task_name(task));
}
