#ifndef PROCESS_UTILS_H
#define PROCESS_UTILS_H

#include <glib.h>
#include <sys/types.h>

typedef enum {
    CHILD_TASK_WRITE_DATE,
    CHILD_TASK_WRITE_HEARTBEAT,
    CHILD_TASK_IDLE
} ChildTaskType;

char *get_process_list(void);
int kill_process_by_pid(pid_t pid, char **error_message);
char *fork_child_task(ChildTaskType task, const char *output_path, int interval_seconds);

#endif
